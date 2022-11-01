#include "pch.h"
#include "helper.h"
#include <intrin.h>

unsigned short SwapEndianness(unsigned short value)
{
	return _byteswap_ushort(value);
}

unsigned long SwapEndianness(unsigned long value)
{
	return _byteswap_ulong(value);
}

unsigned __int64 SwapEndianness(unsigned __int64 value)
{
	return _byteswap_uint64(value);
}