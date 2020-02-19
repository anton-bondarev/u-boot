#ifndef DDR_INIT_H
#define DDR_INIT_H

//---------------------------------
//----------- CRG defs ------------
#define CRG_DDR_BASE                          0x38007000
          
#define CRG_DDR_PLL_STATE                     0x000
#define CRG_DDR_PLL_CTRL                      0x004
#define CRG_DDR_PLL_DIVMODE                   0x008
#define CRG_DDR_WR_LOCK                       0x00C
#define CRG_DDR_RST_MON                       0x010
#define CRG_DDR_RST_CTRL                      0x014
#define CRG_DDR_RST_EN                        0x018
#define CRG_DDR_UPD_CK                        0x01C
#define CRG_DDR_CKALIGN                       0x020
          
#define CRG_DDR_CKDIVMODE_LSIF                0x100
#define CRG_DDR_CKEN_LSIF                     0x104
          
#define CRG_DDR_CKDIVMODE_HSIF                0x110
#define CRG_DDR_CKEN_HSIF                     0x114
          
#define CRG_DDR_CKDIVMODE_DDR3_1X             0x120
#define CRG_DDR_CKEN_DDR3_1X                  0x124
          
#define CRG_DDR_CKDIVMODE_DDR3_4X             0x150
#define CRG_DDR_CKEN_DDR3_4X                  0x154

//---------------------------------
//----------- DDR EM0 PHY ---------
#define EM0_PHY_DCR_BASE   0x80060000

#define DDR3PHY_PHYREG00   0x000
#define DDR3PHY_PHYREG01   0x004
#define DDR3PHY_PHYREG02   0x008
#define DDR3PHY_PHYREG03   0x00c
#define DDR3PHY_PHYREG04   0x010
#define DDR3PHY_PHYREG05   0x014
#define DDR3PHY_PHYREG06   0x018
#define DDR3PHY_PHYREG0b   0x02c
#define DDR3PHY_PHYREG0c   0x030
#define DDR3PHY_PHYREG11   0x044
#define DDR3PHY_PHYREG12   0x048
#define DDR3PHY_PHYREG13   0x04c
#define DDR3PHY_PHYREG14   0x050
#define DDR3PHY_PHYREG16   0x058
#define DDR3PHY_PHYREG20   0x080
#define DDR3PHY_PHYREG21   0x084
#define DDR3PHY_PHYREG26   0x098
#define DDR3PHY_PHYREG27   0x09c
#define DDR3PHY_PHYREG28   0x0a0
#define DDR3PHY_PHYREG30   0x0c0
#define DDR3PHY_PHYREG31   0x0c4
#define DDR3PHY_PHYREG36   0x0d8
#define DDR3PHY_PHYREG37   0x0d8
#define DDR3PHY_PHYREG38   0x0dc
#define DDR3PHY_PHYREG40   0x100
#define DDR3PHY_PHYREG41   0x104
#define DDR3PHY_PHYREG46   0x118
#define DDR3PHY_PHYREG47   0x11c
#define DDR3PHY_PHYREG48   0x120
#define DDR3PHY_PHYREG50   0x140
#define DDR3PHY_PHYREG51   0x144
#define DDR3PHY_PHYREG56   0x158
#define DDR3PHY_PHYREG57   0x15c
#define DDR3PHY_PHYREG58   0x160
#define DDR3PHY_PHYREGB0   0x2c0
#define DDR3PHY_PHYREGB1   0x2c4
#define DDR3PHY_PHYREGB2   0x2c8
#define DDR3PHY_PHYREGB3   0x2cc
#define DDR3PHY_PHYREGB4   0x2d0
#define DDR3PHY_PHYREGB5   0x2d4
#define DDR3PHY_PHYREGB6   0x2d8
#define DDR3PHY_PHYREGB7   0x2dc
#define DDR3PHY_PHYREGB8   0x2e0
#define DDR3PHY_PHYREGB9   0x2e4
#define DDR3PHY_PHYREGBA   0x2e8
#define DDR3PHY_PHYREGBB   0x2ec
#define DDR3PHY_PHYREGBC   0x2f0
#define DDR3PHY_PHYREGBD   0x2f4
#define DDR3PHY_PHYREGBE   0x2f8
#define DDR3PHY_PHYREG70   0x1c0
#define DDR3PHY_PHYREG71   0x1c4
#define DDR3PHY_PHYREG72   0x1c8
#define DDR3PHY_PHYREG73   0x1cc
#define DDR3PHY_PHYREG74   0x1d0
#define DDR3PHY_PHYREG75   0x1d4
#define DDR3PHY_PHYREG76   0x1d8
#define DDR3PHY_PHYREG77   0x1dc
#define DDR3PHY_PHYREG78   0x1e0
#define DDR3PHY_PHYREG79   0x1e4
#define DDR3PHY_PHYREG7A   0x1e8
#define DDR3PHY_PHYREG7B   0x1ec
#define DDR3PHY_PHYREG7C   0x1f0
#define DDR3PHY_PHYREG7D   0x1f4
#define DDR3PHY_PHYREG7E   0x1f8
#define DDR3PHY_PHYREG7F   0x1fc
#define DDR3PHY_PHYREG80   0x200
#define DDR3PHY_PHYREG81   0x204
#define DDR3PHY_PHYREG82   0x208
#define DDR3PHY_PHYREG83   0x20c
#define DDR3PHY_PHYREG84   0x210
#define DDR3PHY_PHYREG85   0x214
#define DDR3PHY_PHYREG86   0x218
#define DDR3PHY_PHYREG87   0x21c
#define DDR3PHY_PHYREG88   0x220
#define DDR3PHY_PHYREG89   0x224
#define DDR3PHY_PHYREG8A   0x228
#define DDR3PHY_PHYREG8B   0x22c
#define DDR3PHY_PHYREG8C   0x230
#define DDR3PHY_PHYREG8D   0x234
#define DDR3PHY_PHYREG8E   0x238
#define DDR3PHY_PHYREG8F   0x23c
#define DDR3PHY_PHYREG90   0x240
#define DDR3PHY_PHYREG91   0x244
#define DDR3PHY_PHYREG92   0x248
#define DDR3PHY_PHYREG93   0x24c
#define DDR3PHY_PHYREG94   0x250
#define DDR3PHY_PHYREG95   0x254
#define DDR3PHY_PHYREG96   0x258
#define DDR3PHY_PHYREG97   0x25c
#define DDR3PHY_PHYREG98   0x260
#define DDR3PHY_PHYREG99   0x264
#define DDR3PHY_PHYREG9A   0x268
#define DDR3PHY_PHYREG9B   0x26c
#define DDR3PHY_PHYREGC0   0x300
#define DDR3PHY_PHYREGC1   0x304
#define DDR3PHY_PHYREGC2   0x308
#define DDR3PHY_PHYREGC3   0x30c
#define DDR3PHY_PHYREGC4   0x310
#define DDR3PHY_PHYREGC5   0x314
#define DDR3PHY_PHYREGC6   0x318
#define DDR3PHY_PHYREGC7   0x31c
#define DDR3PHY_PHYREGC8   0x320
#define DDR3PHY_PHYREGC9   0x324
#define DDR3PHY_PHYREGCA   0x328
#define DDR3PHY_PHYREGCB   0x32c
#define DDR3PHY_PHYREGCC   0x330
#define DDR3PHY_PHYREGCD   0x334
#define DDR3PHY_PHYREGCE   0x338
#define DDR3PHY_PHYREGCF   0x33c
#define DDR3PHY_PHYREGD0   0x340
#define DDR3PHY_PHYREGD1   0x344
#define DDR3PHY_PHYREGD2   0x348
#define DDR3PHY_PHYREGD3   0x34c
#define DDR3PHY_PHYREGD4   0x350
#define DDR3PHY_PHYREGD5   0x354
#define DDR3PHY_PHYREGD6   0x358
#define DDR3PHY_PHYREGD7   0x35c
#define DDR3PHY_PHYREGD8   0x360
#define DDR3PHY_PHYREGD9   0x364
#define DDR3PHY_PHYREGDA   0x368
#define DDR3PHY_PHYREGDB   0x36c
#define DDR3PHY_PHYREGDC   0x370
#define DDR3PHY_PHYREGDD   0x374
#define DDR3PHY_PHYREGDE   0x378
#define DDR3PHY_PHYREGDF   0x37c
#define DDR3PHY_PHYREGE0   0x380
#define DDR3PHY_PHYREGE1   0x384
#define DDR3PHY_PHYREGE2   0x388
#define DDR3PHY_PHYREGE3   0x38c
#define DDR3PHY_PHYREGE4   0x390
#define DDR3PHY_PHYREGE5   0x394
#define DDR3PHY_PHYREGE6   0x398
#define DDR3PHY_PHYREGE7   0x39c
#define DDR3PHY_PHYREGE8   0x3a0
#define DDR3PHY_PHYREGE9   0x3a4
#define DDR3PHY_PHYREGEA   0x3a8
#define DDR3PHY_PHYREGEB   0x3ac
#define DDR3PHY_PHYREGEC   0x3b0
#define DDR3PHY_PHYREGED   0x3b4
#define DDR3PHY_PHYREGEE   0x3b8
#define DDR3PHY_PHYREGEF   0x3bc
#define DDR3PHY_PHYREGF0   0x3c0
#define DDR3PHY_PHYREGF1   0x3c4
#define DDR3PHY_PHYREGF2   0x3c8
#define DDR3PHY_PHYREGF8   0x3e0
#define DDR3PHY_PHYREGFA   0x3e8
#define DDR3PHY_PHYREGFB   0x3ec
#define DDR3PHY_PHYREGFC   0x3f0
#define DDR3PHY_PHYREGFD   0x3f4
#define DDR3PHY_PHYREGFE   0x3f8
#define DDR3PHY_PHYREGFF   0x3fc

#define DDR3PHY_CS0DQS_REG1 0x0b0
#define DDR3PHY_CS0DQS_REG2 0x0f0
#define DDR3PHY_CS0DQS_REG3 0x130
#define DDR3PHY_CS0DQS_REG4 0x170
#define DDR3PHY_CS1DQS_REG1 0x0b4
#define DDR3PHY_CS1DQS_REG2 0x0f4
#define DDR3PHY_CS1DQS_REG3 0x134
#define DDR3PHY_CS1DQS_REG4 0x174

#define DDR3PHY_REG_2C 0xB0
#define DDR3PHY_REG_3C 0xF0
#define DDR3PHY_REG_4C 0x130
#define DDR3PHY_REG_5C 0x170
#define DDR3PHY_REG_6C 0x1B0



//---------------------------------
//----------- PLB EM0 resources ---
#define EM0_PLB6MCIF2_DCR_BASE                0x80010000
#define PLB6MCIF2_BESR_read                   0x00
#define PLB6MCIF2_BESR_clear                  0x00
#define PLB6MCIF2_BESR_set                    0x01
#define PLB6MCIF2_BEARL                       0x02
#define PLB6MCIF2_BEARU                       0x03
#define PLB6MCIF2_INTR_EN                     0x04
#define PLB6MCIF2_PLBASYNC                    0x07
#define PLB6MCIF2_PLBORD                      0x08
#define PLB6MCIF2_PLBCFG                      0x09
#define PLB6MCIF2_MAXMEM                      0x0f
#define PLB6MCIF2_PUABA                       0x10
#define PLB6MCIF2_MR0CF                       0x11
#define PLB6MCIF2_MR1CF                       0x12
#define PLB6MCIF2_MR2CF                       0x13
#define PLB6MCIF2_MR3CF                       0x14

#define EM0_AXIMCIF2_DCR_BASE                 0x80020000
#define AXIMCIF2_BESR_read                    0x00
#define AXIMCIF2_BESR_clear                   0x00
#define AXIMCIF2_BESR_set                     0x01
#define AXIMCIF2_ERRID                        0x02
#define AXIMCIF2_AEARL                        0x03
#define AXIMCIF2_AEARU                        0x04
#define AXIMCIF2_AXIERRMSK                    0x05
#define AXIMCIF2_AXIASYNC                     0x06
#define AXIMCIF2_AXICFG                       0x07
#define AXIMCIF2_AXISTS                       0x09
#define AXIMCIF2_MR0CF                        0x10
#define AXIMCIF2_MR1CF                        0x11
#define AXIMCIF2_MR2CF                        0x12
#define AXIMCIF2_MR3CF                        0x13
#define AXIMCIF2_RID                          0x20

#define EM0_MCLFIR_DCR_BASE                   0x80030000

#define EM0_MCIF2ARB_DCR_BASE                 0x80040000
#define MCIF2ARB4_MACR                        0x00

#define EM0_DDR3LMC_DCR_BASE                  0x80050000
#define DDR34LMC_MCSTAT                       0x10                   // 0x60000000
#define DDR34LMC_MCOPT1                       0x20                   // 0x01020000
#define DDR34LMC_MCOPT2                       0x21                   // 0x80f30000 (Normal Reset) 0x80000000 (Recovery Reset)
#define DDR34LMC_IOCNTL                       0x30                   // 0x00000000
#define DDR34LMC_PHYCNTL                      0x31                   // 0x00000000
#define DDR34LMC_PHYSTAT                      0x32                   // System Dependant
#define DDR34LMC_PHYMASK                      0x33                   // 0x00000000
#define DDR34LMC_CFGR0                        0x40
#define DDR34LMC_CFGR1                        0x41
#define DDR34LMC_CFGR2                        0x42
#define DDR34LMC_CFGR3                        0x43
#define DDR34LMC_RHZEN                        0x48
#define DDR34LMC_INITSEQ0                     0x50
#define DDR34LMC_INITSEQ1                     0x52
#define DDR34LMC_INITSEQ2                     0x54
#define DDR34LMC_INITSEQ3                     0x56
#define DDR34LMC_INITSEQ4                     0x58
#define DDR34LMC_INITSEQ5                     0x5a
#define DDR34LMC_INITCMD0                     0x51
#define DDR34LMC_INITCMD1                     0x53
#define DDR34LMC_INITCMD2                     0x55
#define DDR34LMC_INITCMD3                     0x57
#define DDR34LMC_INITCMD4                     0x59
#define DDR34LMC_INITCMD5                     0x5b
#define DDR34LMC_SDTR0                        0x80
#define DDR34LMC_SDTR1                        0x81
#define DDR34LMC_SDTR2                        0x82
#define DDR34LMC_SDTR3                        0x83
#define DDR34LMC_SDTR4                        0x84
#define DDR34LMC_SDTR5                        0x85
#define DDR34LMC_DBG0                         0x8a
#define DDR34LMC_SMR0                         0x90
#define DDR34LMC_SMR1                         0x91
#define DDR34LMC_SMR2                         0x92
#define DDR34LMC_SMR3                         0x93
#define DDR34LMC_SMR4                         0x94
#define DDR34LMC_SMR5                         0x95
#define DDR34LMC_SMR6                         0x96
#define DDR34LMC_ODTR0                        0xa0
#define DDR34LMC_ODTR1                        0xa1
#define DDR34LMC_ODTR2                        0xa2
#define DDR34LMC_ODTR3                        0xa3
#define DDR34LMC_T_PHYUPD0                    0xcc
#define DDR34LMC_T_PHYUPD1                    0xcd
#define DDR34LMC_T_PHYUPD2                    0xce
#define DDR34LMC_T_PHYUPD3                    0xcf
                                              
#define PLB6_BC_BASE                          0x80000200
#define PLB6BC_TSNOOP                         0x02

//---------------------------------
//----------- Misc defs -----------
//SCTL
#define SCTL_BASE                             0x38000000
#define SCTL_PLL_STATE                        0x004


//---------------------------------
//----------- DDR parameters ------
#define DDR_ddr3_speed                        1//ddr3_speed: 1 - 1066; 2 - 1333
#define DDR_BurstLength                       0b00 //0b00: 8   0b10: 4
#define DDR_initMode                          0 //Spec requires 200us + 500us wait period before starting DDR reads/writes
#define DDR_eccMode                           0b00 //0b00 - off; 0b10 - GenerateCheckButDoNotCorrect; 0b11 - GenerateCheckAndCorrect
#define DDR_powerManagementMode               0b0000 //DisableAllPowerSavingFeatures
#define DDR_partialWriteMode                  0b1 //PartialWriteMode_DataMask; 0 - ReadModifyWrite

//For 1066
#if (DDR_ddr3_speed == 1)
    #define DDR_T_RDDATA_EN_BC4                   12  
    #define DDR_T_RDDATA_EN_BL8                   12  
    #define DDR_CL_PHY                            8  
    #define DDR_CL_MC                             0b100  
    #define DDR_CL_CHIP                           0b100  
    #define DDR_CWL_PHY                           6  
    #define DDR_CWL_MC                            0b000  
    #define DDR_CWL_CHIP                          0b001  
    #define DDR_AL_PHY                            7  
    #define DDR_AL_MC                             0b01  
    #define DDR_AL_CHIP                           0b01  
    #define DDR_WR                                0b100   //8
    #define DDR_T_REFI                            5999  
    #define DDR_T_RFC_XPR                         72   //MC_CLOCK       3750
    #define DDR_T_RCD                             0b1000  
    #define DDR_T_RP                              0b1000  
    #define DDR_T_RC                              28  
    #define DDR_T_RAS                             20  
    #define DDR_T_WTR                             0b0100   //4
    #define DDR_T_RTP                             0b0100  
    #define DDR_T_XSDLL                           32   //dfi_clk_1x       3750
    #define DDR_T_MOD                             8  
#endif
#if (DDR_ddr3_speed == 2)
    //For 1333
    #define DDR_T_RDDATA_EN_BC4       14  
    #define DDR_T_RDDATA_EN_BL8       14  
    #define DDR_CL_PHY                9  
    #define DDR_CL_MC                 0b101  
    #define DDR_CL_CHIP               0b101  
    #define DDR_CWL_PHY               7  
    #define DDR_CWL_MC                0b001  
    #define DDR_CWL_CHIP              0b001  
    #define DDR_AL_PHY                8  
    #define DDR_AL_MC                 0b01  
    #define DDR_AL_CHIP               0b01  
    #define DDR_WR                    0b101   //10
    #define DDR_T_REFI                7999  
    #define DDR_T_RFC_XPR             90  
    #define DDR_T_RCD                 0b1110  
    #define DDR_T_RP                  0b1110  
    #define DDR_T_RC                  33  
    #define DDR_T_RAS                 24  
    #define DDR_T_WTR                 0b0101  
    #define DDR_T_RTP                 0b0101  
    #define DDR_T_XSDLL               21  
    #define DDR_T_MOD                 10
#endif
#if (DDR_ddr3_speed == 3)
    // For 1600
    #define DDR_T_RDDATA_EN_BC4  18
    #define DDR_T_RDDATA_EN_BL8  18
    #define DDR_CL_PHY           11
    #define DDR_CL_MC            0b111
    #define DDR_CL_CHIP          0b111
    #define DDR_CWL_PHY          11
    #define DDR_CWL_MC           0b101
    #define DDR_CWL_CHIP         0b101
    #define DDR_AL_PHY           10
    #define DDR_AL_MC            0b01
    #define DDR_AL_CHIP          0b01
    #define DDR_WR               0b110
    #define DDR_T_REFI           9999
    #define DDR_T_RFC_XPR        108
    #define DDR_T_RCD            0b1101
    #define DDR_T_RP             0b1101
    #define DDR_T_RC             39
    #define DDR_T_RAS            28
    #define DDR_T_WTR            0b0110
    #define DDR_T_RTP            0b0110
    #define DDR_T_XSDLL          16
    #define DDR_T_MOD            12
#endif

//NWL PCIe AXI registers
#define BRIDGE_CORE_CFG_PCIE_RX1            0x00000004
#define BRIDGE_CORE_CFG_AXI_MASTER          0x00000008
#define BRIDGE_CORE_CFG_INTERRUPT           0x00000010
#define BRIDGE_CORE_CFG_PCIE_RX_MSG_FILTER  0x00000020

#define E_BREG_CONTROL                      0x00000208
#define E_BREG_BASE_LO                      0x00000210
#define E_BREG_BASE_HI                      0x00000214

#define E_ECAM_CONTROL                      0x00000228
#define E_ECAM_BASE_LO                      0x00000230
#define E_ECAM_BASE_HI                      0x00000234

#define E_DREG_CONTROL                      0x00000288
#define E_DREG_BASE_LO                      0x00000290
#define E_DREG_BASE_HI                      0x00000294

#define I_MSII_CONTROL                      0x00000308
#define I_MSII_BASE_LO                      0x00000310

#define I_MSIX_CONTROL                      0x00000328
#define I_MSIX_BASE_LO                      0x00000330

#define DMA0_SRC_Q_PTR_LO                   0x00000000
#define DMA0_SRC_Q_PTR_HI                   0x00000004
#define DMA0_SRC_Q_SIZE                     0x00000008
#define DMA0_SRC_Q_LIMIT                    0x0000000c
#define DMA0_DST_Q_PTR_LO                   0x00000010
#define DMA0_DST_Q_PTR_HI                   0x00000014
#define DMA0_DST_Q_SIZE                     0x00000018
#define DMA0_DST_Q_LIMIT                    0x0000001c
#define DMA0_STAS_Q_PTR_LO                  0x00000020
#define DMA0_STAS_Q_PTR_HI                  0x00000024
#define DMA0_STAS_Q_SIZE                    0x00000028
#define DMA0_STAS_Q_LIMIT                   0x0000002c
#define DMA0_STAD_Q_PTR_LO                  0x00000030
#define DMA0_STAD_Q_PTR_HI                  0x00000034
#define DMA0_STAD_Q_SIZE                    0x00000038
#define DMA0_STAD_Q_LIMIT                   0x0000003c
#define DMA0_SRC_Q_NEXT                     0x00000040
#define DMA0_DST_Q_NEXT                     0x00000044
#define DMA0_STAS_Q_NEXT                    0x00000048
#define DMA0_STAD_Q_NEXT                    0x0000004c
#define DMA0_AXI_INTERRUPT_CONTROL          0x00000068
#define DMA0_AXI_INTERRUPT_STATUS           0x0000006c
#define DMA0_DMA_CONTROL                    0x00000078
#define MSI_INT_BASE                        0x80000000

void ddr_init (int slow_down);  //  DDR initialisation subroutine

#endif /* end of include guard: DDR_INIT_H */
