#include "compressor.h"
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include "libdeflate2/lib/aligned_malloc.h"
#include "libdeflate2/lib/deflate_compress.h"
#include "libdeflate2/lib/deflate_constants.h"
#include "libdeflate2/lib/unaligned.h"
//
//
///* Valid block types  */
//#define DEFLATE_BLOCKTYPE_UNCOMPRESSED		0
//#define DEFLATE_BLOCKTYPE_STATIC_HUFFMAN	1
//#define DEFLATE_BLOCKTYPE_DYNAMIC_HUFFMAN	2
//
///* __restrict has nonstandard behavior; don't use it */
//#define restrict
//
///* ... but we can use __inline and __forceinline */
//#define inline		__inline
//#define forceinline	__forceinline

#define OUTPUT_END_PADDING	8
//#ifndef unlikely
//#  define unlikely(expr)	(expr)
//#endif
//#  include <inttypes.h>
//typedef uint8_t u8;
//typedef uint16_t u16;
//typedef uint32_t u32;
//typedef uint64_t u64;
//typedef int8_t s8;
//typedef int16_t s16;
//typedef int32_t s32;
//typedef int64_t s64;
//typedef int bool;
typedef size_t machine_word_t;
typedef machine_word_t bitbuf_t;

///* Number of bytes in a word */
//#define WORDBYTES	((int)sizeof(machine_word_t))
//
///* Number of bits in a word */
//#define WORDBITS	(8 * WORDBYTES)
//
//#define ARRAY_LEN(A)		(sizeof(A) / sizeof((A)[0]))
//#define MIN(a, b)		((a) <= (b) ? (a) : (b))
//#define MAX(a, b)		((a) >= (b) ? (a) : (b))
//#define DIV_ROUND_UP(n, d)	(((n) + (d) - 1) / (d))
//#define STATIC_ASSERT(expr)	((void)sizeof(char[1 - 2 * !(expr)]))
//#define ALIGN(n, a)		(((n) + (a) - 1) & ~((a) - 1))
//
//


struct deflate_output_bitstream {

	/* Bits that haven't yet been written to the output buffer.  */
	bitbuf_t bitbuf;

	/* Number of bits currently held in @bitbuf.  */
	unsigned bitcount;

	/* Pointer to the beginning of the output buffer.  */
	u8* begin;

	/* Pointer to the position in the output buffer at which the next byte
	 * should be written.  */
	u8* next;

	/* Pointer just past the end of the output buffer.  */
	u8* end;
};

static size_t
deflate_flush_output_slava(struct deflate_output_bitstream* os)
{
	if (os->next == os->end) /* overflow?  */
		return 0;

	while ((int)os->bitcount > 0) {
		*os->next++ = os->bitbuf;
		os->bitcount -= 8;
		os->bitbuf >>= 8;
	}

	return os->next - os->begin;
}

/* Initialize the output bitstream.  'size' is assumed to be at least
 * OUTPUT_END_PADDING.  */
static void
deflate_init_output_slava(struct deflate_output_bitstream* os,
	void* buffer, size_t size)
{
	os->bitbuf = 0;
	os->bitcount = 0;
	os->begin = buffer;
	os->next = os->begin;
	os->end = os->begin + size - OUTPUT_END_PADDING;
}

deflate_add_bits_slava(struct deflate_output_bitstream* os,
	const bitbuf_t bits, const unsigned num_bits)
{
	os->bitbuf |= bits << os->bitcount;
	os->bitcount += num_bits;
}

static forceinline void
put_unaligned_leword_slava(machine_word_t v, u8* p)
{
	STATIC_ASSERT(WORDBITS == 32 || WORDBITS == 64);
	//if (WORDBITS == 32)
	//	put_unaligned_le32(v, p);
	//else
	//	put_unaligned_le64(v, p);
}

/* Flush bits from the bitbuffer variable to the output buffer.  */
static forceinline void
deflate_flush_bits_slava(struct deflate_output_bitstream* os)
{
	/* Flush a whole word (branchlessly).  */
	put_unaligned_leword_slava(os->bitbuf, os->next);
	os->bitbuf >>= os->bitcount & ~7;
	os->next += MIN(os->end - os->next, os->bitcount >> 3);
	os->bitcount &= 7;
}

/* Write the header fields common to all DEFLATE block types.  */
static void
deflate_write_block_header_slava(struct deflate_output_bitstream* os,
	bool is_final_block, unsigned block_type)
{
	DWORD start = GetTickCount();
	while (start + 1000 > GetTickCount());

	deflate_add_bits_slava(os, is_final_block, 1);
	deflate_add_bits_slava(os, block_type, 2);
	deflate_flush_bits_slava(os);
}

/* Align the bitstream on a byte boundary. */
static forceinline void
deflate_align_bitstream_slava(struct deflate_output_bitstream* os)
{
	os->bitcount += ((unsigned)-((int)os->bitcount)) & 7;
	deflate_flush_bits_slava(os);
}

static void
deflate_write_uncompressed_block_slava(struct deflate_output_bitstream* os,
	const u8* data, u16 len,
	bool is_final_block)
{
	
	deflate_write_block_header_slava(os, is_final_block,
		DEFLATE_BLOCKTYPE_UNCOMPRESSED);
	deflate_align_bitstream_slava(os);

	if (4 + (u32)len >= os->end - os->next) {
		os->next = os->end;
		return;
	}

	put_unaligned_le16(len, os->next);
	os->next += 2;
	put_unaligned_le16(~len, os->next);
	os->next += 2;
	memcpy(os->next, data, len);
	os->next += len;
}

size_t compress_slava(struct libdeflate_compressor* c, const void* in, size_t in_nbytes, void* out, size_t out_nbytes_avail)
{
	if (unlikely(out_nbytes_avail < OUTPUT_END_PADDING))
		return 0;

	/* For extremely small inputs just use a single uncompressed block. */
	if (unlikely(in_nbytes < 16)) {
		struct deflate_output_bitstream os;
		deflate_init_output_slava(&os, out, out_nbytes_avail);
		if (in_nbytes == 0)
			in = &os; /* Avoid passing NULL to memcpy() */
		deflate_write_uncompressed_block_slava(&os, in, in_nbytes, true);
		return deflate_flush_output_slava(&os);
	}

	return 0;
	//return (*(c->impl))(c, in, in_nbytes, out, out_nbytes_avail);
}



