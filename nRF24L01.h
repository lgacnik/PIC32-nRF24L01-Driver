#ifndef NRF24L01_H
#define	NRF24L01_H


/******************************************************************************/
/*----------------------------------Includes----------------------------------*/
/******************************************************************************/

/** Compiler libs **/
//#include <sys/attribs.h>

/** Custom libs **/
#include "nRF24L01_sfr.h"
#include "Spi.h"
#include "Tmr.h"

/******************************************************************************/
/*---------------------------------MACROS-------------------------------------*/
/******************************************************************************/

/**************************Interrupt vector priority***************************/

/* NOTE: IPL = 0 means interrupt disabled. ISR_IPL level must equal ICX_IPL */
/* NOTE: Vectors of same priority and sub-priority are services in their
 *       natural order */

/* User-defined (sub)priority levels (IPL: 0-7, ISL: 0-3) */
#define NRF_ISR_IPL     IPL1SOFT
#define NRF_ICX_IPL     1
#define NRF_ICX_ISL     1

/******************************************************************************/

/* User-defined External Interrupt INTx vector */
#define INT2_ISR_MACRO


/* Macro used by the ISR definitions in nRF library */
#if defined INT0_ISR_MACRO

    #define NRF_ISR_VECTOR      EXTERNAL_0_VECTOR
    #define NRF_INTxIF_MASK     IC_INT0IF_MASK
    #define NRF_INTxIE_MASK     IC_INT0IE_MASK

#elif defined INT1_ISR_MACRO

    #define NRF_ISR_VECTOR      EXTERNAL_1_VECTOR
    #define NRF_INTxIF_MASK     IC_INT1IF_MASK
    #define NRF_INTxIE_MASK     IC_INT1IE_MASK

#elif defined INT2_ISR_MACRO

    #define NRF_ISR_VECTOR      EXTERNAL_2_VECTOR
    #define NRF_INTxIF_MASK     IC_INT2IF_MASK
    #define NRF_INTxIE_MASK     IC_INT2IE_MASK

#elif defined INT3_ISR_MACRO

    #define NRF_ISR_VECTOR      EXTERNAL_3_VECTOR
    #define NRF_INTxIF_MASK     IC_INT3IF_MASK
    #define NRF_INTxIE_MASK     IC_INT3IE_MASK

#elif defined INT4_ISR_MACRO

    #define NRF_ISR_VECTOR      EXTERNAL_4_VECTOR
    #define NRF_INTxIF_MASK     IC_INT4IF_MASK
    #define NRF_INTxIE_MASK     IC_INT4IE_MASK

#else

    #error "Define INTx vector for nRF24L01.c"

#endif

/******************************************************************************/
/*----------------------------Enumeration Types-------------------------------*/
/******************************************************************************/

/** Type for ISR function pointer configuration **/

typedef enum {
    ISR_NRF_MODE_0 = 0,
    ISR_NRF_MODE_1 = 1,
    ISR_NRF_MODE_2 = 2
} IsrNrfMode_t;


/** Types for SFR bit-field values **/

typedef enum {
    NRF_NO_ACK = 0,
    NRF_ACK = 1
} NrfIsAck_t;

typedef enum {
    NRF_ARD_250 = 0,
    NRF_ARD_500 = 1,
    NRF_ARD_750 = 2,
    NRF_ARD_1000 = 3,
    NRF_ARD_1250 = 4,
    NRF_ARD_1500 = 5,
    NRF_ARD_1750 = 6,
    NRF_ARD_2000 = 7,
    NRF_ARD_2250 = 8,
    NRF_ARD_2500 = 9,
    NRF_ARD_2750 = 10,
    NRF_ARD_3000 = 11,
    NRF_ARD_3250 = 12,
    NRF_ARD_3500 = 13,
    NRF_ARD_3750 = 14,
    NRF_ARD_4000 = 15            
} NrfRetransmitDelay_t;

typedef enum {
    NRF_ARC_DI = 0,
    NRF_ARC_1 = 1,
    NRF_ARC_2 = 2,
    NRF_ARC_3 = 3,
    NRF_ARC_4 = 4,
    NRF_ARC_5 = 5,
    NRF_ARC_6 = 6,
    NRF_ARC_7 = 7,
    NRF_ARC_8 = 8,
    NRF_ARC_9 = 9,
    NRF_ARC_10 = 10,
    NRF_ARC_11 = 11,
    NRF_ARC_12 = 12,
    NRF_ARC_13 = 13,
    NRF_ARC_14 = 14,
    NRF_ARC_15 = 15  
} NrfRetransmitCount_t;

typedef enum {
    NRF_RF_CH_0 = 0,
    NRF_RF_CH_1 = 1,
    NRF_RF_CH_2 = 2,
    NRF_RF_CH_3 = 3,
    NRF_RF_CH_4 = 4,
    NRF_RF_CH_5 = 5,
    NRF_RF_CH_6 = 6,
    NRF_RF_CH_7 = 7,
    NRF_RF_CH_8 = 8,
    NRF_RF_CH_9 = 9,
    NRF_RF_CH_10 = 10,
    NRF_RF_CH_11 = 11,
    NRF_RF_CH_12 = 12,
    NRF_RF_CH_13 = 13,
    NRF_RF_CH_14 = 14,
    NRF_RF_CH_15 = 15,
    NRF_RF_CH_16 = 16,
    NRF_RF_CH_17 = 17,
    NRF_RF_CH_18 = 18,
    NRF_RF_CH_19 = 19,
    NRF_RF_CH_20 = 20,
    NRF_RF_CH_21 = 21,
    NRF_RF_CH_22 = 22,
    NRF_RF_CH_23 = 23,
    NRF_RF_CH_24 = 24,
    NRF_RF_CH_25 = 25,
    NRF_RF_CH_26 = 26,
    NRF_RF_CH_27 = 27,
    NRF_RF_CH_28 = 28,
    NRF_RF_CH_29 = 29,
    NRF_RF_CH_30 = 30,
    NRF_RF_CH_31 = 31,
    NRF_RF_CH_32 = 32,
    NRF_RF_CH_33 = 33,
    NRF_RF_CH_34 = 34,
    NRF_RF_CH_35 = 35,
    NRF_RF_CH_36 = 36,
    NRF_RF_CH_37 = 37,
    NRF_RF_CH_38 = 38,
    NRF_RF_CH_39 = 39,
    NRF_RF_CH_40 = 40,
    NRF_RF_CH_41 = 41,
    NRF_RF_CH_42 = 42,
    NRF_RF_CH_43 = 43,
    NRF_RF_CH_44 = 44,
    NRF_RF_CH_45 = 45,
    NRF_RF_CH_46 = 46,
    NRF_RF_CH_47 = 47,
    NRF_RF_CH_48 = 48,
    NRF_RF_CH_49 = 49,
    NRF_RF_CH_50 = 50,
    NRF_RF_CH_51 = 51,
    NRF_RF_CH_52 = 52,
    NRF_RF_CH_53 = 53,
    NRF_RF_CH_54 = 54,
    NRF_RF_CH_55 = 55,
    NRF_RF_CH_56 = 56,
    NRF_RF_CH_57 = 57,
    NRF_RF_CH_58 = 58,
    NRF_RF_CH_59 = 59,
    NRF_RF_CH_60 = 60,
    NRF_RF_CH_61 = 61,
    NRF_RF_CH_62 = 62,
    NRF_RF_CH_63 = 63,
    NRF_RF_CH_64 = 64,
    NRF_RF_CH_65 = 65,
    NRF_RF_CH_66 = 66,
    NRF_RF_CH_67 = 67,
    NRF_RF_CH_68 = 68,
    NRF_RF_CH_69 = 69,
    NRF_RF_CH_70 = 70,
    NRF_RF_CH_71 = 71,
    NRF_RF_CH_72 = 72,
    NRF_RF_CH_73 = 73,
    NRF_RF_CH_74 = 74,
    NRF_RF_CH_75 = 75,
    NRF_RF_CH_76 = 76,
    NRF_RF_CH_77 = 77,
    NRF_RF_CH_78 = 78,
    NRF_RF_CH_79 = 79,
    NRF_RF_CH_80 = 80,
    NRF_RF_CH_81 = 81,
    NRF_RF_CH_82 = 82,
    NRF_RF_CH_83 = 83,
    NRF_RF_CH_84 = 84,
    NRF_RF_CH_85 = 85,
    NRF_RF_CH_86 = 86,
    NRF_RF_CH_87 = 87,
    NRF_RF_CH_88 = 88,
    NRF_RF_CH_89 = 89,
    NRF_RF_CH_90 = 90,
    NRF_RF_CH_91 = 91,
    NRF_RF_CH_92 = 92,
    NRF_RF_CH_93 = 93,
    NRF_RF_CH_94 = 94,
    NRF_RF_CH_95 = 95,
    NRF_RF_CH_96 = 96,
    NRF_RF_CH_97 = 97,
    NRF_RF_CH_98 = 98,
    NRF_RF_CH_99 = 99,
    NRF_RF_CH_100 = 100,
    NRF_RF_CH_101 = 101,
    NRF_RF_CH_102 = 102,
    NRF_RF_CH_103 = 103,
    NRF_RF_CH_104 = 104,
    NRF_RF_CH_105 = 105,
    NRF_RF_CH_106 = 106,
    NRF_RF_CH_107 = 107,
    NRF_RF_CH_108 = 108,
    NRF_RF_CH_109 = 109,
    NRF_RF_CH_110 = 110,
    NRF_RF_CH_111 = 111,
    NRF_RF_CH_112 = 112,
    NRF_RF_CH_113 = 113,
    NRF_RF_CH_114 = 114,
    NRF_RF_CH_115 = 115,
    NRF_RF_CH_116 = 116,
    NRF_RF_CH_117 = 117,
    NRF_RF_CH_118 = 118,
    NRF_RF_CH_119 = 119,
    NRF_RF_CH_120 = 120,
    NRF_RF_CH_121 = 121,
    NRF_RF_CH_122 = 122,
    NRF_RF_CH_123 = 123,
    NRF_RF_CH_124 = 124,
    NRF_RF_CH_125 = 125,
    NRF_RF_CH_126 = 126,
    NRF_RF_CH_127 = 127
} NrfRfChannel_t;

typedef enum {
    NRF_RF_DR_250 = 2,
    NRF_RF_DR_1000 = 0,
    NRF_RF_DR_2000 = 1
} NrfDataRate_t;

typedef enum {
    NRF_RF_PWR_MIN = 0,
    NRF_RF_PWR_LOW = 1,
    NRF_RF_PWR_HIGH = 2,
    NRF_RF_PWR_MAX = 3
} NrfRfPower_t;

typedef enum {
    NRF_RX_PIPE_0 = 0,
    NRF_RX_PIPE_1 = 1,
    NRF_RX_PIPE_2 = 2,
    NRF_RX_PIPE_3 = 3,
    NRF_RX_PIPE_4 = 4,
    NRF_RX_PIPE_5 = 5,
    NRF_RX_NO_PIPE = 7
} NrfRxPipeNo_t;

typedef enum {
    NRF_FLAG_NO_RP = 0x00,
    NRF_FLAG_MAX_RT = 0x10,
    NRF_FLAG_TX_DS = 0x20,
    NRF_FLAG_RX_DR = 0x40,
    NRF_FLAG_ACK_PLD = 0x60,
    NRF_FLAG_NO_STATUS = 0xFF,
} NrfStatusFlag_t;

typedef enum {
    NRF_CLBK_RX_PAYLOAD_RECEIVE = 0,
    NRF_CLBK_TX_ACK_PAYLOAD_RECEIVE = 1,
    NRF_CLBK_TX_START = 3,
    NRF_CLBK_TX_TIMEOUT = 4,
} NrfUserCallback_t;

/******************************************************************************/
/*-----------------------------Data Structures--------------------------------*/
/******************************************************************************/

/* Pin settings for nRF device (SPI pins handled by SpiStandardConfig_t type) */
typedef struct {
    uint32_t    cePin;
    uint32_t    csPin;
    uint32_t    irqPin;
} NrfPinConfig_t;

/* Pipe address structure for PRX */
typedef struct {
    uint64_t    pipe0;
    uint64_t    pipe1;
    uint64_t    pipe2;
    uint64_t    pipe3;
    uint64_t    pipe4;
    uint64_t    pipe5;
} const NrfPipeAddr_t;

/* NRF24L01+ configuration of transmitter properties */
typedef struct {
    SpiSfr_t *const         spiSfr;
    NrfIsAck_t              isAck;
    NrfRetransmitDelay_t    retrDelay;
    NrfRetransmitCount_t    retrCount;
    NrfRfChannel_t          rfChannel;
    NrfRfPower_t            rfPower;
    NrfDataRate_t           dataRate;
    NrfPinConfig_t          pinConfig;
} const NrfPtxConfig_t;

/* NRF24L01+ configuration of receiver properties */
typedef struct {
    SpiSfr_t *const         spiSfr;
    NrfIsAck_t              isAck;
    NrfRfChannel_t          rfChannel;
    NrfDataRate_t           dataRate;
    NrfPipeAddr_t           pipeAddr;
    NrfPinConfig_t          pinConfig;
} const NrfPrxConfig_t;

/* Payload configuration for device addressing */
typedef struct {
    SpiSfr_t               *spiSfr;
    NrfPinConfig_t          pinConfig;
    uint64_t                pipeAddr;       // Only for PTX
} NrfPayloadConfig_t;

/******************************************************************************/
/*---------------------------- Function Prototypes----------------------------*/
/******************************************************************************/

/* PTX functions */
bool NRF_ConfigPtxSfr(const NrfPtxConfig_t ptxConfig);
bool NRF_SendReceivePayload(NrfPayloadConfig_t payldConfig, void *rxPtr, void *txPtr, uint8_t txSize);
bool NRF_SendPayload(NrfPayloadConfig_t payldConfig, void *rxPtr, void *txPtr, uint8_t txSize);
INLINE NrfPayloadConfig_t NRF_ConfigPtxPayloadStruct(NrfPtxConfig_t ptxConfig, const uint64_t pipeAddr);

/* PRX functions */
bool NRF_ConfigPrxSfr(const NrfPrxConfig_t prxConfig);
bool NRF_StoreAckPayload(NrfPayloadConfig_t payldConfig, NrfRxPipeNo_t pipeNo, void *txPtr, uint8_t txSize);
bool NRF_StartReception(NrfPayloadConfig_t payldConfig, void *rxPtr);
bool NRF_StopReception(NrfPayloadConfig_t payldConfig);
void NRF_FlushRxFifo(NrfPayloadConfig_t payldConfig);
bool NRF_IsRxFifoLoading(void);
uint64_t NRF_ReadPrxPipeAddr(void);
INLINE NrfPayloadConfig_t NRF_ConfigPrxPayloadStruct(NrfPrxConfig_t prxConfig);

/* PTX and PRX functions */
NrfStatusFlag_t NRF_ReadStatus(void);
void NRF_SetUserCallback(NrfUserCallback_t cType, void (*fPtr)(void));
void NRF_ReleaseUserCallback(NrfUserCallback_t cType);

/******************************************************************************/
/*-----------------------------Function In-lines------------------------------*/
/******************************************************************************/

/*
 *  Copy structure data from configuration to payload structure
 */
INLINE NrfPayloadConfig_t NRF_ConfigPtxPayloadStruct(NrfPtxConfig_t ptxConfig, const uint64_t pipeAddr)
{
    NrfPayloadConfig_t payldConfig = {
        .spiSfr = ptxConfig.spiSfr,
        .pinConfig = ptxConfig.pinConfig,
        .pipeAddr = pipeAddr,
    };
    
    return payldConfig;
}


/*
 *  Copy structure data from configuration to payload structure
 */
INLINE NrfPayloadConfig_t NRF_ConfigPrxPayloadStruct(NrfPrxConfig_t prxConfig)
{
    NrfPayloadConfig_t payldConfig = {
        .spiSfr = prxConfig.spiSfr,
        .pinConfig = prxConfig.pinConfig,
        .pipeAddr = 0,
    };
    
    return payldConfig;
}


#endif	/* NRF24L01_H */