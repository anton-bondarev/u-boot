/**
 *
 */

#ifndef __DDR_AXIMCIF2_REG_ACCESS_H__
#define __DDR_AXIMCIF2_REG_ACCESS_H__


#include "reg.h"
#include "../../mivem_regs_access.h"


inline static uint32_t aximcif2_dcr_read_AXIMCIF2_BESR( uint32_t const base_addr ) {
    return mfdcrx( base_addr+AXIMCIF2_BESR_read );
}
inline static void aximcif2_dcr_set_AXIMCIF2_BESR( uint32_t const base_addr, uint32_t const value ) {
    mtdcrx( base_addr+AXIMCIF2_BESR_set, value );
}
inline static void aximcif2_dcr_clear_AXIMCIF2_BESR( uint32_t const base_addr, uint32_t const value ) {
    mtdcrx( base_addr+AXIMCIF2_BESR_clear, value );
}

REG_READ_DCR( aximcif2, AXIMCIF2_ERRID, 32 )

REG_READ_DCR( aximcif2, AXIMCIF2_AEARL, 32 )

REG_READ_DCR( aximcif2, AXIMCIF2_AEARU, 32 )

REG_READ_DCR( aximcif2, AXIMCIF2_AXIERRMSK, 32 )
REG_WRITE_DCR( aximcif2, AXIMCIF2_AXIERRMSK, 32 )

REG_READ_DCR( aximcif2, AXIMCIF2_AXIASYNC, 32 )
REG_WRITE_DCR( aximcif2, AXIMCIF2_AXIASYNC, 32 )

REG_READ_DCR( aximcif2, AXIMCIF2_AXICFG, 32 )
REG_WRITE_DCR( aximcif2, AXIMCIF2_AXICFG, 32 )

REG_READ_DCR( aximcif2, AXIMCIF2_AXISTS, 32 )

REG_READ_DCR( aximcif2, AXIMCIF2_MR0CF, 32 )
REG_WRITE_DCR( aximcif2, AXIMCIF2_MR0CF, 32 )

REG_READ_DCR( aximcif2, AXIMCIF2_MR1CF, 32 )
REG_WRITE_DCR( aximcif2, AXIMCIF2_MR1CF, 32 )

REG_READ_DCR( aximcif2, AXIMCIF2_MR2CF, 32 )
REG_WRITE_DCR( aximcif2, AXIMCIF2_MR2CF, 32 )

REG_READ_DCR( aximcif2, AXIMCIF2_MR3CF, 32 )
REG_WRITE_DCR( aximcif2, AXIMCIF2_MR3CF, 32 )


REG_READ_DCR( aximcif2, AXIMCIF2_RID, 32 )


#endif // __DDR_AXIMCIF2_REG_ACCESS_H__
