//#include <iostream>
//#include <stdint.h>
#include "libdeflate2/libdeflate.h"

using std::cout;
using std::cin;
using std::endl;

//const int SIZE = 100000000;
const int SIZE = 1000000;
const int LEVEL = 1;

int main()
{
	uint8_t* source = (uint8_t*)malloc(3 * SIZE),
		* buffer = source + SIZE,
		* uncompressed_buffer = buffer + SIZE;
	for (int i = 0; i < SIZE; i++)
		source[i] = i%256?i:1;
	
	if (!source) return 1;
	libdeflate_compressor* compr = libdeflate_alloc_compressor(LEVEL);
	if (compr) {
		int real_size = libdeflate_deflate_compress(compr, source, SIZE, buffer, SIZE);
		libdeflate_free_compressor(compr);
		if (real_size) {
			libdeflate_decompressor* decompr = libdeflate_alloc_decompressor();
			if (decompr) {
				size_t uncompressed_size;
				libdeflate_result res = libdeflate_deflate_decompress(decompr, buffer, real_size, uncompressed_buffer, SIZE, &uncompressed_size);
 				libdeflate_free_decompressor(decompr);
				if (uncompressed_size == SIZE && !memcmp(source, uncompressed_buffer, SIZE)) cout << "EQUAL" << endl;
				else cout << "NOT EQUAL" << endl;
			}
		}
	}
	free(source);
	return 0;
}
