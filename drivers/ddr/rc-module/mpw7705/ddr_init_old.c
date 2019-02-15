
#include <stdint.h>
#include "ddr_init_old.h"
#include "timer.h"

volatile uint32_t ioread32(uint32_t const base_addr)
{
    return *((volatile uint32_t*)(base_addr));
}

void iowrite32(uint32_t const value, uint32_t const base_addr)
{
    *((volatile uint32_t*)(base_addr)) = value;
}

void mpw7705_writeb(uint8_t const value, uint32_t const base_addr)
{
    *((volatile uint8_t*)(base_addr)) = value;
}

//DCR bus access write, usage: <dcrwrite32(value, base_addr + REG)>
void dcrwrite32(uint32_t const wval, uint32_t const addr)
{
    asm volatile
    (
        "mtdcrx %0, %1 \n\t"
        ::"r"(addr), "r"(wval)
    );
}

//DCR bus access read, usage: <dcrread32(base_addr + REG)>
volatile uint32_t dcrread32(uint32_t const addr)
{
    uint32_t rval=0;
    asm volatile
    (
        "mfdcrx %0, %1 \n\t"
        :"=r"(rval)
        :"r"(addr)
    );
    return rval;
}

//*****************************************************************************
//  This file contains several functions
//  Main of them is ddr_init
//  Now no arguments are available for ddr_init
//  All other functions must not be used alone
//  
//  Now work with 1066 MHz and burst 8 only supported
//*****************************************************************************


//*****************************************************************************
//  Initialisation of clocking system
//  Includes:
//    DDR external CRG config
//    DDR internal CRG turning ON and config
//  Arguments:
//    ddr3_speed
//      1 - 1066
//      2 - 1333
//      3 - 1600
//*****************************************************************************
static void ddr_init_crg_pll (uint32_t ddr3_speed)
{
    //****************** DDR external CRG config*********************
    iowrite32(0x1ACCE551, CRG_DDR_BASE + CRG_DDR_WR_LOCK);
    iowrite32(0x01010040, CRG_DDR_BASE + CRG_DDR_PLL_DIVMODE);
    switch (ddr3_speed)
    {
        case 1: iowrite32(0x0b, CRG_DDR_BASE + CRG_DDR_CKDIVMODE_DDR3_1X); break;
        case 2: iowrite32(0x0b, CRG_DDR_BASE + CRG_DDR_CKDIVMODE_DDR3_1X); break;
        case 3: iowrite32(0x07, CRG_DDR_BASE + CRG_DDR_CKDIVMODE_DDR3_1X); break;
    }
    iowrite32(0x1, CRG_DDR_BASE + CRG_DDR_UPD_CK);
    
    //****************** DDR internal CRG turning ON and config******
    dcrwrite32(0xf3, EM0_PHY_DCR_BASE + DDR3PHY_PHYREG00); // Soft reset enter
    dcrwrite32(0xff, EM0_PHY_DCR_BASE + DDR3PHY_PHYREG00); // Soft reset exit
    usleep(1);                                             // let's wait 1us
    dcrwrite32(0x2, EM0_PHY_DCR_BASE + DDR3PHY_PHYREGEE);  // set PLL normal Pre-divide
    switch (ddr3_speed)
    {
        case 1: dcrwrite32(0x20, EM0_PHY_DCR_BASE + DDR3PHY_PHYREGEC); break;
        case 2: dcrwrite32(0x28, EM0_PHY_DCR_BASE + DDR3PHY_PHYREGEC); break;
        case 3: dcrwrite32(0x20, EM0_PHY_DCR_BASE + DDR3PHY_PHYREGEC); break;
    }
    dcrwrite32(0x18, EM0_PHY_DCR_BASE + DDR3PHY_PHYREGED); //PLL clock out enable

    while (((ioread32(SCTL_BASE + SCTL_PLL_STATE)>>2)&1) != 1)
    {
    }
    usleep(5); //let's wait 5us
    dcrwrite32(0x1, EM0_PHY_DCR_BASE + DDR3PHY_PHYREGEF); //Clock source = PHY internal PLL
}
//*****************************************************************************

//*****************************************************************************
//  PLB6 to MCIF2 bridge initialisation
//  Includes:
//    TODO
//  Arguments:
//    TODO
//*****************************************************************************
static void ddr_plb6mcif2_init(const uint32_t baseAddr, const uint32_t puaba)
{
  dcrwrite32(0xffff0000, baseAddr + PLB6MCIF2_INTR_EN); //enable logging but disable interrupts
  dcrwrite32(0x40000000, baseAddr + PLB6MCIF2_PLBASYNC); //PLB clock equals MCIF2 clock
  dcrwrite32(0x20000000, baseAddr + PLB6MCIF2_PLBORD); //16 deep
  dcrwrite32(0x00000000, baseAddr + PLB6MCIF2_MAXMEM); //Set MAXMEM = 8GB
  dcrwrite32(puaba, baseAddr + PLB6MCIF2_PUABA); //Set PLB upper address base address from I_S_ADDR[0:30]
  dcrwrite32(0x00009001, baseAddr + PLB6MCIF2_MR0CF); //Set Rank0 base addr M_BA[0:2] = PUABA[28:30], M_BA[3:12] = 0b0000000000, Set Rank0 size = 4GB, Enable Rank0
  dcrwrite32(0x00089001, baseAddr + PLB6MCIF2_MR1CF); //Set Rank1 base addr M_BA[0:2] = PUABA[28:30], M_BA[3:12] = 0b1000000000, Set Rank1 size = 4GB, Enable Rank1
  dcrwrite32(0x00000000, baseAddr + PLB6MCIF2_MR2CF); //Disable Rank2
  dcrwrite32(0x00000000, baseAddr + PLB6MCIF2_MR3CF); //Disable Rank3
  const uint32_t Tsnoop = (dcrread32(PLB6_BC_BASE + PLB6BC_TSNOOP) & (0b1111 << 28)) >> 28;
  dcrwrite32((Tsnoop<<27|0xf1), baseAddr + PLB6MCIF2_PLBCFG); //PLB6MCIF2 enable
}
//*****************************************************************************

//*****************************************************************************
//  AXI to MCIF2 bridge initialisation
//  Software must properly configure the AXIASYNC, AXICFG, AXIRNK0, AXIRNK1,
//    AXIRNK2 and AXIRNK3 registers before starting read/write transfers.
//  Includes:
//    TODO
//  Arguments:
//    TODO
//*****************************************************************************
static void ddr_aximcif2_init(const uint32_t baseAddr)
{
  //Mask errors, do not generate interrupt
  dcrwrite32 (0xff000000, baseAddr + AXIMCIF2_AXIERRMSK);

  dcrwrite32 ((0b000 << 25) //AXINUM,
            | (0b0000000 << 18) //MCIFNUM
            | (0b0000 << 8) //RDRDY_TUNE
            | (0b0 << 5) //DCR_CLK_MN
            | (0b00000 << 0), //DCR_CLK_RATIO
                          baseAddr + AXIMCIF2_AXIASYNC);
                                                    
  dcrwrite32 ((0b0 << 15) //IT_OOO,
            | (0b00 << 13) //TOM
            | (0b0 << 1) //EQ_ID
            | (0b0 << 0), // EXM_ID
                          baseAddr + AXIMCIF2_AXICFG);

  //EM0 and EM1 use 4GB x 2rank external DDR memory configuration
  uint32_t R_RA = 0;
  uint32_t R_RS = 0;
  uint32_t R_RO = 0;

  //EM0 R_RA - AXI Address Bits [35:24] EM0 AXI address 0x0_40000000 is mapped to the starting 1GB of EM0 rank0
  R_RA = 0x040;
  R_RS = 0b0110; //1GB
  R_RO = 0; //zero offset

  dcrwrite32 (((R_RA & 0xfff) << 20)
             | ((R_RS & 0xf) << 16)
             | ((R_RO & 0b111111111111111) << 1)
             | (0b1 << 0), //R_RV
                          baseAddr + AXIMCIF2_MR0CF);

  //disable other ranks as not accessible in AXI address space
  dcrwrite32(0, baseAddr + AXIMCIF2_MR1CF);
  dcrwrite32(0, baseAddr + AXIMCIF2_MR2CF);
  dcrwrite32(0, baseAddr + AXIMCIF2_MR3CF);
}
//*****************************************************************************

//*****************************************************************************
//  DDR3 Phy initialisation
//  Includes:
//    DDR protocol timing registers initialisation
//  Arguments:
//    baseAddr - base address of DDR3 Phy in DCR space
//    burstLen - length of DDR packets
//*****************************************************************************
static void ddr3phy_init(uint32_t baseAddr, const uint32_t burstLen)
{
  uint32_t reg;
  
  reg = dcrread32(baseAddr + DDR3PHY_PHYREG01);
  reg &= ~0b111;
  dcrwrite32((burstLen == 0b10) ? (reg | 0b000) : (reg | 0b100), baseAddr + DDR3PHY_PHYREG01); // Set DDR3 mode

  reg = dcrread32(baseAddr + DDR3PHY_PHYREG0b);
  reg &= ~0xff;
  dcrwrite32(reg | (DDR_CL_PHY << 4) | (DDR_AL_PHY << 0), baseAddr + DDR3PHY_PHYREG0b); // Set CL, AL

  reg = dcrread32(baseAddr + DDR3PHY_PHYREG0c);
  reg &= ~0xf;
  dcrwrite32(reg | (DDR_CWL_PHY << 0), baseAddr + DDR3PHY_PHYREG0c); // CWL = 8
  // email from INNO, did
  // please configure the register 0x11b[7:4] to 0x07, this will decrease the skew of the clock
  reg = dcrread32(baseAddr + 0x11B);
  reg &= ~0xf0;
  dcrwrite32(reg | 0x70, baseAddr + 0x11B);
}
//*****************************************************************************

//*****************************************************************************
//  DDR3 MAC initialisation
//  Includes:
//    Few steps - description in MAC documentation
//  Arguments:
//    TODO
//*****************************************************************************
static void ddr_ddr34lmc_init (
        const uint32_t baseAddr,
        const uint32_t initMode,
        const uint32_t eccMode,
        const uint32_t powerManagementMode,
        const uint32_t burstLength,
        const uint32_t partialWriteMode
)
{
    uint32_t reg = 0;
    
    //Trigger DDR PHY initialization to provide stable clock
    
    //****************** 1. Power, resets - made by hardware*******************
    //****************** 2. Start DFI interface, wait stable clk***************
    reg = dcrread32(baseAddr + DDR34LMC_MCOPT2);
    dcrwrite32(0x00001000 | reg, baseAddr + DDR34LMC_MCOPT2); //MCOPT2[19] = 0b1 DFI_INIT_START
    //PHY spec says we must wait for 2500ps*2000dfi_clk1x = 5us
    usleep(5);
    //****************** 3. SDRAM RESET_N can be deasserted********************
    usleep(200);  //  in microseconds
    uint32_t mcopt2_reg = dcrread32(baseAddr + DDR34LMC_MCOPT2);
    dcrwrite32(mcopt2_reg & 0xff0fffff, baseAddr + DDR34LMC_MCOPT2); //MCOPT2[8:11] = 0b0000 RESET_RANK

    //****************** 4. after previous steps, mode SELF REFRESH ON*********
    //****************** 5. prepare to SDRAM initialisation********************
    dcrwrite32(((partialWriteMode << 30) | (eccMode << 28) | 0x00052001)  , baseAddr + DDR34LMC_MCOPT1);

    dcrwrite32(0, baseAddr + DDR34LMC_IOCNTL);

    dcrwrite32(0, baseAddr + DDR34LMC_PHYCNTL);

    //Init ODTR0 - ODTR3
    dcrwrite32(0, baseAddr + DDR34LMC_ODTR0);
    dcrwrite32(0, baseAddr + DDR34LMC_ODTR1);
    dcrwrite32(0, baseAddr + DDR34LMC_ODTR2);
    dcrwrite32(0, baseAddr + DDR34LMC_ODTR3);

    //Init CFGR0 - CFGR3
    //dcrwrite32(0x4301, baseAddr + DDR34LMC_CFGR0); // did modified
    // 0x4201, [17:19]=100, Row addr - 16-bits, [20:23]=0010, Mode 2 - Nx10 
    dcrwrite32(0x4201, baseAddr + DDR34LMC_CFGR0); // did, check SPD, 22.08.17
    //dcrwrite32(0x4301, baseAddr + DDR34LMC_CFGR1);
    dcrwrite32(0x0, baseAddr + DDR34LMC_CFGR1); // rank1 - off, did, check SPD, 22.08.17
    dcrwrite32(0x0, baseAddr + DDR34LMC_CFGR2); // rank2 - off, did, check SPD, 22.08.17
    dcrwrite32(0x0, baseAddr + DDR34LMC_CFGR3); // rank3 - off, did, check SPD, 22.08.17

    //Init SMR0 - SMR3
    // old, <init_ddr.h>
    //  MR0, M12-PD, M11:M9-WR, M8-DLL, M6:M2-CL, M3-BT, M1, M0 - BL
    // SMR0, PPD(19), WR/WR/RTP(20:22), DLL(23), CL(3:1)(25:27), RBT(28), CL(0)(29), BL(0:1)
    //dcrwrite32(((burstLength << 0) | (DDR_CL_MC << 4) | (0b1 << 8) | (DDR_WR << 9) | (0b1 << 12)), baseAddr + DDR34LMC_SMR0); // OLD
    dcrwrite32((0x00000100 | (burstLength << 0) | (DDR_CL_MC << 4) | (DDR_WR << 9) | (0b1 << 12)), baseAddr + DDR34LMC_SMR0); // did, 24.08.17
    // did modified, 22.03.17, <ddr_init_HX316LS9IB.h>
    //dcrwrite32(((burstLength << 0) | ((DDR_CL_MC & 0xE) << 3)  | ((DDR_CL_MC & 0x1) << 2) | (0b1 << 8) | (DDR_WR << 9) | (0b1 << 12)), baseAddr + DDR34LMC_SMR0); // NEW
    //  MR1, M12-Q off, M11- TDQS, M9,M6,M2 - Rtt, M7 - Write levelization, M4-3, Additive latency, M5,M1 - ODS, M0 - DLL
    // SMR1, Qoff(19), TDQS(20), Rtt(22, 25, 29), WR_lev(24), DIC(26, 30), AL(27-28), DLL(31)
    //dcrwrite32(((0b1 << 0) | (DDR_AL_MC << 3)), baseAddr + DDR34LMC_SMR1); // OLD
    dcrwrite32((0x00000046 | (DDR_AL_MC << 3)), baseAddr + DDR34LMC_SMR1); // did, 22.08.17, AL
    //  MR2, M10,M9-Rtt, M7 - SelfRef Tem, M6 - Auto SelfRef, M5,M4,M3 - CWL
    // SMR2, Rtt_WR(21:22), SRT(24), ART(23), CWL(26:28),     
    dcrwrite32((DDR_CWL_MC << 3), baseAddr + DDR34LMC_SMR2); // did, 22.08.17
    //  MR3, M2- MPR, M1,M0 - MPR_REF
    // SMR3, MPR(29), MPR_SEL(30:31)
    dcrwrite32(0x0, baseAddr + DDR34LMC_SMR3);

    //Init SDTR0 - SDTR5
    dcrwrite32(((DDR_T_RFC_XPR << 0) | (DDR_T_REFI << 16)), baseAddr + DDR34LMC_SDTR0);
    dcrwrite32(0x0, baseAddr + DDR34LMC_SDTR1);
    dcrwrite32(((DDR_T_RAS << 0) | (DDR_T_RC << 8) | (DDR_T_RP << 16) | (DDR_T_RCD << 24) | (0b1000 << 28)), baseAddr + DDR34LMC_SDTR2);
    dcrwrite32(((DDR_T_XSDLL << 0) | (0b0100 << 8) | (0b0010 << 12) | (DDR_T_RTP << 16) | (0b10 << 20) | (DDR_T_WTR << 24) | (0b0011 << 28)), baseAddr + DDR34LMC_SDTR3);
// did, tCCD=4 cycles, 2-7, (21:23), T_MOD = 8, 10, 12 cycles, 1-31, (27:31), T_SYS_RDLAT=0xe, 0-63, (10:15), T_RDDATA_EN=0xc, 0-63, (1:7)
    dcrwrite32(((DDR_T_MOD << 0) | (0b100 << 8) | (0b0100 << 12) | (0xe << 16) | (0xc << 24)), baseAddr + DDR34LMC_SDTR4);
// did, T_PHY_WR_DATA=0, 0-7 cycles, (5:7)
    dcrwrite32(0x0, baseAddr + DDR34LMC_SDTR5);

    
    //***************************************************************
    //  This is DDR external memory initialisation sequence
    //    It configures MR2, MR3, MR1, MR0 registers
    //***************************************************************
    // Even, 0 - Enable, 4:15 - Wait, 27 - EN_MULTI_RANK_SEL, 28:31 - RANK
    // Odd,  0:2 - CMD, 3 - CMD_ACTN, 9:11 - BANK, 16:31 - ADDR
    dcrwrite32(((0b1111 << 0) | (1 << 4) | (2 << 16) | (0x1 << 31)), baseAddr + DDR34LMC_INITSEQ0);
		//dcrwrite32(((DDR_CWL_CHIP << 3) | (0b1 << 21)), baseAddr + DDR34LMC_INITCMD0); // OLD
    dcrwrite32(0x00200000 | (DDR_CWL_CHIP << 3), baseAddr + DDR34LMC_INITCMD0); // MR2, did, 24.08.17
    dcrwrite32(((0b1111 << 0) | (1 << 4) | (2 << 16) | (0x1 << 31)), baseAddr + DDR34LMC_INITSEQ1);
    //dcrwrite32(((0b1 << 20) | (0b1 << 21)), baseAddr + DDR34LMC_INITCMD1); // OLD
    dcrwrite32(0x00300000, baseAddr + DDR34LMC_INITCMD1); // MR3, did, 24.08.17
    dcrwrite32(((0b1111 << 0) | (1 << 4) | (2 << 16) | (0x1 << 31)), baseAddr + DDR34LMC_INITSEQ2);
    //dcrwrite32(((DDR_AL_CHIP << 3) | (0b1 << 20)), baseAddr + DDR34LMC_INITCMD2); // OLD
    dcrwrite32(0x00100046 | (DDR_AL_CHIP << 3), baseAddr + DDR34LMC_INITCMD2); // MR1, did, 24.08.17 846
    dcrwrite32(((0b1111 << 0) | (1 << 4) | (6 << 16) | (0x1 << 31)), baseAddr + DDR34LMC_INITSEQ3);
    //  Looks like Phy calibration pass only if burst = 8. So burst is changed after calibration.
    // dcrwrite32(((burstLength << 0) | (DDR_CL_CHIP << 4) | (0b1 << 8) | (0b110 << 9) | (0b1 << 12)), baseAddr + DDR34LMC_INITCMD3);
    //dcrwrite32(((0b00 << 0) | (DDR_CL_CHIP << 4) | (0b1 << 8) | (0b110 << 9) | (0b1 << 12)), baseAddr + DDR34LMC_INITCMD3); // OLD
    dcrwrite32(0x00000100 | (DDR_CL_CHIP << 4) |  (DDR_WR << 9) | (0b1 << 12), baseAddr + DDR34LMC_INITCMD3); // MR0, did, 24.08.17
    dcrwrite32(((0b1111 << 0) | (1 << 4) | (256 << 16) | (0x1 << 31)), baseAddr + DDR34LMC_INITSEQ4);
    //dcrwrite32(((0b1 << 10) | (0b1 << 30) | (0x1 << 31)), baseAddr + DDR34LMC_INITCMD4); // OLD
    dcrwrite32(0xC0000400, baseAddr + DDR34LMC_INITCMD4); // ZQCL, did, 24.08.17
    //***************************************************************    

    //***************************************************************
    //  This is DDR external memory ZQ calibration command
    //***************************************************************
    dcrwrite32(0x0, baseAddr + DDR34LMC_INITSEQ5);
    dcrwrite32(0x0, baseAddr + DDR34LMC_INITCMD5);

    //**************************** 6. Configure MC calibration timeout regs****
    //**************************** (optional)**********************************
    dcrwrite32(0x00001000, baseAddr + DDR34LMC_T_PHYUPD0);
    dcrwrite32(0x00001000, baseAddr + DDR34LMC_T_PHYUPD1);
    dcrwrite32(0x00001000, baseAddr + DDR34LMC_T_PHYUPD2);
    dcrwrite32(0x00001000, baseAddr + DDR34LMC_T_PHYUPD3);

    //**************************** 7. Exit power-on mode, initialise SDRAM*****
    //Must wait for 500us after SDRAM reset deactivated
    usleep(500); //in microseconds

    dcrwrite32((dcrread32(baseAddr + DDR34LMC_MCOPT2) | 0x20000000) & 0x7fffffff, baseAddr + DDR34LMC_MCOPT2);

    while ((dcrread32(baseAddr + DDR34LMC_MCSTAT) & 0x80000000) == 0)
    {
    }

    //**************************** 8. Configure Phy calibration regs***********
    //  TODO
    //**************************** 9. Trigger Phy initialisation***************
    //  TODO
    //**************************** 10. Exit power-on mode, initialise SDRAM****
    mcopt2_reg = dcrread32(baseAddr + DDR34LMC_MCOPT2);
    dcrwrite32((mcopt2_reg | 0x10000000), baseAddr + DDR34LMC_MCOPT2); //MCOPT2[3] = 0b1 MC_ENABLE
}

//*****************************************************************************
//  Phy calibration function
//    It somehow calibrates Phy DLL latencies
//    Without it read data has some byte errors
//    After calibration looks like read latency in Phy increased,
//      so i had to increase MAC T_SYS_RDLAT parameter C -> E
//*****************************************************************************
static void ddr3phy_calibrate(const uint32_t baseAddr)
{
   dcrwrite32(0x01, baseAddr + DDR3PHY_PHYREG02);
   usleep(6);  //  in microseconds
   dcrwrite32(0x00, baseAddr + DDR3PHY_PHYREG02);  //  Stop calibration
}

//*****************************************************************************
//  This function writes burst = 4 into MR3 memory register
//*****************************************************************************
static void ddr_burst4 (void)
{
    dcrwrite32(((DDR_BurstLength << 0) | (DDR_CL_CHIP << 4) | (0b1 << 8) | (0b110 << 9) | (0b1 << 12)), EM0_DDR3LMC_DCR_BASE + DDR34LMC_INITCMD0);
    dcrwrite32(((0b1111 << 0) | (1 << 4) | (2 << 16) | (0x1 << 31)), EM0_DDR3LMC_DCR_BASE + DDR34LMC_INITSEQ0);
    dcrwrite32(0, EM0_DDR3LMC_DCR_BASE + DDR34LMC_INITSEQ1);
    dcrwrite32(0, EM0_DDR3LMC_DCR_BASE + DDR34LMC_INITSEQ2);
    dcrwrite32(0, EM0_DDR3LMC_DCR_BASE + DDR34LMC_INITSEQ3);
    dcrwrite32(0, EM0_DDR3LMC_DCR_BASE + DDR34LMC_INITSEQ4);
    
    dcrwrite32((dcrread32(EM0_DDR3LMC_DCR_BASE + DDR34LMC_MCOPT2) | 0x20000000) & 0x7fffffff, EM0_DDR3LMC_DCR_BASE + DDR34LMC_MCOPT2);

    while ((dcrread32(EM0_DDR3LMC_DCR_BASE + DDR34LMC_MCSTAT) & 0x80000000) == 0)
    {
    }
}

//*****************************************************************************
//  DDR common initialisation function
//    It include CRG, interconnect, Phy and MAC initialisation
//*****************************************************************************
static void _ddr_init (void)
{
    ddr_init_crg_pll (DDR_ddr3_speed);
    ddr_plb6mcif2_init (EM0_PLB6MCIF2_DCR_BASE, 0x00000000);
    ddr_aximcif2_init (EM0_AXIMCIF2_DCR_BASE);
    dcrwrite32 ((0b0000 << 28) | (0b1 << 27) | (0b00 << 8), EM0_MCIF2ARB_DCR_BASE + MCIF2ARB4_MACR);
    ddr3phy_init (EM0_PHY_DCR_BASE, DDR_BurstLength);
    ddr_ddr34lmc_init (
        EM0_DDR3LMC_DCR_BASE,
        DDR_initMode,
        DDR_eccMode,
        DDR_powerManagementMode,
        DDR_BurstLength,
        DDR_partialWriteMode);
    ddr3phy_calibrate (EM0_PHY_DCR_BASE);
    if (DDR_BurstLength == 0b10)
    {
        ddr_burst4 ();
    }
}

static uint32_t invalidate_tlb_entry (void)
{
    uint32_t res;
    asm volatile
    (
        "addis  %0, r0, 0x0000 \n\t"  // res  = 0x00000000
        "addis  r3, r0, 0x4000 \n\t"  // [r3] = 0x4000_0000, (%2)
        "ori    r3, r3, 0x0070 \n\t"  // [r3] = 0x4000_03F0, (%1) // ? 0870
        "tlbsx. r4, r0, r3     \n\t"  // [r3] - EA, [r4] - Way, Index
        "bne end               \n\t"  // branch if not found
        "oris   r4, r4, 0x8000 \n\t"  // r4[0]=1, r4[4]=0
        "tlbwe  r3, r4, 0      \n\t"  // [r3] - EA[0:19], V[20], [r4]- Way[1:2], Index[8:15], index is NU
        "end:or     %0, r4, r4     \n\t"
    
        "isync \n\t"
        "msync \n\t"
        :"=r"(res) 
        ::
    );
    return res;
};

static void write_tlb_entry_1st(void)
{
    asm volatile
    (
        "addis r3, r0, 0x0000 \n\t"  // 0x0000
        "addis r4, r0, 0x4000 \n\t"  // 0x0000
        "ori   r4, r4, 0x09F0 \n\t"  // 0x0BF0  set 0x0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F (4K, 16K, 64K, 1M, 16M, 256M, 1GB)
        "tlbwe r4, r3, 0      \n\t"
        
        "addis r4, r0, 0x0000 \n\t"  // 0x0000
        "ori   r4, r4, 0x0000 \n\t"  // 0x0000
        "tlbwe r4, r3, 1      \n\t"
        
        "addis r4, r0, 0x0003 \n\t"  // 0000
        "ori   r4, r4, 0x043F \n\t"  // 023F
        "tlbwe r4, r3, 2      \n\t"
        
        "isync \n\t"
        "msync \n\t"
        :::
    );  
};

static void init_ppc(void)
{
    asm volatile
    (
        // initialize MMUCR
        "addis r0, r0, 0x0000   \n\t"  // 0x0000
        "ori   r0, r0, 0x0000   \n\t"  // 0x0000
        "mtspr 946, r0          \n\t"  // SPR_MMUCR, 946
        "mtspr 48,  r0          \n\t"  // SPR_PID,    48
        // Set up TLB search priority
        "addis r0, r0, 0x1234   \n\t"  // 0x1234
        "ori   r0, r0, 0x5678   \n\t"  // 0x5678  
        "mtspr 830, r0          \n\t"  // SSPCR, 830
        "mtspr 831, r0          \n\t"  // USPCR, 831
        "mtspr 829, r0          \n\t"  // ISPCR, 829
        :::
    );   
};

static void TLB_interconnect_init(void)
{
    uint32_t dcr_val = 0;
    
    //***************************************************************
    //  Setting PPC for translation elaboration
    //    Made by assembler insertion. No other way to do it.
    //***************************************************************
    init_ppc();
    //***************************************************************

    // Set Seg0, Seg1, Seg2 and BC_CR0
    dcrwrite32(0x00000010, 0x80000204);  // BC_SGD1
    dcrwrite32(0x00000010, 0x80000205);  // BC_SGD2
    dcr_val = dcrread32(0x80000200);     // BC_CR0
    dcrwrite32( dcr_val | 0x80000000, 0x80000200);
}

void ddr_init(void)
{
	_ddr_init();
	TLB_interconnect_init();

	invalidate_tlb_entry();
	write_tlb_entry_1st();
}
