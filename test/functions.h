#pragma once
#define restrict
int add(int a, int b);
#include "libdeflate2/lib/common_defs.h"
size_t compress_slava(struct libdeflate_compressor* c, const void* in, size_t in_nbytes, void* out, size_t out_nbytes_avail);
struct libdeflate_compressor* get_compressor(int LEVEL);



/*
 * This is the "greedy" DEFLATE compressor. It always chooses the longest match.
 */
static size_t
deflate_compress_greedy_slava(struct libdeflate_compressor* restrict c,
	const u8* restrict in, size_t in_nbytes,
	u8* restrict out, size_t out_nbytes_avail);
//{
//	const u8* in_next = in;
//	const u8* in_end = in_next + in_nbytes;
//	struct deflate_output_bitstream os;
//	const u8* in_cur_base = in_next;
//	unsigned max_len = DEFLATE_MAX_MATCH_LEN;
//	unsigned nice_len = MIN(c->nice_match_length, max_len);
//	u32 next_hashes[2] = { 0, 0 };
//
//	deflate_init_output(&os, out, out_nbytes_avail);
//	hc_matchfinder_init(&c->p.g.hc_mf);
//
//	do {
//		/* Starting a new DEFLATE block.  */
//
//		const u8* const in_block_begin = in_next;
//		const u8* const in_max_block_end =
//			in_next + MIN(in_end - in_next, SOFT_MAX_BLOCK_LENGTH);
//		u32 litrunlen = 0;
//		struct deflate_sequence* next_seq = c->p.g.sequences;
//
//		init_block_split_stats(&c->split_stats);
//		deflate_reset_symbol_frequencies(c);
//
//		do {
//			u32 length;
//			u32 offset;
//
//			/* Decrease the maximum and nice match lengths if we're
//			 * approaching the end of the input buffer.  */
//			if (unlikely(max_len > in_end - in_next)) {
//				max_len = in_end - in_next;
//				nice_len = MIN(nice_len, max_len);
//			}
//
//			length = hc_matchfinder_longest_match(&c->p.g.hc_mf,
//				&in_cur_base,
//				in_next,
//				DEFLATE_MIN_MATCH_LEN - 1,
//				max_len,
//				nice_len,
//				c->max_search_depth,
//				next_hashes,
//				&offset);
//
//			if (length >= DEFLATE_MIN_MATCH_LEN) {
//				/* Match found.  */
//				deflate_choose_match(c, length, offset,
//					&litrunlen, &next_seq);
//				observe_match(&c->split_stats, length);
//				in_next = hc_matchfinder_skip_positions(&c->p.g.hc_mf,
//					&in_cur_base,
//					in_next + 1,
//					in_end,
//					length - 1,
//					next_hashes);
//			}
//			else {
//				/* No match found.  */
//				deflate_choose_literal(c, *in_next, &litrunlen);
//				observe_literal(&c->split_stats, *in_next);
//				in_next++;
//			}
//
//			/* Check if it's time to output another block.  */
//		} while (in_next < in_max_block_end &&
//			!should_end_block(&c->split_stats, in_block_begin, in_next, in_end));
//
//		deflate_finish_sequence(next_seq, litrunlen);
//		deflate_flush_block(c, &os, in_block_begin,
//			in_next - in_block_begin,
//			in_next == in_end, false);
//	} while (in_next != in_end);
//
//	return deflate_flush_output(&os);
//}