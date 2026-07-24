// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include "Common.h"
#include "common/Pcsx2Defs.h"
#include "common/HashCombine.h"

#include "sljitLir.h"

#define typeXMM (SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32)

typedef sljit_s32 vReg;
typedef sljit_s32 sReg;

static inline s64 GetPCDisplacement(const void* current, const void* target)
{
	return static_cast<s64>((reinterpret_cast<ptrdiff_t>(target) - reinterpret_cast<ptrdiff_t>(current)) >> 2);
}

extern thread_local sljit_compiler* C;
extern thread_local u8* sljitAsmPtr;
extern thread_local size_t sljitAsmCapacity;

static __fi bool sljitHasBlock()
{
	return (C != nullptr);
}

static __fi u8* sljitHGetCurrentCodePointer()
{
	return static_cast<u8*>(sljitAsmPtr) + C->executable_offset;
}

__fi static u8* sljitGetAsmPtr()
{
	return sljitAsmPtr;
}

void sljitSetAsmPtr(void* ptr, size_t capacity);
void sljitAlignAsmPtr();
u8* sljitStartBlock();
u8* sljitEndBlock();


void cLoadSimd(vReg reg, const void* ptr);
/*
void armDisassembleAndDumpCode(const void* ptr, size_t size);
void armEmitJmp(const void* ptr, bool force_inline = false);
void armEmitCall(const void* ptr, bool force_inline = false);
void armEmitCbnz(const vixl::aarch64::Register& reg, const void* ptr);
void armEmitCondBranch(vixl::aarch64::Condition cond, const void* ptr);
void armMoveAddressToReg(const vixl::aarch64::Register& reg, const void* addr);
void armLoadPtr(const vixl::aarch64::CPURegister& reg, const void* addr);
void armStorePtr(const vixl::aarch64::CPURegister& reg, const void* addr);
void armBeginStackFrame(bool save_fpr);
void armEndStackFrame(bool save_fpr);
bool armIsCalleeSavedRegister(int reg);

vixl::aarch64::MemOperand armOffsetMemOperand(const vixl::aarch64::MemOperand& op, s64 offset);
void armGetMemOperandInRegister(const vixl::aarch64::Register& addr_reg,
	const vixl::aarch64::MemOperand& op, s64 extra_offset = 0);

void armLoadConstant128(const vixl::aarch64::VRegister& reg, const void* ptr);

// may clobber RSCRATCH/RSCRATCH2. they shouldn't be inputs.
void armEmitVTBL(const vixl::aarch64::VRegister& dst, const vixl::aarch64::VRegister& src1,
	const vixl::aarch64::VRegister& src2, const vixl::aarch64::VRegister& tbl);

	*/
//////////////////////////////////////////////////////////////////////////
