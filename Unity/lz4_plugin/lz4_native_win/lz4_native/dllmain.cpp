// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "lz4.h"
#include "lz4frame.h"
#include "lz4hc.h"

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

DLLAPI int lz4_compress_default_native(const char* source, char* dest, int srcSize, int maxDestSize) {
	return LZ4_compress_default(source, dest, srcSize, maxDestSize);
}

DLLAPI int lz4_compressBound_native(int inputSize) {
	return LZ4_compressBound(inputSize);
}

DLLAPI int lz4_compress_HC_native(const char* source, char* dest, int srcSize, int maxDstSize, int compressionLevel) {
	return LZ4_compress_HC(source, dest, srcSize, maxDstSize, compressionLevel);
}