// SPDX-FileCopyrightText: 2002-2024 PCSX2 Dev Team
// SPDX-License-Identifier: LGPL-3.0+

#include "DataInspectorNode.h"

#include "DebugTools/DebugInterface.h"

QString DataInspectorLocation::name() const
{
	switch (type)
	{
		case EE_REGISTER:
			return r5900Debug.getRegisterName(EECAT_GPR, address);
		case IOP_REGISTER:
			return r3000Debug.getRegisterName(IOPCAT_GPR, address);
		case EE_MEMORY:
			return QString("%1").arg(address, 8, 16);
		case IOP_MEMORY:
			return QString("IOP:%1").arg(address, 8, 16);
		default:
		{
		}
	}
	return QString();
}

DataInspectorLocation DataInspectorLocation::addOffset(u32 offset) const
{
	DataInspectorLocation location;
	switch (type)
	{
		case EE_MEMORY:
		case IOP_MEMORY:
			location.type = type;
			location.address = address + offset;
			break;
		default:
		{
		}
	}
	return location;
}

DataInspectorLocation DataInspectorLocation::createAddress(u32 address) const
{
	DataInspectorLocation location;
	switch (type)
	{
		case EE_REGISTER:
			location.type = EE_MEMORY;
			location.address = address;
			break;
		case IOP_REGISTER:
			location.type = IOP_MEMORY;
			location.address = address;
			break;
		case EE_MEMORY:
		case IOP_MEMORY:
			location.type = type;
			location.address = address;
			break;
		default:
		{
		}
	}
	return location;
}

DebugInterface& DataInspectorLocation::cpu() const
{
	if (type == IOP_REGISTER || type == IOP_MEMORY)
		return r3000Debug;
	else
		return r5900Debug;
}

u8 DataInspectorLocation::read8()
{
	switch (type)
	{
		case EE_REGISTER:
			if (address < 32) // TODO: Convert between DBX register numbers and PCSX2 scheme.
				return r5900Debug.getRegister(EECAT_GPR, address)._u8[0];
			break;
		case IOP_REGISTER:
			if (address < 32) // TODO: Convert between DBX register numbers and PCSX2 scheme.
				return r3000Debug.getRegister(IOPCAT_GPR, address)._u8[0];
			break;
		case EE_MEMORY:
			return (u8)r5900Debug.read8(address);
		case IOP_MEMORY:
			return (u8)r3000Debug.read8(address);
		default:
		{
		}
	}
	return 0;
}

u16 DataInspectorLocation::read16()
{
	switch (type)
	{
		case EE_REGISTER:
			if (address < 32) // TODO: Convert between DBX register numbers and PCSX2 scheme.
				return r5900Debug.getRegister(EECAT_GPR, address)._u16[0];
			break;
		case IOP_REGISTER:
			if (address < 32) // TODO: Convert between DBX register numbers and PCSX2 scheme.
				return r3000Debug.getRegister(IOPCAT_GPR, address)._u16[0];
			break;
		case EE_MEMORY:
			return (u16)r5900Debug.read16(address);
		case IOP_MEMORY:
			return (u16)r3000Debug.read16(address);
		default:
		{
		}
	}
	return 0;
}

u32 DataInspectorLocation::read32()
{
	switch (type)
	{
		case EE_REGISTER:
			if (address < 32) // TODO: Convert between DBX register numbers and PCSX2 scheme.
				return r5900Debug.getRegister(EECAT_GPR, address)._u32[0];
			break;
		case IOP_REGISTER:
			if (address < 32) // TODO: Convert between DBX register numbers and PCSX2 scheme.
				return r3000Debug.getRegister(IOPCAT_GPR, address)._u32[0];
			break;
		case EE_MEMORY:
			return r5900Debug.read32(address);
		case IOP_MEMORY:
			return r3000Debug.read32(address);
		default:
		{
		}
	}
	return 0;
}

u64 DataInspectorLocation::read64()
{
	switch (type)
	{
		case EE_REGISTER:
			if (address < 32) // TODO: Convert between DBX register numbers and PCSX2 scheme.
				return r5900Debug.getRegister(EECAT_GPR, address)._u64[0];
			break;
		case IOP_REGISTER:
			if (address < 32) // TODO: Convert between DBX register numbers and PCSX2 scheme.
				return r3000Debug.getRegister(IOPCAT_GPR, address)._u64[0];
			break;
		case EE_MEMORY:
			return r5900Debug.read64(address);
		case IOP_MEMORY:
			return r3000Debug.read64(address);
		default:
		{
		}
	}
	return 0;
}

u128 DataInspectorLocation::read128()
{
	switch (type)
	{
		case EE_REGISTER:
			if (address < 32) // TODO: Convert between DBX register numbers and PCSX2 scheme.
				return r5900Debug.getRegister(EECAT_GPR, address);
			break;
		case IOP_REGISTER:
			if (address < 32) // TODO: Convert between DBX register numbers and PCSX2 scheme.
				return r3000Debug.getRegister(IOPCAT_GPR, address);
			break;
		case EE_MEMORY:
			return r5900Debug.read128(address);
		case IOP_MEMORY:
			return r3000Debug.read128(address);
		default:
		{
		}
	}
	return u128::From32(0);
}

void DataInspectorLocation::write8(u8 value)
{
	switch (type)
	{
		case EE_REGISTER:
			if (address < 32) // TODO: Convert between DBX register numbers and PCSX2 scheme.
				r5900Debug.setRegister(EECAT_GPR, address, u128::From32(value));
			break;
		case IOP_REGISTER:
			if (address < 32) // TODO: Convert between DBX register numbers and PCSX2 scheme.
				r3000Debug.setRegister(IOPCAT_GPR, address, u128::From32(value));
			break;
		case EE_MEMORY:
			r5900Debug.write8(address, value);
			break;
		case IOP_MEMORY:
			r3000Debug.write8(address, value);
		default:
		{
		}
	}
}

void DataInspectorLocation::write16(u16 value)
{
	switch (type)
	{
		case EE_REGISTER:
			if (address < 32) // TODO: Convert between DBX register numbers and PCSX2 scheme.
				r5900Debug.setRegister(EECAT_GPR, address, u128::From32(value));
			break;
		case IOP_REGISTER:
			if (address < 32) // TODO: Convert between DBX register numbers and PCSX2 scheme.
				r3000Debug.setRegister(IOPCAT_GPR, address, u128::From32(value));
			break;
		case EE_MEMORY:
			r5900Debug.write16(address, value);
			break;
		case IOP_MEMORY:
			r3000Debug.write16(address, value);
		default:
		{
		}
	}
}

void DataInspectorLocation::write32(u32 value)
{
	switch (type)
	{
		case EE_REGISTER:
			if (address < 32) // TODO: Convert between DBX register numbers and PCSX2 scheme.
				r5900Debug.setRegister(EECAT_GPR, address, u128::From32(value));
			break;
		case IOP_REGISTER:
			if (address < 32) // TODO: Convert between DBX register numbers and PCSX2 scheme.
				r3000Debug.setRegister(IOPCAT_GPR, address, u128::From32(value));
			break;
		case EE_MEMORY:
			r5900Debug.write32(address, value);
			break;
		case IOP_MEMORY:
			r3000Debug.write32(address, value);
		default:
		{
		}
	}
}

void DataInspectorLocation::write64(u64 value)
{
	switch (type)
	{
		case EE_REGISTER:
			if (address < 32) // TODO: Convert between DBX register numbers and PCSX2 scheme.
				r5900Debug.setRegister(EECAT_GPR, address, u128::From64(value));
			break;
		case IOP_REGISTER:
			if (address < 32) // TODO: Convert between DBX register numbers and PCSX2 scheme.
				r3000Debug.setRegister(IOPCAT_GPR, address, u128::From64(value));
			break;
		case EE_MEMORY:
			r5900Debug.write64(address, value);
			break;
		case IOP_MEMORY:
			r3000Debug.write64(address, value);
		default:
		{
		}
	}
}

void DataInspectorLocation::write128(u128 value)
{
	switch (type)
	{
		case EE_REGISTER:
			if (address < 32) // TODO: Convert between DBX register numbers and PCSX2 scheme.
				r5900Debug.setRegister(EECAT_GPR, address, value);
			break;
		case IOP_REGISTER:
			if (address < 32) // TODO: Convert between DBX register numbers and PCSX2 scheme.
				r3000Debug.setRegister(IOPCAT_GPR, address, value);
			break;
		case EE_MEMORY:
			r5900Debug.write128(address, value);
			break;
		case IOP_MEMORY:
			r3000Debug.write128(address, value);
		default:
		{
		}
	}
}