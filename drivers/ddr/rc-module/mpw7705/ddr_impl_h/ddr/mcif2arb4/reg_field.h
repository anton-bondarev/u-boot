/**
 *
 */

#ifndef __DDR_MCIF2ARB4_REG_FIELD_H__
#define __DDR_MCIF2ARB4_REG_FIELD_H__


#include "../../mivem_types.h"
#include "../../mivem_macro.h"


BEGIN_ENUM( MCIF2ARB4_MACR_FIELD )
DECLARE_ENUM_VAL( MCIF2ARB4_MACR_MPM_i,         28 )    // MCIF2 Arbiter Priority Mode
DECLARE_ENUM_VAL( MCIF2ARB4_MACR_MPM_n,         4 )
DECLARE_ENUM_VAL( MCIF2ARB4_MACR_MPM_e,         IBM_BIT_INDEX(32, MCIF2ARB4_MACR_MPM_i) )
DECLARE_ENUM_VAL( MCIF2ARB4_MACR_WD_DLY_i,      27 )    // Delay from MCIF Write Data Request to Write Data Returned
DECLARE_ENUM_VAL( MCIF2ARB4_MACR_WD_DLY_n,      1 )
DECLARE_ENUM_VAL( MCIF2ARB4_MACR_WD_DLY_e,      IBM_BIT_INDEX(32, MCIF2ARB4_MACR_WD_DLY_i) )
DECLARE_ENUM_VAL( MCIF2ARB4_MACR_ORDER_ATT_i,   8 )     // Configure the value of output signal O_M_ORDER_ATT[0:1]
DECLARE_ENUM_VAL( MCIF2ARB4_MACR_ORDER_ATT_n,   2 )
DECLARE_ENUM_VAL( MCIF2ARB4_MACR_ORDER_ATT_e,   IBM_BIT_INDEX(32, MCIF2ARB4_MACR_ORDER_ATT_i) )
/*
    TODO update according final specification
    #define MCIF2ARB4_MACR_PRIx( i, z )\
    REG_FIELD( CAT( MCIF2ARB4_MACR_WR_PRI, i ), (2*i+1),    1 ),\
    REG_FIELD( CAT( MCIF2ARB4_MACR_RD_PRI, i ), (2*i),      1 ),
    REPEAT( 4, MCIF2ARB4_MACR_PRIx, z )
*/
END_ENUM( MCIF2ARB4_BESR_FIELD )

BEGIN_ENUM( MCIF2ARB4_MPM )
DECLARE_ENUM_VAL( MCIF2ARB4_MPM_Fair_arbitration,   0x0 )   // Arbitration goes to the next higher master port with the pending request
DECLARE_ENUM_VAL( MCIF2ARB4_MPM_Fixed_arbitration,  0x1 )   // Arbitration uses the fixed mechanism to determine the highest priority master
END_ENUM( MCIF2ARB4_MPM )

BEGIN_ENUM( MCIF2ARB4_WD_DLY )
DECLARE_ENUM_VAL( MCIF2ARB4_WD_DLY_1_cycle_response,    0 )
DECLARE_ENUM_VAL( MCIF2ARB4_WD_DLY_2_cycle_response,    1 )
END_ENUM( MCIF2ARB4_WD_DLY )

BEGIN_ENUM( MCIF2ARB4_ORDER )
DECLARE_ENUM_VAL( MCIF2ARB4_Strict_in_order,        0x0 )
DECLARE_ENUM_VAL( MCIF2ARB4_Partial_out_of_order,   0x2 )
DECLARE_ENUM_VAL( MCIF2ARB4_Full_out_of_order,      0x3 )
END_ENUM( MCIF2ARB4_ORDER )


#endif // __DDR_MCIF2ARB4_REG_FIELD_H__
