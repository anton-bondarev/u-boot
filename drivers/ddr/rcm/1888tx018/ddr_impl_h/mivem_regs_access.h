
#ifndef MIVEM_REGS_ACCESS_H_
#define MIVEM_REGS_ACCESS_H_

#include <stdint.h>
#include <configs/1888tx018.h>

#define RCM_DDR_LE32_TO_CPU(VAL) le32_to_cpu(VAL)
#define RCM_DDR_CPU_TO_LE32(VAL) cpu_to_le32(VAL)

#define REG_READ(DEV_NAME, REG, REG_SIZE)                                                               \
inline static uint##REG_SIZE##_t DEV_NAME##_read_##REG(uint32_t const base_addr)                        \
{                                                                                                       \
    return RCM_DDR_LE32_TO_CPU(*((volatile uint##REG_SIZE##_t*)(base_addr + REG)));                             \
}

#define REG_READ_ADDR_NAME(DEV_NAME, REG_ADDR, REG_NAME, REG_SIZE)                                      \
inline static uint##REG_SIZE##_t DEV_NAME##_read_##REG_NAME(uint32_t const base_addr)                   \
{                                                                                                       \
    return RCM_DDR_LE32_TO_CPU(*((volatile uint##REG_SIZE##_t*)(base_addr + REG_ADDR)));                        \
}

#define REG_READ_DCR(DEV_NAME, REG, REG_SIZE)                                                           \
inline static uint##REG_SIZE##_t DEV_NAME##_dcr_read_##REG(uint32_t const base_addr)                    \
{                                                                                                       \
    return mfdcrx(base_addr + REG);                                                                     \
}

#define REG_READ_DCR_ADDR_NAME(DEV_NAME, REG_ADDR, REG_NAME, REG_SIZE)                                  \
inline static uint##REG_SIZE##_t DEV_NAME##_dcr_read_##REG_NAME(uint32_t const base_addr)               \
{                                                                                                       \
    return mfdcrx(base_addr + REG_ADDR);                                                                \
}

#define REG_WRITE(DEV_NAME, REG, REG_SIZE)                                                              \
inline static void DEV_NAME##_write_##REG(uint32_t const base_addr, uint##REG_SIZE##_t const value)     \
{                                                                                                       \
    *((volatile uint##REG_SIZE##_t*)(base_addr + REG)) = RCM_DDR_CPU_TO_LE32(value);                            \
}

#define REG_WRITE_ADDR_NAME(DEV_NAME, REG_ADDR, REG_NAME, REG_SIZE)                                     \
inline static void DEV_NAME##_write_##REG_NAME(uint32_t const base_addr, uint##REG_SIZE##_t const value) \
{                                                                                                       \
    *((volatile uint##REG_SIZE##_t*)(base_addr + REG_ADDR)) = RCM_DDR_CPU_TO_LE32(value);                       \
}

#define REG_WRITE_DCR(DEV_NAME, REG, REG_SIZE)                                                          \
inline static void DEV_NAME##_dcr_write_##REG(uint32_t const base_addr, uint##REG_SIZE##_t const value) \
{                                                                                                       \
    mtdcrx(base_addr + REG, value);                                                                     \
}

#define REG_WRITE_DCR_ADDR_NAME(DEV_NAME, REG_ADDR, REG_NAME, REG_SIZE)                                 \
inline static void DEV_NAME##_dcr_write_##REG_NAME(uint32_t const base_addr, uint##REG_SIZE##_t const value) \
{                                                                                                       \
    mtdcrx(base_addr + REG_ADDR, value);                                                                \
}

#define REG_SET(DEV_NAME, REG, REG_SIZE)                                                                \
inline static void DEV_NAME##_set_##REG(uint32_t const base_addr, uint##REG_SIZE##_t const value)       \
{                                                                                                       \
    *((volatile uint##REG_SIZE##_t*)(base_addr + REG)) |= RCM_DDR_CPU_TO_LE32( value );                         \
}

#define REG_CLEAR(DEV_NAME, REG, REG_SIZE)                                                              \
inline static void DEV_NAME##_clear_##REG(uint32_t const base_addr, uint##REG_SIZE##_t const value)     \
{                                                                                                       \
    *((volatile uint##REG_SIZE##_t*)(base_addr + REG)) &= ~RCM_DDR_CPU_TO_LE32( value );                        \
}

#define REG_SET_DCR(DEV_NAME, REG, REG_SIZE)                                                            \
inline static void DEV_NAME##_dcr_set_##REG(uint32_t const base_addr, uint##REG_SIZE##_t const value)   \
{                                                                                                       \
    mtdcrx(base_addr + REG, (mfdcrx(base_addr + REG) | value));                                         \
}

#define REG_CLEAR_DCR(DEV_NAME, REG, REG_SIZE)                                                          \
inline static void DEV_NAME##_dcr_clear_##REG(uint32_t const base_addr, uint##REG_SIZE##_t const value) \
{                                                                                                       \
    mtdcrx(base_addr + REG, (mfdcrx(base_addr + REG) & ~value));                                        \
}

#define READ_MEM_REG(REG_NAME, REG_OFFSET, REG_SIZE)\
inline static uint##REG_SIZE##_t read_##REG_NAME(uint32_t const base_addr)\
{\
    return RCM_DDR_LE32_TO_CPU( *((volatile uint##REG_SIZE##_t*)(base_addr + REG_OFFSET)) );\
}

#define WRITE_MEM_REG(REG_NAME, REG_OFFSET, REG_SIZE)\
inline static void write_##REG_NAME(uint32_t const base_addr, uint32_t const value)\
{\
    *((volatile uint##REG_SIZE##_t*)(base_addr + REG_OFFSET)) = RCM_DDR_CPU_TO_LE32( value );\
}

#define READ_DCR_REG(REG_NAME, REG_OFFSET)\
inline static uint32_t read_##REG_NAME(uint32_t const base_addr)\
{\
    return mfdcrx(base_addr + REG_OFFSET);\
}

#define WRITE_DCR_REG(REG_NAME, REG_OFFSET)\
inline static void write_##REG_NAME(uint32_t const base_addr, uint32_t const value)\
{\
    mtdcrx(base_addr + REG_OFFSET, value);\
}

static inline void mtdcrx( uint32_t const addr, uint32_t const wval )
{
    asm volatile
    (
        "mtdcrx %0, %1 \n\t"
        ::"r"(addr), "r"(wval)
    );
}

static inline uint32_t mfdcrx( uint32_t const addr )
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


#endif /* MIVEM_REGS_ACCESS_H_ */
