#define INPUT 0
#define OUTPUT 1
#define HW 1
#define SOFT 0
#define LOW 0
#define HIGH 1

#define LSIF0 0 
#define LSIF1 1

#define mGPIO0 0  
#define mGPIO1 1
#define mGPIO2 2  
#define mGPIO3 3  
#define mGPIO4 4  
#define mGPIO5 5  
#define mGPIO6 6  
#define mGPIO7 7  
#define mGPIO8 8  
#define mGPIO9 9  
#define mGPIO10 10  
#define mGPIO11 11  
#define mGPIO12 12  
#define mGPIO13 13  
#define mGPIO14 14  
#define mGPIO15 15  
 

#define PINMUX_PIN(no, func, inout) \
(((no) & 0x7) | (((func) & 0x1) << 4) | (((inout) & 0x1) << 5))

#define PINMUX_SW_PIN(no, func, inout, def) \
(((no) & 0x7) | (((func) & 0x1) << 4) | (((inout) & 0x1) << 5) | (((def) & 0x1) << 11))

#define PINMUX_HW_PIN(lsif, mgpio, no) \
(((no) & 0x7) | 0x10 | (((lsif) & 0x1) << 6) | (((mgpio) & 0xf) << 7))


/* simple pinmux */
#define PIN0_SOFT_IN			PINMUX_PIN(0, SOFT, INPUT)
#define PIN0_SOFT_OUT			PINMUX_PIN(0, SOFT, OUTPUT)
#define PIN0_HW	        		PINMUX_PIN(0, HW, 0)
#define PIN1_SOFT_IN			PINMUX_PIN(1, SOFT, INPUT)
#define PIN1_SOFT_OUT			PINMUX_PIN(1, SOFT, OUTPUT)
#define PIN1_HW	        		PINMUX_PIN(1, HW, 0)
#define PIN2_SOFT_IN			PINMUX_PIN(2, SOFT, INPUT)
#define PIN2_SOFT_OUT			PINMUX_PIN(2, SOFT, OUTPUT)
#define PIN2_HW	        		PINMUX_PIN(2, HW, 0)
#define PIN3_SOFT_IN			PINMUX_PIN(3, SOFT, INPUT)
#define PIN3_SOFT_OUT			PINMUX_PIN(3, SOFT, OUTPUT)
#define PIN3_HW	        		PINMUX_PIN(3, HW, 0)
#define PIN4_SOFT_IN			PINMUX_PIN(4, SOFT, INPUT)
#define PIN4_SOFT_OUT			PINMUX_PIN(4, SOFT, OUTPUT)
#define PIN4_HW	        		PINMUX_PIN(4, HW, 0)
#define PIN5_SOFT_IN			PINMUX_PIN(5, SOFT, INPUT)
#define PIN5_SOFT_OUT			PINMUX_PIN(5, SOFT, OUTPUT)
#define PIN5_HW	        		PINMUX_PIN(5, HW, 0)
#define PIN6_SOFT_IN			PINMUX_PIN(6, SOFT, INPUT)
#define PIN6_SOFT_OUT			PINMUX_PIN(6, SOFT, OUTPUT)
#define PIN6_HW	        		PINMUX_PIN(6, HW, 0)
#define PIN7_SOFT_IN			PINMUX_PIN(7, SOFT, INPUT)
#define PIN7_SOFT_OUT			PINMUX_PIN(7, SOFT, OUTPUT)
#define PIN7_HW	        		PINMUX_PIN(7, HW, 0)

/* LSIF0 HW pinmux */

#define LSCB0_CLK_IN_0          PINMUX_HW_PIN(LSIF1, mGPIO2, 0)
#define LSCB0_TXA0              PINMUX_HW_PIN(LSIF1, mGPIO0, 0)
#define LSCB0_nTXA0             PINMUX_HW_PIN(LSIF1, mGPIO0, 1)
#define LSCB0_RXA0              PINMUX_HW_PIN(LSIF1, mGPIO0, 2)
#define LSCB0_nRXA0             PINMUX_HW_PIN(LSIF1, mGPIO0, 3)
#define LSCB0_TX_INH_A0         PINMUX_HW_PIN(LSIF1, mGPIO2, 1)
#define LSCB0_TXB0              PINMUX_HW_PIN(LSIF1, mGPIO0, 4)
#define LSCB0_nTXB0             PINMUX_HW_PIN(LSIF1, mGPIO0, 5)
#define LSCB0_RXB0              PINMUX_HW_PIN(LSIF1, mGPIO0, 6)
#define LSCB0_nRXB0             PINMUX_HW_PIN(LSIF1, mGPIO0, 7)
#define LSCB0_TX_INH_B0         PINMUX_HW_PIN(LSIF1, mGPIO2, 1)
#define SPI0_SCLK               PINMUX_HW_PIN(LSIF1, mGPIO3, 4)
#define SPI0_MOSI               PINMUX_HW_PIN(LSIF1, mGPIO3, 5)
#define SPI0_MISO               PINMUX_HW_PIN(LSIF1, mGPIO3, 6)
#define SPI1_SCLK               PINMUX_HW_PIN(LSIF1, mGPIO4, 0)
#define SPI1_MOSI               PINMUX_HW_PIN(LSIF1, mGPIO4, 1)
#define SPI1_MISO               PINMUX_HW_PIN(LSIF1, mGPIO4, 2)
#define UART0_RXD               PINMUX_HW_PIN(LSIF1, mGPIO2, 6)
#define UART0_TXD               PINMUX_HW_PIN(LSIF1, mGPIO2, 7)
#define UART1_RXD               PINMUX_HW_PIN(LSIF1, mGPIO3, 0)
#define UART1_TXD               PINMUX_HW_PIN(LSIF1, mGPIO3, 1)
#define UART2_RXD               PINMUX_HW_PIN(LSIF1, mGPIO3, 2)
#define UART2_TXD               PINMUX_HW_PIN(LSIF1, mGPIO3, 3)
#define SDIO_CMD                PINMUX_HW_PIN(LSIF1, mGPIO4, 3)
#define SDIO_D0                 PINMUX_HW_PIN(LSIF1, mGPIO4, 4)
#define SDIO_D1                 PINMUX_HW_PIN(LSIF1, mGPIO4, 5)
#define SDIO_D2                 PINMUX_HW_PIN(LSIF1, mGPIO4, 6)
#define SDIO_D3                 PINMUX_HW_PIN(LSIF1, mGPIO4, 7)

#define GBETH0_RX_DV            PINMUX_HW_PIN(LSIF0, mGPIO0, 0)
#define GBETH0_RX_ER            PINMUX_HW_PIN(LSIF0, mGPIO0, 1)
#define GBETH0_RX_COL           PINMUX_HW_PIN(LSIF0, mGPIO0, 2)
#define GBETH0_RX_CRS           PINMUX_HW_PIN(LSIF0, mGPIO0, 3)
#define GBETH0_RXD0             PINMUX_HW_PIN(LSIF0, mGPIO0, 4)
#define GBETH0_RXD1             PINMUX_HW_PIN(LSIF0, mGPIO0, 5)
#define GBETH0_RXD2             PINMUX_HW_PIN(LSIF0, mGPIO0, 6)
#define GBETH0_RXD3             PINMUX_HW_PIN(LSIF0, mGPIO0, 7)

#define GBETH0_RXD4             PINMUX_HW_PIN(LSIF0, mGPIO1, 0)
#define GBETH0_RXD5             PINMUX_HW_PIN(LSIF0, mGPIO1, 1)
#define GBETH0_RXD6             PINMUX_HW_PIN(LSIF0, mGPIO1, 2)
#define GBETH0_RXD7             PINMUX_HW_PIN(LSIF0, mGPIO1, 3)

#define GBETH0_TX_EN            PINMUX_HW_PIN(LSIF0, mGPIO4, 2)
#define GBETH0_TX_ER            PINMUX_HW_PIN(LSIF0, mGPIO4, 5)
#define GBETH0_TXD0             PINMUX_HW_PIN(LSIF0, mGPIO4, 6)
#define GBETH0_TXD1             PINMUX_HW_PIN(LSIF0, mGPIO4, 7)

#define GBETH0_TXD2             PINMUX_HW_PIN(LSIF0, mGPIO5, 0)
#define GBETH0_TXD3             PINMUX_HW_PIN(LSIF0, mGPIO5, 1)
#define GBETH0_TXD4             PINMUX_HW_PIN(LSIF0, mGPIO5, 2)
#define GBETH0_TXD5             PINMUX_HW_PIN(LSIF0, mGPIO5, 3)
#define GBETH0_TXD6             PINMUX_HW_PIN(LSIF0, mGPIO5, 4)
#define GBETH0_TXD7             PINMUX_HW_PIN(LSIF0, mGPIO5, 5)
#define GBETH0_MDC              PINMUX_HW_PIN(LSIF0, mGPIO5, 6)
#define GBETH0_MDIO             PINMUX_HW_PIN(LSIF0, mGPIO5, 7)


