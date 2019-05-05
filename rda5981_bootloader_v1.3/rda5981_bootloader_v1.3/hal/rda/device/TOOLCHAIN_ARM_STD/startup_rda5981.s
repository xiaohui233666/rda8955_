;/*****************************************************************************
; * @file:    startup_RDA5981.s
; * @purpose: CMSIS Cortex-M4 Core Device Startup File
; *           for the RDA RDA5991H Device Series
; * @version: V1.0 init version
; * @date:    09. January 2016
; *------- <<< Use Configuration Wizard in Context Menu >>> ------------------
; *
; * Copyright (C) 2009 ARM Limited. All rights reserved.
; * ARM Limited (ARM) is supplying this software for use with Cortex-M3
; * processor based microcontrollers.  This file can be freely distributed
; * within development tools that are supporting such ARM based processors.
; *
; * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
; * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
; * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
; * ARM SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
; * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
; *
; *****************************************************************************/

; <h> Stack Configuration
;   <o> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>

Stack_Size      EQU     0x0004000

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp


; <h> Heap Configuration
;   <o>  Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>

Heap_Size       EQU     0x00004000

                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit

                PRESERVE8
                THUMB


; Vector Table Mapped to Address 0 at Reset

                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size


__Vectors       DCD     __initial_sp ; Top of Stack
                DCD     Reset_Handler             ; Reset Handler
                DCD     NMI_Handler               ; NMI Handler
                DCD     HardFault_Handler         ; Hard Fault Handler
                DCD     MemManage_Handler         ; MPU Fault Handler
                DCD     BusFault_Handler          ; Bus Fault Handler
                DCD     UsageFault_Handler        ; Usage Fault Handler
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     SVC_Handler               ; SVCall Handler
                DCD     DebugMon_Handler          ; Debug Monitor Handler
                DCD     0                         ; Reserved
                DCD     PendSV_Handler            ; PendSV Handler
                DCD     SysTick_Handler           ; SysTick Handler

                ; External Interrupts
                DCD     SPIFLASH_IRQHandler       ; 16: SPI Flash
                DCD     PTA_IRQHandler            ; 17: PTA
                DCD     SDIO_IRQHandler           ; 18: SDIO
                DCD     USBDMA_IRQHandler         ; 19: USB DMA
                DCD     USB_IRQHandler            ; 20: USB
                DCD     GPIO_IRQHandler           ; 21: GPIO
                DCD     TIMER0_IRQHandler         ; 22: Timer0
                DCD     UART0_IRQHandler          ; 23: UART0
                DCD     MACHW_IRQHandler          ; 24: MAC Hardware
                DCD     UART1_IRQHandler          ; 25: UART1
                DCD     AHBDMA_IRQHandler         ; 26: AHB DMA
                DCD     PSRAM_IRQHandler          ; 27: PSRAM
                DCD     SDMMC_IRQHandler          ; 28: SDMMC
                DCD     EXIF_IRQHandler           ; 29: EXIF
                DCD     I2C_IRQHandler            ; 30: I2C
__Vectors_End

__Vectors_Size  EQU     __Vectors_End - __Vectors


                AREA    |.text|, CODE, READONLY


; Reset Handler

Reset_Handler   PROC
                EXPORT  Reset_Handler             [WEAK]
                IMPORT  SystemInit
                IMPORT  __main
                LDR     R0, =SystemInit
                BLX     R0
                LDR     R0, =__main
                BX      R0
                ENDP

; Soft Reset

Soft_Reset      PROC
                MOV     R1, #0x04
                LDR     R0, [R1]
                BX      R0
                ENDP

; Dummy Exception Handlers (infinite loops which can be modified)

NMI_Handler     PROC
                EXPORT  NMI_Handler               [WEAK]
                B       .
                ENDP
HardFault_Handler\
                PROC
                EXPORT  HardFault_Handler         [WEAK]
                B       .
                ENDP
MemManage_Handler\
                PROC
                EXPORT  MemManage_Handler         [WEAK]
                B       .
                ENDP
BusFault_Handler\
                PROC
                EXPORT  BusFault_Handler          [WEAK]
                B       .
                ENDP
UsageFault_Handler\
                PROC
                EXPORT  UsageFault_Handler        [WEAK]
                B       .
                ENDP
SVC_Handler     PROC
                EXPORT  SVC_Handler               [WEAK]
                B       .
                ENDP
DebugMon_Handler\
                PROC
                EXPORT  DebugMon_Handler          [WEAK]
                B       .
                ENDP
PendSV_Handler  PROC
                EXPORT  PendSV_Handler            [WEAK]
                B       .
                ENDP
SysTick_Handler PROC
                EXPORT  SysTick_Handler           [WEAK]
                B       .
                ENDP

Default_Handler PROC

                EXPORT  SPIFLASH_IRQHandler       [WEAK]
                EXPORT  PTA_IRQHandler            [WEAK]
                EXPORT  SDIO_IRQHandler           [WEAK]
                EXPORT  USBDMA_IRQHandler         [WEAK]
                EXPORT  USB_IRQHandler            [WEAK]
                EXPORT  GPIO_IRQHandler           [WEAK]
                EXPORT  TIMER0_IRQHandler         [WEAK]
                EXPORT  UART0_IRQHandler          [WEAK]
                EXPORT  MACHW_IRQHandler          [WEAK]
                EXPORT  UART1_IRQHandler          [WEAK]
                EXPORT  AHBDMA_IRQHandler         [WEAK]
                EXPORT  PSRAM_IRQHandler          [WEAK]
                EXPORT  SDMMC_IRQHandler          [WEAK]
                EXPORT  EXIF_IRQHandler           [WEAK]
                EXPORT  I2C_IRQHandler            [WEAK]

SPIFLASH_IRQHandler
PTA_IRQHandler
SDIO_IRQHandler
USBDMA_IRQHandler
USB_IRQHandler
GPIO_IRQHandler
TIMER0_IRQHandler
UART0_IRQHandler
MACHW_IRQHandler
UART1_IRQHandler
AHBDMA_IRQHandler
PSRAM_IRQHandler
SDMMC_IRQHandler
EXIF_IRQHandler
I2C_IRQHandler

                B       .

                ENDP

                ALIGN


; User Initial Stack & Heap

		IF      :DEF:__MICROLIB

		EXPORT  __initial_sp
		EXPORT  __heap_base
		EXPORT  __heap_limit

		ELSE

		IMPORT  __use_two_region_memory
		EXPORT  __user_initial_stackheap
__user_initial_stackheap

		LDR     R0, =  Heap_Mem
		LDR     R1, =(Stack_Mem + Stack_Size)
		LDR     R2, = (Heap_Mem +  Heap_Size)
		LDR     R3, = Stack_Mem
		BX      LR

		ALIGN

		ENDIF


                END
