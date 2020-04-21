#include <stdint.h>
#include "libdeflate2/lib/libdeflate.h"
#include <Windows.h>
#include <stdio.h>
#include "compressor.h"

const int SIZE_ARR = 10;
//const int SIZE_ARR = 100000000;//13953
//const int SIZE_ARR = 1000000;//172
const int LEVEL = 1;

int main()
{
	uint8_t* source = (uint8_t*)malloc(3 * SIZE_ARR),
		* buffer = source + SIZE_ARR,
		* uncompressed_buffer = buffer + SIZE_ARR;
	for (int i = 0; i < SIZE_ARR; i++)
		source[i] = i%256?i:1;
	
	if (!source) return 1;
	struct libdeflate_compressor* compr = libdeflate_alloc_compressor(LEVEL);
	if (compr) {
		DWORD end, start = GetTickCount();
		int real_size = libdeflate_deflate_compress(compr, source, SIZE_ARR, buffer, SIZE_ARR);
		end = GetTickCount();
		printf("real size: %d\ntime: %d\n", real_size, end - start);
		start = GetTickCount();
		real_size = compress_slava(compr, source, SIZE_ARR, buffer, SIZE_ARR);
		end = GetTickCount();
		printf("real size: %d\ntime: %d\n", real_size, end - start);
		libdeflate_free_compressor(compr);
		if (real_size) {
			struct libdeflate_decompressor* decompr = libdeflate_alloc_decompressor();
			if (decompr) {
				size_t uncompressed_size;
				enum libdeflate_result res = libdeflate_deflate_decompress(decompr, buffer, real_size, uncompressed_buffer, SIZE_ARR, &uncompressed_size);
 				
				libdeflate_free_decompressor(decompr);
				if (uncompressed_size == SIZE_ARR && !memcmp(source, uncompressed_buffer, SIZE_ARR)) printf("EQUAL\n");
				printf("NOT EQUAL\n");
			}
		}
	}
	free(source);
	return 0;
}

