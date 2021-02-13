#include <gl/glew.h>
#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime_api.h>
#include <cuda_gl_interop.h>
#include <cutil.h>
//#include <cutil_gl_error.h>

#include "radixsort.cuh"

#include "../util.h"

////////////////////////////////////////////////////////////////////////////////
// Globals
////////////////////////////////////////////////////////////////////////////////
static GLuint g_gridBuffer0 = 0;
static GLuint g_gridBuffer1 = 0;

////////////////////////////////////////////////////////////////////////////////
// Entry point for Cuda functionality on host side
////////////////////////////////////////////////////////////////////////////////

extern "C" 
{
	void cudaRegisterGridBuffer0( GLuint bufferId )
	{
		CUDA_SAFE_CALL( cudaGLRegisterBufferObject( bufferId ) );
		CUT_CHECK_ERROR( "cudaGLRegisterBufferObject failed.\n" );
		g_gridBuffer0 = bufferId;
	}

	void cudaRegisterGridBuffer1( GLuint bufferId )
	{
		CUDA_SAFE_CALL( cudaGLRegisterBufferObject( bufferId ) );
		CUT_CHECK_ERROR( "cudaGLRegisterBufferObject failed.\n" );
		g_gridBuffer1 = bufferId;
	}

	void dump( const char* name, float* curr, int size )
	{
		//fprintf( getLogFile(), "	dumping %s:\n", name );

		for( int i = 0; i < size; ++i )
		{
			//fprintf( getLogFile(), "	v[%d]=%.1f\n", i, curr[i] );
		}
	}

	void cudaRunSort( uint numElements )
	{
		float* d_grid0;
		float* d_grid1;
		
		// Map in/out buffers
		//fprintf( getLogFile(), "  cuda map pbo's %d e %d\n", g_gridBuffer0, g_gridBuffer1 );

		CUDA_SAFE_CALL( cudaGLMapBufferObject( (void**)&d_grid0, g_gridBuffer0 ) );
		CUDA_SAFE_CALL( cudaGLMapBufferObject( (void**)&d_grid1, g_gridBuffer1 ) );
		
		//unsigned int* grid0 = (unsigned int*)d_grid0;
		//unsigned int* grid1 = (unsigned int*)d_grid1;
		
		// Sort
		//fprintf( getLogFile(), "  cuda run sort\n" );

		//dump( "sort input", d_grid0, numElements * 2 );
		//exit( 1 );

	    RadixSort( (KeyValuePair*) d_grid0, (KeyValuePair*) d_grid1, numElements, 32);
		CUT_CHECK_ERROR( "Sort execution failed.\n" );
		
		// TODO: testing
		//int numBytes = numElements * 2 * sizeof(float);
		//float* testout = (float*)malloc( numBytes );
		//CUDA_SAFE_CALL( cudaMemcpy( testout, d_grid0, numBytes, cudaMemcpyDeviceToHost ) );
		//CUT_CHECK_ERROR( "cudaMemcpy failed.\n" );
		
		// Unmap grid buffers
		//fprintf( getLogFile(), "  cuda unmap pbo's\n" );

		CUDA_SAFE_CALL( cudaGLUnmapBufferObject( g_gridBuffer1 ) );
 		CUDA_SAFE_CALL( cudaGLUnmapBufferObject( g_gridBuffer0 ) );
	}

} // extern "C"
