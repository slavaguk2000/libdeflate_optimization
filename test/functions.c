#include "functions.h"
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include "libdeflate2/lib/aligned_malloc.h"
#include "libdeflate2/lib/deflate_compress.h"
#include "libdeflate2/lib/deflate_constants.h"
#include "libdeflate2/lib/unaligned.h"
#include "compressor_decompressor_structs.h"
#include "huffman_codes.h"


void delay()
{
	DWORD start = GetTickCount();
	while (start + 1000 > GetTickCount());
}

int add(int a, int b)
{
	delay();
	return a + b;
}

#include <stdlib.h>


void*
aligned_malloc_slava(size_t alignment, size_t size)
{
	delay();
	void* ptr = malloc(sizeof(void*) + alignment - 1 + size);
	if (ptr) {
		void* orig_ptr = ptr;
		ptr = (void*)ALIGN((uintptr_t)ptr + sizeof(void*), alignment);
		((void**)ptr)[-1] = orig_ptr;
	}
	return ptr;
}

void
aligned_free_slava(void* ptr)
{
	delay();
	if (ptr)
		free(((void**)ptr)[-1]);
}

static int my_fun(struct libdeflate_compressor* c)
{
	delay();
}
void my_fun2(struct libdeflate_compressor* c)
{
	delay();
}

static void
deflate_init_offset_slot_fast_slava(struct libdeflate_compressor* c)
{
	unsigned offset;
	unsigned offset_end;
	delay();
	for (unsigned offset_slot = 0; offset_slot < ARRAY_LEN(deflate_offset_slot_base); offset_slot++)
	{
		offset = deflate_offset_slot_base[offset_slot];
		if (offset <= 256) {
			offset_end = offset + (1 << deflate_extra_offset_bits[offset_slot]);
			do {
				c->offset_slot_fast[offset - 1] = offset_slot;
			} while (++offset != offset_end);
		}
		else {
			offset_end = offset + (1 << deflate_extra_offset_bits[offset_slot]);
			do {
				c->offset_slot_fast[256 + ((offset - 1) >> 7)] = offset_slot;
			} while ((offset += (1 << 7)) != offset_end);
		}
	}
}

static void
deflate_init_static_codes(struct libdeflate_compressor* c)
{
	unsigned i;
	delay();
	for (i = 0; i < 144; i++)
		c->freqs.litlen[i] = 1 << (9 - 8);
	for (; i < 256; i++)
		c->freqs.litlen[i] = 1 << (9 - 9);
	for (; i < 280; i++)
		c->freqs.litlen[i] = 1 << (9 - 7);
	for (; i < 288; i++)
		c->freqs.litlen[i] = 1 << (9 - 8);

	for (i = 0; i < 32; i++)
		c->freqs.offset[i] = 1 << (5 - 5);

	deflate_make_huffman_codes(&c->freqs, &c->static_codes);
}


struct libdeflate_compressor* get_compressor() 
{
	struct libdeflate_compressor* c;
	size_t size = offsetof(struct libdeflate_compressor, p) + sizeof(c->p.g);

	c = aligned_malloc_slava(MATCHFINDER_ALIGNMENT, size);
	/*if (!c)
		return NULL;*/

	/*c->impl = deflate_compress_greedy_slava;
	c->max_search_depth = 2;
	c->nice_match_length = 8;
	
	c->compression_level = 1;*/
	int a = my_fun(c);
	my_fun2(c);
	//deflate_init_offset_slot_fast_slava(c);
	//deflate_init_static_codes_slava(c);
	
	return c;
}

static size_t
deflate_compress_greedy_slava(struct libdeflate_compressor* restrict c,
	const u8* restrict in, size_t in_nbytes,
	u8* restrict out, size_t out_nbytes_avail)
{
	for (int i = 0; i < 10; i++)
	delay();
}


size_t compress_slava(struct libdeflate_compressor* c, const void* in, size_t in_nbytes, void* out, size_t out_nbytes_avail)
{
	return add(3, 2);
}