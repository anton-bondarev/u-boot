/**
 *
 */

#ifndef __DDR_MCIF2ARB4_H__
#define __DDR_MCIF2ARB4_H__


#include "mcif2arb4/reg_access.h"
#include <stdint.h>
#include <stdbool.h>


typedef struct mcif2arb4_cfg
{
    MCIF2ARB4_MPM       priority_mode:      MCIF2ARB4_MACR_MPM_n;
    MCIF2ARB4_WD_DLY    write_data_delay:   MCIF2ARB4_MACR_WD_DLY_n;
    MCIF2ARB4_ORDER     order_att:          MCIF2ARB4_MACR_ORDER_ATT_n;

} mcif2arb4_cfg;

void mcif2arb4_init( uint32_t base_addr,  mcif2arb4_cfg const * cfg );


#endif // __DDR_MCIF2ARB4_H__
