/*
 *  Created on: Aug 11, 2015
 *      Author: a.korablinov
 */

#ifndef MM7705_PPC_BOOT_ADDR_MAPPING_H_
#define MM7705_PPC_BOOT_ADDR_MAPPING_H_

#include <mpw7705_plb6_addr.h>

/*
 * Each address has a suffix *__<x>, where <x> is a mem window where address is valid
 * If address has no suffix, then it is valid in all windows. See ppc470s_mmu.h
 * __   - MEM_WINDOW_SHARED
 * __W0 - MEM_WINDOW_0
 * __W1 - MEM_WINDOW_1
 * ...
 * __W11 - MEM_WINDOW_11
 */

#define BOOT_ROM__              0xFFFC0000
#define BOOT_ROM_SIZE           BOOT_ROM_PLB6_SIZE
#define BOOT_ROM_INITIAL_4K__   0xFFFFF000
#define BOOT_ROM_ENTRY__        0xFFFFFFFC
#define IM0__                   0x00040000
#define IM0_SIZE                IM0_PLB6_SIZE

#define SCTL__                  0x38000000
#define SCTL_SIZE               SCTL_PLB6_SIZE

#define GPIO0__                 0x3C03B000
#define GPIO0_SIZE              GPIO0_PLB6_SIZE

#define GPIO1__                 0x3C03C000
#define GPIO1_SIZE              GPIO1_PLB6_SIZE

#define UART0__                 0x3C034000
#define UART0_SIZE              UART0_PLB6_SIZE

#define UART1__                 0x3C035000
#define UART1_SIZE              UART1_PLB6_SIZE

#define UART2__                 0x3C05C000
#define UART2_SIZE              UART2_PLB6_SIZE

#define I2C2__                 0x3C05D000
#define I2C2_SIZE              I2C2_PLB6_SIZE


#define SYS_TIMER__             0x38001000
#define SYS_TIMER_SIZE          SYS_TIMER_PLB6_SIZE

#define SYS_WATCHDOG__              0x38002000
#define SYS_WATCHDOG_PLB6_SIZE      0x1000

#define TVSENS__                0x38003000
#define TVSENS_SIZE             TVSENS_PLB6_SIZE

#define FCx__              0x3C020000
#define FCx_SIZE           FCx_PLB6_SIZE

#define SPI_CTRL0__         0x3C03E000
#define SPI_CTRL0_SIZE      SPI_CTRL0_PLB6_SIZE

#define SPI_CTRL1__         0x3C05A000
#define SPI_CTRL1_SIZE      SPI_CTRL1_PLB6_SIZE

#define LSIF0_GPIO0__           0x3C040000
#define LSIF0_GPIO0_SIZE        LSIF0_GPIO0_PLB6_SIZE
#define LSIF0_GPIO1__           0x3C041000
#define LSIF0_GPIO1_SIZE        LSIF0_GPIO1_PLB6_SIZE
#define LSIF0_GPIO2__           0x3C042000
#define LSIF0_GPIO2_SIZE        LSIF0_GPIO2_PLB6_SIZE
#define LSIF0_GPIO3__           0x3C043000
#define LSIF0_GPIO3_SIZE        LSIF0_GPIO3_PLB6_SIZE
#define LSIF0_GPIO4__           0x3C044000
#define LSIF0_GPIO4_SIZE        LSIF0_GPIO4_PLB6_SIZE
#define LSIF0_GPIO5__           0x3C045000
#define LSIF0_GPIO5_SIZE        LSIF0_GPIO5_PLB6_SIZE
#define LSIF0_GPIO6__           0x3C046000
#define LSIF0_GPIO6_SIZE        LSIF0_GPIO6_PLB6_SIZE
#define LSIF0_GPIO7__           0x3C047000
#define LSIF0_GPIO7_SIZE        LSIF0_GPIO7_PLB6_SIZE
#define LSIF0_GPIO8__           0x3C048000
#define LSIF0_GPIO8_SIZE        LSIF0_GPIO8_PLB6_SIZE
#define LSIF0_GPIO9__           0x3C049000
#define LSIF0_GPIO9_SIZE        LSIF0_GPIO9_PLB6_SIZE
#define LSIF0_GPIO10__          0x3C04A000
#define LSIF0_GPIO10_SIZE       LSIF0_GPIO10_PLB6_SIZE

#define LSIF1_GPIO0__           0x3C060000
#define LSIF1_GPIO0_SIZE        LSIF1_GPIO0_SIZE
#define LSIF1_GPIO1__           0x3C061000
#define LSIF1_GPIO1_SIZE        LSIF1_GPIO1_SIZE
#define LSIF1_GPIO2__           0x3C062000
#define LSIF1_GPIO2_SIZE        LSIF1_GPIO2_SIZE
#define LSIF1_GPIO3__           0x3C063000
#define LSIF1_GPIO3_SIZE        LSIF1_GPIO3_SIZE
#define LSIF1_GPIO4__           0x3C064000
#define LSIF1_GPIO4_SIZE        LSIF1_GPIO4_SIZE
#define LSIF1_GPIO5__           0x3C065000
#define LSIF1_GPIO5_SIZE        LSIF1_GPIO5_SIZE
#define LSIF1_GPIO6__           0x3C066000
#define LSIF1_GPIO6_SIZE        LSIF1_GPIO6_SIZE
#define LSIF1_GPIO7__           0x3C067000
#define LSIF1_GPIO7_SIZE        LSIF1_GPIO7_SIZE
#define LSIF1_GPIO8__           0x3C068000
#define LSIF1_GPIO8_SIZE        LSIF1_GPIO8_SIZE
#define LSIF1_GPIO9__           0x3C069000
#define LSIF1_GPIO9_SIZE        LSIF1_GPIO9_SIZE

#define LSIF0_MGPIO0__           0x3C040000
#define LSIF0_MGPIO1__           0x3C041000
#define LSIF0_MGPIO2__           0x3C042000
#define LSIF0_MGPIO3__           0x3C043000
#define LSIF0_MGPIO4__           0x3C044000
#define LSIF0_MGPIO5__           0x3C045000
#define LSIF0_MGPIO6__           0x3C046000
#define LSIF0_MGPIO7__           0x3C047000
#define LSIF0_MGPIO8__           0x3C048000
#define LSIF0_MGPIO9__           0x3C049000
#define LSIF0_MGPIO10__          0x3C04A000

#define LSIF1_MGPIO0__           0x3C067000
#define LSIF1_MGPIO1__           0x3C068000
#define LSIF1_MGPIO2__           0x3C069000
#define LSIF1_MGPIO3__           0x3C06A000

#define SPISDIO__             0x3C06A000
#define SPISDIO_SIZE          SPISDIO_PLB6_SIZE

#define SDIO__                0x3C06A000
#define SDIO_SIZE             SDIO_PLB6_SIZE

#define GSPI__                0x3C059000
#define GSPI_SIZE             GSPI_PLB6_SIZE

#define NAND__             0x3C032000
#define NAND_SIZE          NAND_PLB6_SIZE

#define GRETH_GBIT0__       0x3C050000
#define GRETH_GBIT0_SIZE    GRETH_GBIT0_PLB6_SIZE

#define GRETH_GBIT1__       0x3C051000
#define GRETH_GBIT1_SIZE    GRETH_GBIT1_PLB6_SIZE

#define XHSIF0_CTRL__      0x3C005000
#define XHSIF0_CTRL_SIZE   XHSIF0_CTRL_PLB6_SIZE

#define XHSIF1_CTRL__      0x3C015000
#define XHSIF1_CTRL_SIZE   XHSIF1_CTRL_PLB6_SIZE

#define NORMC__         0x3C030000
#define NORMC_SIZE      NORMC_PLB6_SIZE

#define NORMEM__        0x20000000
#define NORMEM_SIZE     NORMEM_PLB6_SIZE

#define SRAMMC__         0x3C031000
#define SRAMMC_SIZE      SRAMMC_PLB6_SIZE

#define SRAMMEM__       0x30000000
#define SRAMMEM_SIZE    SRAMMEM_PLB6_SIZE

#define LSIF0_LSCB0__        0x3C036000
#define LSIF0_LSCB0_SIZE     LSIF0_LSCB0_PLB6_SIZE

#define LSIF1_LSCB0__        0x3C054000
#define LSIF1_LSCB0_SIZE     LSIF1_LSCB0_PLB6_SIZE

#define I2C0__          0x3C03D000
#define I2C0_SIZE       I2C0_PLB6_SIZE

#define I2C1__          0x3C05B000
#define I2C1_SIZE       I2C1_PLB6_SIZE

#define NIC301_A_CFG__   0x38100000
#define NIC301_A_CFG_SIZE   NIC301_A_CFG_PLB6_SIZE

#define CRGCPU__        0x38006000
#define CRGCPU_SIZE        CRGCPU_PLB6_SIZE

#define CRGDDR__        0x38007000
#define CRGDDR_SIZE        CRGDDR_PLB6_SIZE

#define RFBISR__      0x3800C000
#define RFBISR_SIZE   RFBISR_PLB6_SIZE

//Power Management Unit
#define PMU__            0x38004000
#define PMU_SIZE            PMU_PLB6_SIZE

#define USB20OTG0__          0x3C052000
#define USB20OTG0_SIZE       USB20OTG0_PLB6_SIZE

#define LSIF1_CTRLREG__       0x3C05E000
#define LSIF1_CTRLREG_SIZE       LSIF1_CTRLREG_PLB6_SIZE

#define XHSIF0_CTRL__    0x3C005000
#define XHSIF0_CTRL_SIZE    XHSIF0_CTRL_PLB6_SIZE
#define XHSIF1_CTRL__    0x3C015000
#define XHSIF1_CTRL_SIZE    XHSIF1_CTRL_PLB6_SIZE

#define LSIF0_CONFIG__       0x3C03F000
#define LSIF0_CONFIG_SIZE       LSIF0_CONFIG_PLB6_SIZE

#define LSIF0_MATRIX__       0x3C700000
#define LSIF0_MATRIX_SIZE       LSIF0_MATRIX_PLB6_SIZE
#define LSIF1_AXI32_S__      0x3C900000
#define LSIF1_AXI32_S_SIZE      LSIF1_AXI32_S_PLB6_SIZE
#define LSIF1_AXI32_M__      0x3CA00000
#define LSIF1_AXI32_M_SIZE      LSIF1_AXI32_M_PLB6_SIZE
#define HSIF_PL301_S__       0x3C500000
#define HSIF_PL301_S_SIZE       HSIF_PL301_S_PLB6_SIZE
#define HSIF_PL301_M__       0x3C600000
#define HSIF_PL301_M_SIZE       HSIF_PL301_M_PLB6_SIZE

#define HSVI8__              0x3C021000
#define HSVI8_SIZE              HSVI8_PLB6_SIZE

#define RMACE__              0x3C024000
#define RMACE_SIZE              RMACE_PLB6_SIZE

#define HSCB0__              0x3C025000
#define HSCB0_SIZE              HSCB0_PLB6_SIZE
#define HSCB1__              0x3C026000
#define HSCB1_SIZE              HSCB1_PLB6_SIZE

#define HSIF_CTRL__          0x3C023000
#define HSIF_CTRL_SIZE          HSIF_CTRL_PLB6_SIZE

#define XHSIF0_PCIe__        0x3C000000
#define XHSIF0_PCIe_SIZE        XHSIF0_PCIe_PLB6_SIZE
#define XHSIF1_PCIe__        0x3C010000
#define XHSIF1_PCIe_SIZE        XHSIF1_PCIe_PLB6_SIZE

#define XHSIF0_SERDES__      0x3CB00000
#define XHSIF0_SERDES_SIZE      XHSIF0_SERDES_PLB6_SIZE
#define XHSIF1_SERDES__      0x3CB40000
#define XHSIF1_SERDES_SIZE      XHSIF1_SERDES_PLB6_SIZE

/*
 * Put stack at the very end of internal memory.
 * Note: stack grows towards lower memory addresses.
 */
#define STACK_SIZE              0x4000
#define PRIMARY_CPU_STACK__     (IM0__ + IM0_SIZE)
#define SECONDARY_CPU_STACK__   (PRIMARY_CPU_STACK__ - STACK_SIZE)

#define SECTION_START_                 .start
#define SECTION_BOOT_ROM_INITIAL_4K_   .text.initial
#define SECTION_TEXT_                  .text
#define SECTION_RODATA_                .rodata

#ifdef __ASSEMBLER__

#define CODE_SECTION(sec_name)   .section STRINGIZE(sec_name),"ax",@progbits
#define DATA_SECTION(sec_name)   .section STRINGIZE(sec_name),"a",@progbits

#else //!__ASSEMBLER__

#define CODE_SECTION(sec_name)   section(STRINGIZE(sec_name)",\"ax\",@progbits#")
#define DATA_SECTION(sec_name)   section(STRINGIZE(sec_name)",\"a\",@progbits#")

#endif //__ASSEMBLER__

#define SECTION_START()                 CODE_SECTION(SECTION_START_)
#define SECTION_BOOT_ROM_INITIAL_4K()   CODE_SECTION(SECTION_BOOT_ROM_INITIAL_4K_)
#define SECTION_TEXT()                  CODE_SECTION(SECTION_TEXT_)
#define SECTION_RODATA()                DATA_SECTION(SECTION_RODATA_)

#endif /* MM7705_PPC_BOOT_ADDR_MAPPING_H_ */
