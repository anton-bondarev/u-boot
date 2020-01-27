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

#define NAND_D0                 PINMUX_HW_PIN(LSIF0, mGPIO3, 0)
#define NAND_D1                 PINMUX_HW_PIN(LSIF0, mGPIO3, 1)
#define NAND_D2                 PINMUX_HW_PIN(LSIF0, mGPIO3, 2)
#define NAND_D3                 PINMUX_HW_PIN(LSIF0, mGPIO3, 3)
#define NAND_D4                 PINMUX_HW_PIN(LSIF0, mGPIO3, 4)
#define NAND_D5                 PINMUX_HW_PIN(LSIF0, mGPIO3, 5)
#define NAND_D6                 PINMUX_HW_PIN(LSIF0, mGPIO3, 6)
#define NAND_D7                 PINMUX_HW_PIN(LSIF0, mGPIO3, 7)

#define NAND_READY1N            PINMUX_HW_PIN(LSIF0, mGPIO4, 0)
#define NAND_READY2N            PINMUX_HW_PIN(LSIF0, mGPIO4, 1)

#define NAND_REN                PINMUX_HW_PIN(LSIF0, mGPIO7, 4)
#define NAND_CLE                PINMUX_HW_PIN(LSIF0, mGPIO7, 5)
#define NAND_CE2N               PINMUX_HW_PIN(LSIF0, mGPIO7, 6)
#define NAND_CE1N               PINMUX_HW_PIN(LSIF0, mGPIO7, 7)

#define NAND_ALE                PINMUX_HW_PIN(LSIF0, mGPIO8, 0)
#define NAND_WEN                PINMUX_HW_PIN(LSIF0, mGPIO8, 1)
#define NAND_WPN                PINMUX_HW_PIN(LSIF0, mGPIO8, 7)

#define SRAM_NOR_D0             PINMUX_HW_PIN(LSIF0, mGPIO0, 0)
#define SRAM_NOR_D1             PINMUX_HW_PIN(LSIF0, mGPIO0, 1)
#define SRAM_NOR_D2             PINMUX_HW_PIN(LSIF0, mGPIO0, 2)
#define SRAM_NOR_D3             PINMUX_HW_PIN(LSIF0, mGPIO0, 3)
#define SRAM_NOR_D4             PINMUX_HW_PIN(LSIF0, mGPIO0, 4)
#define SRAM_NOR_D5             PINMUX_HW_PIN(LSIF0, mGPIO0, 5)
#define SRAM_NOR_D6             PINMUX_HW_PIN(LSIF0, mGPIO0, 6)
#define SRAM_NOR_D7             PINMUX_HW_PIN(LSIF0, mGPIO0, 7)
#define SRAM_NOR_D8             PINMUX_HW_PIN(LSIF0, mGPIO1, 0)
#define SRAM_NOR_D9             PINMUX_HW_PIN(LSIF0, mGPIO1, 1)
#define SRAM_NOR_D10            PINMUX_HW_PIN(LSIF0, mGPIO1, 2)
#define SRAM_NOR_D11            PINMUX_HW_PIN(LSIF0, mGPIO1, 3)
/*#define SRAM_NOR_D12*/
/*#define SRAM_NOR_D13*/
#define SRAM_NOR_D14            PINMUX_HW_PIN(LSIF0, mGPIO1, 4)
#define SRAM_NOR_D15            PINMUX_HW_PIN(LSIF0, mGPIO1, 5)
#define SRAM_NOR_D16            PINMUX_HW_PIN(LSIF0, mGPIO1, 6)
#define SRAM_NOR_D17            PINMUX_HW_PIN(LSIF0, mGPIO1, 7)
#define SRAM_NOR_D18            PINMUX_HW_PIN(LSIF0, mGPIO2, 0)
#define SRAM_NOR_D19            PINMUX_HW_PIN(LSIF0, mGPIO2, 1)
#define SRAM_NOR_D20            PINMUX_HW_PIN(LSIF0, mGPIO2, 2)
#define SRAM_NOR_D21            PINMUX_HW_PIN(LSIF0, mGPIO2, 3)
#define SRAM_NOR_D22            PINMUX_HW_PIN(LSIF0, mGPIO2, 4)
#define SRAM_NOR_D23            PINMUX_HW_PIN(LSIF0, mGPIO2, 5)
#define SRAM_NOR_D24            PINMUX_HW_PIN(LSIF0, mGPIO2, 6)
#define SRAM_NOR_D25            PINMUX_HW_PIN(LSIF0, mGPIO2, 7)
/*#define SRAM_NOR_D26*/
/*#define SRAM_NOR_D27*/
#define SRAM_NOR_D28            PINMUX_HW_PIN(LSIF0, mGPIO3, 0)
#define SRAM_NOR_D29            PINMUX_HW_PIN(LSIF0, mGPIO3, 1)
#define SRAM_NOR_D30            PINMUX_HW_PIN(LSIF0, mGPIO3, 2)
#define SRAM_NOR_D31            PINMUX_HW_PIN(LSIF0, mGPIO3, 3)
#define SRAM_NOR_D32            PINMUX_HW_PIN(LSIF0, mGPIO3, 4)
#define SRAM_NOR_D33            PINMUX_HW_PIN(LSIF0, mGPIO3, 5)
#define SRAM_NOR_D34            PINMUX_HW_PIN(LSIF0, mGPIO3, 6)
#define SRAM_NOR_D35            PINMUX_HW_PIN(LSIF0, mGPIO3, 7)
#define SRAM_NOR_D36            PINMUX_HW_PIN(LSIF0, mGPIO4, 0)
#define SRAM_NOR_D37            PINMUX_HW_PIN(LSIF0, mGPIO4, 1)
#define SRAM_NOR_D38            PINMUX_HW_PIN(LSIF0, mGPIO4, 2)
#define SRAM_NOR_D39            PINMUX_HW_PIN(LSIF0, mGPIO4, 3)
#define SRAM_NOR_A0             PINMUX_HW_PIN(LSIF0, mGPIO4, 4)
#define SRAM_NOR_A1             PINMUX_HW_PIN(LSIF0, mGPIO4, 5)
#define SRAM_NOR_A2             PINMUX_HW_PIN(LSIF0, mGPIO4, 6)
#define SRAM_NOR_A3             PINMUX_HW_PIN(LSIF0, mGPIO4, 7)
#define SRAM_NOR_A4             PINMUX_HW_PIN(LSIF0, mGPIO5, 0)
#define SRAM_NOR_A5             PINMUX_HW_PIN(LSIF0, mGPIO5, 1)
#define SRAM_NOR_A6             PINMUX_HW_PIN(LSIF0, mGPIO5, 2)
#define SRAM_NOR_A7             PINMUX_HW_PIN(LSIF0, mGPIO5, 3)
#define SRAM_NOR_A8             PINMUX_HW_PIN(LSIF0, mGPIO5, 4)
#define SRAM_NOR_A9             PINMUX_HW_PIN(LSIF0, mGPIO5, 5)
/*#define SRAM_NOR_A10*/
#define SRAM_NOR_A11            PINMUX_HW_PIN(LSIF0, mGPIO5, 6)
#define SRAM_NOR_A12            PINMUX_HW_PIN(LSIF0, mGPIO5, 7)
#define SRAM_NOR_A13            PINMUX_HW_PIN(LSIF0, mGPIO6, 0)
#define SRAM_NOR_A14            PINMUX_HW_PIN(LSIF0, mGPIO6, 1)
#define SRAM_NOR_A15            PINMUX_HW_PIN(LSIF0, mGPIO6, 2)
#define SRAM_NOR_A16            PINMUX_HW_PIN(LSIF0, mGPIO6, 3)
#define SRAM_NOR_A17            PINMUX_HW_PIN(LSIF0, mGPIO6, 4)
#define SRAM_NOR_A18            PINMUX_HW_PIN(LSIF0, mGPIO6, 5)
#define SRAM_NOR_A19            PINMUX_HW_PIN(LSIF0, mGPIO6, 6)
#define SRAM_NOR_A20            PINMUX_HW_PIN(LSIF0, mGPIO6, 7)
#define SRAM_NOR_A21            PINMUX_HW_PIN(LSIF0, mGPIO7, 0)
#define SRAM_NOR_A22            PINMUX_HW_PIN(LSIF0, mGPIO7, 1)
/*#define SRAM_NOR_A23*/
#define SRAM_NOR_A24            PINMUX_HW_PIN(LSIF0, mGPIO7, 2)
#define SRAM_NOR_A25            PINMUX_HW_PIN(LSIF0, mGPIO7, 3)
#define SRAM_NOR_CE0            PINMUX_HW_PIN(LSIF0, mGPIO7, 4)
#define SRAM_NOR_CE1            PINMUX_HW_PIN(LSIF0, mGPIO7, 5)
#define SRAM_NOR_CE2            PINMUX_HW_PIN(LSIF0, mGPIO7, 6)
#define SRAM_NOR_CE3            PINMUX_HW_PIN(LSIF0, mGPIO7, 7)
#define SRAM_NOR_CE4            PINMUX_HW_PIN(LSIF0, mGPIO8, 0)
#define SRAM_NOR_CE5            PINMUX_HW_PIN(LSIF0, mGPIO8, 1)
#define SRAM_NOR_BHE_H          PINMUX_HW_PIN(LSIF0, mGPIO8, 2)
#define SRAM_NOR_BHE_L          PINMUX_HW_PIN(LSIF0, mGPIO8, 3)
#define SRAM_NOR_BLE_H          PINMUX_HW_PIN(LSIF0, mGPIO8, 4)
#define SRAM_NOR_BLE_L          PINMUX_HW_PIN(LSIF0, mGPIO8, 5)
#define SRAM_NOR_OE             PINMUX_HW_PIN(LSIF0, mGPIO8, 6)
#define SRAM_NOR_WE             PINMUX_HW_PIN(LSIF0, mGPIO8, 7)
