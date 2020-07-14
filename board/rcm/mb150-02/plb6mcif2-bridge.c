#include <common.h>
#include <asm/dcr.h>
#include "plb6mcif2-bridge.h"

DECLARE_GLOBAL_DATA_PTR;

#define DCR_PLB6_BC_BASE 0x80000200
#define DCR_EM2_PLB6MCIF2_BASE 0x80050000


#define IBM_BIT_INDEX(size, index ) (((size)-1) - ((index)%(size)))

#define reg_field(field_right_bit_num_from_ppc_user_manual, value) \
	((value) << IBM_BIT_INDEX(32, field_right_bit_num_from_ppc_user_manual))


#define PLB6BC_TSNOOP 0x02


#define PLB6MCIF2_INTR_EN 0x04
#define PLB6MCIF2_PLBASYNC 0x07
#define PLB6MCIF2_PLBORD 0x08
#define PLB6MCIF2_PLBCFG 0x09
#define PLB6MCIF2_MAXMEM 0x0F
#define PLB6MCIF2_PUABA 0x10
#define PLB6MCIF2_MR0CF 0x11
#define PLB6MCIF2_MR1CF 0x12
#define PLB6MCIF2_MR2CF 0x13
#define PLB6MCIF2_MR3CF 0x14


void plb6mcif2_init(void)
{
	const uint32_t puaba = 0x00;

	// enable logging but disable interrupts
	dcr_write(DCR_EM2_PLB6MCIF2_BASE + PLB6MCIF2_INTR_EN,
		reg_field(15, 0xFFFF) | reg_field(31, 0x0000));

	// PLB clock equals MCIF2 clock
	dcr_write(DCR_EM2_PLB6MCIF2_BASE + PLB6MCIF2_PLBASYNC,
		reg_field(1, 0b1));

	dcr_write(DCR_EM2_PLB6MCIF2_BASE + PLB6MCIF2_PLBORD,
		reg_field(0, 0b0) // raw
		| reg_field(3, 0b010) // threshold[1:3]
		| reg_field(7, 0b0) // threshold[0]
		| reg_field(16, 0b0) // IT_OOO
		| reg_field(18, 0b00)); // TOM

	// set MAXMEM = 8 GB
	dcr_write(DCR_EM2_PLB6MCIF2_BASE + PLB6MCIF2_MAXMEM,
		reg_field(1, 0b00));

	// set PLB upper address base address from I_S_ADDR[0:30]
	dcr_write(DCR_EM2_PLB6MCIF2_BASE + PLB6MCIF2_PUABA,
		puaba);

	// set Rank0 base addr
	// M_BA[0:2] = PUABA[28:30]
	// M_BA[3:12] = 0b0000000000
	dcr_write(DCR_EM2_PLB6MCIF2_BASE + PLB6MCIF2_MR0CF,
		reg_field(3, (puaba & 0b1110)) | reg_field(12, 0b0000000000)
		| reg_field(19, 0b1001) // set Rank0 size = 4GB
		| reg_field(31, 0b1)); // enable Rank0

	// set Rank1 base addr
	// M_BA[0:2] = PUABA[28:30]
	// M_BA[3:12] = 0b1000000000
	dcr_write(DCR_EM2_PLB6MCIF2_BASE + PLB6MCIF2_MR1CF,
		reg_field(3, (puaba & 0b1110)) | reg_field(12, 0b1000000000)
		| reg_field(19, 0b1001) // set Rank1 size = 4GB
		| reg_field(31, 0b1)); // enable Rank1

	// disable Rank2
	dcr_write(DCR_EM2_PLB6MCIF2_BASE + PLB6MCIF2_MR2CF,
		reg_field(31, 0b0));

	// disable Rank3
	dcr_write(DCR_EM2_PLB6MCIF2_BASE + PLB6MCIF2_MR3CF,
		reg_field(31, 0b0));

	const uint32_t Tsnoop = (dcr_read(DCR_PLB6_BC_BASE + PLB6BC_TSNOOP) & (0b1111 << 28)) >> 28;

	// PLB6MCIF2 enable [31] = 0b1
	dcr_write(DCR_EM2_PLB6MCIF2_BASE + PLB6MCIF2_PLBCFG,
		reg_field(0, 0b0) // Non-coherent
		| reg_field(4, Tsnoop) // T-snoop
		| reg_field(5, 0b0) // PmstrID_chk_EN
		| reg_field(27, 0b1111) // Parity enables
		| reg_field(30, 0b000) // CG_Enable
		| reg_field(31, 0b1)); // BR_Enable
}