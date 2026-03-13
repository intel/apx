/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __KVM_X86_VMENTER_H
#define __KVM_X86_VMENTER_H

#include "kvm-asm-offsets.h"

#define KVM_ENTER_VMRESUME			BIT(0)
#define KVM_ENTER_SAVE_SPEC_CTRL		BIT(1)
#define KVM_ENTER_CLEAR_CPU_BUFFERS_FOR_MMIO	BIT(2)
#define KVM_ENTER_EGPR_SWITCH			BIT(3)

#ifdef __ASSEMBLER__
.macro RESTORE_GUEST_SPEC_CTRL_BODY guest_spec_ctrl:req, label:req
	/*
	 * SPEC_CTRL handling: if the guest's SPEC_CTRL value differs from the
	 * host's, write the MSR.  This is kept out-of-line so that the common
	 * case does not have to jump.
	 *
	 * IMPORTANT: To avoid RSB underflow attacks and any other nastiness,
	 * there must not be any returns or indirect branches between this code
	 * and vmentry.
	 */
#ifdef CONFIG_X86_64
	mov \guest_spec_ctrl, %rdx
	cmp PER_CPU_VAR(x86_spec_ctrl_current), %rdx
	je \label
	movl %edx, %eax
	shr $32, %rdx
#else
	mov \guest_spec_ctrl, %eax
	mov PER_CPU_VAR(x86_spec_ctrl_current), %ecx
	xor %eax, %ecx
	mov 4 + \guest_spec_ctrl, %edx
	mov PER_CPU_VAR(x86_spec_ctrl_current + 4), %esi
	xor %edx, %esi
	or %esi, %ecx
	je \label
#endif
	mov $MSR_IA32_SPEC_CTRL, %ecx
	wrmsr
.endm

.macro RESTORE_HOST_SPEC_CTRL_BODY guest_spec_ctrl:req, enter_flags:req, label:req
	/* Same for after vmexit.  */
	mov $MSR_IA32_SPEC_CTRL, %ecx

	/*
	 * Load the value that the guest had written into MSR_IA32_SPEC_CTRL,
	 * if it was not intercepted during guest execution.
	 */
	testl $KVM_ENTER_SAVE_SPEC_CTRL, \enter_flags
	jz 998f
	rdmsr
	movl %eax, \guest_spec_ctrl
	movl %edx, 4 + \guest_spec_ctrl
998:
	/* Now restore the host value of the MSR if different from the guest's.  */
#ifdef CONFIG_X86_64
	mov PER_CPU_VAR(x86_spec_ctrl_current), %rdx
	cmp \guest_spec_ctrl, %rdx
	/*
	 * For legacy IBRS, the IBRS bit always needs to be written after
	 * transitioning from a less privileged predictor mode, regardless of
	 * whether the guest/host values differ.
	 */
	ALTERNATIVE __stringify(je \label), "", X86_FEATURE_KERNEL_IBRS
	movl %edx, %eax
	shr $32, %rdx
#else
	mov PER_CPU_VAR(x86_spec_ctrl_current), %eax
	mov \guest_spec_ctrl, %esi
	xor %eax, %esi
	mov PER_CPU_VAR(x86_spec_ctrl_current + 4), %edx
	mov 4 + \guest_spec_ctrl, %edi
	xor %edx, %edi
	or %edi, %esi
	ALTERNATIVE __stringify(je \label), "", X86_FEATURE_KERNEL_IBRS
#endif
	wrmsr
.endm

#define WORD_SIZE (BITS_PER_LONG / 8)
#define SEV_ES_GPRS_BASE 0x300

#ifdef CONFIG_X86_64
.macro CLEAR_REGS regs:vararg
 .irp i, \regs
	xor %r\i, %r\i
 .endr
.endm
.macro VMX_LOAD_REGS src:req, regs:vararg
 .irp i, \regs
	mov (VMX_vcpu_arch_regs + \i * WORD_SIZE)(\src), %r\i
 .endr
.endm
.macro VMX_STORE_REGS dst:req, regs:vararg
 .irp i, \regs
	mov %r\i, (VMX_vcpu_arch_regs + \i * WORD_SIZE)(\dst)
 .endr
.endm
.macro SVM_LOAD_REGS src:req, regs:vararg
 .irp i, \regs
	mov (SVM_vcpu_arch_regs + \i * WORD_SIZE)(\src), %r\i
 .endr
.endm
.macro SVM_STORE_REGS dst:req, regs:vararg
 .irp i, \regs
	mov %r\i, (SVM_vcpu_arch_regs + \i * WORD_SIZE)(\dst)
 .endr
.endm
.macro SEV_STORE_REGS dst:req, regs:vararg
 .irp i, \regs
	mov %r\i, (SEV_ES_GPRS_BASE + \i * WORD_SIZE)(\dst)
 .endr
.endm
#endif

#ifdef CONFIG_KVM_APX
.macro CLEAR_EGPRS
	CLEAR_REGS 16d,17d,18d,19d,20d,21d,22d,23d,24d,25d,26d,27d,28d,29d,30d,31d
.endm
.macro VMX_LOAD_EGPRS src:req
	VMX_LOAD_REGS \src, 16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
.endm
.macro VMX_STORE_EGPRS dst:req
	VMX_STORE_REGS \dst, 16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
.endm
#endif

#endif /* __ASSEMBLER__ */
#endif /* __KVM_X86_ENTER_FLAGS_H */
