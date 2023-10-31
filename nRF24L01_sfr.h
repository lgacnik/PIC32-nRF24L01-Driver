#ifndef NRF24L01_SFR_H
#define	NRF24L01_SFR_H

/** Standard libs **/
#include <stdint.h>

/******************************************************************************/
/*--------------------------------SFR Addresses-------------------------------*/
/******************************************************************************/

/* SPI commands */
#define NRF_WRITE_CMD(regAddr)          ({ (0x20 | (regAddr & 0x1F)); })
#define NRF_READ_CMD(regAddr)           ({ (0x00 | (regAddr & 0x1F)); })
#define NRF_WRITE_ACK_PL_CMD(pipeNum)   ({ (0xA8 | (pipeNum & 0x07)); })   
#define NRF_READ_RX_PL_CMD              (0x61)
#define NRF_WRITE_TX_PL_CMD             (0xA0)
#define NRF_FLUSH_TX_CMD                (0xE1)
#define NRF_FLUSH_RX_CMD                (0xE2)
#define NRF_REUSE_TX_PL_CMD             (0xE3)
#define NRF_READ_RX_PL_WID_CMD          (0x60)
#define NRF_WRITE_TX_PL_NO_ACK_CMD      (0xB0)
#define NRF_NOP_CMD                     (0xFF)

/* Device registers */
#define NRF_CONFIG_REG                  (0x00)
#define NRF_EN_AA_REG                   (0x01)
#define NRF_EN_RXADDR_REG               (0x02)
#define NRF_SETUP_AW_REG                (0x03)
#define NRF_SETUP_RETR_REG              (0x04)
#define NRF_RF_CH_REG                   (0x05)
#define NRF_RF_SETUP_REG                (0x06)
#define NRF_STATUS_REG                  (0x07)
#define NRF_OBSERVE_TX_REG              (0x08)
#define NRF_RPD_REG                     (0x09)
#define NRF_RX_ADDR_P0_REG              (0x0A)
#define NRF_RX_ADDR_P1_REG              (0x0B)
#define NRF_RX_ADDR_P2_REG              (0x0C)
#define NRF_RX_ADDR_P3_REG              (0x0D)
#define NRF_RX_ADDR_P4_REG              (0x0E)
#define NRF_RX_ADDR_P5_REG              (0x0F)
#define NRF_TX_ADDR_REG                 (0x10)
#define NRF_RX_PW_P0_REG                (0x11)
#define NRF_RX_PW_P1_REG                (0x12)
#define NRF_RX_PW_P2_REG                (0x13)
#define NRF_RX_PW_P3_REG                (0x14)
#define NRF_RX_PW_P4_REG                (0x15)
#define NRF_RX_PW_P5_REG                (0x16)
#define NRF_FIFO_STATUS_REG             (0x17)
#define NRF_DYNPD_REG                   (0x1C)
#define NRF_FEATURE_REG                 (0x1D)


/******************************************************************************/
/*---------------------------------Bit Masks----------------------------------*/
/******************************************************************************/

/* CONFIG register */
#define NRF_PRIM_RX_POS         (0)
#define NRF_PRIM_RX_MASK        (1 << 0)
#define NRF_PWR_UP_POS          (1)
#define NRF_PWR_UP_MASK         (1 << 1)
#define NRF_CRCO_POS            (2)
#define NRF_CRCO_MASK           (1 << 2)
#define NRF_EN_CRC_POS          (3)
#define NRF_EN_CRC_MASK         (1 << 3)
#define NRF_MASK_MAX_RT_POS     (4)
#define NRF_MASK_MAX_RT_MASK    (1 << 4)
#define NRF_MASK_TX_DS_POS      (5)
#define NRF_MASK_TX_DS_MASK     (1 << 5)
#define NRF_MASK_RX_DR_POS      (6)
#define NRF_MASK_RX_DR_MASK     (1 << 6)

/* EN_AA register */
#define NRF_ENAA_P0_POS         (0)
#define NRF_ENAA_P0_MASK        (1 << 0)
#define NRF_ENAA_P1_POS         (1)
#define NRF_ENAA_P1_MASK        (1 << 1)
#define NRF_ENAA_P2_POS         (2)
#define NRF_ENAA_P2_MASK        (1 << 2)
#define NRF_ENAA_P3_POS         (3)
#define NRF_ENAA_P3_MASK        (1 << 3)
#define NRF_ENAA_P4_POS         (4)
#define NRF_ENAA_P4_MASK        (1 << 4)
#define NRF_ENAA_P5_POS         (5)
#define NRF_ENAA_P5_MASK        (1 << 5)

/* EN_RXADDR register */
#define NRF_ERX_P0_POS          (0)
#define NRF_ERX_P0_MASK         (1 << 0)
#define NRF_ERX_P1_POS          (1)
#define NRF_ERX_P1_MASK         (1 << 1)
#define NRF_ERX_P2_POS          (2)
#define NRF_ERX_P2_MASK         (1 << 2)
#define NRF_ERX_P3_POS          (3)
#define NRF_ERX_P3_MASK         (1 << 3)
#define NRF_ERX_P4_POS          (4)
#define NRF_ERX_P4_MASK         (1 << 4)
#define NRF_ERX_P5_POS          (5)
#define NRF_ERX_P5_MASK         (1 << 5)

/* SETUP_AW register */
#define NRF_AW_POS              (0)
#define NRF_AW_MASK             (3 << 0)

/* SETUP_RETR register */
#define NRF_ARC_POS             (0)
#define NRF_ARC_MASK            (0xF << 0)
#define NRF_ARD_POS             (4)
#define NRF_ARD_MASK            (0xF << 4)

/* RF_CH register */
#define NRF_RF_CH_POS           (0)
#define NRF_RF_CH_MASK          (0x7F << 0)

/* RF_SETUP register */
#define NRF_RF_PWR_POS          (1)
#define NRF_RF_PWR_MASK         (3 << 1)
#define NRF_RF_DR_HIGH_POS      (3)
#define NRF_RF_DR_HIGH_MASK     (1 << 3)
#define NRF_PLL_LOCK_POS        (4)
#define NRF_PLL_LOCK_MASK       (1 << 4)
#define NRF_RF_DR_LOW_POS       (5)
#define NRF_RF_DR_LOW_MASK      (1 << 5)
#define NRF_CONT_WAVE_POS       (7)
#define NRF_CONT_WAVE_MASK      (1 << 7)

/* STATUS register */
#define NRF_TX_FULL_POS         (0)
#define NRF_TX_FULL_MASK        (1 << 0)
#define NRF_RX_P_NO_POS         (1)
#define NRF_RX_P_NO_MASK        (7 << 1)
#define NRF_MAX_RT_POS          (4)
#define NRF_MAX_RT_MASK         (1 << 4)
#define NRF_TX_DS_POS           (5)
#define NRF_TX_DS_MASK          (1 << 5)
#define NRF_RX_DR_POS           (6)
#define NRF_RX_DR_MASK          (1 << 6)

/* OBSERVE_TX register */
#define NRF_ARC_CNT_POS         (0)
#define NRF_ARC_CNT_MASK        (0xF << 0)
#define NRF_PLOS_CNT_POS        (4)
#define NRF_PLOS_CNT_MASK       (0xF << 4)

/* RPD register */
#define NRF_RPD_POS             (0)
#define NRF_RPD_MASK            (1 << 0)

/* RX_ADDR_P0 register */
#define NRF_RX_ADDR_P0_MASK     (0xFFFFFFFFFF)

/* RX_ADDR_P1 register */
#define NRF_RX_ADDR_P1_MASK     (0xFFFFFFFFFF)

/* RX_ADDR_P2 register */
#define NRF_RX_ADDR_P2_MASK     (0xFF)

/* RX_ADDR_P3 register */
#define NRF_RX_ADDR_P3_MASK     (0xFF)

/* RX_ADDR_P4 register */
#define NRF_RX_ADDR_P4_MASK     (0xFF)

/* RX_ADDR_P5 register */
#define NRF_RX_ADDR_P5_MASK     (0xFF)

/* TX_ADDR register */
#define NRF_TX_ADDR_MASK        (0xFFFFFFFFFF)

/* RX_PW_P0 register */
#define NRF_RX_PW_P0_MASK       (0x3F)

/* RX_PW_P1 register */
#define NRF_RX_PW_P1_MASK       (0x3F)

/* RX_PW_P2 register */
#define NRF_RX_PW_P2_MASK       (0x3F)

/* RX_PW_P3 register */
#define NRF_RX_PW_P3_MASK       (0x3F)

/* RX_PW_P4 register */
#define NRF_RX_PW_P4_MASK       (0x3F)

/* RX_PW_P5 register */
#define NRF_RX_PW_P5_MASK       (0x3F)

/* FIFO_STATUS register */
#define NRF_RX_FIFO_EMPTY_POS   (0)
#define NRF_RX_FIFO_EMPTY_MASK  (1 << 0)
#define NRF_RX_FIFO_FULL_POS    (1)
#define NRF_RX_FIFO_FULL_MASK   (1 << 1)
#define NRF_TX_FIFO_EMPTY_POS   (4)
#define NRF_TX_FIFO_EMPTY_MASK  (1 << 4)
#define NRF_TX_FIFO_FULL_POS    (5)
#define NRF_TX_FIFO_FULL_MASK   (1 << 5)
#define NRF_TX_FIFO_REUSE_POS   (6)
#define NRF_TX_FIFO_REUSE_MASK  (1 << 6)

/* DYNPD register */
#define NRF_DPL_P0_POS          (0)
#define NRF_DPL_P0_MASK         (1 << 0)
#define NRF_DPL_P1_POS          (1)
#define NRF_DPL_P1_MASK         (1 << 1)
#define NRF_DPL_P2_POS          (2)
#define NRF_DPL_P2_MASK         (1 << 2)
#define NRF_DPL_P3_POS          (3)
#define NRF_DPL_P3_MASK         (1 << 3)
#define NRF_DPL_P4_POS          (4)
#define NRF_DPL_P4_MASK         (1 << 4)
#define NRF_DPL_P5_POS          (5)
#define NRF_DPL_P5_MASK         (1 << 5)

/* FEATURE register */
#define NRF_EN_DYN_ACK_POS      (0)
#define NRF_EN_DYN_ACK_MASK     (1 << 0)
#define NRF_EN_ACK_PAY_POS      (1)
#define NRF_EN_ACK_PAY_MASK     (1 << 1)
#define NRF_EN_DPL_POS          (2)
#define NRF_EN_DPL_MASK         (1 << 2)


/******************************************************************************/
/*-----------------------------Data Structures--------------------------------*/
/******************************************************************************/



#endif	/* NRF24L01_SFR_H */

