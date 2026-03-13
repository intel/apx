/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Convert register names to their sizes and the indices used when
 * encoding instructions.
 */
#ifndef X86_KVM_INST_H
#define X86_KVM_INST_H

#ifdef __ASSEMBLER__

#define REG_NUM_INVALID		100

	.macro R32_NUM opd r32
	\opd = REG_NUM_INVALID
	.ifc \r32,%eax
	\opd = 0
	.endif
	.ifc \r32,%ecx
	\opd = 1
	.endif
	.ifc \r32,%edx
	\opd = 2
	.endif
	.ifc \r32,%ebx
	\opd = 3
	.endif
	.ifc \r32,%esp
	\opd = 4
	.endif
	.ifc \r32,%ebp
	\opd = 5
	.endif
	.ifc \r32,%esi
	\opd = 6
	.endif
	.ifc \r32,%edi
	\opd = 7
	.endif
#ifdef CONFIG_X86_64
	.ifc \r32,%r8d
	\opd = 8
	.endif
	.ifc \r32,%r9d
	\opd = 9
	.endif
	.ifc \r32,%r10d
	\opd = 10
	.endif
	.ifc \r32,%r11d
	\opd = 11
	.endif
	.ifc \r32,%r12d
	\opd = 12
	.endif
	.ifc \r32,%r13d
	\opd = 13
	.endif
	.ifc \r32,%r14d
	\opd = 14
	.endif
	.ifc \r32,%r15d
	\opd = 15
	.endif
#endif
	.endm

	.macro R64_NUM opd r64
	\opd = REG_NUM_INVALID
#ifdef CONFIG_X86_64
	.ifc \r64,%rax
	\opd = 0
	.endif
	.ifc \r64,%rcx
	\opd = 1
	.endif
	.ifc \r64,%rdx
	\opd = 2
	.endif
	.ifc \r64,%rbx
	\opd = 3
	.endif
	.ifc \r64,%rsp
	\opd = 4
	.endif
	.ifc \r64,%rbp
	\opd = 5
	.endif
	.ifc \r64,%rsi
	\opd = 6
	.endif
	.ifc \r64,%rdi
	\opd = 7
	.endif
	.ifc \r64,%r8
	\opd = 8
	.endif
	.ifc \r64,%r9
	\opd = 9
	.endif
	.ifc \r64,%r10
	\opd = 10
	.endif
	.ifc \r64,%r11
	\opd = 11
	.endif
	.ifc \r64,%r12
	\opd = 12
	.endif
	.ifc \r64,%r13
	\opd = 13
	.endif
	.ifc \r64,%r14
	\opd = 14
	.endif
	.ifc \r64,%r15
	\opd = 15
	.endif
#endif
#ifdef CONFIG_KVM_APX
	.ifc \r64,%r16
	\opd = 16
	.endif
	.ifc \r64,%r17
	\opd = 17
	.endif
	.ifc \r64,%r18
	\opd = 18
	.endif
	.ifc \r64,%r19
	\opd = 19
	.endif
	.ifc \r64,%r20
	\opd = 20
	.endif
	.ifc \r64,%r21
	\opd = 21
	.endif
	.ifc \r64,%r22
	\opd = 22
	.endif
	.ifc \r64,%r23
	\opd = 23
	.endif
	.ifc \r64,%r24
	\opd = 24
	.endif
	.ifc \r64,%r25
	\opd = 25
	.endif
	.ifc \r64,%r26
	\opd = 26
	.endif
	.ifc \r64,%r27
	\opd = 27
	.endif
	.ifc \r64,%r28
	\opd = 28
	.endif
	.ifc \r64,%r29
	\opd = 29
	.endif
	.ifc \r64,%r30
	\opd = 30
	.endif
	.ifc \r64,%r31
	\opd = 31
	.endif
#endif
	.endm

.macro REG_NUM reg_num reg
#ifdef CONFIG_X86_64
	R64_NUM \reg_num \reg
#else
	R32_NUM \reg_num \reg
#endif
.endm


#endif

#endif
