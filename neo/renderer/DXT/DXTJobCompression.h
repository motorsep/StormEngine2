/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

StormEngine2 additions.

===========================================================================
*/
#ifndef __DXTJOBCOMPRESSION_H__
#define __DXTJOBCOMPRESSION_H__

/*
================================================================================================

	Parallel DXT Compression

	Splits DXT block compression across multiple std::threads.
	Each thread processes a contiguous range of 4x4 blocks, writing directly to the output
	buffer at computed offsets. The block-level output is position-independent (each block
	writes to BLOCK_OFFSET(x,y,w,16)), so threads never overlap in their writes.

	Currently supports:
	  - CompressYCoCgDXT5HQ (GIMP DDS path — exhaustive search, highest quality)
	  - CompressNormalMapDXT5HQ
	  - CompressImageDXT5HQ
	  - CompressImageDXT1HQ

	The Fast paths (SSE2) are already fast enough to not need parallelization.

================================================================================================
*/

// ============================================================================
// Compression type enum — selects which HQ encoder each thread invokes
// ============================================================================
enum dxtCompressionType_t
{
	DXT_COMPRESS_YCOCG_DXT5_HQ,		// CompressYCoCgDXT5HQ (GIMP DDS path)
	DXT_COMPRESS_IMAGE_DXT5_HQ,			// CompressImageDXT5HQ
	DXT_COMPRESS_NORMALMAP_DXT5_HQ,		// CompressNormalMapDXT5HQ
	DXT_COMPRESS_IMAGE_DXT1_HQ,			// CompressImageDXT1HQ
};

// ============================================================================
// Per-thread parameters
//
// Each thread gets a pointer to one of these. All fields are set by the
// dispatcher and are read-only to the thread, except that the thread writes
// into outBuf at computed offsets within its block range.
//
// Block range [blockStart, blockEnd) covers a subset of the total
// block_count = ((height+3)>>2) * ((width+3)>>2).
// ============================================================================
struct dxtCompressionJobParms_t
{
	// --- input (read-only by thread) ---
	const byte*				inBuf;			// source image (RGBA or YCoCg-converted)
	byte*					outBuf;			// destination DXT buffer (shared, non-overlapping writes)
	int						width;			// image width in pixels
	int						height;			// image height in pixels
	unsigned int			blockStart;		// first block index (inclusive)
	unsigned int			blockEnd;		// last block index (exclusive)
	dxtCompressionType_t	compressionType;// which encoder to use

	// padding to keep struct size friendly for cache lines
	int						pad[1];
};

// ============================================================================
// Thread worker function
// ============================================================================
void DXTCompressionJob( dxtCompressionJobParms_t* parms );

// ============================================================================
// High-level dispatcher
//
// Call this instead of the single-threaded CompressYCoCgDXT5HQ etc.
// It splits the work across N threads using std::thread, and blocks
// until all threads complete before returning.
//
// Parameters match the original idDxtEncoder methods.
// ============================================================================
void CompressDXT_Parallel(
	const byte*				inBuf,
	byte*					outBuf,
	int						width,
	int						height,
	dxtCompressionType_t	compressionType
);

#endif // !__DXTJOBCOMPRESSION_H__
