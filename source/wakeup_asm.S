/* ----------------------------------------------------------------------------
 * Copyright (c) 2017-2019 Semiconductor Components Industries, LLC
 * (d/b/a ON Semiconductor). All Rights Reserved.
 *
 * This code is the property of ON Semiconductor and may not be redistributed
 * in any form without prior written permission from ON Semiconductor. The
 * terms of use and warranty for this code are covered by contractual
 * agreements between ON Semiconductor and the licensee.
 * ----------------------------------------------------------------------------
 * wakeup_asm.S
 * - Initialize stack pointer and jump to Wakeup_From_Sleep_Application
 *   function to continue execution
 * ----------------------------------------------------------------------------
 * $Revision: 1.7 $
 * $Date: 2019/02/08 20:17:08 $
 * ------------------------------------------------------------------------- */

/* If using GCC compiler */
#if defined ( __GNUC__ )
    .thumb_func
    .section .app_wakeup_asm
    .globl Wakeup_From_Sleep_Application_asm
    .extern __stack, 4
    .extern Wakeup_From_Sleep_Application

Wakeup_From_Sleep_Application_asm:
    movw r0, #:lower16:__stack
    movt r0, #:upper16:__stack
    msr msp, r0
    b Wakeup_From_Sleep_Application
    .end

/* If using ARM compiler */
#elif defined ( __CC_ARM )
    THUMB
    AREA app_wakeup_asm, CODE, READONLY
    EXPORT Wakeup_From_Sleep_Application_asm
    IMPORT ||Image$$ARM_LIB_STACK$$ZI$$Limit||
    IMPORT Wakeup_From_Sleep_Application

Wakeup_From_Sleep_Application_asm PROC
    movw r0, #:lower16:||Image$$ARM_LIB_STACK$$ZI$$Limit||
    movt r0, #:upper16:||Image$$ARM_LIB_STACK$$ZI$$Limit||
    msr msp, r0
    b Wakeup_From_Sleep_Application
    ENDP
    END

/* If using IAR compiler */	
#else
    SECTION .app_wakeup_asm :CODE  
    THUMB
    PUBLIC Wakeup_From_Sleep_Application_asm
    EXTERN CSTACK$$Limit
    EXTERN Wakeup_From_Sleep_Application
    
Wakeup_From_Sleep_Application_asm;
    Mov32 r0, CSTACK$$Limit
    msr msp, r0
    b Wakeup_From_Sleep_Application    
    END
#endif
