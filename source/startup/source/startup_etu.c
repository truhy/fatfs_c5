/*
	MIT License

	Copyright (c) 2023 Truong Hy

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.

	Version: 20240205

	Bare-metal C startup initialisations for the Intel Cyclone V SoC (HPS), ARM Cortex-A9.
	Mostly using HWLib.

	References:
		- Cyclone V SoC: Cyclone V Hard Processor System Technical Reference Manual
		- MMU          : ARM Architecture Reference Manual ARMv7-A and ARMv7-R edition. Notable refs: B3.5.1 Short-descriptor translation table format descriptors
*/

#include "tru_config.h"

#if(TRU_EXIT_TO_UBOOT)

#include "alt_cache.h"
#include "alt_mmu.h"
#include "alt_interrupt.h"

#if(TRU_MMU_ENABLE)
	void mmu_init(void);
#endif

// =============
// Reset handler
// =============

// Notes:
	// - Newlib's startup() will initialise the .bss section
// - In Altera's HWLib their vector table is named __intc_interrupt_vector, see alt_interrupt.c
// - In Altera's HWLib, their vector table branches to _socfpga_main() as the reset handler, see alt_interrupt.c
void __attribute__((naked)) reset_handler(int argc, char *const argv[]){
	//uboot_argc = argc;
	//uboot_argv = (char **)argv;

	__asm__ volatile(
		"CPSID if                                      \n"  // Mask interrupts

		// Save U-Boot argc
		"LDR r3, =uboot_argc                           \n"
		"STR r0, [r3]                                  \n"

		// Save U-Boot argv
		"LDR r3, =uboot_argv                           \n"
		"STR r1, [r3]                                  \n"

		// Save U-Boot return address
		"LDR r3, =uboot_lr                             \n"
		"STR lr, [r3]                                  \n"

		// Save U-Boot processor mode
		"LDR r3, =uboot_cpsr                           \n"
		"MRS r2, cpsr                                  \n"
		"STR r2, [r3]                                  \n"

		// Save U-Boot stack pointer for each mode
		"CPS #0x11                                     \n"
		"LDR r3, =uboot_fiq_sp                         \n"
		"STR sp, [r3]                                  \n"
		"CPS #0x12                                     \n"
		"LDR r3, =uboot_irq_sp                         \n"
		"STR sp, [r3]                                  \n"
		"CPS #0x13                                     \n"
		"LDR r3, =uboot_svc_sp                         \n"
		"STR sp, [r3]                                  \n"
		"CPS #0x17                                     \n"
		"LDR r3, =uboot_abt_sp                         \n"
		"STR sp, [r3]                                  \n"
		"CPS #0x1B                                     \n"
		"LDR r3, =uboot_und_sp                         \n"
		"STR sp, [r3]                                  \n"
		"CPS #0x1F                                     \n"
		"LDR r3, =uboot_sys_sp                         \n"
		"STR sp, [r3]                                  \n"

		// Save VBAR (vector base address)
		"LDR r3, =uboot_vbar                           \n"
		"MRC p15, 0, r2, c12, c0, 0                    \n"
		"STR r2, [r3]                                  \n"

		// Switch into secure access mode
		"MRC p15, 0, r0, c1, c1, 2                          \n"  // Read NSACR (Non-secure Access Control Register)
		"ORR r0, r0, #(0x3 << 20)                           \n"  // Setup bits to enable access permissions.  Undocumented Altera/Intel Cyclone V SoC vendor specific
		"MCR p15, 0, r0, c1, c1, 2                          \n"  // Write NSACR
		"ISB                                                \n"

		// Setup stack for each exception mode
		// Note: When you call HWLib's interrupt init function the stacks will be change to a global variable array, and this setup will be dropped
		"CPS #0x11                                     \n"
		"LDR sp, =__FIQ_STACK_LIMIT                    \n"
		"CPS #0x12                                     \n"
		"LDR sp, =__IRQ_STACK_LIMIT                    \n"
		"CPS #0x13                                     \n"
		"LDR sp, =__SVC_STACK_LIMIT                    \n"
		"CPS #0x17                                     \n"
		"LDR sp, =__ABT_STACK_LIMIT                    \n"
		"CPS #0x1B                                     \n"
		"LDR sp, =__UND_STACK_LIMIT                    \n"
		"CPS #0x1F                                     \n"
		"LDR sp, =__SYS_STACK_LIMIT                    \n"
	);

#if(TRU_CLEAN_CACHE == 1U)
	// Since we are starting from U-Boot which may have the cache enabled,
	// loaded file(s) and some global variables may be cached and stay dirty.
	// Let's make sure that all dirty lines are written back into memory - in
	// case cache settings are changed later on
	alt_cache_l1_data_clean_all();
	alt_cache_l2_clean_all();
#endif

#if(TRU_SMP_COHERENCY_ENABLE != 2U)
	__asm__ volatile(
		// Disable SMP cache coherency support
		"MRC p15, 0, r0, c1, c0, 1                     \n"  // Read ACTLR
		"BIC r0, r0, #(0x1 << 6)                       \n"  // Clear bit 6 to participate in SMP coherency
		"BIC r0, r0, #(0x1 << 0)                       \n"  // Clear bit 0 to enable maintenance broadcast
		"MCR p15, 0, r0, c1, c0, 1                     \n"  // Write ACTLR
	);
#endif

#if(TRU_SCU_ENABLE != 2U)
	__asm__ volatile(
		// Disable SCU
		"LDR r0, =0xfffec000UL                         \n"  // Load SCU base register
		"LDR r1, [r0, #0x0]                            \n"  // Read SCU register
		"ORR r1, r1, #0x0                              \n"  // Clear bit 0 (The Enable bit)
		"STR r1, [r0, #0x0]                            \n"  // Write back modified value
	);
#endif

#if(TRU_L2_CACHE_ENABLE != 2U)
	alt_cache_l2_disable();
	alt_cache_l2_parity_disable();
	alt_cache_l2_prefetch_disable();
	alt_cache_l2_uninit();
#endif

#if(TRU_L1_CACHE_ENABLE != 2U)
	alt_cache_l1_disable_all();
#endif

#if(TRU_NEON_ENABLE == 1U)
	__asm__ volatile(
		// Enable permission and turn on NEON/VFP (FPU)
		"MRC p15, 0, r0, c1, c0, 2                     \n"  // Read CPACR (Coprocessor Access Control Register)
		"ORR r0, r0, #0x00F00000                       \n"  // Setup bits to enable access to NEON/VFP (Coprocessors 10 and 11)
		"MCR p15, 0, r0, c1, c0, 2                     \n"  // Write CPACR (Coprocessor Access Control Register)
		"ISB                                           \n"  // Ensures CPACR write have completed before continuing

		// Enable NEON
		"VMRS r0, fpexc                                \n"  // Read FPEXC (Floating-Point Exception Control register)
		"ORR r0, r0, #0x40000000                       \n"  // Setup bits to enable the NEON/VFP (Advanced SIMD and floating-point extensions)
		"VMSR fpexc, r0                                \n"  // Write FPEXC (Floating-Point Exception Control register)

		// Initialise VFP/NEON registers to 0
		"MOV r0, #0                                    \n"
		"VMOV d0, r0, r0                               \n"
		"VMOV d1, r0, r0                               \n"
		"VMOV d2, r0, r0                               \n"
		"VMOV d3, r0, r0                               \n"
		"VMOV d4, r0, r0                               \n"
		"VMOV d5, r0, r0                               \n"
		"VMOV d6, r0, r0                               \n"
		"VMOV d7, r0, r0                               \n"
		"VMOV d8, r0, r0                               \n"
		"VMOV d9, r0, r0                               \n"
		"VMOV d10, r0, r0                              \n"
		"VMOV d11, r0, r0                              \n"
		"VMOV d12, r0, r0                              \n"
		"VMOV d13, r0, r0                              \n"
		"VMOV d14, r0, r0                              \n"
		"VMOV d15, r0, r0                              \n"

		// Initialise FPSCR to a known state
		"VMRS r0, fpscr                                \n"
		"LDR r1, =0x00086060                           \n"
		"AND r0, r0, r1                                \n"
		"VMSR fpscr, r0                                \n"
	);
#endif

#if(TRU_MMU_ENABLE == 1U)
	mmu_init();                                             // Setup MMU table and enable MMU
#endif

#if(TRU_L1_CACHE_ENABLE == 1U)
	alt_cache_l1_enable_all();                              // Enable and invalidate L1 cache
#endif

#if(TRU_L2_CACHE_ENABLE == 1U)
	alt_cache_l2_init();
	alt_cache_l2_prefetch_enable();
	alt_cache_l2_parity_enable();
	alt_cache_l2_enable();                                  // Enable and invalidate L2 cache
#endif

#if(TRU_SCU_ENABLE == 1U)
	__asm__ volatile(
		// Invalidate SCU
		"LDR r0, =0xfffec000UL                         \n"  // Load SCU base register
		"LDR r1, =0xffff                               \n"  // Value to write
		"STR r1, [r0, #0xc]                            \n"  // Write to SCU Invalidate All register (0xfffec00c)

		// Enable SCU
		"LDR r0, =0xfffec000UL                         \n"  // Load SCU base register
		"LDR r1, [r0, #0x0]                            \n"  // Read SCU register
		"ORR r1, r1, #0x1                              \n"  // Set bit 0 (The Enable bit)
		"STR r1, [r0, #0x0]                            \n"  // Write back modified value
	);
#endif

#if(TRU_SMP_COHERENCY_ENABLE == 1U)
	__asm__ volatile(
		// Enable SMP cache coherency support
		"MRC p15, 0, r0, c1, c0, 1                     \n"  // Read ACTLR
		"ORR r0, r0, #(0x1 << 6)                       \n"  // Set bit 6 to participate in SMP coherency
		"ORR r0, r0, #(0x1 << 0)                       \n"  // Set bit 0 to enable maintenance broadcast
		"MCR p15, 0, r0, c1, c0, 1                     \n"  // Write ACTLR
	);
#endif

	__asm__ volatile(
		"CPSIE if                                      \n"  // Unmask interrupts

		"B _mainCRTStartup                             \n"  // Call C Run-Time library startup from newlib or libc, which will later call our main().  Alternatively, for newlib call BL _start (alias of the same function)
		//"BL _mainCRTStartup                            \n"  // Call C Run-Time library startup from newlib or libc, which will later call our main().  Alternatively, for newlib call BL _start (alias of the same function)
		// We don't expect the above to return
		//"_infinity_loop:                               \n"  // Catch unexpected main() return
		//"B _infinity_loop                              \n"
	);
}

// =======================
// Set HWLib reset handler
// =======================

// For exit to U-Boot we won't be using HWLib's vector, just setting this up to make it happy
#if(ALT_INT_PROVISION_VECTOR_SUPPORT != 0U)
	static void reset_handler_helper(void){ reset_handler(0, 0); }
	void _socfpga_main(void) __attribute__((unused, alias("reset_handler_helper")));  // Alias Altera's HWLib reset handler to our function
#endif

// =============================
// Override newlib _stack_init()
// =============================

// This makes newlib setup only the system stack
void _stack_init(void){
}

// ===================
// MMU initialisations
// ===================

// *Note: Altera's MMU alt_mmu_va_space_create() function will create a huge local array!!  Ensure your stack space is greater than 4K = 4096 bytes!
#if(TRU_MMU_ENABLE == 1U)
	#define TTB_ATTRIB_ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
	long unsigned int __attribute__((__section__("MMU_TTB"))) mmu_ttb[4096];  // This array is the MMU table.  It is placed at the specified linker section, aligned to 16KB, defined in the linker file

	// Define a dummy memory alloc for Altera's MMU function
	void *mmu_ttb_alloc(const size_t size, void *context){
		return mmu_ttb;
	}

	void mmu_init(void){
		long unsigned int *ttb1 = NULL;

		// Create MMU attributes (properties)
		// This is passed to the MMU function telling it what entries to create, and fills them into the MMU table
		// We only need multiple section entries (1 MB regions) of these 2 types
		ALT_MMU_MEM_REGION_t regions[] = {
			// Memory area: 3GB DDR-3 SDRAM
			{
				.va         = (void *)0x00000000,
				.pa         = (void *)0x00000000,
				.size       = 0xc0000000,
				.access     = ALT_MMU_AP_FULL_ACCESS,
				.attributes = ALT_MMU_ATTR_WBA,
				.shareable  = ALT_MMU_TTB_S_SHAREABLE,
				.execute    = ALT_MMU_TTB_XN_DISABLE,
				.security   = ALT_MMU_TTB_NS_NON_SECURE
			},
			// Device area: 1GB of everything else
			{
				.va         = (void *)0xc0000000,
				.pa         = (void *)0xc0000000,
				.size       = 0x40000000,
				.access     = ALT_MMU_AP_FULL_ACCESS,
				.attributes = ALT_MMU_ATTR_DEVICE,
				.shareable  = ALT_MMU_TTB_S_SHAREABLE,
				.execute    = ALT_MMU_TTB_XN_ENABLE,
				.security   = ALT_MMU_TTB_NS_SECURE
			}

		};

		alt_mmu_init();
		alt_mmu_va_space_storage_required(regions, TTB_ATTRIB_ARRAY_SIZE(regions));
		alt_mmu_va_space_create(&ttb1, regions, TTB_ATTRIB_ARRAY_SIZE(regions), mmu_ttb_alloc, NULL);
		alt_mmu_va_space_enable(ttb1);
	}
#endif

#endif
