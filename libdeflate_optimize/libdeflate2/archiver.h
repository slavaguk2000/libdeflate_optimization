#define MEMORY_ERROR -1
namespace libdeflate{
    int my_compress(int pointer, int buffer, int source_size, int level);
    int my_decompress(int compressedBuffer, int compressedSize, int uncompressedBuffer, int uncompressedSize);
    int gzipCompress(int sourcePointer, int gzipPointer, int size);
    void check_library();
}