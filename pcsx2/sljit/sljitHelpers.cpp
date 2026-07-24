#include "sljitHelpers.h"

thread_local sljit_compiler* C;
thread_local u8* sljitAsmPtr;
thread_local size_t sljitAsmCapacity;


void sljitSetAsmPtr(void* ptr, size_t capacity)
{
	pxAssert(!C);
	sljitAsmPtr = static_cast<u8*>(ptr);
	sljitAsmCapacity = capacity;
}

void sljitAlignAsmPtr()
{
	static constexpr uintptr_t ALIGNMENT = 16;
	u8* new_ptr = reinterpret_cast<u8*>((reinterpret_cast<uintptr_t>(sljitAsmPtr) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1));
	pxAssert(static_cast<size_t>(new_ptr - sljitAsmPtr) <= sljitAsmCapacity);
	sljitAsmCapacity -= (new_ptr - sljitAsmPtr);
	sljitAsmPtr = new_ptr;
}

u8* sljitStartBlock()
{
	sljitAlignAsmPtr();

	HostSys::BeginCodeWrite();

	pxAssert(!C);
	C = sljit_create_compiler(nullptr);

	// sljit_emit_enter(...) to automagically save regs??

	return sljitAsmPtr;
}

u8* sljitEndBlock()
{
	pxAssert(C);

	sljit_generate_code_buffer code_buffer = 
	{
		.buffer = sljitAsmPtr,
		.size = sljitAsmCapacity, 
		.executable_offset = 0
	};

	auto ret = sljit_generate_code(C, SLJIT_GENERATE_CODE_BUFFER, &code_buffer);
	


	const u32 size = static_cast<u32>(sljit_get_generated_code_size(C));

	Console.Warning("sljitEndBlock. ret = %d, size = %d, ptr = %p ",ret,size,sljitAsmPtr);

	pxAssert(size < sljitAsmCapacity);

	sljit_free_compiler(C);
	C = nullptr;

	HostSys::EndCodeWrite();

	HostSys::FlushInstructionCache(sljitAsmPtr, size);

	sljitAsmPtr = sljitAsmPtr + size;
	sljitAsmCapacity -= size;
	return sljitAsmPtr;
}

void cLoadSimd(vReg reg, const void* ptr)
{
	pxAssert(C);
	sljit_emit_simd_mov(C, SLJIT_SIMD_LOAD | typeXMM, reg,
		SLJIT_MEM0(), reinterpret_cast<sljit_sw>(ptr));
}
