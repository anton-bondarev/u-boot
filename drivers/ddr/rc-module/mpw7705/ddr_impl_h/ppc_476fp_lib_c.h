#ifndef PPC_476FP_LIB_C_H
#define PPC_476FP_LIB_C_H

#include <stdint.h>

#define MSR_EE              48
#define MSR_CE              46

//MSR access
static inline void mtmsr( uint32_t const wval )
{
    asm volatile
    (
        "mtmsr %0 \n\t"
        ::"r"(wval)
    );
}

static inline uint32_t mfmsr(void)
{
    uint32_t rval=0;
    asm volatile
    (
        "mfmsr %0 \n\t"
        :"=r"(rval)
    );
    return rval;
}

#define rfi()\
    asm volatile\
    (\
        "rfi \n\t"\
    )

#define rfci()\
    asm volatile\
    (\
        "rfci \n\t"\
    )

#define rfmci()\
    asm volatile\
    (\
        "rfmci \n\t"\
    )

#define isync()\
    asm volatile\
    (\
        "isync \n\t"\
    )

#define mbar()\
    asm volatile\
    (\
        "mbar \n\t"\
    )

#define msync()\
    asm volatile\
    (\
        "msync \n\t"\
    )

#define lwsync()\
    asm volatile\
    (\
        "lwsync \n\t"\
    )

#define ici( CT )\
    asm volatile\
    (\
        "ici %0 \n\t"\
        ::"i"(CT)\
    )

#define dci( CT )\
    asm volatile\
    (\
        "dci %0 \n\t"\
        ::"i"(CT)\
    )

inline static void dcbi( void* const addr )
{
    asm volatile 
    (
        "dcbi 0, %0\n\t"
        ::"r"(&addr)
    );
}

inline static void sc(void) 
{
    asm volatile
    (
        "sc\n\t"
    );
}

inline static void trap(void) 
{
    asm volatile
    (
        "trap\n\t"
    );
}

#define nop()\
    asm volatile\
    (\
        "nop \n\t"\
    )



#endif  /* PPC_476FP_LIB_C_H */
