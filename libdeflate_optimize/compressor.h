#pragma once
#include <stddef.h>
size_t compress_slava(struct libdeflate_compressor* c, const void* in, size_t in_nbytes, void* out, size_t out_nbytes_avail);
