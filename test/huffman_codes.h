#pragma once
#include "libdeflate2/lib/common_defs.h"
#include "compressor_decompressor_structs.h"
#define DEFLATE_NUM_PRECODE_SYMS		19
#define DEFLATE_NUM_LITLEN_SYMS			288
#define DEFLATE_NUM_OFFSET_SYMS			32

/* The maximum number of symbols across all codes  */
#define DEFLATE_MAX_NUM_SYMS			288


#define NUM_SYMBOL_BITS 10
#define SYMBOL_MASK ((1 << NUM_SYMBOL_BITS) - 1)

#define GET_NUM_COUNTERS(num_syms)	((((num_syms) + 3 / 4) + 3) & ~3)

/* The maximum codeword length across all codes  */
#define DEFLATE_MAX_CODEWORD_LEN		15
static void
build_tree(u32 A[], unsigned sym_count)
{
	delay();
	/* Index, in 'A', of next lowest frequency symbol that has not
	 * yet been processed.  */
	unsigned i = 0;

	/* Index, in 'A', of next lowest frequency parentless non-leaf
	 * node; or, if equal to 'e', then no such node exists yet.  */
	unsigned b = 0;

	/* Index, in 'A', of next node to allocate as a non-leaf.  */
	unsigned e = 0;

	do {
		unsigned m, n;
		u32 freq_shifted;

		/* Choose the two next lowest frequency entries.  */

		if (i != sym_count &&
			(b == e || (A[i] >> NUM_SYMBOL_BITS) <= (A[b] >> NUM_SYMBOL_BITS)))
			m = i++;
		else
			m = b++;

		if (i != sym_count &&
			(b == e || (A[i] >> NUM_SYMBOL_BITS) <= (A[b] >> NUM_SYMBOL_BITS)))
			n = i++;
		else
			n = b++;

		/* Allocate a non-leaf node and link the entries to it.
		 *
		 * If we link an entry that we're visiting for the first
		 * time (via index 'i'), then we're actually linking a
		 * leaf node and it will have no effect, since the leaf
		 * will be overwritten with a non-leaf when index 'e'
		 * catches up to it.  But it's not any slower to
		 * unconditionally set the parent index.
		 *
		 * We also compute the frequency of the non-leaf node as
		 * the sum of its two children's frequencies.  */

		freq_shifted = (A[m] & ~SYMBOL_MASK) + (A[n] & ~SYMBOL_MASK);

		A[m] = (A[m] & SYMBOL_MASK) | (e << NUM_SYMBOL_BITS);
		A[n] = (A[n] & SYMBOL_MASK) | (e << NUM_SYMBOL_BITS);
		A[e] = (A[e] & SYMBOL_MASK) | freq_shifted;
		e++;
	} while (sym_count - e > 1);
	/* When just one entry remains, it is a "leaf" that was
	 * linked to some other node.  We ignore it, since the
	 * rest of the array contains the non-leaves which we
	 * need.  (Note that we're assuming the cases with 0 or 1
	 * symbols were handled separately.) */
}


/*
 * Given the stripped-down Huffman tree constructed by build_tree(),
 * determine the number of codewords that should be assigned each
 * possible length, taking into account the length-limited constraint.
 *
 * @A
 *	The array produced by build_tree(), containing parent index
 *	information for the non-leaf nodes of the Huffman tree.  Each
 *	entry in this array is a node; a node's parent always has a
 *	greater index than that node itself.  This function will
 *	overwrite the parent index information in this array, so
 *	essentially it will destroy the tree.  However, the data in the
 *	low NUM_SYMBOL_BITS of each entry will be preserved.
 *
 * @root_idx
 *	The 0-based index of the root node in 'A', and consequently one
 *	less than the number of tree node entries in 'A'.  (Or, really 2
 *	less than the actual length of 'A'.)
 *
 * @len_counts
 *	An array of length ('max_codeword_len' + 1) in which the number of
 *	codewords having each length <= max_codeword_len will be
 *	returned.
 *
 * @max_codeword_len
 *	The maximum permissible codeword length.
 */
static void
compute_length_counts(u32 A[restrict], unsigned root_idx,
	unsigned len_counts[restrict], unsigned max_codeword_len)
{
	delay();
	unsigned len;
	int node;

	/* The key observations are:
	 *
	 * (1) We can traverse the non-leaf nodes of the tree, always
	 * visiting a parent before its children, by simply iterating
	 * through the array in reverse order.  Consequently, we can
	 * compute the depth of each node in one pass, overwriting the
	 * parent indices with depths.
	 *
	 * (2) We can initially assume that in the real Huffman tree,
	 * both children of the root are leaves.  This corresponds to two
	 * codewords of length 1.  Then, whenever we visit a (non-leaf)
	 * node during the traversal, we modify this assumption to
	 * account for the current node *not* being a leaf, but rather
	 * its two children being leaves.  This causes the loss of one
	 * codeword for the current depth and the addition of two
	 * codewords for the current depth plus one.
	 *
	 * (3) We can handle the length-limited constraint fairly easily
	 * by simply using the largest length available when a depth
	 * exceeds max_codeword_len.
	 */

	for (len = 0; len <= max_codeword_len; len++)
		len_counts[len] = 0;
	len_counts[1] = 2;

	/* Set the root node's depth to 0.  */
	A[root_idx] &= SYMBOL_MASK;

	for (node = root_idx - 1; node >= 0; node--) {

		/* Calculate the depth of this node.  */

		unsigned parent = A[node] >> NUM_SYMBOL_BITS;
		unsigned parent_depth = A[parent] >> NUM_SYMBOL_BITS;
		unsigned depth = parent_depth + 1;
		unsigned len = depth;

		/* Set the depth of this node so that it is available
		 * when its children (if any) are processed.  */

		A[node] = (A[node] & SYMBOL_MASK) | (depth << NUM_SYMBOL_BITS);

		/* If needed, decrease the length to meet the
		 * length-limited constraint.  This is not the optimal
		 * method for generating length-limited Huffman codes!
		 * But it should be good enough.  */
		if (len >= max_codeword_len) {
			len = max_codeword_len;
			do {
				len--;
			} while (len_counts[len] == 0);
		}

		/* Account for the fact that we have a non-leaf node at
		 * the current depth.  */
		len_counts[len]--;
		len_counts[len + 1] += 2;
	}
}

/*
 * Generate the codewords for a canonical Huffman code.
 *
 * @A
 *	The output array for codewords.  In addition, initially this
 *	array must contain the symbols, sorted primarily by frequency and
 *	secondarily by symbol value, in the low NUM_SYMBOL_BITS bits of
 *	each entry.
 *
 * @len
 *	Output array for codeword lengths.
 *
 * @len_counts
 *	An array that provides the number of codewords that will have
 *	each possible length <= max_codeword_len.
 *
 * @max_codeword_len
 *	Maximum length, in bits, of each codeword.
 *
 * @num_syms
 *	Number of symbols in the alphabet, including symbols with zero
 *	frequency.  This is the length of the 'A' and 'len' arrays.
 */
static void
gen_codewords(u32 A[restrict], u8 lens[restrict],
	const unsigned len_counts[restrict],
	unsigned max_codeword_len, unsigned num_syms)
{
	delay();
	u32 next_codewords[DEFLATE_MAX_CODEWORD_LEN + 1];
	unsigned i;
	unsigned len;
	unsigned sym;

	/* Given the number of codewords that will have each length,
	 * assign codeword lengths to symbols.  We do this by assigning
	 * the lengths in decreasing order to the symbols sorted
	 * primarily by increasing frequency and secondarily by
	 * increasing symbol value.  */
	for (i = 0, len = max_codeword_len; len >= 1; len--) {
		unsigned count = len_counts[len];
		while (count--)
			lens[A[i++] & SYMBOL_MASK] = len;
	}

	/* Generate the codewords themselves.  We initialize the
	 * 'next_codewords' array to provide the lexicographically first
	 * codeword of each length, then assign codewords in symbol
	 * order.  This produces a canonical code.  */
	next_codewords[0] = 0;
	next_codewords[1] = 0;
	for (len = 2; len <= max_codeword_len; len++)
		next_codewords[len] =
		(next_codewords[len - 1] + len_counts[len - 1]) << 1;

	for (sym = 0; sym < num_syms; sym++)
		A[sym] = next_codewords[lens[sym]]++;
}


static void
heapify_subtree(u32 A[], unsigned length, unsigned subtree_idx)
{
	delay();
	unsigned parent_idx;
	unsigned child_idx;
	u32 v;

	v = A[subtree_idx];
	parent_idx = subtree_idx;
	while ((child_idx = parent_idx * 2) <= length) {
		if (child_idx < length && A[child_idx + 1] > A[child_idx])
			child_idx++;
		if (v >= A[child_idx])
			break;
		A[parent_idx] = A[child_idx];
		parent_idx = child_idx;
	}
	A[parent_idx] = v;
}

/* Rearrange the array 'A' so that it satisfies the maxheap property.
 * 'A' uses 1-based indices, so the children of A[i] are A[i*2] and A[i*2 + 1].
 */
static void
heapify_array(u32 A[], unsigned length)
{
	delay();
	unsigned subtree_idx;

	for (subtree_idx = length / 2; subtree_idx >= 1; subtree_idx--)
		heapify_subtree(A, length, subtree_idx);
}

/*
 * Sort the array 'A', which contains 'length' unsigned 32-bit integers.
 *
 * Note: name this function heap_sort() instead of heapsort() to avoid colliding
 * with heapsort() from stdlib.h on BSD-derived systems --- though this isn't
 * necessary when compiling with -D_ANSI_SOURCE, which is the better solution.
 */
static void
heap_sort(u32 A[], unsigned length)
{
	delay();
	A--; /* Use 1-based indices  */

	heapify_array(A, length);

	while (length >= 2) {
		u32 tmp = A[length];
		A[length] = A[1];
		A[1] = tmp;
		length--;
		heapify_subtree(A, length, 1);
	}
}


static unsigned
sort_symbols(unsigned num_syms, const u32 freqs[restrict],
	u8 lens[restrict], u32 symout[restrict])
{
	delay();
	unsigned sym;
	unsigned i;
	unsigned num_used_syms;
	unsigned num_counters;
	unsigned counters[GET_NUM_COUNTERS(DEFLATE_MAX_NUM_SYMS)];

	/* We rely on heapsort, but with an added optimization.  Since
	 * it's common for most symbol frequencies to be low, we first do
	 * a count sort using a limited number of counters.  High
	 * frequencies will be counted in the last counter, and only they
	 * will be sorted with heapsort.
	 *
	 * Note: with more symbols, it is generally beneficial to have more
	 * counters.  About 1 counter per 4 symbols seems fast.
	 *
	 * Note: I also tested radix sort, but even for large symbol
	 * counts (> 255) and frequencies bounded at 16 bits (enabling
	 * radix sort by just two base-256 digits), it didn't seem any
	 * faster than the method implemented here.
	 *
	 * Note: I tested the optimized quicksort implementation from
	 * glibc (with indirection overhead removed), but it was only
	 * marginally faster than the simple heapsort implemented here.
	 *
	 * Tests were done with building the codes for LZX.  Results may
	 * vary for different compression algorithms...!  */

	num_counters = GET_NUM_COUNTERS(num_syms);

	memset(counters, 0, num_counters * sizeof(counters[0]));

	/* Count the frequencies.  */
	for (sym = 0; sym < num_syms; sym++)
		counters[MIN(freqs[sym], num_counters - 1)]++;

	/* Make the counters cumulative, ignoring the zero-th, which
	 * counted symbols with zero frequency.  As a side effect, this
	 * calculates the number of symbols with nonzero frequency.  */
	num_used_syms = 0;
	for (i = 1; i < num_counters; i++) {
		unsigned count = counters[i];
		counters[i] = num_used_syms;
		num_used_syms += count;
	}

	/* Sort nonzero-frequency symbols using the counters.  At the
	 * same time, set the codeword lengths of zero-frequency symbols
	 * to 0.  */
	for (sym = 0; sym < num_syms; sym++) {
		u32 freq = freqs[sym];
		if (freq != 0) {
			symout[counters[MIN(freq, num_counters - 1)]++] =
				sym | (freq << NUM_SYMBOL_BITS);
		}
		else {
			lens[sym] = 0;
		}
	}

	/* Sort the symbols counted in the last counter.  */
	heap_sort(symout + counters[num_counters - 2],
		counters[num_counters - 1] - counters[num_counters - 2]);

	return num_used_syms;
}

static void
make_canonical_huffman_code(unsigned num_syms, unsigned max_codeword_len,
	const u32 freqs[restrict],
	u8 lens[restrict], u32 codewords[restrict])
{
	delay();
	u32* A = codewords;
	unsigned num_used_syms;

	STATIC_ASSERT(DEFLATE_MAX_NUM_SYMS <= 1 << NUM_SYMBOL_BITS);

	/* We begin by sorting the symbols primarily by frequency and
	 * secondarily by symbol value.  As an optimization, the array
	 * used for this purpose ('A') shares storage with the space in
	 * which we will eventually return the codewords.  */

	num_used_syms = sort_symbols(num_syms, freqs, lens, A);

	/* 'num_used_syms' is the number of symbols with nonzero
	 * frequency.  This may be less than @num_syms.  'num_used_syms'
	 * is also the number of entries in 'A' that are valid.  Each
	 * entry consists of a distinct symbol and a nonzero frequency
	 * packed into a 32-bit integer.  */

	 /* Handle special cases where only 0 or 1 symbols were used (had
	  * nonzero frequency).  */

	if (unlikely(num_used_syms == 0)) {
		/* Code is empty.  sort_symbols() already set all lengths
		 * to 0, so there is nothing more to do.  */
		return;
	}

	if (unlikely(num_used_syms == 1)) {
		/* Only one symbol was used, so we only need one
		 * codeword.  But two codewords are needed to form the
		 * smallest complete Huffman code, which uses codewords 0
		 * and 1.  Therefore, we choose another symbol to which
		 * to assign a codeword.  We use 0 (if the used symbol is
		 * not 0) or 1 (if the used symbol is 0).  In either
		 * case, the lesser-valued symbol must be assigned
		 * codeword 0 so that the resulting code is canonical.  */

		unsigned sym = A[0] & SYMBOL_MASK;
		unsigned nonzero_idx = sym ? sym : 1;

		codewords[0] = 0;
		lens[0] = 1;
		codewords[nonzero_idx] = 1;
		lens[nonzero_idx] = 1;
		return;
	}

	/* Build a stripped-down version of the Huffman tree, sharing the
	 * array 'A' with the symbol values.  Then extract length counts
	 * from the tree and use them to generate the final codewords.  */

	build_tree(A, num_used_syms);

	{
		unsigned len_counts[DEFLATE_MAX_CODEWORD_LEN + 1];

		compute_length_counts(A, num_used_syms - 2,
			len_counts, max_codeword_len);

		gen_codewords(A, lens, len_counts, max_codeword_len, num_syms);
	}
}

/*
 * Clear the Huffman symbol frequency counters.
 * This must be called when starting a new DEFLATE block.
 */
static void
deflate_reset_symbol_frequencies(struct libdeflate_compressor* c)
{
	delay();
	memset(&c->freqs, 0, sizeof(c->freqs));
}

/* Reverse the Huffman codeword 'codeword', which is 'len' bits in length.  */
static u32
deflate_reverse_codeword(u32 codeword, u8 len)
{
	delay();
	/* The following branchless algorithm is faster than going bit by bit.
	 * Note: since no codewords are longer than 16 bits, we only need to
	 * reverse the low 16 bits of the 'u32'.  */
	STATIC_ASSERT(DEFLATE_MAX_CODEWORD_LEN <= 16);

	/* Flip adjacent 1-bit fields  */
	codeword = ((codeword & 0x5555) << 1) | ((codeword & 0xAAAA) >> 1);

	/* Flip adjacent 2-bit fields  */
	codeword = ((codeword & 0x3333) << 2) | ((codeword & 0xCCCC) >> 2);

	/* Flip adjacent 4-bit fields  */
	codeword = ((codeword & 0x0F0F) << 4) | ((codeword & 0xF0F0) >> 4);

	/* Flip adjacent 8-bit fields  */
	codeword = ((codeword & 0x00FF) << 8) | ((codeword & 0xFF00) >> 8);

	/* Return the high 'len' bits of the bit-reversed 16 bit value.  */
	return codeword >> (16 - len);
}

/* Make a canonical Huffman code with bit-reversed codewords.  */
static void
deflate_make_huffman_code(unsigned num_syms, unsigned max_codeword_len,
	const u32 freqs[], u8 lens[], u32 codewords[])
{
	delay();
	unsigned sym;

	make_canonical_huffman_code(num_syms, max_codeword_len,
		freqs, lens, codewords);

	for (sym = 0; sym < num_syms; sym++)
		codewords[sym] = deflate_reverse_codeword(codewords[sym], lens[sym]);
}

/*
 * Build the literal/length and offset Huffman codes for a DEFLATE block.
 *
 * This takes as input the frequency tables for each code and produces as output
 * a set of tables that map symbols to codewords and codeword lengths.
 */
static void
deflate_make_huffman_codes(const struct deflate_freqs* freqs,
	struct deflate_codes* codes)
{
	delay();
	STATIC_ASSERT(MAX_LITLEN_CODEWORD_LEN <= DEFLATE_MAX_LITLEN_CODEWORD_LEN);
	STATIC_ASSERT(MAX_OFFSET_CODEWORD_LEN <= DEFLATE_MAX_OFFSET_CODEWORD_LEN);

	deflate_make_huffman_code(DEFLATE_NUM_LITLEN_SYMS,
		MAX_LITLEN_CODEWORD_LEN,
		freqs->litlen,
		codes->lens.litlen,
		codes->codewords.litlen);

	deflate_make_huffman_code(DEFLATE_NUM_OFFSET_SYMS,
		MAX_OFFSET_CODEWORD_LEN,
		freqs->offset,
		codes->lens.offset,
		codes->codewords.offset);
}