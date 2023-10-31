/** Compiler libs **/
#include <xc.h>             // Using standard macros and register access for debug

/** Custom libs **/
#include "ConfigBits.h"     // Provide configuration bits, specific to PIC32 devices
#include "nRF24L01.h"

/** Test macros **/
#define PTX_CS          GPIO_RPB7
#define PTX_CE          GPIO_RPB10
#define PTX_SDI         SDI1_RPB8
#define PTX_SDO         SDO1_RPB11
//#define PTX_SCK       GPIO_RPB14      // Fixed pin for SPI1 module
#define PTX_IRQ         INT2_RPB13
#define PTX_SPI_MODULE  SPI1_MODULE
#define PTX_ADDR        (0xB3B4B5B605)

/* Test callbacks */
void TxStartCallback(void);
void TxAckCallback(void);
void TxTimeoutCallback(void);

int main(int argc, char** argv)
{   
    /* Programmable I/O structure */
    struct {
        PioSfr_t   *A;
        PioSfr_t   *B;
    } pio;
    
    /* Oscillator configuration parameters */
    OscConfig_t oscConfig = {
        .oscSource = OSC_COSC_FRCPLL,
        .sysFreq = 40000000,
        .pbFreq = 40000000
    };

    /* SPI configuration parameters for PRX (receiver) */
    SpiStandardConfig_t spiPtxConfig = {
        .pinSelect = {
            .sdiPin = PTX_SDI,
            .sdoPin = PTX_SDO,
            .ss1Pin = PTX_CS
        /*   sckPin on RPB14   */
        },
        .isMasterEnabled = true,
        .frameWidth = SPI_WIDTH_8BIT,
        .sckFreq = 1000000,
        .clkMode = SPI_CLK_MODE_0
    };

    /* Configure pre-initialized oscillator module */
    OSC_ConfigOsc(oscConfig);
    
    /* Initialize SPI modules for nRF24L01+ communication */
    SPI_ConfigStandardModeSfr(&PTX_SPI_MODULE, spiPtxConfig);
    
    /* PTX configuration structure */
    NrfPtxConfig_t ptxConfig = {
        .spiSfr = &PTX_SPI_MODULE,
        .isAck = true,
        .retrDelay = NRF_ARD_1000,
        .retrCount = NRF_ARC_15,
        .rfChannel = NRF_RF_CH_2,
        .rfPower = NRF_RF_PWR_MIN,
        .dataRate = NRF_RF_DR_2000,
        .pinConfig = {
            .cePin = PTX_CE,
            .csPin = PTX_CS,
            .irqPin = PTX_IRQ
        }
    };

    /* Configure debug pin */
    pio.A = &PIOA_MODULE;
    pio.B = &PIOB_MODULE;  
    pio.B->PIOxTRIS.SET = PIO_TRISB3_MASK;
    pio.B->PIOxANSEL.CLR = PIO_ANSB3_MASK;
    
    /* Configure the device as a PTX */
    NRF_ConfigPtxSfr(ptxConfig);
    
    /* Configure payload structure */
    NrfPayloadConfig_t ptxPayloadConfig = NRF_ConfigPtxPayloadStruct(ptxConfig, PRX_ADDR);

    /* Set user callbacks */
    NRF_SetUserCallback(NRF_CLBK_TX_START, TxStartCallback);
    NRF_SetUserCallback(NRF_CLBK_TX_ACK_PAYLOAD_RECEIVE, TxAckCallback);
    NRF_SetUserCallback(NRF_CLBK_TX_TIMEOUT, TxTimeoutCallback);
    
    uint8_t txData[] = {0x5A, 0x32, 0x3D, 0x01, 0xD9, 0x56, 0x43, 0x5F,
                        0x4D, 0x3F, 0xE2, 0xFD, 0x55, 0x8E, 0xEE, 0xE7,
                        0x90, 0xFC, 0x57, 0xE1, 0xE8, 0x4C, 0xFD, 0xAC,
                        0x04, 0xAE, 0x45, 0xF5, 0xD3, 0x41, 0x20, 0x0D};
    
    /* Send data and wait for ACK payload */
    uint8_t ptxRxData[3];
    NRF_SendPayload(ptxPayloadConfig, ptxRxData, txData, sizeof(txData));
    
    /* Main program execution */
    while(1)
    {
        PIO_TogglePin(GPIO_RPA2);

        /* When payload is sent the 'TxStartCallback' is called from internal ISR handler */

        /* When ACK (with payload or no payload) arrives from PRX the 'TxAckCallback' is called */

        /* When transmit timeout happens (link failed) the 'TxTimeoutCallback' is called */
    }

    return 0;
}

void TxStartCallback(void)
{
  /* Toggle a pin or handle some data */
}

void TxAckCallback(void)
{
  /* Toggle a pin or handle some data */
}

void TxTimeoutCallback(void)
{
  /* Toggle a pin or handle some data */
}