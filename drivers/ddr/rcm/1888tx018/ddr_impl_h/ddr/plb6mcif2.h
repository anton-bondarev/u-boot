/**
 *
 */

#ifndef __DDR_PLB6MCIF2_H__
#define __DDR_PLB6MCIF2_H__


#include <stdint.h>
#include <stdbool.h>
#include "plb6mcif2/reg_access.h"

void plb6mcif2_simple_init( uint32_t base_addr, const uint32_t puaba );
void plb6mcif2_enable( uint32_t base_addr );
void plb6mcif2_disable( uint32_t base_addr );

#endif // __DDR_PLB6MCIF2_H__
