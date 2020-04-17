#include <iostream>
#include "libdeflate.h"
#define MEMORY_ERROR -1

using std::endl;
using std::cout;

namespace libdeflate{
    int my_compress(int pointer, int buffer, int source_size, int level){
        //cout << "libdeflate" << endl;
        libdeflate_compressor* compressor = libdeflate_alloc_compressor(level);
        size_t size = libdeflate_deflate_compress_bound(compressor,source_size);
        return libdeflate_deflate_compress(compressor, (const void*)pointer, source_size, (void*)buffer, size);
    }
    int my_decompress(int compressedBuffer, int compressedSize, int uncompressedBuffer, int uncompressedSize)
    {
        // cout << "libdeflate" << endl;
        libdeflate_decompressor* decompressor = libdeflate_alloc_decompressor();
        size_t actual_out_size;
        if (libdeflate_deflate_decompress(decompressor, (const void*)compressedBuffer, compressedSize, 
        (void*)uncompressedBuffer, uncompressedSize, &actual_out_size) != LIBDEFLATE_SUCCESS) return 0;
        return actual_out_size; 
    }
    int gzipCompress(int sourcePointer, int gzipPointer, int size)
    {
        // cout << "libdeflate" << endl;
        return 0;
    }
    void check_library()
    {
        cout << "libdeflate" << endl;
        int count = 1000000;
        uint8_t* sourcePointer = (uint8_t*)malloc(count);
        uint8_t* uncompressBuffer = (uint8_t*)malloc(count);
        for (int i = 0; i < count; i++)
            sourcePointer[i] = i%10+48;
        libdeflate_compressor* compressor = libdeflate_alloc_compressor(1);
        size_t size = libdeflate_deflate_compress_bound(compressor, count);
		void* compress_buff = malloc(size+sizeof(int32_t));
		if (!compress_buff)
		{
			cout << "Memory Error" << endl;
		}
        uint32_t compress_buff_size = libdeflate_deflate_compress(compressor, (const void*)sourcePointer, count, (uint32_t*)compress_buff+sizeof(int32_t), size);
        cout << "compress size = " << compress_buff_size << endl;
        libdeflate_decompressor* decompressor = libdeflate_alloc_decompressor();
        size_t actual_out_size;
        if (libdeflate_deflate_decompress(decompressor, (const void*)((uint32_t*)compress_buff+sizeof(int32_t)), compress_buff_size, 
        (void*)uncompressBuffer, count, &actual_out_size) != LIBDEFLATE_SUCCESS) cout << "not success" << endl;
        cout << "decompress actual size = " << actual_out_size << endl;
        int equal = 1;
        for (int i = 0; i < count; i++){
            if (sourcePointer[i] != uncompressBuffer[i]) equal = 0;
        }        
        if (equal) cout << "equal" << endl;
        else cout << "not equal" << endl;
        cout << "pointer = " << (int)((uint32_t*)compress_buff+sizeof(int32_t))<< endl;
    }
}