#ifndef __BOARD_H__
#define __BOARD_H__


/* Compiler Related Definitions */
#ifdef __CC_ARM                         /* ARM Compiler */
    #include <stdarg.h>
    #define SECTION(x)                  __attribute__((section(x)))
    #define ALIGN(n)                    __attribute__((aligned(n)))
    #define WEAK                        __weak
    #define rt_inline                   static __inline

#elif defined (__IAR_SYSTEMS_ICC__)     /* for IAR Compiler */
    #include <stdarg.h>
    #define SECTION(x)                  @ x
    #define PRAGMA(x)                   _Pragma(#x)
    #define ALIGN(n)                    PRAGMA(data_alignment=n)
    #define WEAK                        __weak
    #define rt_inline                   static inline

#elif defined (__GNUC__)                /* GNU GCC Compiler */
    #include <stdarg.h>
    #define SECTION(x)                  __attribute__((section(x)))
    #define ALIGN(n)                    __attribute__((aligned(n)))
    #define WEAK                        __attribute__((weak))
    #define rt_inline                   static __inline
#endif

void board_init(void);

#endif
