#include "nRF24L01.h"

/******************************************************************************/
/*--------------------------Local Data Variables------------------------------*/
/******************************************************************************/

/** Write/Read storage variables **/
static volatile uint8_t nullData[33] = {0};     // Used for dummy writes only
static volatile uint8_t txData[33] = {0};
static volatile uint8_t rxData[33] = {0};

/** Payload related variables **/
static volatile NrfPayloadConfig_t isrPayldConfig;
static volatile uint8_t isrPayldWidth;

/** RX data related variables **/
static volatile NrfRxPipeNo_t rxPipeNo;
static volatile uint64_t rxPipeAddr[6];
static volatile uint8_t *isrRxPtr = NULL;

/** Status flags **/
static volatile NrfStatusFlag_t statusFlag = NRF_FLAG_NO_STATUS;
static volatile bool isRxFifoLoading = false;

/** Timeout related variables **/
static volatile bool isTimeoutEnabled;
static volatile uint32_t timeoutCount;
static volatile uint32_t timeout;
static const uint32_t timeoutVal = 30;      // 30 ms timeout

/** System clock for timeout purpose **/
static uint32_t sysFreq;

/** Pointer to Interrupt Controller **/
static IcSfr_t *const icSfr = &IC_MODULE;

/** ISR function pointers to internal callbacks **/
static void (*isrHandlerPtr)(void);
static void (*isrTimeoutHandlerPtr)(void);

/** ISR function pointers to user defined callbacks **/
static void (*userClbkReadPayload)(void);
static void (*userClbkReadAckPayload)(void);
static void (*userClbkStartTransmission)(void);
static void (*userClbkPayloadTimeout)(void);

/** Register sequence for series of register writes (used with "regConfig") **/
static const uint8_t configRegMap[10] = {
    NRF_STATUS_REG, NRF_CONFIG_REG, NRF_EN_AA_REG, NRF_EN_RXADDR_REG,
    NRF_SETUP_AW_REG, NRF_SETUP_RETR_REG, NRF_RF_CH_REG, NRF_RF_SETUP_REG,
    NRF_DYNPD_REG, NRF_FEATURE_REG
};

/******************************************************************************/
/*--------------------------Local Data Structures-----------------------------*/
/******************************************************************************/

/* NOTE: Registers RX_PW_Px not used since Dynamic Payload is always enabled */

/* Structure for storing nRF configuration data */
typedef struct {
    uint8_t     STATUS;     // First write to STATUS to clear flags
    uint8_t     CONFIG;
    uint8_t     EN_AA;
    uint8_t     EN_RXADDR;
    uint8_t     SETUP_AW;
    uint8_t     SETUP_RETR;
    uint8_t     RF_CH;
    uint8_t     RF_SETUP;
    uint8_t     DYNPD;
    uint8_t     FEATURE;
} const RegConfig_t;

/* Structure for separate write session */
typedef struct {
    uint64_t    RX_ADDR_P0;
    uint64_t    RX_ADDR_P1;
    uint8_t     RX_ADDR_P2;
    uint8_t     RX_ADDR_P3;
    uint8_t     RX_ADDR_P4;
    uint8_t     RX_ADDR_P5;
} const PipeAddrConfig_t;


/******************************************************************************/
/*------------------------Local Function Prototypes---------------------------*/
/******************************************************************************/

/** Non-ISR sub-function **/
static void IsrHandlerPtrConfig(IsrNrfMode_t isrMode);

/** ISR handlers **/
static void ISR_NrfHandler_ReadAckPayload(void);
static void ISR_NrfHandler_ReadPayload(void);
static void ISR_NrfHandler_SendPayloadCont(void);
static void ISR_NrfHandler_ReadPayloadCont(void);
static void ISR_NrfHandler_StartTransmission(void);
static void ISR_NrfHandler_RestartReception(void);
static void ISR_NrfTimeoutHandler_SendPayload(void);

/** Other functions **/
INLINE static void InterruptSfrConfig(const uint32_t pinCode);


/******************************************************************************/
/*----------------------External Function Definitions-------------------------*/
/******************************************************************************/

/*
 *  Configures nRF as PTX with the given settings
 */
extern bool NRF_ConfigPtxSfr(const NrfPtxConfig_t ptxConfig)
{
    /* IRQ pin as non-GPIO, controlled by Interrupt Controller */
    PIO_ConfigPpsSfr(ptxConfig.pinConfig.irqPin);
    
    /* Configure PIO settings for CE and IRQ pins (CS configured by SPI) */
    PIO_ConfigGpioPin(ptxConfig.pinConfig.cePin, PIO_TYPE_DIGITAL, PIO_DIR_OUTPUT);
    PIO_ConfigPpsPin(ptxConfig.pinConfig.irqPin, PIO_TYPE_DIGITAL);
    PIO_ConfigGpioPinPull(ptxConfig.pinConfig.irqPin, PIO_CN_PULLUP);
    PIO_ClearPin(ptxConfig.pinConfig.cePin);

    /* Enable current slave */
    SPI_EnableSsState(ptxConfig.pinConfig.csPin);
    
    /* Power-up the device */
    txData[0] = NRF_WRITE_CMD(NRF_CONFIG_REG);
    txData[1] = NRF_PWR_UP_MASK;
    SPI_MasterReadWrite(ptxConfig.spiSfr, rxData, txData, 2);
    
    /* Delay after power-up */
    TMR_DelayUs(1500);
    
    /* Device not responding or SPI not configured */
    if( rxData[0] == NRF_FLAG_NO_RP )
    {
        return false;
    }
    
    /* Set timeout callback for interrupt mode */
    TMR_SetCoreTimerCallback(ISR_NrfTimeoutHandler_SendPayload);
    isTimeoutEnabled = false;
            
    /* Store nRF register configuration settings */
    RegConfig_t regConfig = {
        .STATUS =       (NRF_MAX_RT_MASK | NRF_TX_DS_MASK | NRF_RX_DR_MASK),
        .CONFIG =       (NRF_PWR_UP_MASK | NRF_CRCO_MASK | NRF_EN_CRC_MASK),
        .SETUP_AW =     (NRF_AW_MASK),
        .SETUP_RETR =   ((ptxConfig.retrCount << NRF_ARC_POS) |
                        (ptxConfig.retrDelay << NRF_ARD_POS)),
        .RF_CH =        (ptxConfig.rfChannel << NRF_RF_CH_POS),
        .RF_SETUP =     ((ptxConfig.rfPower << NRF_RF_PWR_POS) |
                        ((ptxConfig.dataRate & 0x1) << NRF_RF_DR_HIGH_POS) |
                        ((ptxConfig.dataRate & 0x2) << NRF_RF_DR_LOW_POS)),
        .FEATURE =      ((ptxConfig.isAck << NRF_EN_ACK_PAY_POS) | NRF_EN_DPL_MASK),
        .EN_AA =        (ptxConfig.isAck ? 0x01 : 0x00),
        .EN_RXADDR =    (0x01),
        .DYNPD =        (0x01),
    };
    
    /* Flush TX + RX FIFO */
    txData[0] = NRF_FLUSH_RX_CMD;
    SPI_MasterReadWrite(ptxConfig.spiSfr, rxData, txData, 1);
    txData[0] = NRF_FLUSH_TX_CMD;
    SPI_MasterReadWrite(ptxConfig.spiSfr, rxData, txData, 1);
    
    /* Pointer for indirect member access of structure */
    const uint8_t *txPtr = (uint8_t *)&regConfig;
    
    /* Modify configuration registers */
    for(uint8_t i = 0; i < 10; i++, txPtr++)
    {
        txData[0] = NRF_WRITE_CMD(configRegMap[i]);
        txData[1] = *txPtr;
        
        SPI_MasterReadWrite(ptxConfig.spiSfr, rxData, txData, 2);
    }
    
    /* SYS_CLK is read for timeout purpose */
    sysFreq = OSC_GetSysFreq();
    
    /* Disable current slave */
    SPI_DisableSsState(ptxConfig.pinConfig.csPin);
    
    /* Configures interrupt SFRs (based on IRQ pin's PPS register code) */
    InterruptSfrConfig(ptxConfig.pinConfig.irqPin);
    
    return true;
}


/*
 *  Configures nRF as PRX with the given settings
 */
extern bool NRF_ConfigPrxSfr(const NrfPrxConfig_t prxConfig)
{
    /* IRQ pin as non-GPIO, controlled by Interrupt Controller */
    PIO_ConfigPpsSfr(prxConfig.pinConfig.irqPin);
    
    /* Configure PIO settings for CE and IRQ pins (CS configured by SPI) */
    PIO_ConfigGpioPin(prxConfig.pinConfig.cePin, PIO_TYPE_DIGITAL, PIO_DIR_OUTPUT);
    PIO_ConfigPpsPin(prxConfig.pinConfig.irqPin, PIO_TYPE_DIGITAL);
    PIO_ConfigGpioPinPull(prxConfig.pinConfig.irqPin, PIO_CN_PULLUP);
    PIO_ClearPin(prxConfig.pinConfig.cePin);
    
    /* Enable current slave */
    SPI_EnableSsState(prxConfig.pinConfig.csPin);
    
    /* Power-up the device (if needed) */
    txData[0] = NRF_WRITE_CMD(NRF_CONFIG_REG);
    txData[1] = NRF_PWR_UP_MASK;
    SPI_MasterReadWrite(prxConfig.spiSfr, rxData, txData, 2);
    
    /* Delay after power-up */
    TMR_DelayUs(1500);
    
    /* Device not responding or SPI not configured */
    if( rxData[0] == NRF_FLAG_NO_RP )
    {
        return false;
    }
     
    /* Individual bits reflect whether PIPE_x is enabled or not */
    uint8_t pipeStatus = ( ((bool)prxConfig.pipeAddr.pipe0 << 0) |
                           ((bool)prxConfig.pipeAddr.pipe1 << 1) |
                           ((bool)prxConfig.pipeAddr.pipe2 << 2) |
                           ((bool)prxConfig.pipeAddr.pipe3 << 3) |
                           ((bool)prxConfig.pipeAddr.pipe4 << 4) |
                           ((bool)prxConfig.pipeAddr.pipe5 << 5) );
    
    /* No PIPE_x address check */
    if( pipeStatus == 0x00 )
    {
        return false;
    }
    
    /* Store nRF register configuration settings */
    RegConfig_t regConfig = {
        .STATUS =       (NRF_MAX_RT_MASK | NRF_TX_DS_MASK | NRF_RX_DR_MASK),
        .CONFIG =       (NRF_PWR_UP_MASK | NRF_PRIM_RX_MASK | NRF_CRCO_MASK | NRF_EN_CRC_MASK),
        .SETUP_AW =     (NRF_AW_MASK),
        .SETUP_RETR =   (0x00),
        .RF_CH =        (prxConfig.rfChannel << NRF_RF_CH_POS),
        .RF_SETUP =     (((prxConfig.dataRate & 0x1) << NRF_RF_DR_HIGH_POS) |
                        ((prxConfig.dataRate & 0x2) << NRF_RF_DR_LOW_POS)),
        .FEATURE =      ((prxConfig.isAck << NRF_EN_ACK_PAY_POS) | NRF_EN_DPL_MASK),
        .EN_AA =        (prxConfig.isAck ? (pipeStatus & 0x3F) : 0x00),
        .EN_RXADDR =    (pipeStatus & 0x3F),
        .DYNPD =        (pipeStatus & 0x3F),
    };
    
    /* Flush TX + RX FIFO */
    txData[0] = NRF_FLUSH_RX_CMD;
    SPI_MasterReadWrite(prxConfig.spiSfr, rxData, txData, 1);
    txData[0] = NRF_FLUSH_TX_CMD;
    SPI_MasterReadWrite(prxConfig.spiSfr, rxData, txData, 1);
    
    /* Pointers for indirect member access of structure */
    const uint8_t *txPtr;
    const uint64_t *txPtr64;
    
    txPtr = (uint8_t *)&regConfig;
    
    /* Modify configuration registers */
    for(uint8_t i = 0; i < 10; i++, txPtr++)
    {
        txData[0] = NRF_WRITE_CMD(configRegMap[i]);
        txData[1] = *txPtr;
        
        SPI_MasterReadWrite(prxConfig.spiSfr, rxData, txData, 2);
    }
    
    txPtr64 = &prxConfig.pipeAddr.pipe0;
    uint64_t txData64 = 0;
    
    /* Modify PIPE_x addresses */
    for(uint8_t i = 0; i < 6; i++, txPtr64++)
    {
        /* Non-zero address is valid */
        if( (pipeStatus >> i) & 0x01 )
        {
            /* Write 5-byte address for first two pipes */
            if( i < 2 )
            {
                txData64 = (*txPtr64 << 8) | (NRF_WRITE_CMD(NRF_RX_ADDR_P0_REG + i));
                SPI_MasterReadWrite(prxConfig.spiSfr, rxData, &txData64, 6);
            }
            /* Write 1-byte address for other pipes */
            else
            {
                txData64 = ((*txPtr64 & 0xFF) << 8) | (NRF_WRITE_CMD(NRF_RX_ADDR_P0_REG + i));
                SPI_MasterReadWrite(prxConfig.spiSfr, rxData, &txData64, 2);
            }
        }
        
        rxPipeAddr[i] = txData64;   // Store all addresses into local array
    }
    
    /* Configures interrupt SFRs (based on IRQ pin's PPS register code) */
    InterruptSfrConfig(prxConfig.pinConfig.irqPin);
    
    /* Disable current slave */
    SPI_DisableSsState(prxConfig.pinConfig.csPin);
    
    return true;
}

/*
 *  Set user callback for a certain type of operation.
 */
void NRF_SetUserCallback(NrfUserCallback_t cType, void (*fPtr)(void))
{
    /* Callback after PRX payload receive operation */
    if( cType == NRF_CLBK_RX_PAYLOAD_RECEIVE )
    {
        userClbkReadPayload = fPtr;
    }
    /* Callback after PTX ACK payload receive from PRX operation */
    else if( cType == NRF_CLBK_TX_ACK_PAYLOAD_RECEIVE )
    {
        userClbkReadAckPayload = fPtr;
    }
    /* Callback after PTX start transmission operation */
    else if( cType == NRF_CLBK_TX_START )
    {
        userClbkStartTransmission = fPtr;
    }
    /* Callback after PTX send payload timeout operation */
    else 
    {
        userClbkPayloadTimeout = fPtr;
    }
}

/*
 *  Release user callback for a certain type of operation.
 */
void NRF_ReleaseUserCallback(NrfUserCallback_t cType)
{
    /* Callback after PRX payload receive operation */
    if( cType == NRF_CLBK_RX_PAYLOAD_RECEIVE )
    {
        userClbkReadPayload = NULL;
    }
    /* Callback after PTX ACK payload receive from PRX operation */
    else if( cType == NRF_CLBK_TX_ACK_PAYLOAD_RECEIVE )
    {
        userClbkReadAckPayload = NULL;
    }
    /* Callback after PTX start transmission operation */
    else if( cType == NRF_CLBK_TX_START )
    {
        userClbkStartTransmission = NULL;
    }
    /* Callback after PTX send payload timeout operation */
    else 
    {
        userClbkPayloadTimeout = NULL;
    }
}

/*
 *  Sends payload and waits (polling) for ACK payload (or successful
 *  transmission if no-acknowledge is enabled)
 */
 extern bool NRF_SendReceivePayload(NrfPayloadConfig_t payldConfig, void *rxPtr, void *txPtr, uint8_t txSize)
{
    /* Reset status */
    statusFlag = NRF_FLAG_NO_STATUS;
    
    /* Enable current slave */
    SPI_EnableSsState(payldConfig.pinConfig.csPin);
    
    /* Flush TX + RX FIFO */
    txData[0] = NRF_FLUSH_TX_CMD;
    SPI_MasterReadWrite(payldConfig.spiSfr, rxData, txData, 1);
    txData[0] = NRF_FLUSH_RX_CMD;
    SPI_MasterReadWrite(payldConfig.spiSfr, rxData, txData, 1);
    
    /* Clear device status - in case of previous MAX_RT */
    txData[0] = NRF_WRITE_CMD(NRF_STATUS_REG);
    txData[1] = NRF_MAX_RT_MASK | NRF_TX_DS_MASK | NRF_RX_DR_MASK;
    SPI_MasterReadWrite(payldConfig.spiSfr, rxData, txData, 2); 
    
    uint64_t txData64 = 0;
    
    /* Configure RX_PIPE_0_ADDR for ACK payload */
    txData64 = (payldConfig.pipeAddr << 8) | NRF_WRITE_CMD(NRF_RX_ADDR_P0_REG);
    SPI_MasterReadWrite(payldConfig.spiSfr, rxData, &txData64, 6);
    
    /* Configure TX_ADDR */
    txData64 = (payldConfig.pipeAddr << 8) | NRF_WRITE_CMD(NRF_TX_ADDR_REG);
    SPI_MasterReadWrite(payldConfig.spiSfr, rxData, &txData64, 6);

    /* Send combined command and data */
    txSize = (txSize > 32) ? 32 : txSize;   // Max 32 bytes per payload
    txData[0] = NRF_WRITE_TX_PL_CMD;
    txData[1] = 0x00;
    for(uint8_t i = 1; i <= txSize; i++)
    {
        txData[i] = *((uint8_t *)(txPtr+i-1));
    }
    SPI_MasterReadWrite(payldConfig.spiSfr, rxData, txData, txSize+1);
    
    /* Start transmission */
    PIO_ClearPin(payldConfig.pinConfig.cePin);  // Clear if not cleared yet
    PIO_SetPin(payldConfig.pinConfig.cePin);
    TMR_DelayUs(15);
    PIO_ClearPin(payldConfig.pinConfig.cePin);
    
    /* Use Core timer for timeout of unresponsive device */
    uint32_t delay = timeoutVal * (sysFreq / 1000 / 2);
    timeout = _CP0_GET_COUNT() + delay;

    /* Wait for nRF response */
    while( PIO_ReadPin(payldConfig.pinConfig.irqPin) && (timeout > _CP0_GET_COUNT()) );

    /* Check nRF status */
    txData[0] = NRF_NOP_CMD;
    SPI_MasterReadWrite(payldConfig.spiSfr, rxData, txData, 1);
    statusFlag = (rxData[0] & 0x70);
    
    bool retVal = true;
    
    /* Payload with ACK */
    if( statusFlag == NRF_FLAG_ACK_PLD )
    {
        /* Payload width check */
        txData[0] = NRF_READ_RX_PL_WID_CMD;
        txData[1] = 0x00;
        SPI_MasterReadWrite(payldConfig.spiSfr, rxData, txData, 2);
        uint8_t payldWidth = rxData[1];

        /* Read payload */
        txData[0] = NRF_READ_RX_PL_CMD;
        SPI_MasterReadWrite(payldConfig.spiSfr, rxData, txData, payldWidth+1);
        
        /* Dump status and copy ACK payload */
        for(uint8_t i = 0; i < payldWidth; i++)
        {
            *((uint8_t *)rxPtr + i) = rxData[1 + i];
        }
    }
    /* Link lost or unresponsive device or successful send without ACK */
    else
    {
        /* True return if send done with/without ACK */
        retVal = (statusFlag == NRF_FLAG_TX_DS) ? true : false;
    }
    
    /* Clear device status */
    txData[0] = NRF_WRITE_CMD(NRF_STATUS_REG);
    txData[1] = NRF_MAX_RT_MASK | NRF_TX_DS_MASK | NRF_RX_DR_MASK;
    SPI_MasterReadWrite(payldConfig.spiSfr, rxData, txData, 2); 
    
    /* Disable current slave */
    SPI_DisableSsState(payldConfig.pinConfig.csPin);
    
    return retVal;   
}


/*
 *  Loads TX FIFO and sends data (ISR based)
 */
extern bool NRF_SendPayload(NrfPayloadConfig_t payldConfig, void *rxPtr, void *txPtr, uint8_t txSize)
{
    /* Reset status */
    statusFlag = NRF_FLAG_NO_STATUS;
    
    /* Enable current slave */
    SPI_EnableSsState(payldConfig.pinConfig.csPin);
    
    /* Configure ISR variables */
    isrRxPtr = rxPtr;
    isrPayldConfig = payldConfig;
    
    /* Configure ISR handler */
    IsrHandlerPtrConfig(ISR_NRF_MODE_0);
    
    /* Flush TX + RX FIFO */
    txData[0] = NRF_FLUSH_TX_CMD;
    SPI_MasterReadWrite(payldConfig.spiSfr, rxData, txData, 1);
    txData[0] = NRF_FLUSH_RX_CMD;
    SPI_MasterReadWrite(payldConfig.spiSfr, rxData, txData, 1);
    
    /* Clear device status - in case of previous MAX_RT */
    txData[0] = NRF_WRITE_CMD(NRF_STATUS_REG);
    txData[1] = NRF_MAX_RT_MASK | NRF_TX_DS_MASK | NRF_RX_DR_MASK;
    SPI_MasterReadWrite(payldConfig.spiSfr, rxData, txData, 2); 
    
    uint64_t txData64 = 0;
    
    /* Configure RX_PIPE_0_ADDR for ACK payload */
    txData64 = (payldConfig.pipeAddr << 8) | NRF_WRITE_CMD(NRF_RX_ADDR_P0_REG);
    SPI_MasterReadWrite(payldConfig.spiSfr, rxData, &txData64, 6);
    
    /* Configure TX_ADDR */
    txData64 = (payldConfig.pipeAddr << 8) | NRF_WRITE_CMD(NRF_TX_ADDR_REG);
    SPI_MasterReadWrite(payldConfig.spiSfr, rxData, &txData64, 6);

    /* Send combined command and data */
    txSize = (txSize > 32) ? 32 : txSize;   // Max 32 bytes per payload
    txData[0] = NRF_WRITE_TX_PL_CMD;
    txData[1] = 0x00;
    for(uint8_t i = 1; i <= txSize; i++)
    {
        txData[i] = *((uint8_t *)(txPtr+i-1));
    }
    
    /* Start transmission after packet upload */
    SPI_MasterWrite2(payldConfig.spiSfr, NULL, txData, txSize+1, ISR_NrfHandler_StartTransmission);

    return true;
}


/*
 *  Starts RX mode for PRX
 */
extern bool NRF_StartReception(NrfPayloadConfig_t payldConfig, void *rxPtr)
{
    /* Enable current slave */
    SPI_EnableSsState(payldConfig.pinConfig.csPin);
    
    /* Configure ISR variables */
    isrRxPtr = rxPtr;
    isrPayldConfig = payldConfig;
    
    /* Configure ISR handler */
    IsrHandlerPtrConfig(ISR_NRF_MODE_1);
    
    /* Wait if ACK payload is being loaded */
    while( isRxFifoLoading == true );
    
    /* Flush RX FIFO */
    txData[0] = NRF_FLUSH_RX_CMD;
    SPI_MasterReadWrite(payldConfig.spiSfr, rxData, txData, 1);
    
    /* Clear device status  */
    txData[0] = NRF_WRITE_CMD(NRF_STATUS_REG);
    txData[1] = NRF_MAX_RT_MASK | NRF_TX_DS_MASK | NRF_RX_DR_MASK;
    SPI_MasterReadWrite(payldConfig.spiSfr, rxData, txData, 2); 
    
    /* INTx interrupt source enabled */
    icSfr->ICxIFS0.CLR = NRF_INTxIF_MASK;
    icSfr->ICxIEC0.SET = NRF_INTxIE_MASK;
    
    /* Start reception */
    PIO_ClearPin(payldConfig.pinConfig.cePin);
    PIO_SetPin(payldConfig.pinConfig.cePin);
    
    return true;
}


/*
 *  Stops RX mode for PRX
 */
extern bool NRF_StopReception(NrfPayloadConfig_t payldConfig)
{
    /* INTx interrupt source disabled */
    icSfr->ICxIFS0.CLR = NRF_INTxIF_MASK;
    icSfr->ICxIEC0.CLR = NRF_INTxIE_MASK;
    
    /* Stop reception */
    PIO_ClearPin(payldConfig.pinConfig.cePin);
    
    /* Clear device status (just in case) */
    txData[0] = NRF_WRITE_CMD(NRF_STATUS_REG);
    txData[1] = NRF_MAX_RT_MASK | NRF_TX_DS_MASK | NRF_RX_DR_MASK;
    SPI_MasterReadWrite(payldConfig.spiSfr, rxData, txData, 2); 
    
    /* Disable current slave */
    SPI_DisableSsState(payldConfig.pinConfig.csPin);
    
    return true;
}


/*
 *  Load ACK payload into the PRX TX FIFO
 */
extern bool NRF_StoreAckPayload(NrfPayloadConfig_t payldConfig, NrfRxPipeNo_t pipeNo, void *txPtr, uint8_t txSize)
{
    /* Temporarily disable reception */
    PIO_ClearPin(payldConfig.pinConfig.cePin);
    
    /* Enable current slave */
    SPI_EnableSsState(payldConfig.pinConfig.csPin);
    
    /* Configure ISR variable */
    isrPayldConfig = payldConfig;
    
    /* Reception may not be started until ACK payload is loaded or the reception
     * is temporarily disabled if already active */
    isRxFifoLoading = true;
    
    /* Load combined command and data */
    txSize = (txSize > 32) ? 32 : txSize;       // Max 32 bytes per payload
    txData[0] = NRF_WRITE_ACK_PL_CMD(pipeNo);
    txData[1] = 0x00;
    for(uint8_t i = 1; i <= txSize; i++)
    {
        txData[i] = *((uint8_t *)(txPtr+i-1));
    }
    
    SPI_MasterWrite2(payldConfig.spiSfr, NULL, txData, txSize+1, ISR_NrfHandler_RestartReception);
    
    return true;
}


/*
 *  Reads status of the latest nRF operation
 */
extern NrfStatusFlag_t NRF_ReadStatus(void)
{
    NrfStatusFlag_t tempFlag = statusFlag;
    
    /* Status valid after ISR done */
    if( (icSfr->ICxIEC0.W & NRF_INTxIE_MASK) && (icSfr->ICxIFS0.W & NRF_INTxIF_MASK) )
    {
        return NRF_FLAG_NO_STATUS;
    }
    else
    {    
        statusFlag = NRF_FLAG_NO_STATUS;    // Reset after new status update
        return tempFlag;
    }
}


/*
 *  Flushes the RX FIFO (intended for PRX use)
 */
extern void NRF_FlushRxFifo(NrfPayloadConfig_t payldConfig)
{
    /* Flush TX FIFO */
    txData[0] = NRF_FLUSH_TX_CMD;
    SPI_MasterReadWrite(payldConfig.spiSfr, rxData, txData, 1);
}


/*
 *  Checks if ACK payload is being loaded in the RX FIFO
 */
extern bool NRF_IsRxFifoLoading(void)
{
    return isRxFifoLoading;
}


/*
 *  Reads pipe address of the latest reception
 */
extern uint64_t NRF_ReadPrxPipeAddr(void)
{
    NrfRxPipeNo_t tempNo = rxPipeNo;
    
    rxPipeNo = NRF_RX_NO_PIPE;      // Reset pipe state (number)
    
    /* Data in valid pipe number */
    if( tempNo != NRF_RX_NO_PIPE)
    {
        return rxPipeAddr[tempNo];
    }
    /* Data haven't arrived yet */
    else
    {
        return 0;
    }
}


/******************************************************************************/
/*------------------------Local Function Definitions--------------------------*/
/******************************************************************************/

/*
 *  Configures TX and RX ISR handler function pointers
 */
static void IsrHandlerPtrConfig(IsrNrfMode_t isrMode)
{
    switch( isrMode )
    {
        /* Standard nRF payload transmission */
        case ISR_NRF_MODE_0:
            isrHandlerPtr = ISR_NrfHandler_ReadAckPayload;
            break;
        /* Standard nRF payload reception */
        case ISR_NRF_MODE_1:
            isrHandlerPtr = ISR_NrfHandler_ReadPayload;
            break;
        default:
            break;
    }
}


/*
 *  ISR handler for NRF_SendPayload()
 */
static void ISR_NrfHandler_ReadAckPayload(void)
{
    /* Read and clear nRF status */
    txData[0] = NRF_WRITE_CMD(NRF_STATUS_REG);
    txData[1] = NRF_MAX_RT_MASK | NRF_TX_DS_MASK | NRF_RX_DR_MASK;
    SPI_MasterReadWrite(isrPayldConfig.spiSfr, rxData, txData, 2);
    statusFlag = (rxData[0] & 0x70);
    
    /* Payload with ACK */
    if( statusFlag == NRF_FLAG_ACK_PLD )
    {
        /* Payload width check */
        txData[0] = NRF_READ_RX_PL_WID_CMD;
        txData[1] = 0x00;
        SPI_MasterReadWrite(isrPayldConfig.spiSfr, rxData, txData, 2);
        isrPayldWidth = rxData[1];

        /* Read payload */
        nullData[0] = NRF_READ_RX_PL_CMD;
        nullData[1] = 0x00;
        SPI_MasterWrite2(isrPayldConfig.spiSfr, rxData, nullData, isrPayldWidth+1, ISR_NrfHandler_SendPayloadCont);
    }
    /* Link lost or successful send without ACK */
    else
    {
        /* Disable INTx interrupt source */
        icSfr->ICxIEC0.CLR = NRF_INTxIE_MASK;
        icSfr->ICxIFS0.CLR = NRF_INTxIF_MASK;
    }
    
    /* Call user callback */
    if (userClbkReadAckPayload != NULL) {
        userClbkReadAckPayload();
    }
}


/*
 *  Handles reading of RX FIFO during enabled reception
 *  Initiated by the NRF_StartReception()
 */
static void ISR_NrfHandler_ReadPayload(void)
{
    /* Read and clear nRF status */
    txData[0] = NRF_WRITE_CMD(NRF_STATUS_REG);
    txData[1] = NRF_MAX_RT_MASK | NRF_TX_DS_MASK | NRF_RX_DR_MASK;
    SPI_MasterReadWrite(isrPayldConfig.spiSfr, rxData, txData, 2);
    statusFlag = (rxData[0] & 0x70);
    rxPipeNo = (rxData[0] & 0x0E) >> 1;
    
    /* Payload in RX FIFO */
    if( (statusFlag == NRF_FLAG_RX_DR) || (statusFlag == NRF_FLAG_ACK_PLD) )
    {
        /* Stop reception */
        PIO_ClearPin(isrPayldConfig.pinConfig.cePin);
        
        /* Payload width check */
        txData[0] = NRF_READ_RX_PL_WID_CMD;
        txData[1] = 0x00;
        SPI_MasterReadWrite(isrPayldConfig.spiSfr, rxData, txData, 2);
        uint8_t payldWidth = rxData[1];

        /* Read payload */
        nullData[0] = NRF_READ_RX_PL_CMD;
        nullData[1] = 0x00;
        SPI_MasterWrite2(isrPayldConfig.spiSfr, rxData, nullData, payldWidth+1, ISR_NrfHandler_ReadPayloadCont);
    }
    /* PTX couldn't establish link */
    else
    {
        /* Clear flag only */
        icSfr->ICxIFS0.CLR = NRF_INTxIF_MASK;
    }
    
    /* Call user callback */
    if (userClbkReadPayload != NULL) {
        userClbkReadPayload();
    }
}


/*
 *  ISR handler for ISR_NrfHandler_SendPayload() (executed within scope of SPI
 *  ISR), where the former is initially triggered by the NRF_SendPayload()
 */
static void ISR_NrfHandler_SendPayloadCont(void)
{
    /* Disable INTx interrupt source */
    icSfr->ICxIEC0.CLR = NRF_INTxIE_MASK;
    icSfr->ICxIFS0.CLR = NRF_INTxIF_MASK;
    
    /* Disable current slave */
    SPI_EnableSsState(isrPayldConfig.pinConfig.csPin);
    
    /* "isrRxPtr" starts at initial "rxPtr" address,
     * while "rxData" is a temporary storage */
    /* Dump status and copy ACK payload */
    for(uint8_t i = 0; i < isrPayldWidth; i++)
    {
        *((uint8_t *)isrRxPtr + i) = rxData[1 + i];
    }
}


/*
 *  ISR handler for ISR_NrfHandler_ReadPayload() (executed within scope of SPI
 *  ISR), where the former is initially triggered by the NRF_StartReception()
 */
static void ISR_NrfHandler_ReadPayloadCont(void)
{
    /* Clear flag only */
    icSfr->ICxIFS0.CLR = NRF_INTxIF_MASK;
    
    /* "isrRxPtr" starts at initial "rxPtr" address,
     * while "rxData" is a temporary storage */
    /* Dump status and copy ACK payload */
    for(uint8_t i = 0; i < isrPayldWidth; i++)
    {
        *((uint8_t *)isrRxPtr + i) = rxData[1 + i];
    }
    
    /* Start new reception */
    PIO_SetPin(isrPayldConfig.pinConfig.cePin);
}


/*
 *  ISR handler for NRF_SendPayload() (executed within scope of SPI ISR)
 */
static void ISR_NrfHandler_StartTransmission(void)
{
    /* Start transmission */
    PIO_ClearPin(isrPayldConfig.pinConfig.cePin);  // Clear if not cleared yet
    PIO_SetPin(isrPayldConfig.pinConfig.cePin);
    TMR_DelayUs(15);
    PIO_ClearPin(isrPayldConfig.pinConfig.cePin);
    
    /* Use Core timer for timeout of unresponsive device */
    timeoutCount = 0;
    isTimeoutEnabled = true;
    
    /* INTx interrupt source enabled */
    icSfr->ICxIFS0.CLR = NRF_INTxIF_MASK;
    icSfr->ICxIEC0.SET = NRF_INTxIE_MASK;
    
    /* Call user callback */
    if (userClbkStartTransmission != NULL) {
        userClbkStartTransmission();
    }
}

/*
 *  ISR handler for NRF_StoreAckPayload() (executed within scope of SPI ISR)
 */
static void ISR_NrfHandler_RestartReception(void)
{    
    /* Continue reception if initially started by NRF_StartReception() */
    if( icSfr->ICxIEC0.W & NRF_INTxIE_MASK )
    {
        PIO_SetPin(isrPayldConfig.pinConfig.cePin);
    }
    
    /* Reception may be activated (if not already) */
    isRxFifoLoading = false;
}

/*
 *  ISR handler for timeout of NRF_SendPayload()
 */
static void ISR_NrfTimeoutHandler_SendPayload(void)
{
    timeoutCount++;
    if( (timeoutCount > timeoutVal) && (isTimeoutEnabled == true) )
    {
        statusFlag = NRF_FLAG_NO_RP;    // No response status

        /* Try clearing status even in case of unresponsive device */
        txData[0] = NRF_WRITE_CMD(NRF_STATUS_REG);
        txData[1] = NRF_MAX_RT_MASK | NRF_TX_DS_MASK | NRF_RX_DR_MASK;
        SPI_MasterReadWrite(isrPayldConfig.spiSfr, rxData, txData, 2);

        /* Disable current slave */
        SPI_DisableSsState(isrPayldConfig.pinConfig.csPin);
        
        timeoutCount = 0;
        isTimeoutEnabled = false;
    }
    
    /* Call user callback */
    if (userClbkPayloadTimeout != NULL) {
        userClbkPayloadTimeout();
    }
}

/*
 *  Determines which external interrupt source is used based on pin code (which
 *  triggers ISR) and configures interrupt SFRs
 */
INLINE static void InterruptSfrConfig(const uint32_t pinCode)
{
    /* Check which INTx is used by PPS register code */
    uint8_t regCode = pinCode & 0xFF;
    
    icSfr->ICxIEC0.CLR = NRF_INTxIE_MASK;       // Disable source
    
    /* External interrupt INT0 */
    if( regCode == 0xFF )
    {
        icSfr->ICxIPC0.CLR = (IC_INT0IS_MASK | IC_INT0IP_MASK);                                 // Clear (sub)priority
        icSfr->ICxIPC0.SET = ((NRF_ICX_IPL << IC_INT0IS_POS) | (NRF_ICX_ISL << IC_INT0IP_POS)); // Set (sub)priority
        icSfr->ICxINTCON.CLR = IC_INT0EP_MASK;  // Falling-edge triggered
    }
    /* External interrupt INT1 */
    else if( regCode == 0x04 )
    {
        icSfr->ICxIPC1.CLR = (IC_INT1IS_MASK | IC_INT1IP_MASK);
        icSfr->ICxIPC1.SET = ((NRF_ICX_IPL << IC_INT1IS_POS) | (NRF_ICX_ISL << IC_INT1IP_POS));
        icSfr->ICxINTCON.CLR = IC_INT1EP_MASK;
    }
    /* External interrupt INT2 */
    else if( regCode == 0x08 )
    {
        icSfr->ICxIPC2.CLR = (IC_INT2IS_MASK | IC_INT2IP_MASK);
        icSfr->ICxIPC2.SET = ((NRF_ICX_IPL << IC_INT2IS_POS) | (NRF_ICX_ISL << IC_INT2IP_POS));
        icSfr->ICxINTCON.CLR = IC_INT2EP_MASK;
    }
    /* External interrupt INT3 */
    else if( regCode == 0x0C )
    {
        icSfr->ICxIPC3.CLR = (IC_INT3IS_MASK | IC_INT3IP_MASK);
        icSfr->ICxIPC3.SET = ((NRF_ICX_IPL << IC_INT3IS_POS) | (NRF_ICX_ISL << IC_INT3IP_POS));
        icSfr->ICxINTCON.CLR = IC_INT3EP_MASK;
    }
    /* External interrupt INT4 */
    else if( regCode == 0x10 )
    {
        icSfr->ICxIPC4.CLR = (IC_INT4IS_MASK | IC_INT4IP_MASK);
        icSfr->ICxIPC4.SET = ((NRF_ICX_IPL << IC_INT4IS_POS) | (NRF_ICX_ISL << IC_INT4IP_POS));
        icSfr->ICxINTCON.CLR = IC_INT4EP_MASK;
    }
    /* False input */
    else
    {
        
    }
    
    icSfr->ICxIFS0.CLR = NRF_INTxIF_MASK;       // Clear flag
}

/******************************************************************************/
/*-----------------------------ISR  Definition--------------------------------*/
/******************************************************************************/

/*
 *  ISR handler for nRF operation
 */
void __ISR(NRF_ISR_VECTOR, NRF_ISR_IPL) ISR_Nrf(void)
{
    if( (icSfr->ICxIEC0.W & NRF_INTxIE_MASK) && (icSfr->ICxIFS0.W & NRF_INTxIF_MASK) )
    {   
        isrHandlerPtr();
    } 
}