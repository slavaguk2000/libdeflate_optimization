#pragma once
#include "libdeflate2/lib/aligned_malloc.h"
#include "libdeflate2/lib/deflate_compress.h"
#include "libdeflate2/lib/deflate_constants.h"
#include "libdeflate2/lib/unaligned.h"

#define MATCHFINDER_WINDOW_ORDER	15
#include "libdeflate2/lib/hc_matchfinder.h"
void delay();

/*
 * These are the compressor-side limits on the codeword lengths for each Huffman
 * code.  To make outputting bits slightly faster, some of these limits are
 * lower than the limits defined by the DEFLATE format.  This does not
 * significantly affect the compression ratio, at least for the block lengths we
 * use.
 */
#define MAX_LITLEN_CODEWORD_LEN		14
#define SOFT_MAX_BLOCK_LENGTH	300000
#define MAX_OFFSET_CODEWORD_LEN		DEFLATE_MAX_OFFSET_CODEWORD_LEN
#define MAX_PRE_CODEWORD_LEN		DEFLATE_MAX_PRE_CODEWORD_LEN

 /* Table: length slot => length slot base value  */
static const unsigned deflate_length_slot_base[] = {
	3   , 4   , 5   , 6   , 7   , 8   , 9   , 10  ,
	11  , 13  , 15  , 17  , 19  , 23  , 27  , 31  ,
	35  , 43  , 51  , 59  , 67  , 83  , 99  , 115 ,
	131 , 163 , 195 , 227 , 258 ,
};

/* Table: length slot => number of extra length bits  */
static const u8 deflate_extra_length_bits[] = {
	0   , 0   , 0   , 0   , 0   , 0   , 0   , 0 ,
	1   , 1   , 1   , 1   , 2   , 2   , 2   , 2 ,
	3   , 3   , 3   , 3   , 4   , 4   , 4   , 4 ,
	5   , 5   , 5   , 5   , 0   ,
};

/* Table: offset slot => offset slot base value  */
static const unsigned deflate_offset_slot_base[] = {
	1    , 2    , 3    , 4     , 5     , 7     , 9     , 13    ,
	17   , 25   , 33   , 49    , 65    , 97    , 129   , 193   ,
	257  , 385  , 513  , 769   , 1025  , 1537  , 2049  , 3073  ,
	4097 , 6145 , 8193 , 12289 , 16385 , 24577 ,
};

/* Table: offset slot => number of extra offset bits  */
static const u8 deflate_extra_offset_bits[] = {
	0    , 0    , 0    , 0     , 1     , 1     , 2     , 2     ,
	3    , 3    , 4    , 4     , 5     , 5     , 6     , 6     ,
	7    , 7    , 8    , 8     , 9     , 9     , 10    , 10    ,
	11   , 11   , 12   , 12    , 13    , 13    ,
};

/* Table: length => length slot  */
static const u8 deflate_length_slot[DEFLATE_MAX_MATCH_LEN + 1] = {
	0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 12,
	12, 13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 16,
	16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18,
	18, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
	21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
	22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
	23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25,
	25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26,
	26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
	26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
	27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
	27, 27, 28,
};

/* The order in which precode codeword lengths are stored */
static const u8 deflate_precode_lens_permutation[DEFLATE_NUM_PRECODE_SYMS] = {
	16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

/* Codewords for the DEFLATE Huffman codes.  */
struct deflate_codewords {
	u32 litlen[DEFLATE_NUM_LITLEN_SYMS];
	u32 offset[DEFLATE_NUM_OFFSET_SYMS];
};

/* Codeword lengths (in bits) for the DEFLATE Huffman codes.
 * A zero length means the corresponding symbol had zero frequency.  */
struct deflate_lens {
	u8 litlen[DEFLATE_NUM_LITLEN_SYMS];
	u8 offset[DEFLATE_NUM_OFFSET_SYMS];
};

/* Codewords and lengths for the DEFLATE Huffman codes.  */
struct deflate_codes {
	struct deflate_codewords codewords;
	struct deflate_lens lens;
};

/* Symbol frequency counters for the DEFLATE Huffman codes.  */
struct deflate_freqs {
	u32 litlen[DEFLATE_NUM_LITLEN_SYMS];
	u32 offset[DEFLATE_NUM_OFFSET_SYMS];
};

/* Block split statistics.  See "Block splitting algorithm" below. */
#define NUM_LITERAL_OBSERVATION_TYPES 8
#define NUM_MATCH_OBSERVATION_TYPES 2
#define NUM_OBSERVATION_TYPES (NUM_LITERAL_OBSERVATION_TYPES + NUM_MATCH_OBSERVATION_TYPES)
struct block_split_stats {
	u32 new_observations[NUM_OBSERVATION_TYPES];
	u32 observations[NUM_OBSERVATION_TYPES];
	u32 num_new_observations;
	u32 num_observations;
};

/*
 * Represents a run of literals followed by a match or end-of-block.  This
 * struct is needed to temporarily store items chosen by the parser, since items
 * cannot be written until all items for the block have been chosen and the
 * block's Huffman codes have been computed.
 */
struct deflate_sequence {

	/* Bits 0..22: the number of literals in this run.  This may be 0 and
	 * can be at most about SOFT_MAX_BLOCK_LENGTH.  The literals are not
	 * stored explicitly in this structure; instead, they are read directly
	 * from the uncompressed data.
	 *
	 * Bits 23..31: the length of the match which follows the literals, or 0
	 * if this literal run was the last in the block, so there is no match
	 * which follows it.  */
	u32 litrunlen_and_length;

	/* If 'length' doesn't indicate end-of-block, then this is the offset of
	 * the match which follows the literals.  */
	u16 offset;

	/* If 'length' doesn't indicate end-of-block, then this is the offset
	 * symbol of the match which follows the literals.  */
	u8 offset_symbol;

	/* If 'length' doesn't indicate end-of-block, then this is the length
	 * slot of the match which follows the literals.  */
	u8 length_slot;
};

/* The main DEFLATE compressor structure  */
struct libdeflate_compressor {

	/* Pointer to the compress() implementation chosen at allocation time */
	size_t(*impl)(struct libdeflate_compressor*,
		const u8*, size_t, u8*, size_t);

	/* Frequency counters for the current block  */
	struct deflate_freqs freqs;

	/* Dynamic Huffman codes for the current block  */
	struct deflate_codes codes;

	/* Static Huffman codes */
	struct deflate_codes static_codes;

	/* Block split statistics for the currently pending block */
	struct block_split_stats split_stats;

	/* A table for fast lookups of offset slot by match offset.
	 *
	 * If the full table is being used, it is a direct mapping from offset
	 * to offset slot.
	 *
	 * If the condensed table is being used, the first 256 entries map
	 * directly to the offset slots of offsets 1 through 256.  The next 256
	 * entries map to the offset slots for the remaining offsets, stepping
	 * through the offsets with a stride of 128.  This relies on the fact
	 * that each of the remaining offset slots contains at least 128 offsets
	 * and has an offset base that is a multiple of 128.  */
#if USE_FULL_OFFSET_SLOT_FAST
	u8 offset_slot_fast[DEFLATE_MAX_MATCH_OFFSET + 1];
#else
	u8 offset_slot_fast[512];
#endif

	/* The "nice" match length: if a match of this length is found, choose
	 * it immediately without further consideration.  */
	unsigned nice_match_length;

	/* The maximum search depth: consider at most this many potential
	 * matches at each position.  */
	unsigned max_search_depth;

	/* The compression level with which this compressor was created.  */
	unsigned compression_level;

	/* Temporary space for Huffman code output  */
	u32 precode_freqs[DEFLATE_NUM_PRECODE_SYMS];
	u8 precode_lens[DEFLATE_NUM_PRECODE_SYMS];
	u32 precode_codewords[DEFLATE_NUM_PRECODE_SYMS];
	unsigned precode_items[DEFLATE_NUM_LITLEN_SYMS + DEFLATE_NUM_OFFSET_SYMS];
	unsigned num_litlen_syms;
	unsigned num_offset_syms;
	unsigned num_explicit_lens;
	unsigned num_precode_items;

	union {
		/* Data for greedy or lazy parsing  */
		struct {
			/* Hash chain matchfinder  */
			struct hc_matchfinder hc_mf;

			/* The matches and literals that the parser has chosen
			 * for the current block.  The required length of this
			 * array is limited by the maximum number of matches
			 * that can ever be chosen for a single block, plus one
			 * for the special entry at the end.  */
			struct deflate_sequence sequences[
				DIV_ROUND_UP(SOFT_MAX_BLOCK_LENGTH,
					DEFLATE_MIN_MATCH_LEN) + 1];
		} g; /* (g)reedy */
	} p; /* (p)arser */
};
