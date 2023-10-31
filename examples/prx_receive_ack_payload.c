/** Compiler libs **/
#include <xc.h>         // Using standard macros and register access for debug

/** Custom libs **/
#include "ConfigBits.h" // Provide configuration bits, specific to PIC32 devices
#include "nRF24L01.h"

/** Test macros **/
#define PRX_CS          GPIO_RPB4
#define PRX_CE          GPIO_RPB3
#define PRX_SDI         SDI2_RPA4
#define PRX_SDO         SDO2_RPB5
//#define PRX_SCK       GPIO_RPB15      // Fixed pin for SPI2 module
#define PRX_IRQ         INT2_RPB13
#define PRX_SPI_MODULE  SPI2_MODULE
#define PRX_ADDR        (0xB3B4B5B605)

/* Test callback */
void RxCallback(void);

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
    SPI_ConfigStandardModeSfr(&PRX_SPI_MODULE, spiPrxConfig);
    
    /* PRX configuration structure */
    NrfPrxConfig_t prxConfig = {
        .spiSfr = &PRX_SPI_MODULE,
        .isAck = true,
        .dataRate = NRF_RF_DR_2000,
        .rfChannel = NRF_RF_CH_2,
        .pipeAddr = {
            .pipe0 = 0x7878787878,
            .pipe1 = 0xB3B4B5B6F1,
            .pipe2 = 0xB3B4B5B6CD,
            .pipe3 = 0xB3B4B5B6A3,
            .pipe4 = 0xB3B4B5B60F,
            .pipe5 = 0xB3B4B5B605
        },
        .pinConfig = {
            .cePin = PRX_CE,
            .csPin = PRX_CS,
            .irqPin = PRX_IRQ
        }
    };
    
    /* Configure debug pin */
    pio.A = &PIOA_MODULE;
    pio.B = &PIOB_MODULE;  
    pio.A->PIOxTRIS.CLR = PIO_TRISA2_MASK;
    
    /* Configure the device as a PRX */
    NRF_ConfigPrxSfr(prxConfig);
    
    /* Configure payload structure */
    NrfPayloadConfig_t prxPayloadConfig = NRF_ConfigPrxPayloadStruct(prxConfig);

    /* Set user callback */
    NRF_SetUserCallback(NRF_CLBK_RX_PAYLOAD_RECEIVE, RxCallback);
    
    /* Store ACK payload */
    uint8_t prxTxData[3];
    prxTxData[0] = 0x12;
    prxTxData[1] = 0x23;
    prxTxData[2] = 0x45;
    NRF_StoreAckPayload(prxPayloadConfig, NRF_RX_PIPE_5, prxTxData, 3);
    
    /* Main program execution */
    while( NRF_IsRxFifoLoading() )
    {
        PIO_TogglePin(GPIO_RPA2);
    }
    
    /* Start reception */
    NRF_StartReception(prxPayloadConfig, prxRxData);

    /* Main program execution */
    while(1)
    {
        PIO_TogglePin(GPIO_RPA2);

        /* When payload is received the 'RxCallback' is called from internal ISR handler */
    }

    return 0;
}

void RxCallback(void)
{
  /* Toggle a pin or handle some data */
}