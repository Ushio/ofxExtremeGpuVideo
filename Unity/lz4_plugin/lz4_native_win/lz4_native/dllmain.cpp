// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "lz4.h"

#define DLLAPI extern "C" __declspec(dllexport)

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


DLLAPI int lz4_decompress_safe_native(const char* source, char* dest, int compressedSize, int maxDecompressedSize) {
	return LZ4_decompress_safe(source, dest, compressedSize, maxDecompressedSize);
}