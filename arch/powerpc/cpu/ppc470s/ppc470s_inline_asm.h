/*
 *  Created on: Aug 7, 2015
 *      Author: a.korablinov
 */

#ifndef PPC470S_INLINE_ASM_H_
#define PPC470S_INLINE_ASM_H_

//DCR bus access
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

//MSR access
static inline void mtmsr( uint32_t const wval )
{
    asm volatile
    (
        "mtmsr %0 \n\t"
        ::"r"(wval)
    );
}

static inline uint32_t mfmsr()
{
    uint32_t rval=0;
    asm volatile
    (
        "mfmsr %0 \n\t"
        :"=r"(rval)
    );
    return rval;
}

#define rfi() \
    asm volatile\
    (\
        "rfi \n\t"\
    )

#define rfci() \
    asm volatile\
    (\
        "rfci \n\t"\
    )

#define rfmci() \
    asm volatile\
    (\
        "rfmci \n\t"\
    )

#define isync() \
    asm volatile\
    (\
        "isync \n\t"\
    )

#define mbar() \
    asm volatile\
    (\
        "mbar \n\t"\
    )

#define msync() \
    asm volatile\
    (\
        "msync \n\t"\
    )

inline static void dcbi(void* const addr)
{
    asm volatile
    (
        "dcbi 0, %0\n\t"
        ::"r"(&addr)
    );
}

#define ici(CT) \
    asm volatile\
    (\
        "ici %0 \n\t"\
        :\
        :"i"(CT)\
        :\
    )

#define dci(CT) \
    asm volatile\
    (\
        "dci %0 \n\t"\
        :\
        :"i"(CT)\
        :\
    )

inline static void sc()
{
    asm volatile
    (
        "sc\n\t"
    );
}

inline static void trap()
{
    asm volatile
    (
        "trap\n\t"
    );
}

#define nop() \
    asm volatile\
    (\
        "nop \n\t"\
    )

#endif /* PPC470S_INLINE_ASM_H_ */
