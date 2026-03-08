/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

StormEngine2 additions.

Parallel DXT compression using std::thread.

The BFG parallel job system (idParallelJobList) is designed for short
frame-scoped jobs with a fixed thread pool of MAX_JOB_THREADS=2.
DXT HQ compression is a long-running batch operation that benefits
from all available cores, so we use std::thread directly instead.

===========================================================================
*/
#pragma hdrstop
#include "precompiled.h"
#include "framework/Common_local.h"
#include "DXTCodec.h"
#include "DXTJobCompression.h"

#include <thread>
#include <algorithm>

// We need the BLOCK_OFFSET macro — same definition as DXTEncoder.cpp
#define BLOCK_OFFSET( x, y, w, bs )  ( ( ( y ) >> 2 ) * ( ( bs ) * ( ( ( w ) + 3 ) >> 2 ) ) + ( ( bs ) * ( ( x ) >> 2 ) ) )

/*
========================
DXTCompressionJob

Thread worker function. Processes blocks [blockStart, blockEnd) of the image.

Thread safety:
  - Each thread reads from inBuf (shared, read-only) via ExtractBlockGimpDDS(x,y,w,h)
  - Each thread writes to non-overlapping regions of outBuf via BLOCK_OFFSET(x,y,w,16)
  - No shared mutable state between threads
  - idDxtEncoder instances are stack-local per thread
========================
*/
void DXTCompressionJob( dxtCompressionJobParms_t* parms )
{
	const byte*		inBuf		= parms->inBuf;
	byte*			outBuf		= parms->outBuf;
	const int		width		= parms->width;
	const int		height		= parms->height;
	const unsigned int blockStart = parms->blockStart;
	const unsigned int blockEnd	= parms->blockEnd;
	const int		blocksPerRow = ( width + 3 ) >> 2;

	// Each thread gets its own encoder instance — the class is trivially constructible
	// and carries only (width, height, outData, srcPadding, dstPadding) as member state.
	idDxtEncoder dxt;

	switch( parms->compressionType )
	{
		case DXT_COMPRESS_YCOCG_DXT5_HQ:
		{
			// GIMP DDS path — writes directly to computed output offsets.
			// This is the path ported from gimp-dds that does the exhaustive search
			// and produces the highest quality YCoCg DXT5 output.
			ALIGN16( byte block[64] );

			for( unsigned int i = blockStart; i < blockEnd; i++ )
			{
				int x = ( i % blocksPerRow ) << 2;
				int y = ( i / blocksPerRow ) << 2;
				byte* p = outBuf + BLOCK_OFFSET( x, y, width, 16 );

				dxt.ExtractBlockGimpDDS( inBuf, x, y, width, height, block );
				dxt.EncodeAlphaBlockBC3GimpDDS( p, block, 0 );
				dxt.EncodeYCoCgBlockGimpDDS( p + 8, block );
			}
			break;
		}

		case DXT_COMPRESS_IMAGE_DXT5_HQ:
		{
			ALIGN16( byte block[64] );
			byte alphaIndices1[6];
			byte alphaIndices2[6];
			unsigned int colorIndices;
			byte col1[4];
			byte col2[4];

			for( unsigned int i = blockStart; i < blockEnd; i++ )
			{
				int x = ( i % blocksPerRow ) << 2;
				int y = ( i / blocksPerRow ) << 2;
				byte* p = outBuf + BLOCK_OFFSET( x, y, width, 16 );

				dxt.ExtractBlockGimpDDS( inBuf, x, y, width, height, block );

				dxt.GetMinMaxColorsHQ( block, col1, col2, true );
				dxt.GetMinMaxAlphaHQ( block, 3, col1, col2 );

				// Alpha block: try both endpoint orderings, pick lower error
				int error1 = dxt.FindAlphaIndices( block, 3, col1[3], col2[3], alphaIndices1 );
				int error2 = dxt.FindAlphaIndices( block, 3, col2[3], col1[3], alphaIndices2 );

				if( error1 < error2 )
				{
					p[0] = col1[3];
					p[1] = col2[3];
					memcpy( p + 2, alphaIndices1, 6 );
				}
				else
				{
					p[0] = col2[3];
					p[1] = col1[3];
					memcpy( p + 2, alphaIndices2, 6 );
				}

				// NV4X hardware bug fix — ensure endpoints are sorted correctly
				dxt.NV4XHardwareBugFix( col2, col1 );

				// Color block
				unsigned short scol1 = dxt.ColorTo565( col1 );
				unsigned short scol2 = dxt.ColorTo565( col2 );

				p[8]  = scol1 & 0xFF;
				p[9]  = ( scol1 >> 8 ) & 0xFF;
				p[10] = scol2 & 0xFF;
				p[11] = ( scol2 >> 8 ) & 0xFF;

				dxt.FindColorIndices( block, scol1, scol2, colorIndices );
				p[12] = ( colorIndices >>  0 ) & 0xFF;
				p[13] = ( colorIndices >>  8 ) & 0xFF;
				p[14] = ( colorIndices >> 16 ) & 0xFF;
				p[15] = ( colorIndices >> 24 ) & 0xFF;
			}
			break;
		}

		case DXT_COMPRESS_NORMALMAP_DXT5_HQ:
		{
			// Normal map DXT5 HQ path — must match CompressNormalMapDXT5HQ exactly:
			// 1. Extract block
			// 2. Swizzle: R→A, zero R and B (normal map channel layout)
			// 3. GetMinMaxNormalYHQ (NOT GetMinMaxNormalsDXT5HQ — different function!)
			// 4. GetMinMaxAlphaHQ for the alpha (Nx) channel
			// 5. FindAlphaIndices, FindColorIndices
			// 6. NV4X hardware bug fix on endpoints
			ALIGN16( byte block[64] );
			byte alphaIndices1[6];
			byte alphaIndices2[6];
			unsigned int colorIndices;
			byte col1[4];
			byte col2[4];

			for( unsigned int i = blockStart; i < blockEnd; i++ )
			{
				int x = ( i % blocksPerRow ) << 2;
				int y = ( i / blocksPerRow ) << 2;
				byte* p = outBuf + BLOCK_OFFSET( x, y, width, 16 );

				dxt.ExtractBlockGimpDDS( inBuf, x, y, width, height, block );

				// Swizzle: move Nx (red) to alpha, zero out red and blue.
				// This matches CompressNormalMapDXT5HQ lines 3350-3355.
				for( int k = 0; k < 16; k++ )
				{
					block[k * 4 + 3] = block[k * 4 + 0];
					block[k * 4 + 0] = 0;
					block[k * 4 + 2] = 0;
				}

				dxt.GetMinMaxNormalYHQ( block, col1, col2, true, 1 );
				dxt.GetMinMaxAlphaHQ( block, 3, col1, col2 );

				// Alpha block: try both endpoint orderings, pick lower error
				int error1 = dxt.FindAlphaIndices( block, 3, col1[3], col2[3], alphaIndices1 );
				int error2 = dxt.FindAlphaIndices( block, 3, col2[3], col1[3], alphaIndices2 );

				if( error1 < error2 )
				{
					p[0] = col1[3];
					p[1] = col2[3];
					memcpy( p + 2, alphaIndices1, 6 );
				}
				else
				{
					p[0] = col2[3];
					p[1] = col1[3];
					memcpy( p + 2, alphaIndices2, 6 );
				}

				// NV4X hardware bug fix — ensure endpoints are sorted correctly
				dxt.NV4XHardwareBugFix( col2, col1 );

				// Color block
				unsigned short scol1 = dxt.ColorTo565( col1 );
				unsigned short scol2 = dxt.ColorTo565( col2 );

				p[8]  = scol1 & 0xFF;
				p[9]  = ( scol1 >> 8 ) & 0xFF;
				p[10] = scol2 & 0xFF;
				p[11] = ( scol2 >> 8 ) & 0xFF;

				dxt.FindColorIndices( block, scol1, scol2, colorIndices );
				p[12] = ( colorIndices >>  0 ) & 0xFF;
				p[13] = ( colorIndices >>  8 ) & 0xFF;
				p[14] = ( colorIndices >> 16 ) & 0xFF;
				p[15] = ( colorIndices >> 24 ) & 0xFF;
			}
			break;
		}

		case DXT_COMPRESS_IMAGE_DXT1_HQ:
		{
			ALIGN16( byte block[64] );
			unsigned int colorIndices;
			byte col1[4];
			byte col2[4];

			for( unsigned int i = blockStart; i < blockEnd; i++ )
			{
				int x = ( i % blocksPerRow ) << 2;
				int y = ( i / blocksPerRow ) << 2;
				byte* p = outBuf + BLOCK_OFFSET( x, y, width, 8 );

				dxt.ExtractBlockGimpDDS( inBuf, x, y, width, height, block );

				dxt.GetMinMaxColorsHQ( block, col1, col2, false );

				unsigned short scol1 = dxt.ColorTo565( col1 );
				unsigned short scol2 = dxt.ColorTo565( col2 );

				if( scol2 > scol1 )
				{
					unsigned short tmp = scol1; scol1 = scol2; scol2 = tmp;
				}

				p[0] = scol1 & 0xFF;
				p[1] = ( scol1 >> 8 ) & 0xFF;
				p[2] = scol2 & 0xFF;
				p[3] = ( scol2 >> 8 ) & 0xFF;

				dxt.FindColorIndices( block, scol1, scol2, colorIndices );
				p[4] = ( colorIndices >>  0 ) & 0xFF;
				p[5] = ( colorIndices >>  8 ) & 0xFF;
				p[6] = ( colorIndices >> 16 ) & 0xFF;
				p[7] = ( colorIndices >> 24 ) & 0xFF;
			}
			break;
		}
	}
}

/*
========================
CompressDXT_Parallel

High-level dispatcher using std::thread for true N-core parallelism.

We use std::thread instead of the engine's idParallelJobList because:
  1. The BFG job system thread pool is capped at MAX_JOB_THREADS (2)
  2. AllocJobList/FreeJobList interact with the renderer's worker threads
     in ways that can deadlock during level load binarization
  3. DXT compression is a batch operation that doesn't need the job system's
     sync points, priorities, or profiling infrastructure

Each thread gets its own block range and encoder instance.
The main thread blocks until all workers finish.
========================
*/
void CompressDXT_Parallel(
	const byte*				inBuf,
	byte*					outBuf,
	int						width,
	int						height,
	dxtCompressionType_t	compressionType )
{
	// --- Edge cases: fall back to single-threaded ---
	if( width < 4 || height < 4 )
	{
		idDxtEncoder dxt;
		switch( compressionType )
		{
			case DXT_COMPRESS_YCOCG_DXT5_HQ:
				dxt.CompressYCoCgDXT5HQ( inBuf, outBuf, width, height );
				break;
			case DXT_COMPRESS_IMAGE_DXT5_HQ:
				dxt.CompressImageDXT5HQ( inBuf, outBuf, width, height );
				break;
			case DXT_COMPRESS_NORMALMAP_DXT5_HQ:
				dxt.CompressNormalMapDXT5HQ( inBuf, outBuf, width, height );
				break;
			case DXT_COMPRESS_IMAGE_DXT1_HQ:
				dxt.CompressImageDXT1HQ( inBuf, outBuf, width, height );
				break;
		}
		return;
	}

	if( ( width > 4 && ( width & 3 ) != 0 ) || ( height > 4 && ( height & 3 ) != 0 ) )
	{
		idDxtEncoder dxt;
		switch( compressionType )
		{
			case DXT_COMPRESS_YCOCG_DXT5_HQ:
				dxt.CompressYCoCgDXT5HQ( inBuf, outBuf, width, height );
				break;
			case DXT_COMPRESS_IMAGE_DXT5_HQ:
				dxt.CompressImageDXT5HQ( inBuf, outBuf, width, height );
				break;
			case DXT_COMPRESS_NORMALMAP_DXT5_HQ:
				dxt.CompressNormalMapDXT5HQ( inBuf, outBuf, width, height );
				break;
			case DXT_COMPRESS_IMAGE_DXT1_HQ:
				dxt.CompressImageDXT1HQ( inBuf, outBuf, width, height );
				break;
		}
		return;
	}

	// --- Compute block count and determine thread count ---
	const unsigned int blocksPerRow = ( width + 3 ) >> 2;
	const unsigned int blocksPerCol = ( height + 3 ) >> 2;
	const unsigned int totalBlocks  = blocksPerRow * blocksPerCol;

	// Use hardware concurrency to determine thread count.
	// std::thread::hardware_concurrency() returns logical cores (including HT).
	// Cap at a reasonable maximum.
	int numThreads = (int)std::thread::hardware_concurrency();
	if( numThreads < 1 )
	{
		numThreads = 4;		// safe fallback
	}
	if( numThreads > 32 )
	{
		numThreads = 32;
	}

	// Don't create more threads than meaningful — at least 16 blocks per thread.
	const unsigned int MIN_BLOCKS_PER_THREAD = 16;
	while( numThreads > 1 && ( totalBlocks / (unsigned int)numThreads ) < MIN_BLOCKS_PER_THREAD )
	{
		numThreads--;
	}

	// Single thread fast path — no threading overhead
	if( numThreads <= 1 )
	{
		idDxtEncoder dxt;
		switch( compressionType )
		{
			case DXT_COMPRESS_YCOCG_DXT5_HQ:
				dxt.CompressYCoCgDXT5HQ( inBuf, outBuf, width, height );
				break;
			case DXT_COMPRESS_IMAGE_DXT5_HQ:
				dxt.CompressImageDXT5HQ( inBuf, outBuf, width, height );
				break;
			case DXT_COMPRESS_NORMALMAP_DXT5_HQ:
				dxt.CompressNormalMapDXT5HQ( inBuf, outBuf, width, height );
				break;
			case DXT_COMPRESS_IMAGE_DXT1_HQ:
				dxt.CompressImageDXT1HQ( inBuf, outBuf, width, height );
				break;
		}
		return;
	}

	// --- Allocate per-thread parameter blocks ---
	dxtCompressionJobParms_t* threadParms = ( dxtCompressionJobParms_t* )Mem_Alloc(
		sizeof( dxtCompressionJobParms_t ) * numThreads, TAG_TEMP );

	// --- Partition blocks across threads ---
	const unsigned int blocksPerThread = totalBlocks / (unsigned int)numThreads;
	unsigned int remainder = totalBlocks % (unsigned int)numThreads;
	unsigned int currentBlock = 0;

	for( int j = 0; j < numThreads; j++ )
	{
		unsigned int threadBlockCount = blocksPerThread + ( (unsigned int)j < remainder ? 1 : 0 );

		threadParms[j].inBuf			= inBuf;
		threadParms[j].outBuf			= outBuf;
		threadParms[j].width			= width;
		threadParms[j].height			= height;
		threadParms[j].blockStart		= currentBlock;
		threadParms[j].blockEnd			= currentBlock + threadBlockCount;
		threadParms[j].compressionType	= compressionType;
		threadParms[j].pad[0]			= 0;

		currentBlock += threadBlockCount;
	}

	// --- Launch worker threads ---
	// Thread 0's work is done inline on the calling thread to avoid
	// the overhead of creating one extra thread and wasting the main thread.
	std::thread* workers = ( std::thread* )Mem_Alloc(
		sizeof( std::thread ) * ( numThreads - 1 ), TAG_TEMP );

	for( int j = 1; j < numThreads; j++ )
	{
		// Placement new — Mem_Alloc gives raw memory
		new( &workers[j - 1] ) std::thread( DXTCompressionJob, &threadParms[j] );
	}

	// Do thread 0's share on the calling thread
	DXTCompressionJob( &threadParms[0] );

	// Wait for all workers to finish
	for( int j = 1; j < numThreads; j++ )
	{
		workers[j - 1].join();
		workers[j - 1].~thread();
	}

	// --- Update pacifier ---
	commonLocal.LoadPacifierBinarizeProgressIncrement( totalBlocks * 16 );

	// --- Cleanup ---
	Mem_Free( workers );
	Mem_Free( threadParms );
}
