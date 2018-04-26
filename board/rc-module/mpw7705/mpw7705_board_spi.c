
//#define DEBUG 1
#include <common.h>
#include <dm.h>
#include <linux/compat.h>
#include <asm/io.h>
#include <malloc.h>
#include <spi.h>
#include <mpw7705_addr_mapping.h>
#include <mpw7705_timer.h>

//register defines
#define PL022_CR0       0x000
#define PL022_CR1       0x004
#define PL022_DR            0x008
#define PL022_SR            0x00C
#define PL022_CPSR      0x010
#define PL022_IMSC      0x014
#define PL022_RIS       0x018
#define PL022_MIS       0x01C
#define PL022_ICR       0x020
#define PL022_DMACR         0x024
#define PL022_PeriphID0         0xFE0
#define PL022_PeriphID1         0xFE4
#define PL022_PeriphID2         0xFE8
#define PL022_PeriphID3         0xFEC
#define PL022_PCellID0      0xFF0
#define PL022_PCellID1      0xFF4
#define PL022_PCellID2      0xFF8
#define PL022_PCellID3      0xFFC

//default
#define PL022_CR0_DFLT           0x0
#define PL022_CR1_DFLT           0x0
#define PL022_DR_DFLT            0x0
#define PL022_SR_DFLT            0x03
#define PL022_CPSR_DFLT      0x0
#define PL022_IMSC_DFLT      0x0
#define PL022_RIS_DFLT           0x8
#define PL022_MIS_DFLT           0x0
#define PL022_ICR_DFLT           0x0
#define PL022_DMACR_DFLT     0x0
#define PL022_PeriphID0_DFLT 0x22
#define PL022_PeriphID1_DFLT 0x10
#define PL022_PeriphID2_DFLT 0x24
#define PL022_PeriphID3_DFLT 0x00
#define PL022_PCellID0_DFLT  0x0D
#define PL022_PCellID1_DFLT  0xF0
#define PL022_PCellID2_DFLT  0x05
#define PL022_PCellID3_DFLT  0xB1
//Mask
#define PL022_CR0_MSK               0xFFFF
#define PL022_CR1_MSK               0xF
#define PL022_DR_MSK                0xFFFF
#define PL022_SR_MSK                0x1F
#define PL022_CPSR_MSK          0xFF
#define PL022_IMSC_MSK          0xF
#define PL022_RIS_MSK               0xF
#define PL022_MIS_MSK               0xF
#define PL022_ICR_MSK               0x3
#define PL022_DMACR_MSK         0x3
#define PL022_PeriphID0_MSK     0xFF
#define PL022_PeriphID1_MSK     0xFF
#define PL022_PeriphID2_MSK     0xFF
#define PL022_PeriphID3_MSK     0xFF
#define PL022_PCellID0_MSK      0xFF
#define PL022_PCellID1_MSK      0xFF
#define PL022_PCellID2_MSK      0xFF
#define PL022_PCellID3_MSK      0xFF

#define PL022_CR0__SCR_i        8
#define PL022_CR0__SPH_i        7
#define PL022_CR0__SPO_i        6
#define PL022_CR0__FRF_i        4
#define PL022_CR0__DSS_i        0

#define PL022_CR1__SOD_i        3
#define PL022_CR1__MS_i         2
#define PL022_CR1__SSE_i        1
#define PL022_CR1__LBM_i        0

#define PL022_SR__BSY_i        4
#define PL022_SR__RFF_i        3
#define PL022_SR__RNE_i        2
#define PL022_SR__TNF_i        1
#define PL022_SR__TFE_i        0

#define PL022_IMSC__TXIM_i        3
#define PL022_IMSC__RXIM_i        2
#define PL022_IMSC__RTIM_i        1
#define PL022_IMSC__RORIM_i       0

#define PL022_RIS__TXRIS_i        3
#define PL022_RIS__RXRIS_i        2
#define PL022_RIS__RTRIS_i        1
#define PL022_RIS__RORRIS_i       0

#define PL022_MIS__TXMIS_i        3
#define PL022_MIS__RXMIS_i        2
#define PL022_MIS__RTMIS_i        1
#define PL022_MIS__RORMIS_i       0

#define PL022_ICR__RTIC_i        1
#define PL022_ICR__RORIC_i       0

#define PL022_DMACR__TXDMAE_i       1
#define PL022_DMACR__RXDMAE_i       0

#define PL061_DATAx_REGS_COUNT 8

#define PL061_DATA_BASE  0x000
#define PL061_DATA       (PL061_DATA_BASE + (0xff << 2))
#define PL061_DATAx(x)   (PL061_DATA_BASE + (1 << (x+2)))
#define PL061_DIR       0x400
#define PL061_IS        0x404
#define PL061_IBE       0x408
#define PL061_IEV       0x40C
#define PL061_IE        0x410
#define PL061_RIS       0x414
#define PL061_MIS       0x418
#define PL061_IC        0x41C
#define PL061_AFSEL         0x420
#define PL061_PeriphID0         0xFE0
#define PL061_PeriphID1         0xFE4
#define PL061_PeriphID2         0xFE8
#define PL061_PeriphID3         0xFEC
#define PL061_CellID0       0xFF0
#define PL061_CellID1       0xFF4
#define PL061_CellID2       0xFF8
#define PL061_CellID3       0xFFC

#define PL061_DATA_DFLT      0x00
#define PL061_DIR_DFLT       0x00
#define PL061_IS_DFLT            0x00
#define PL061_IBE_DFLT       0x00
#define PL061_IEV_DFLT       0x00
#define PL061_IE_DFLT            0x00
#define PL061_RIS_DFLT       0x00
#define PL061_MIS_DFLT       0x00
#define PL061_IC_DFLT            0x00
#define PL061_AFSEL_DFLT     0x00
#define PL061_PeriphID0_DFLT 0x61
#define PL061_PeriphID1_DFLT 0x10
#define PL061_PeriphID2_DFLT 0x04
#define PL061_PeriphID3_DFLT 0x00
#define PL061_CellID0_DFLT   0x0D 

#define PL061_CellID1_DFLT   0xF0
#define PL061_CellID2_DFLT   0x05
#define PL061_CellID3_DFLT   0xB1

#define READ_MEM_REG(REG_NAME, REG_OFFSET, REG_SIZE)\
inline static uint##REG_SIZE##_t read_##REG_NAME(uint32_t const base_addr)\
{\
    return *((volatile uint##REG_SIZE##_t*)(base_addr + REG_OFFSET));\
}

#define WRITE_MEM_REG(REG_NAME, REG_OFFSET, REG_SIZE)\
inline static void write_##REG_NAME(uint32_t const base_addr, uint32_t const value)\
{\
    *((volatile uint##REG_SIZE##_t*)(base_addr + REG_OFFSET)) = value;\
    debug("0x%x <- %x", (base_addr + REG_OFFSET), value); \
}

READ_MEM_REG(PL022_CR0,PL022_CR0,16);
WRITE_MEM_REG(PL022_CR0,PL022_CR0,16);

READ_MEM_REG(PL022_CR1,PL022_CR1,16);
WRITE_MEM_REG(PL022_CR1,PL022_CR1,16);

READ_MEM_REG(PL022_DR,PL022_DR,16);
WRITE_MEM_REG(PL022_DR,PL022_DR,16);

READ_MEM_REG(PL022_SR,PL022_SR,16);

READ_MEM_REG(PL022_CPSR,PL022_CPSR,16);
WRITE_MEM_REG(PL022_CPSR,PL022_CPSR,16);

READ_MEM_REG(PL022_IMSC,PL022_IMSC,16);
WRITE_MEM_REG(PL022_IMSC,PL022_IMSC,16);

READ_MEM_REG(PL022_RIS,PL022_RIS,16);

READ_MEM_REG(PL022_MIS,PL022_MIS,16);

WRITE_MEM_REG(PL022_ICR,PL022_ICR,16);

READ_MEM_REG(PL022_DMACR,PL022_DMACR,16);
WRITE_MEM_REG(PL022_DMACR,PL022_DMACR,16);

READ_MEM_REG(PL022_PeriphID0,PL022_PeriphID0,16);
READ_MEM_REG(PL022_PeriphID1,PL022_PeriphID1,16);
READ_MEM_REG(PL022_PeriphID2,PL022_PeriphID2,16);
READ_MEM_REG(PL022_PeriphID3,PL022_PeriphID3,16);

READ_MEM_REG(PL022_PCellID0,PL022_PCellID0,16);
READ_MEM_REG(PL022_PCellID1,PL022_PCellID1,16);
READ_MEM_REG(PL022_PCellID2,PL022_PCellID2,16);
READ_MEM_REG(PL022_PCellID3,PL022_PCellID3,16);

#define PL061_DATAi_READ(i, z) \
READ_MEM_REG(PL061_DATA##i, PL061_DATAx(i), 8)

#define PL061_DATAi_WRITE(i, z) \
WRITE_MEM_REG(PL061_DATA##i, PL061_DATAx(i), 8)

/*REPEAT( PL061_DATAx_REGS_COUNT, PL061_DATAi_READ, z )
REPEAT( PL061_DATAx_REGS_COUNT, PL061_DATAi_WRITE, z )*/

READ_MEM_REG(PL061_DATA,PL061_DATA,8)
WRITE_MEM_REG(PL061_DATA,PL061_DATA,8)

READ_MEM_REG(PL061_DIR,PL061_DIR,8)
WRITE_MEM_REG(PL061_DIR,PL061_DIR,8)

READ_MEM_REG(PL061_IS,PL061_IS,8)
WRITE_MEM_REG(PL061_IS,PL061_IS,8)

READ_MEM_REG(PL061_IBE,PL061_IBE,8)
WRITE_MEM_REG(PL061_IBE,PL061_IBE,8)

READ_MEM_REG(PL061_IEV,PL061_IEV,8)
WRITE_MEM_REG(PL061_IEV,PL061_IEV,8)

READ_MEM_REG(PL061_IE,PL061_IE,8)
WRITE_MEM_REG(PL061_IE,PL061_IE,8)

READ_MEM_REG(PL061_RIS,PL061_RIS,8)

READ_MEM_REG(PL061_MIS,PL061_MIS,8)

WRITE_MEM_REG(PL061_IC,PL061_IC,8)

READ_MEM_REG(PL061_AFSEL,PL061_AFSEL,8)
WRITE_MEM_REG(PL061_AFSEL,PL061_AFSEL,8)

READ_MEM_REG(PL061_PeriphID0,PL061_PeriphID0,8)
READ_MEM_REG(PL061_PeriphID1,PL061_PeriphID1,8)
READ_MEM_REG(PL061_PeriphID2,PL061_PeriphID2,8)
READ_MEM_REG(PL061_PeriphID3,PL061_PeriphID3,8)

READ_MEM_REG(PL061_CellID0,PL061_CellID0,8)
READ_MEM_REG(PL061_CellID1,PL061_CellID1,8)
READ_MEM_REG(PL061_CellID2,PL061_CellID2,8)
READ_MEM_REG(PL061_CellID3,PL061_CellID3,8)

#define SPI_READ_CMD_TIMEOUT 1000
#define SPI_READ_TIMEOUT 1000000
#define BOOT_SPI_BASE SPI_CTRL0__
#define BOOT_GPIO_FOR_SPI_BASE  GPIO1__
#define BOOT_GPIO_FOR_SPI_PIN  2

static void spi_ss(uint8_t val)
{
    //set pin value to SS not active
    uint8_t data = read_PL061_DATA(BOOT_GPIO_FOR_SPI_BASE);
    data &= ~(1 << BOOT_GPIO_FOR_SPI_PIN); //clear
    write_PL061_DATA(BOOT_GPIO_FOR_SPI_BASE, data | (val << BOOT_GPIO_FOR_SPI_PIN));
}

static void enable_gpio_for_SPI_CTRL0(void)
{
    debug("enable_gpio_for_SPI_CTRL0\n");

    uint8_t afsel;
    //http://svn.module.ru/r42_mm7705/mm7705/trunk/ifsys/units/lsif0/doc/LSIF0_pinout.xlsx
    //afsel = read_PL061_AFSEL(LSIF0_MGPIO0__);
    //afsel |= 0b00000111;
    //write_PL061_AFSEL(LSIF0_MGPIO0__, afsel);

    //set software control for slave select signal
    //afsel = read_PL061_AFSEL(BOOT_GPIO_FOR_SPI_BASE);
    //afsel &= ~(1 << BOOT_GPIO_FOR_SPI_PIN);
    //write_PL061_AFSEL(BOOT_GPIO_FOR_SPI_BASE, afsel);

    //set SS pin value to not active
    spi_ss(1);

    //set SPI SS pin direction to output
    uint8_t dir = read_PL061_DIR(BOOT_GPIO_FOR_SPI_BASE);
    dir |= (1 << BOOT_GPIO_FOR_SPI_PIN);
    write_PL061_DIR(BOOT_GPIO_FOR_SPI_BASE, dir);

    //One more time to be sure: set SS pin value to not active
    spi_ss(1);
}

static void disable_gpio_for_SPI_CTRL0(void)
{
    debug("disable_gpio_for_SPI_CTRL0\n");

    uint8_t afsel;
    afsel = read_PL061_AFSEL(LSIF1_MGPIO3__);
    afsel &= ~0b01110000;
    write_PL061_AFSEL(LSIF0_MGPIO3__, afsel);

    //set SS pin value to not active
    spi_ss(1);

    //set SPI SS pin direction to input
    uint8_t dir = read_PL061_DIR(BOOT_GPIO_FOR_SPI_BASE);
    dir &= ~(1 << BOOT_GPIO_FOR_SPI_PIN);
    write_PL061_DIR(BOOT_GPIO_FOR_SPI_BASE, dir);
}

static bool spi_core_init(void)
{
    debug("spi_core_init\n");

    /*FixMe: This should be Platform-specific ! */
    write_PL022_CPSR(BOOT_SPI_BASE, 8); //100MHz/8 = 12.5MHz

    write_PL022_CR0(BOOT_SPI_BASE,
              (0b0 << PL022_CR0__SCR_i)
            | (0b0 << PL022_CR0__SPH_i)
            | (0b0 << PL022_CR0__SPO_i)
            | (0b00 << PL022_CR0__FRF_i)
            | (0b0111 << PL022_CR0__DSS_i));

    write_PL022_IMSC(BOOT_SPI_BASE,
              (0b0 << PL022_IMSC__TXIM_i)
            | (0b0 << PL022_IMSC__RXIM_i)
            | (0b0 << PL022_IMSC__RTIM_i)
            | (0b0 << PL022_IMSC__RORIM_i));

    write_PL022_ICR(BOOT_SPI_BASE,
              (0b1 << PL022_ICR__RTIC_i)
            | (0b1 << PL022_ICR__RORIC_i));

    write_PL022_DMACR(BOOT_SPI_BASE,
              (0b0 << PL022_DMACR__TXDMAE_i)
            | (0b0 << PL022_DMACR__RXDMAE_i));

    write_PL022_CR1(BOOT_SPI_BASE,
              (0b1 << PL022_CR1__SOD_i)
            | (0b0 << PL022_CR1__MS_i)
            | (0b1 << PL022_CR1__SSE_i) //enable PL022 SSP
            | (0b0 << PL022_CR1__LBM_i));

    return true;
}

int pl022_spi_init(void)
{

    // test MGPIO:
  	debug("MGPIO1 pids: %02x %02x %02x %02x\n", 
          read_PL061_PeriphID0(0x3C061000),
          read_PL061_PeriphID1(0x3C061000),
          read_PL061_PeriphID2(0x3C061000),
          read_PL061_PeriphID3(0x3C061000)
      );


    // ASTRO TODO: 2nd SPI
    //enable_gpio_for_SPI_CTRL0();

    spi_core_init();

    return 0;
}

void spi_cs_activate(struct spi_slave *slave)
{
    debug("spi_cs_activate\n");
    // ASTRO TODO: 2nd SPI
	spi_ss(0);
}

void spi_cs_deactivate(struct spi_slave *slave)
{
    debug("spi_cs_deactivate\n");
    // ASTRO TODO: 2nd SPI
	spi_ss(1);
}