/**
 *
 */

#ifndef __DDR_PLB6MCIF2_REG_ACCESS_H__
#define __DDR_PLB6MCIF2_REG_ACCESS_H__


#include "reg.h"
#include "../../mivem_regs_access.h"


inline static uint32_t plb6mcif2_dcr_read_PLB6MCIF2_BESR( uint32_t const base_addr ) {
    return mfdcrx( base_addr+PLB6MCIF2_BESR_read );
}
inline static void plb6mcif2_dcr_set_PLB6MCIF2_BESR( uint32_t const base_addr, uint32_t const value ) {
    mtdcrx( base_addr+PLB6MCIF2_BESR_set, value );
}
inline static void plb6mcif2_dcr_clear_PLB6MCIF2_BESR( uint32_t const base_addr, uint32_t const value ) {
    mtdcrx( base_addr+PLB6MCIF2_BESR_clear, value );
}

REG_READ_DCR( plb6mcif2, PLB6MCIF2_BEARL, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_BEARU, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_INTR_EN, 32 )
REG_WRITE_DCR( plb6mcif2, PLB6MCIF2_INTR_EN, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_PLBASYNC, 32 )
REG_WRITE_DCR( plb6mcif2, PLB6MCIF2_PLBASYNC, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_PLBORD, 32 )
REG_WRITE_DCR( plb6mcif2, PLB6MCIF2_PLBORD, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_PLBCFG, 32 )
REG_WRITE_DCR( plb6mcif2, PLB6MCIF2_PLBCFG, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_MAXMEM, 32 )
REG_WRITE_DCR( plb6mcif2, PLB6MCIF2_MAXMEM, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_PUABA, 32 )
REG_WRITE_DCR( plb6mcif2, PLB6MCIF2_PUABA, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_MR0CF, 32 )
REG_WRITE_DCR( plb6mcif2, PLB6MCIF2_MR0CF, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_MR1CF, 32 )
REG_WRITE_DCR( plb6mcif2, PLB6MCIF2_MR1CF, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_MR2CF, 32 )
REG_WRITE_DCR( plb6mcif2, PLB6MCIF2_MR2CF, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_MR3CF, 32 )
REG_WRITE_DCR( plb6mcif2, PLB6MCIF2_MR3CF, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_STATUS, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_RID, 32 )


#endif // __DDR_PLB6MCIF2_REG_ACCESS_H__
