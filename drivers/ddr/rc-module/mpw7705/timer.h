#ifndef TIMER_H
#define TIMER_H

#define TIMER_FREQ_MHZ		800

//SPR access
#define SPR_DEC         22

#define REG_READ_SPR(REG)		                    \
inline static uint32_t REG##_read(void)					\
{													\
    uint32_t rval=0;                                \
    asm volatile   (                                \
        "mfspr %0, %1 \n\t"                         \
        :"=r"(rval)                                 \
        :"i"(REG)                                   \
        :                                           \
    );                                              \
    return rval;                                    \
}													\

#define REG_WRITE_SPR(REG)                      	\
inline static void REG##_write(uint32_t const value)\
{													\
    asm volatile   (                                \
        "mtspr %1, %0 \n\t"                         \
        ::"r"(value), "i"(REG)                      \
        :                                           \
    );                                              \
}													\

REG_WRITE_SPR(SPR_DEC)
REG_READ_SPR(SPR_DEC)

void usleep(uint32_t usec);

#endif  /* TIMER_H */
