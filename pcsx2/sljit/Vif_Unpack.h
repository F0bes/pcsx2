// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include "Common.h"
#include "sljitHelpers.h"
#include "Vif_Dma.h"
#include "Vif_Dynarec.h"

#if SLJIT_NUMBER_OF_VECTOR_REGISTERS < 6
	#error need more vec regs :(
#endif

#define xmmCol0 SLJIT_VR(0)
#define xmmCol1 SLJIT_VR(1)
#define xmmCol2 SLJIT_VR(2)
#define xmmCol3 SLJIT_VR(3)
#define xmmRow SLJIT_VR(4)
#define xmmTemp SLJIT_VR(5)

// --------------------------------------------------------------------------------------
//  VifUnpack_Base
// --------------------------------------------------------------------------------------
class VifUnpack_Base
{
public:
	bool usn; // unsigned flag
	bool doMask; // masking write enable flag
	int UnpkLoopIteration;
	int UnpkNoOfIterations;
	int IsAligned;


protected:
	vReg workReg;
	vReg destReg;
	sReg workGprW;
	const void* dstIndirect;
	const void* srcIndirect;

public:
	VifUnpack_Base();
	virtual ~VifUnpack_Base() = default;

	virtual void xUnpack(int upktype) const;
	virtual bool IsWriteProtectedOp() const = 0;
	virtual bool IsInputMasked() const = 0;
	virtual bool IsUnmaskedOp() const = 0;
	virtual void xMovDest() const;

protected:
	virtual void doMaskWrite(const vReg regX) const = 0;

	virtual void xShiftR(const vReg regX, int n) const;
	virtual void xPMOVXX8(const vReg regX) const;
	virtual void xPMOVXX16(const vReg regX) const;

	virtual void xUPK_S_32() const;
	virtual void xUPK_S_16() const;
	virtual void xUPK_S_8() const;

	virtual void xUPK_V2_32() const;
	virtual void xUPK_V2_16() const;
	virtual void xUPK_V2_8() const;

	virtual void xUPK_V3_32() const;
	virtual void xUPK_V3_16() const;
	virtual void xUPK_V3_8() const;

	virtual void xUPK_V4_32() const;
	virtual void xUPK_V4_16() const;
	virtual void xUPK_V4_8() const;
	virtual void xUPK_V4_5() const;
};

// --------------------------------------------------------------------------------------
//  VifUnpackSSE_Simple
// --------------------------------------------------------------------------------------
class VifUnpack_Simple : public VifUnpack_Base
{
	typedef VifUnpack_Base _parent;

public:
	int curCycle;

public:
	VifUnpack_Simple(bool usn_, bool domask_, int curCycle_);
	virtual ~VifUnpack_Simple() = default;

	virtual bool IsWriteProtectedOp() const { return false; }
	virtual bool IsInputMasked() const { return false; }
	virtual bool IsUnmaskedOp() const { return !doMask; }

protected:
	virtual void doMaskWrite(const vReg regX) const;
};

// --------------------------------------------------------------------------------------
//  VifUnpackSSE_Dynarec
// --------------------------------------------------------------------------------------
class VifUnpack_Dynarec : public VifUnpack_Base
{
	typedef VifUnpack_Base _parent;

public:
	bool isFill;
	int doMode; // two bit value representing difference mode
	bool skipProcessing;
	bool inputMasked;

protected:
	const nVifStruct& v; // vif0 or vif1
	const nVifBlock& vB; // some pre-collected data from VifStruct
	int vCL; // internal copy of vif->cl

public:
	VifUnpack_Dynarec(const nVifStruct& vif_, const nVifBlock& vifBlock_);
	VifUnpack_Dynarec(const VifUnpack_Dynarec& src) // copy constructor
		: _parent(src)
		, v(src.v)
		, vB(src.vB)
	{
		isFill = src.isFill;
		vCL = src.vCL;
	}

	virtual ~VifUnpack_Dynarec() = default;

	virtual bool IsWriteProtectedOp() const { return skipProcessing; }
	virtual bool IsInputMasked() const { return inputMasked; }
	virtual bool IsUnmaskedOp() const { return !doMode && !doMask; }

	void ModUnpack(int upknum, bool PostOp);
	void ProcessMasks();
	void CompileRoutine();

protected:
	virtual void doMaskWrite(const vReg regX) const;
	void SetMasks(int cS) const;
	void writeBackRow() const;

	static VifUnpack_Dynarec FillingWrite(const VifUnpack_Dynarec& src)
	{
		VifUnpack_Dynarec fillingWrite(src);
		fillingWrite.doMask = true;
		fillingWrite.doMode = 0;
		return fillingWrite;
	}
};
