/**
 *
 */

#ifndef __PLB6BC_REG_H__
#define __PLB6BC_REG_H__


#include "reg_field.h"
#include "../mivem_types.h"

BEGIN_ENUM( PLB6BC_REG )
DECLARE_ENUM_VAL( PLB6BC_TSNOOP,                0x02 )          // TSNOOP                       NA              The value of this register is hard coded during configuration and does not change.
END_ENUM( PLB6BC_REG )

#endif // __PLB6BC_REG_H__
