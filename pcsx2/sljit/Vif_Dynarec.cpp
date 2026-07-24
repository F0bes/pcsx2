// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0

#include "sljit/sljitHelpers.h"
#include "sljit/Vif_Unpack.h"
#include "MTVU.h"

#include "common/Assertions.h"
#include "common/Perf.h"
#include "common/StringUtil.h"


static void mVUmergeRegs(const vReg dest, const vReg src, int xyzw, bool modXYZW = false, bool canModifySrc = false)
{
	xyzw &= 0xf;
	if (dest == src || xyzw == 0)
		return;

	constexpr sljit_s32 vector_type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32;

	if (xyzw == 0xf)
	{
		sljit_emit_simd_mov(C, vector_type, dest, src, 0);
		return;
	}

	if (xyzw == 0xe && canModifySrc)
	{
		// Preserve W in src, then copy the now-complete vector to dest.
		sljit_emit_simd_lane_mov(C, SLJIT_SIMD_STORE | vector_type, dest, 3, SLJIT_R2, 0);
		sljit_emit_simd_lane_mov(C, SLJIT_SIMD_LOAD | vector_type, src, 3, SLJIT_R2, 0);
		sljit_emit_simd_mov(C, vector_type, dest, src, 0);
		return;
	}

	const bool move_x_to_selected_lane = modXYZW && (xyzw == 0x1 || xyzw == 0x2 || xyzw == 0x4);

	// VIF masks use X/Y/Z/W in bits 3/2/1/0, while SLJIT lane indices
	// are X/Y/Z/W = 0/1/2/3.
	for (sljit_s32 lane = 0; lane < 4; lane++)
	{
		if (!(xyzw & (0x8 >> lane)))
			continue;

		const sljit_s32 src_lane = move_x_to_selected_lane ? 0 : lane;
		sljit_emit_simd_lane_mov(C, SLJIT_SIMD_STORE | vector_type, src, src_lane, SLJIT_R2, 0);
		sljit_emit_simd_lane_mov(C, SLJIT_SIMD_LOAD | vector_type, dest, lane, SLJIT_R2, 0);
	}
}

static void maskedVecWrite(const vReg reg, const void* addr, int xyzw)
{
	constexpr sljit_s32 vector_type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32;
	const sljit_sw offset = static_cast<sljit_sw>(reinterpret_cast<uintptr_t>(addr));

	const auto store_lane = [reg, offset](sljit_s32 lane) {
		sljit_emit_simd_lane_mov(C, SLJIT_SIMD_STORE | vector_type, reg, lane,
			SLJIT_MEM1(SLJIT_R0), offset + (lane * sizeof(u32)));
	};

	switch (xyzw)
	{
		case 5: // YW
			store_lane(1);
			store_lane(3);
			break;

		case 9: // XW
			store_lane(0);
			store_lane(3);
			break;

		case 10: //XZ
			store_lane(0);
			store_lane(2);
			break;

		case 3: // ZW
			store_lane(2);
			store_lane(3);
			break;

		case 11: //XZW
			store_lane(0);
			store_lane(2);
			store_lane(3);
			break;

		case 13: // XYW
			store_lane(0);
			store_lane(1);
			store_lane(3);
			break;

		case 6: // YZ
			store_lane(1);
			store_lane(2);
			break;

		case 7: // YZW
			store_lane(1);
			store_lane(2);
			store_lane(3);
			break;

		case 12: // XY
			store_lane(0);
			store_lane(1);
			break;

		case 14: // XYZ
			store_lane(0);
			store_lane(1);
			store_lane(2);
			break;

		case 4: // Y
			store_lane(1);
			break;
		case 2: // Z
			store_lane(2);
			break;
		case 1: // W
			store_lane(3);
			break;
		case 8: // X
			store_lane(0);
			break;

		case 0:
			Console.Error("maskedVecWrite case 0!");
			break;

		default:
			sljit_emit_simd_mov(C, SLJIT_SIMD_STORE | vector_type, reg,
				SLJIT_MEM1(SLJIT_R0), offset);
			break; // XYZW
	}
}

static void addVec32(const vReg dest, const vReg src)
{
	constexpr sljit_s32 vector_type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32;

	for (sljit_s32 lane = 0; lane < 4; lane++)
	{
		sljit_emit_simd_lane_mov(C, SLJIT_SIMD_STORE | SLJIT_32 | vector_type, dest, lane, SLJIT_R2, 0);
		sljit_emit_simd_lane_mov(C, SLJIT_SIMD_STORE | SLJIT_32 | vector_type, src, lane, SLJIT_R3, 0);
		sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_R3, 0);
		sljit_emit_simd_lane_mov(C, SLJIT_SIMD_LOAD | SLJIT_32 | vector_type, dest, lane, SLJIT_R2, 0);
	}
}

static const void* offsetPointer(const void* ptr, sljit_sw offset)
{
	return reinterpret_cast<const void*>(reinterpret_cast<uintptr_t>(ptr) + offset);
}

void dVifReset(int idx)
{
	nVif[idx].vifBlocks.reset();

	const size_t offset = idx ? HostMemoryMap::VIF1recOffset : HostMemoryMap::VIF0recOffset;
	const size_t size = idx ? HostMemoryMap::VIF1recSize : HostMemoryMap::VIF0recSize;
	nVif[idx].recWritePtr = SysMemory::GetCodePtr(offset);
	nVif[idx].recEndPtr = nVif[idx].recWritePtr + (size - _256kb);
}

void dVifRelease(int idx)
{
	nVif[idx].vifBlocks.clear();
}

VifUnpack_Dynarec::VifUnpack_Dynarec(const nVifStruct& vif_, const nVifBlock& vifBlock_)
	: v(vif_)
	, vB(vifBlock_)
{
	const int wl = vB.wl ? vB.wl : 256; //0 is taken as 256 (KH2)
	isFill = (vB.cl < wl);
	usn = (vB.upkType >> 5) & 1;
	doMask = (vB.upkType >> 4) & 1;
	doMode = vB.mode & 3;
	IsAligned = vB.aligned;
	vCL = 0;
}

__fi void makeMergeMask(u32& x)
{
	x = ((x & 0x40) >> 6) | ((x & 0x10) >> 3) | (x & 4) | ((x & 1) << 3);
}

__fi void VifUnpack_Dynarec::SetMasks(int cS) const
{
	const int idx = v.idx;
	const vifStruct& vif = MTVU_VifX;

	//This could have ended up copying the row when there was no row to write.1810080
	u32 m0 = vB.mask; //The actual mask example 0x03020100
	u32 m3 = ((m0 & 0xaaaaaaaa) >> 1) & ~m0; //all the upper bits, so our example 0x01010000 & 0xFCFDFEFF = 0x00010000 just the cols (shifted right for maskmerge)
	u32 m2 = (m0 & 0x55555555) & (~m0 >> 1); // 0x1000100 & 0xFE7EFF7F = 0x00000100 Just the row

	if ((doMask && m2) || doMode)
	{
		cLoadSimd(xmmRow, &vif.MaskRow);

		MSKPATH3_LOG("Moving row");
	}
	if (doMask && m3)
	{
		VIF_LOG("Merging Cols");
		cLoadSimd(xmmCol0, &vif.MaskCol);
		if ((cS >= 2) && (m3 & 0x0000ff00))
			sljit_emit_simd_lane_replicate(C, typeXMM, xmmCol1, xmmCol0, 1);
		if ((cS >= 3) && (m3 & 0x00ff0000))
			sljit_emit_simd_lane_replicate(C, typeXMM, xmmCol2, xmmCol0, 2);
		if ((cS >= 4) && (m3 & 0xff000000))
			sljit_emit_simd_lane_replicate(C, typeXMM, xmmCol3, xmmCol0, 3);
		if ((cS >= 1) && (m3 & 0x000000ff))
			sljit_emit_simd_lane_replicate(C, typeXMM, xmmCol0, xmmCol0, 0);
	}
	//if (doMask||doMode) loadRowCol((nVifStruct&)v);
}

void VifUnpack_Dynarec::doMaskWrite(const vReg regX) const
{
	pxAssertMsg(regX == SLJIT_VR0 || regX == SLJIT_VR1, "Reg Overflow! VR2 thru VR5 are reserved for masking.");

	const int cc = std::min(vCL, 3);
	u32 m0 = (vB.mask >> (cc * 8)) & 0xff; //The actual mask example 0xE4 (protect, col, row, clear)
	u32 m3 = ((m0 & 0xaa) >> 1) & ~m0; //all the upper bits (cols shifted right) cancelling out any write protects 0x10
	u32 m2 = (m0 & 0x55) & (~m0 >> 1); // all the lower bits (rows)cancelling out any write protects 0x04
	u32 m4 = (m0 & ~((m3 << 1) | m2)) & 0x55; //  = 0xC0 & 0x55 = 0x40 (for merge mask)

	makeMergeMask(m2);
	makeMergeMask(m3);
	makeMergeMask(m4);

	if (doMask && m2) // Merge MaskRow
	{
		mVUmergeRegs(regX, xmmRow, m2);
	}

	if (doMask && m3) // Merge MaskCol
	{
		mVUmergeRegs(regX, SLJIT_VR(cc), m3);
	}

	if (doMode)
	{
		u32 m5 = ~(m2 | m3 | m4) & 0xf;

		if (!doMask)
			m5 = 0xf;

		if (m5 < 0xf)
		{
			if (doMode == 3)
			{
				mVUmergeRegs(xmmRow, regX, m5, false, false);
			}
			else
			{
				sljit_emit_simd_replicate(C, typeXMM, xmmTemp, SLJIT_IMM, 0);
				mVUmergeRegs(xmmTemp, xmmRow, m5, false, false);
				addVec32(regX, xmmTemp);
				if (doMode == 2)
					mVUmergeRegs(xmmRow, regX, m5, false, false);
			}
		}
		else
		{
			if (doMode == 3)
			{
				sljit_emit_simd_mov(C, typeXMM, xmmRow, regX, 0);
			}
			else
			{
				addVec32(regX, xmmRow);
				if (doMode == 2)
				{
					sljit_emit_simd_mov(C, typeXMM, xmmRow, regX, 0);
				}
			}
		}
	}

	if (doMask && m4)
		maskedVecWrite(regX, dstIndirect, m4 ^ 0xf);
	else
		sljit_emit_simd_mov(C, SLJIT_SIMD_STORE | typeXMM, regX,
			SLJIT_MEM1(SLJIT_R0), static_cast<sljit_sw>(reinterpret_cast<uintptr_t>(dstIndirect)));
}

void VifUnpack_Dynarec::writeBackRow() const
{
	const int idx = v.idx;
	sljit_emit_simd_mov(C, SLJIT_SIMD_STORE | typeXMM, xmmRow,
		SLJIT_MEM0(), reinterpret_cast<sljit_sw>(&(MTVU_VifX.MaskRow)));

	VIF_LOG("nVif: writing back row reg! [doMode = %d]", doMode);
}

void VifUnpack_Dynarec::ModUnpack(int upknum, bool PostOp)
{
	switch (upknum)
	{
		case 0:
		case 1:
		case 2:
			if (PostOp)
			{
				UnpkLoopIteration++;
				UnpkLoopIteration = UnpkLoopIteration & 0x3;
			}
			break;

		case 4:
		case 5:
		case 6:
			if (PostOp)
			{
				UnpkLoopIteration++;
				UnpkLoopIteration = UnpkLoopIteration & 0x1;
			}
			break;

		case 8:
			if (PostOp)
			{
				UnpkLoopIteration++;
				UnpkLoopIteration = UnpkLoopIteration & 0x1;
			}
			break;
		case 9:
			if (!PostOp)
			{
				UnpkLoopIteration++;
			}
			break;
		case 10:
			if (!PostOp)
			{
				UnpkLoopIteration++;
			}
			break;

		case 12:
			break;
		case 13:
			break;
		case 14:
			break;
		case 15:
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

void VifUnpack_Dynarec::ProcessMasks()
{
	skipProcessing = false;
	inputMasked = false;

	if (!doMask)
		return;

	const int cc = std::min(vCL, 3);
	const u32 full_mask = (vB.mask >> (cc * 8)) & 0xff;
	const u32 rowcol_mask = ((full_mask >> 1) | full_mask) & 0x55; // Rows or Cols being written instead of data, or protected.

	// Every channel is write protected for this cycle, no need to process anything.
	skipProcessing = full_mask == 0xff;

	// All channels are masked, no reason to process anything here.
	inputMasked = rowcol_mask == 0x55;
}

void VifUnpack_Dynarec::CompileRoutine()
{
	const int wl = vB.wl ? vB.wl : 256; //0 is taken as 256 (KH2)
	const int upkNum = vB.upkType & 0xf;
	const u8& vift = nVifT[upkNum];
	const int cycleSize = isFill ? vB.cl : wl;
	const int blockSize = isFill ? wl : vB.cl;
	const int skipSize = blockSize - cycleSize;

	uint vNum = vB.num ? vB.num : 256;
	doMode = (upkNum == 0xf) ? 0 : doMode; // V4_5 has no mode feature.
	UnpkNoOfIterations = 0;
	VIF_LOG("Compiling new block, unpack number %x, mode %x, masking %x, vNum %x", upkNum, doMode, doMask, vNum);

	pxAssume(vCL == 0);

	sljit_emit_enter(C, 0, SLJIT_ARGS2V(P_R, P_R),
		4 | SLJIT_ENTER_VECTOR(6), 0, 0);

	// Value passed determines # of col regs we need to load
	SetMasks(isFill ? blockSize : cycleSize);

	while (vNum)
	{
		// Determine if reads/processing can be skipped.
		ProcessMasks();

		if (vCL < cycleSize)
		{
			ModUnpack(upkNum, false);
			xUnpack(upkNum);
			xMovDest();
			ModUnpack(upkNum, true);

			dstIndirect = offsetPointer(dstIndirect, 16);
			srcIndirect = offsetPointer(srcIndirect, vift);

			vNum--;
			if (++vCL == blockSize)
				vCL = 0;
		}
		else if (isFill)
		{
			xUnpack(upkNum);
			xMovDest();

			// dstIndirect += 16;
			dstIndirect = offsetPointer(dstIndirect, 16);

			vNum--;
			if (++vCL == blockSize)
				vCL = 0;
		}
		else
		{
			// dstIndirect += (16 * skipSize);
			dstIndirect = offsetPointer(dstIndirect, 16 * skipSize);
			vCL = 0;
		}
	}

	if (doMode >= 2)
		writeBackRow();

	sljit_emit_return_void(C);
}

static u16 dVifComputeLength(uint cl, uint wl, u8 num, bool isFill)
{
	uint length = (num > 0) ? (num * 16) : 4096; // 0 = 256

	if (!isFill)
	{
		uint skipSize = (cl - wl) * 16;
		uint blocks = (num + (wl - 1)) / wl; //Need to round up num's to calculate skip size correctly.
		length += (blocks - 1) * skipSize;
	}

	return std::min(length, 0xFFFFu);
}

_vifT __fi nVifBlock* dVifCompile(nVifBlock& block, bool isFill)
{
	nVifStruct& v = nVif[idx];

	// Check size before the compilation
	if (v.recWritePtr >= v.recEndPtr)
	{
		DevCon.WriteLn("nVif Recompiler Cache Reset! [0x%016" PRIXPTR " > 0x%016" PRIXPTR "]",
			v.recWritePtr, v.recEndPtr);
		dVifReset(idx);
	}

	// Compile the block now
	sljitSetAsmPtr(v.recWritePtr, v.recEndPtr - v.recWritePtr);
	block.startPtr = reinterpret_cast<uptr>(sljitStartBlock());
	block.length = dVifComputeLength(block.cl, block.wl, block.num, isFill);
	v.vifBlocks.add(block);

	VifUnpack_Dynarec(v, block).CompileRoutine();

	u8* const start_ptr = v.recWritePtr;
	v.recWritePtr = sljitEndBlock();
	Perf::vif.RegisterPC(start_ptr, v.recWritePtr - start_ptr, block.upkType /* FIXME ideally a key*/);

	return &block;
}

_vifT __fi void dVifUnpack(const u8* data, bool isFill)
{
	nVifStruct& v = nVif[idx];
	vifStruct& vif = MTVU_VifX;
	VIFregisters& vifRegs = MTVU_VifXRegs;

	const u8 upkType = (vif.cmd & 0x1f) | (vif.usn << 5);
	const int doMask = isFill ? 1 : (vif.cmd & 0x10);

	nVifBlock block;

	// Performance note: initial code was using u8/u16 field of the struct
	// directly. However reading back the data (as u32) in HashBucket.find
	// leads to various memory stalls. So it is way faster to manually build the data
	// in u32 (aka x86 register).
	//
	// Warning the order of data in hash_key/key0/key1 depends on the nVifBlock struct
	u32 hash_key = (u32)(upkType & 0xFF) << 8 | (vifRegs.num & 0xFF);

	u32 key1 = ((u32)vifRegs.cycle.wl << 24) | ((u32)vifRegs.cycle.cl << 16) | ((u32)(vif.start_aligned & 0xFF) << 8) | ((u32)vifRegs.mode & 0xFF);
	if ((upkType & 0xf) != 9)
		key1 &= 0xFFFF01FF;

	// Zero out the mask parameter if it's unused -- games leave random junk
	// values here which cause false recblock cache misses.
	u32 key0 = doMask ? vifRegs.mask : 0;

	block.hash_key = hash_key;
	block.key0 = key0;
	block.key1 = key1;

	//DevCon.WriteLn("nVif%d: Recompiled Block!", idx);
	//DevCon.WriteLn(L"[num=% 3d][upkType=0x%02x][scl=%d][cl=%d][wl=%d][mode=%d][m=%d][mask=%s]",
	//	block.num, block.upkType, block.scl, block.cl, block.wl, block.mode,
	//	doMask >> 4, doMask ? wxsFormat( L"0x%08x", block.mask ).c_str() : L"ignored"
	//);

	// Seach in cache before trying to compile the block
	nVifBlock* b = v.vifBlocks.find(block);
	if (!b) [[unlikely]]
	{
		b = dVifCompile<idx>(block, isFill);
	}

	{ // Execute the block
		const VURegs& VU = vuRegs[idx];
		const uint vuMemLimit = idx ? 0x4000 : 0x1000;

		u8* startmem = VU.Mem + (vif.tag.addr & (vuMemLimit - 0x10));
		u8* endmem = VU.Mem + vuMemLimit;

		if ((startmem + b->length) <= endmem) [[likely]]
		{
#if 1
			// No wrapping, you can run the fast dynarec
			((nVifrecCall)b->startPtr)((uptr)startmem, (uptr)data);
#else
			// comparison mode
			static u8 tmpbuf[512 * 1024];
			((nVifrecCall)b->startPtr)((uptr)tmpbuf, (uptr)data);

			_nVifUnpack(idx, data, vifRegs.mode, isFill);

			const u32 words = b->length / 4;
			for (u32 i = 0; i < words; i++)
			{
				if (*((u32*)tmpbuf + i) != *((u32*)startmem + i))
				{
					// fprintf(stderr, "%08X %08X @ %u\n", *((u32*)tmpbuf + i), *((u32*)startmem + i), i);
					pauseCCC(*((u32*)tmpbuf + i), *((u32*)startmem + i), i);
					((nVifrecCall)b->startPtr)((uptr)tmpbuf, (uptr)data);
					break;
				}
			}
#endif
		}
		else
		{
			VIF_LOG("Running Interpreter Block: nVif%x - VU Mem Ptr Overflow; falling back to interpreter. Start = %x End = %x num = %x, wl = %x, cl = %x",
				v.idx, vif.tag.addr, vif.tag.addr + (block.num * 16), block.num, block.wl, block.cl);
			_nVifUnpack(idx, data, vifRegs.mode, isFill);
		}
	}
}

template void dVifUnpack<0>(const u8* data, bool isFill);
template void dVifUnpack<1>(const u8* data, bool isFill);
