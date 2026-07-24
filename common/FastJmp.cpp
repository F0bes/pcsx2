// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "FastJmp.h"

// Win32 uses Fastjmp.asm, because MSVC doesn't support inline asm.
#if !defined(_WIN32) || defined(ARCH_ARM64)

#if defined(__APPLE__)
#define PREFIX "_"
#else
#define PREFIX ""
#endif

#if defined(ARCH_X86)

asm(
	"\t.global " PREFIX "fastjmp_set\n"
	"\t.global " PREFIX "fastjmp_jmp\n"
	"\t.text\n"
	"\t" PREFIX "fastjmp_set:" R"(
	movq 0(%rsp), %rax
	movq %rsp, %rdx			# fixup stack pointer, so it doesn't include the call to fastjmp_set
	addq $8, %rdx
	movq %rax, 0(%rdi)	# actually rip
	movq %rbx, 8(%rdi)
	movq %rdx, 16(%rdi)	# actually rsp
	movq %rbp, 24(%rdi)
	movq %r12, 32(%rdi)
	movq %r13, 40(%rdi)
	movq %r14, 48(%rdi)
	movq %r15, 56(%rdi)
	xorl %eax, %eax
	ret
)"
	"\t" PREFIX "fastjmp_jmp:" R"(
	movl %esi, %eax
	movq 0(%rdi), %rdx	# actually rip
	movq 8(%rdi), %rbx
	movq 16(%rdi), %rsp	# actually rsp
	movq 24(%rdi), %rbp
	movq 32(%rdi), %r12
	movq 40(%rdi), %r13
	movq 48(%rdi), %r14
	movq 56(%rdi), %r15
	jmp *%rdx
)");

#elif defined(ARCH_ARM64)

asm(
	"\t.global " PREFIX "fastjmp_set\n"
	"\t.global " PREFIX "fastjmp_jmp\n"
	"\t.text\n"
	"\t.align 16\n"
	"\t" PREFIX "fastjmp_set:" R"(
	mov x16, sp
	stp x16, x30, [x0]
	stp x19, x20, [x0, #16]
	stp x21, x22, [x0, #32]
	stp x23, x24, [x0, #48]
	stp x25, x26, [x0, #64]
	stp x27, x28, [x0, #80]
	str x29, [x0, #96]
	stp d8, d9, [x0, #112]
	stp d10, d11, [x0, #128]
	stp d12, d13, [x0, #144]
	stp d14, d15, [x0, #160]
	mov w0, wzr
	br x30
)"
".align 16\n"
"\t" PREFIX "fastjmp_jmp:" R"(
	ldp x16, x30, [x0]
	mov sp, x16
	ldp x19, x20, [x0, #16]
	ldp x21, x22, [x0, #32]
	ldp x23, x24, [x0, #48]
	ldp x25, x26, [x0, #64]
	ldp x27, x28, [x0, #80]
	ldr x29, [x0, #96]
	ldp d8, d9, [x0, #112]
	ldp d10, d11, [x0, #128]
	ldp d12, d13, [x0, #144]
	ldp d14, d15, [x0, #160]
	mov w0, w1
	br x30
)");

#elif defined(ARCH_LOONGARCH64)

asm(
	"\t.global " PREFIX "fastjmp_set\n"
	"\t.global " PREFIX "fastjmp_jmp\n"
	"\t.text\n"
	"\t.p2align 4\n"

	"\t" PREFIX "fastjmp_set:" R"(
	st.d    $ra,  $a0, 0
	st.d    $sp,  $a0, 8

	st.d    $fp,  $a0, 16
	st.d    $s0,  $a0, 24
	st.d    $s1,  $a0, 32
	st.d    $s2,  $a0, 40
	st.d    $s3,  $a0, 48
	st.d    $s4,  $a0, 56
	st.d    $s5,  $a0, 64
	st.d    $s6,  $a0, 72
	st.d    $s7,  $a0, 80
	st.d    $s8,  $a0, 88

	fst.d   $fs0, $a0, 96
	fst.d   $fs1, $a0, 104
	fst.d   $fs2, $a0, 112
	fst.d   $fs3, $a0, 120
	fst.d   $fs4, $a0, 128
	fst.d   $fs5, $a0, 136
	fst.d   $fs6, $a0, 144
	fst.d   $fs7, $a0, 152

	move    $a0, $zero
	jr      $ra
)"

"\t.p2align 4\n"
"\t" PREFIX "fastjmp_jmp:" R"(
	# a0 = jump-buffer pointer
	# a1 = value returned by fastjmp_set

	# Keep the resumed PC in a caller-saved temporary.
	ld.d    $t0,  $a0, 0

	ld.d    $fp,  $a0, 16
	ld.d    $s0,  $a0, 24
	ld.d    $s1,  $a0, 32
	ld.d    $s2,  $a0, 40
	ld.d    $s3,  $a0, 48
	ld.d    $s4,  $a0, 56
	ld.d    $s5,  $a0, 64
	ld.d    $s6,  $a0, 72
	ld.d    $s7,  $a0, 80
	ld.d    $s8,  $a0, 88

	fld.d   $fs0, $a0, 96
	fld.d   $fs1, $a0, 104
	fld.d   $fs2, $a0, 112
	fld.d   $fs3, $a0, 120
	fld.d   $fs4, $a0, 128
	fld.d   $fs5, $a0, 136
	fld.d   $fs6, $a0, 144
	fld.d   $fs7, $a0, 152

	ld.d    $ra,  $a0, 0
	ld.d    $sp,  $a0, 8

	move    $a0, $a1
	jirl    $zero, $t0, 0
)");

#endif

#endif // __WIN32
