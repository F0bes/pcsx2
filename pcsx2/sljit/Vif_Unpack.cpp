// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0

#include "Vif_Unpack.h"
#include "common/Perf.h"

static constexpr sljit_s32 vector_type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32;

static sljit_sw memOffset(const void* ptr)
{
	return static_cast<sljit_sw>(reinterpret_cast<uintptr_t>(ptr));
}

static void duplicatePair(const vReg dest, const vReg src, sljit_s32 pair)
{
	const sljit_s32 first_lane = pair * 2;

	sljit_emit_simd_lane_mov(C, SLJIT_SIMD_STORE | SLJIT_32 | vector_type, src, first_lane, SLJIT_R2, 0);
	sljit_emit_simd_lane_mov(C, SLJIT_SIMD_STORE | SLJIT_32 | vector_type, src, first_lane + 1, SLJIT_R3, 0);
	sljit_emit_simd_lane_mov(C, SLJIT_SIMD_LOAD | SLJIT_32 | vector_type, dest, 0, SLJIT_R2, 0);
	sljit_emit_simd_lane_mov(C, SLJIT_SIMD_LOAD | SLJIT_32 | vector_type, dest, 1, SLJIT_R3, 0);
	sljit_emit_simd_lane_mov(C, SLJIT_SIMD_LOAD | SLJIT_32 | vector_type, dest, 2, SLJIT_R2, 0);
	sljit_emit_simd_lane_mov(C, SLJIT_SIMD_LOAD | SLJIT_32 | vector_type, dest, 3, SLJIT_R3, 0);
}

static void clearLane(const vReg reg, sljit_s32 lane)
{
	sljit_emit_simd_lane_mov(C, SLJIT_SIMD_LOAD | SLJIT_32 | vector_type,
		reg, lane, SLJIT_IMM, 0);
}

// =====================================================================================================
//  VifUnpackSSE_Base Section
// =====================================================================================================
VifUnpack_Base::VifUnpack_Base()
	: usn(false)
	, doMask(false)
	, UnpkLoopIteration(0)
	, UnpkNoOfIterations(0)
	, IsAligned(0)
	, workReg(SLJIT_VR1)
	, destReg(SLJIT_VR0)
	, workGprW(SLJIT_R2)
	, dstIndirect(nullptr)
	, srcIndirect(nullptr)
{
}

void VifUnpack_Base::xMovDest() const
{
	if (!IsWriteProtectedOp())
	{
		if (IsUnmaskedOp())
			sljit_emit_simd_mov(C, SLJIT_SIMD_STORE | typeXMM, destReg,
				SLJIT_MEM1(SLJIT_R0), memOffset(dstIndirect));
		else
			doMaskWrite(destReg);
	}
}

void VifUnpack_Base::xShiftR(const vReg regX, int n) const
{
	for (sljit_s32 lane = 0; lane < 4; lane++)
	{
		sljit_emit_simd_lane_mov(C, SLJIT_SIMD_STORE | SLJIT_32 | vector_type,
			regX, lane, workGprW, 0);
		if (usn)
			sljit_emit_op2(C, SLJIT_LSHR32, workGprW, 0, workGprW, 0, SLJIT_IMM, n);
		else
			sljit_emit_op2(C, SLJIT_ASHR32, workGprW, 0, workGprW, 0, SLJIT_IMM, n);
		sljit_emit_simd_lane_mov(C, SLJIT_SIMD_LOAD | SLJIT_32 | vector_type,
			regX, lane, workGprW, 0);
	}
}

void VifUnpack_Base::xPMOVXX8(const vReg regX) const
{
	// TODO(Stenzek): Check this
	sljit_s32 type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8 | SLJIT_SIMD_EXTEND_32;
	if (!usn)
		type |= SLJIT_SIMD_EXTEND_SIGNED;
	sljit_emit_simd_extend(C, type, regX, SLJIT_MEM1(SLJIT_R1), memOffset(srcIndirect));
}

void VifUnpack_Base::xPMOVXX16(const vReg regX) const
{
	sljit_s32 type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16 | SLJIT_SIMD_EXTEND_32;
	if (!usn)
		type |= SLJIT_SIMD_EXTEND_SIGNED;
	sljit_emit_simd_extend(C, type, regX, SLJIT_MEM1(SLJIT_R1), memOffset(srcIndirect));
}

void VifUnpack_Base::xUPK_S_32() const
{
	if (UnpkLoopIteration == 0)
		sljit_emit_simd_mov(C, SLJIT_SIMD_LOAD | typeXMM, workReg,
			SLJIT_MEM1(SLJIT_R1), memOffset(srcIndirect));

	if (IsInputMasked())
		return;

	switch (UnpkLoopIteration)
	{
		case 0:
			sljit_emit_simd_lane_replicate(C, typeXMM, destReg, workReg, 0);
			break;
		case 1:
			sljit_emit_simd_lane_replicate(C, typeXMM, destReg, workReg, 1);
			break;
		case 2:
			sljit_emit_simd_lane_replicate(C, typeXMM, destReg, workReg, 2);
			break;
		case 3:
			sljit_emit_simd_lane_replicate(C, typeXMM, destReg, workReg, 3);
			break;
	}
}

void VifUnpack_Base::xUPK_S_16() const
{
	if (UnpkLoopIteration == 0)
		xPMOVXX16(workReg);

	if (IsInputMasked())
		return;

	switch (UnpkLoopIteration)
	{
		case 0:
			sljit_emit_simd_lane_replicate(C, typeXMM, destReg, workReg, 0);
			break;
		case 1:
			sljit_emit_simd_lane_replicate(C, typeXMM, destReg, workReg, 1);
			break;
		case 2:
			sljit_emit_simd_lane_replicate(C, typeXMM, destReg, workReg, 2);
			break;
		case 3:
			sljit_emit_simd_lane_replicate(C, typeXMM, destReg, workReg, 3);
			break;
	}
}

void VifUnpack_Base::xUPK_S_8() const
{
	if (UnpkLoopIteration == 0)
		xPMOVXX8(workReg);

	if (IsInputMasked())
		return;

	switch (UnpkLoopIteration)
	{
		case 0:
			sljit_emit_simd_lane_replicate(C, typeXMM, destReg, workReg, 0);
			break;
		case 1:
			sljit_emit_simd_lane_replicate(C, typeXMM, destReg, workReg, 1);
			break;
		case 2:
			sljit_emit_simd_lane_replicate(C, typeXMM, destReg, workReg, 2);
			break;
		case 3:
			sljit_emit_simd_lane_replicate(C, typeXMM, destReg, workReg, 3);
			break;
	}
}

// The V2 + V3 unpacks have freaky behaviour, the manual claims "indeterminate".
// After testing on the PS2, it's very much determinate in 99% of cases
// and games like Lemmings, And1 Streetball rely on this data to be like this!
// I have commented after each shuffle to show what data is going where - Ref

void VifUnpack_Base::xUPK_V2_32() const
{
	if (UnpkLoopIteration == 0)
	{
		sljit_emit_simd_mov(C, SLJIT_SIMD_LOAD | typeXMM, workReg,
			SLJIT_MEM1(SLJIT_R1), memOffset(srcIndirect));

		if (IsInputMasked())
			return;

		duplicatePair(destReg, workReg, 0); //v1v0v1v0
		if (IsAligned)
			clearLane(destReg, 3); //zero last word - tested on ps2
	}
	else
	{
		if (IsInputMasked())
			return;

		duplicatePair(destReg, workReg, 1); //v3v2v3v2
		if (IsAligned)
			clearLane(destReg, 3); //zero last word - tested on ps2
	}
}

void VifUnpack_Base::xUPK_V2_16() const
{
	if (UnpkLoopIteration == 0)
	{
		xPMOVXX16(workReg);

		if (IsInputMasked())
			return;

		duplicatePair(destReg, workReg, 0); //v1v0v1v0
	}
	else
	{
		if (IsInputMasked())
			return;

		duplicatePair(destReg, workReg, 1); //v3v2v3v2
	}
}

void VifUnpack_Base::xUPK_V2_8() const
{
	if (UnpkLoopIteration == 0)
	{
		xPMOVXX8(workReg);

		if (IsInputMasked())
			return;

		duplicatePair(destReg, workReg, 0); //v1v0v1v0
	}
	else
	{
		if (IsInputMasked())
			return;

		duplicatePair(destReg, workReg, 1); //v3v2v3v2
	}
}

void VifUnpack_Base::xUPK_V3_32() const
{
	if (IsInputMasked())
		return;

	sljit_emit_simd_mov(C, SLJIT_SIMD_LOAD | typeXMM, destReg,
		SLJIT_MEM1(SLJIT_R1), memOffset(srcIndirect));
	if (UnpkLoopIteration != IsAligned)
		clearLane(destReg, 3);
}

void VifUnpack_Base::xUPK_V3_16() const
{
	if (IsInputMasked())
		return;

	xPMOVXX16(destReg);

	//With V3-16, it takes the first vector from the next position as the W vector
	//However - IF the end of this iteration of the unpack falls on a quadword boundary, W becomes 0
	//IsAligned is the position through the current QW in the vif packet
	//Iteration counts where we are in the packet.
	int result = (((UnpkLoopIteration / 4) + 1 + (4 - IsAligned)) & 0x3);

	if ((UnpkLoopIteration & 0x1) == 0 && result == 0)
		clearLane(destReg, 3); //zero last word on QW boundary if whole 32bit word is used - tested on ps2
}

void VifUnpack_Base::xUPK_V3_8() const
{
	if (IsInputMasked())
		return;

	xPMOVXX8(destReg);
	if (UnpkLoopIteration != IsAligned)
		clearLane(destReg, 3);
}

void VifUnpack_Base::xUPK_V4_32() const
{
	if (IsInputMasked())
		return;

	sljit_emit_simd_mov(C, SLJIT_SIMD_LOAD | typeXMM, destReg,
		SLJIT_MEM1(SLJIT_R1), memOffset(srcIndirect));
}

void VifUnpack_Base::xUPK_V4_16() const
{
	if (IsInputMasked())
		return;

	xPMOVXX16(destReg);
}

void VifUnpack_Base::xUPK_V4_8() const
{
	if (IsInputMasked())
		return;

	xPMOVXX8(destReg);
}

void VifUnpack_Base::xUPK_V4_5() const
{
	if (IsInputMasked())
		return;

	sljit_emit_op1(C, SLJIT_MOV_U16, workGprW, 0,
		SLJIT_MEM1(SLJIT_R1), memOffset(srcIndirect));
	sljit_emit_op2(C, SLJIT_SHL32, workGprW, 0, workGprW, 0, SLJIT_IMM, 3); // ABG|R5.000
	sljit_emit_simd_replicate(C, typeXMM, destReg, workGprW, 0); // x|x|x|R
	sljit_emit_op2(C, SLJIT_LSHR32, workGprW, 0, workGprW, 0, SLJIT_IMM, 8); // ABG
	sljit_emit_op2(C, SLJIT_SHL32, workGprW, 0, workGprW, 0, SLJIT_IMM, 3); // AB|G5.000
	sljit_emit_simd_lane_mov(C, SLJIT_SIMD_LOAD | SLJIT_32 | vector_type, destReg, 1, workGprW, 0); // x|x|G|R
	sljit_emit_op2(C, SLJIT_LSHR32, workGprW, 0, workGprW, 0, SLJIT_IMM, 8); // AB
	sljit_emit_op2(C, SLJIT_SHL32, workGprW, 0, workGprW, 0, SLJIT_IMM, 3); // A|B5.000
	sljit_emit_simd_lane_mov(C, SLJIT_SIMD_LOAD | SLJIT_32 | vector_type, destReg, 2, workGprW, 0); // x|B|G|R
	sljit_emit_op2(C, SLJIT_LSHR32, workGprW, 0, workGprW, 0, SLJIT_IMM, 8); // A
	sljit_emit_op2(C, SLJIT_SHL32, workGprW, 0, workGprW, 0, SLJIT_IMM, 7); // A.0000000
	sljit_emit_simd_lane_mov(C, SLJIT_SIMD_LOAD | SLJIT_32 | vector_type, destReg, 3, workGprW, 0); // A|B|G|R

	for (sljit_s32 lane = 0; lane < 4; lane++)
	{
		sljit_emit_simd_lane_mov(C, SLJIT_SIMD_STORE | SLJIT_32 | vector_type, destReg, lane, workGprW, 0);
		sljit_emit_op2(C, SLJIT_AND32, workGprW, 0, workGprW, 0, SLJIT_IMM, 0xff); // can optimize to
		sljit_emit_simd_lane_mov(C, SLJIT_SIMD_LOAD | SLJIT_32 | vector_type, destReg, lane, workGprW, 0); // single AND...
	}
}

void VifUnpack_Base::xUnpack(int upknum) const
{
	switch (upknum)
	{
		case 0:
			xUPK_S_32();
			break;
		case 1:
			xUPK_S_16();
			break;
		case 2:
			xUPK_S_8();
			break;

		case 4:
			xUPK_V2_32();
			break;
		case 5:
			xUPK_V2_16();
			break;
		case 6:
			xUPK_V2_8();
			break;

		case 8:
			xUPK_V3_32();
			break;
		case 9:
			xUPK_V3_16();
			break;
		case 10:
			xUPK_V3_8();
			break;

		case 12:
			xUPK_V4_32();
			break;
		case 13:
			xUPK_V4_16();
			break;
		case 14:
			xUPK_V4_8();
			break;
		case 15:
			xUPK_V4_5();
			break;

		case 3:
		case 7:
		case 11:
			// TODO: Needs hardware testing.
			// Dynasty Warriors 5: Empire  - Player 2 chose a character menu.
			Console.Warning("Vpu/Vif: Invalid Unpack %d", upknum);
			break;
	}
}

// =====================================================================================================
//  VifUnpackSSE_Simple
// =====================================================================================================

VifUnpack_Simple::VifUnpack_Simple(bool usn_, bool domask_, int curCycle_)
{
	curCycle = curCycle_;
	usn = usn_;
	doMask = domask_;
	IsAligned = true;
}

void VifUnpack_Simple::doMaskWrite(const vReg regX) const
{
	const int offX = std::min(curCycle, 3);
	const sljit_sw dst_offset = memOffset(dstIndirect);

	sljit_emit_simd_mov(C, SLJIT_SIMD_LOAD | typeXMM, SLJIT_VR2,
		SLJIT_MEM1(SLJIT_R0), dst_offset);
	sljit_emit_simd_op2(C, SLJIT_SIMD_OP2_AND | typeXMM, regX, regX,
		SLJIT_MEM0(), reinterpret_cast<sljit_sw>(nVifMask[0][offX]));
	sljit_emit_simd_op2(C, SLJIT_SIMD_OP2_AND | typeXMM, SLJIT_VR2, SLJIT_VR2,
		SLJIT_MEM0(), reinterpret_cast<sljit_sw>(nVifMask[1][offX]));
	sljit_emit_simd_op2(C, SLJIT_SIMD_OP2_OR | typeXMM, regX, regX,
		SLJIT_MEM0(), reinterpret_cast<sljit_sw>(nVifMask[2][offX]));
	sljit_emit_simd_op2(C, SLJIT_SIMD_OP2_OR | typeXMM, regX, regX, SLJIT_VR2, 0);
	sljit_emit_simd_mov(C, SLJIT_SIMD_STORE | typeXMM, regX,
		SLJIT_MEM1(SLJIT_R0), dst_offset);
}

// ecx = dest, edx = src
static void nVifGen(int usn, int mask, int curCycle)
{

	int usnpart = usn * 2 * 16;
	int maskpart = mask * 16;

	VifUnpack_Simple vpugen(!!usn, !!mask, curCycle);

	for (int i = 0; i < 16; ++i)
	{
		nVifCall& ucall(nVifUpk[((usnpart + maskpart + i) * 4) + curCycle]);
		ucall = NULL;
		if (nVifT[i] == 0)
			continue;

		ucall = reinterpret_cast<nVifCall>(sljitStartBlock());
		sljit_emit_enter(C, 0, SLJIT_ARGS2V(P_R, P_R),
			4 | SLJIT_ENTER_VECTOR(6), 0, 0);
		vpugen.xUnpack(i);
		vpugen.xMovDest();
		sljit_emit_return_void(C);
		sljitEndBlock();
	}
}

void VifUnpack_Init()
{
	DevCon.WriteLn("Generating SLJIT-optimized unpacking functions for VIF interpreters...");

	u8* const start = SysMemory::GetVIFUnpackRec();
	sljitSetAsmPtr(start, SysMemory::GetVIFUnpackRecEnd() - start);

	for (int a = 0; a < 2; a++)
	{
		for (int b = 0; b < 2; b++)
		{
			for (int c = 0; c < 4; c++)
			{
				nVifGen(a, b, c);
			}
		}
	}

	Perf::any.Register(start, sljitGetAsmPtr() - start, "VIF Unpack");
}
