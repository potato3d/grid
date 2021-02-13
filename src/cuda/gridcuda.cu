#include <gl/glew.h>
#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime_api.h>
#include <cuda_gl_interop.h>
#include <cutil.h>
//#include <cutil_gl_error.h>

#include <cudpp.h>

#include "../util.h"


////////////////////////////////////////////////////////////////////////////////
// Globals
////////////////////////////////////////////////////////////////////////////////
static CUDPPHandle g_accumPlan;
static CUDPPHandle g_maxPlan;

static GLuint g_inputBuffer = 0;
static GLuint g_outputBuffer = 0;

static float* gd_maxElement = NULL;

static int g_maxNumElements = 0;

//static int count = 0;


////////////////////////////////////////////////////////////////////////////////
// Entry point for Cuda functionality on host side
////////////////////////////////////////////////////////////////////////////////

bool equal( float* in, float* out, int size )
{
	for( int i = 0; i < size; ++i )
	{
		if( in[i] != out[i] )
			return false;
	}

	return true;
}

void dump( const char* name, float* ref, float* curr, int size )
{
	//fprintf( getLogFile(), "	dumping %s:\n", name );

	for( int i = 0; i < size; ++i )
	{
		//fprintf( getLogFile(), "	ref[%d]=%.1f, curr[%d]=%.1f\n", i, ref[i], i, curr[i] );
	}
}

extern "C" 
{
	void cudaDeleteScan()
	{
		cudppDestroyPlan( g_accumPlan );
		cudppDestroyPlan( g_maxPlan );

		if( gd_maxElement != NULL )
			cudaFree( gd_maxElement );
	}

	void cudaInitScan( int maxNumElements )
	{
		// Accumulation
		CUDPPConfiguration config;
		config.algorithm = CUDPP_SCAN;
		config.op = CUDPP_ADD;
		config.datatype = CUDPP_FLOAT;
		config.options = CUDPP_OPTION_FORWARD | CUDPP_OPTION_EXCLUSIVE;

		CUDPPResult result = cudppPlan( &g_accumPlan, config, maxNumElements, 1, 0 );

		if( result != CUDPP_SUCCESS )
		{
	        //fprintf( getLogFile(), "Error creating CUDPPPlan, aborting execution...\n" );
			exit(1);
		}

 		// Reduction
		config.algorithm = CUDPP_SCAN;
		config.op = CUDPP_MAX;
		config.datatype = CUDPP_FLOAT;
		config.options = CUDPP_OPTION_BACKWARD | CUDPP_OPTION_INCLUSIVE;

		result = cudppPlan( &g_maxPlan, config, maxNumElements, 1, 0 );

		if( result != CUDPP_SUCCESS )
		{
	        //fprintf( getLogFile(), "Error creating CUDPPPlan, aborting execution...\n" );
			exit(1);
		}

 		unsigned int byteTotal = maxNumElements * sizeof(float);
 		CUDA_SAFE_CALL( cudaMalloc( (void**)&gd_maxElement, byteTotal ) );
 		CUT_CHECK_ERROR( "cudaMalloc failed.\n" );

		g_maxNumElements = maxNumElements;



//count = 0;
	}

	void cudaRegisterInputBuffer( GLuint bufferId )
	{
		//if( g_inputBuffer > 0 )
			//CUDA_SAFE_CALL( cudaGLUnregisterBufferObject( g_inputBuffer ) );

		// we already rgistered with gridsort.cu
		//CUDA_SAFE_CALL( cudaGLRegisterBufferObject( bufferId ) );
		//CUT_CHECK_ERROR( "cudaGLRegisterBufferObject failed.\n" );
		g_inputBuffer = bufferId;
	}

	void cudaRegisterOutputBuffer( GLuint bufferId )
	{
		//if( g_outputBuffer > 0 )
			//CUDA_SAFE_CALL( cudaGLUnregisterBufferObject( g_outputBuffer ) );

		// we already rgistered with gridsort.cu
		//CUDA_SAFE_CALL( cudaGLRegisterBufferObject( bufferId ) );
		//CUT_CHECK_ERROR( "cudaGLRegisterBufferObject failed.\n" );
		g_outputBuffer = bufferId;
	}

	void cudaUnregisterBuffers()
	{
		if( g_inputBuffer > 0 )
			CUDA_SAFE_CALL( cudaGLUnregisterBufferObject( g_inputBuffer ) );
		if( g_outputBuffer > 0 )
			CUDA_SAFE_CALL( cudaGLUnregisterBufferObject( g_outputBuffer ) );
	}

	bool cudaRunScan( int numElements, float* maxCount, float* maxAccum )
	{
		//if( numElements > g_maxNumElements )
		//{
			//fprintf( getLogFile(), "  cuda numElements %d greater than maxNumElements %d\n", numElements, g_maxNumElements );
			//return false;
		//}

		float* d_inputBuffer;
		float* d_outputBuffer;
		
		// Map in/out buffers
		//fprintf( getLogFile(), "  cuda map pbo's %d e %d\n", g_inputBuffer, g_outputBuffer );

		CUDA_SAFE_CALL( cudaGLMapBufferObject( (void**)&d_inputBuffer, g_inputBuffer ) );
		CUDA_SAFE_CALL( cudaGLMapBufferObject( (void**)&d_outputBuffer, g_outputBuffer ) );

		CUT_CHECK_ERROR( "cudaGLMapBufferObject failed.\n" );

		//fprintf( getLogFile(), "  cuda run accum\n" );



//static float* inputRefEven = new float[g_maxNumElements];
//static float* outputRefEven = new float[g_maxNumElements];
//static float* inputRefOdd = new float[g_maxNumElements];
//static float* outputRefOdd = new float[g_maxNumElements];
//static float* mappedInEven = NULL;
//static float* mappedOutEven = NULL;
//static float* mappedInOdd = NULL;
//static float* mappedOutOdd = NULL;
//
//float* tmpin = new float[g_maxNumElements];
//float* tmpout = new float[g_maxNumElements];
//
//
//if( count == 0 )
//{
//	mappedInEven = d_inputBuffer;
//	mappedOutEven = d_outputBuffer;
//
//	std::fill( inputRefEven, inputRefEven + g_maxNumElements, 0.0f );
//
//	CUDA_SAFE_CALL( cudaMemcpy( &inputRefEven[0], &d_inputBuffer[0], sizeof(float) * numElements - 1, cudaMemcpyDeviceToHost ) );
//}
//else if( count == 1 )
//{
//	mappedInOdd = d_inputBuffer;
//	mappedOutOdd = d_outputBuffer;
//
//	std::fill( inputRefOdd, inputRefOdd + g_maxNumElements, 0.0f );
//
//	CUDA_SAFE_CALL( cudaMemcpy( &inputRefOdd[0], &d_inputBuffer[0], sizeof(float) * numElements - 1, cudaMemcpyDeviceToHost ) );
//}
//else if( count % 2 == 0 )
//{
//	if( mappedInEven != d_inputBuffer )
//	{
//		//fprintf( getLogFile(), "  cuda pointers changed since last call!\n" );
//		return false;
//	}
//
//	if( mappedOutEven != d_outputBuffer )
//	{
//		//fprintf( getLogFile(), "  cuda pointers changed since last call!\n" );
//		return false;
//	}
//
//	std::fill( tmpin, tmpin + g_maxNumElements, 0.0f );
//	CUDA_SAFE_CALL( cudaMemcpy( &tmpin[0], &d_inputBuffer[0], sizeof(float) * numElements - 1, cudaMemcpyDeviceToHost ) );
//	if( !equal( inputRefEven, tmpin, numElements - 1 ) )
//	{
//		//fprintf( getLogFile(), "  cuda buffers changed since last call!\n" );
//		dump( "input", inputRefEven, tmpin, numElements - 1 );
//		dump( "output", outputRefEven, tmpout, numElements );
//		return false;
//	}
//
//}
//else
//{
//	if( mappedInOdd != d_inputBuffer )
//	{
//		//fprintf( getLogFile(), "  cuda pointers changed since last call!\n" );
//		return false;
//	}
//
//	if( mappedOutOdd != d_outputBuffer )
//	{
//		//fprintf( getLogFile(), "  cuda pointers changed since last call!\n" );
//		return false;
//	}
//
//	std::fill( tmpin, tmpin + g_maxNumElements, 0.0f );
//	CUDA_SAFE_CALL( cudaMemcpy( &tmpin[0], &d_inputBuffer[0], sizeof(float) * numElements - 1, cudaMemcpyDeviceToHost ) );
//	if( !equal( inputRefOdd, tmpin, numElements - 1 ) )
//	{
//		//fprintf( getLogFile(), "  cuda buffers changed since last call!\n" );
//		dump( "input", inputRefOdd, tmpin, numElements - 1 );
//		dump( "output", outputRefOdd, tmpout, numElements );
//		return false;
//	}
//
//}







		
		// Accumulate
		CUDPPResult result = cudppScan( g_accumPlan, d_outputBuffer, d_inputBuffer, numElements );

		//if( result != CUDPP_SUCCESS )
		//{
			//fprintf( getLogFile(), "  - error in cuda accum: %d\n", result );
		//}

		CUT_CHECK_ERROR( "Scan execution failed.\n" );







//if( count == 0 )
//{
//	std::fill( outputRefEven, outputRefEven + g_maxNumElements, 0.0f );
//
//	CUDA_SAFE_CALL( cudaMemcpy( &outputRefEven[0], &d_outputBuffer[0], sizeof(float) * numElements, cudaMemcpyDeviceToHost ) );
//}
//else if( count == 1 )
//{
//	std::fill( outputRefOdd, outputRefOdd + g_maxNumElements, 0.0f );
//
//	CUDA_SAFE_CALL( cudaMemcpy( &outputRefOdd[0], &d_outputBuffer[0], sizeof(float) * numElements, cudaMemcpyDeviceToHost ) );
//}
//else if( count % 2 == 0 )
//{
//	std::fill( tmpout, tmpout + g_maxNumElements, 0.0f );
//	CUDA_SAFE_CALL( cudaMemcpy( &tmpout[0], &d_outputBuffer[0], sizeof(float) * numElements, cudaMemcpyDeviceToHost ) );
//	if( !equal( outputRefEven, tmpout, numElements ) )
//	{
//		//fprintf( getLogFile(), "  cuda buffers changed since last call!\n" );
//		dump( "input", inputRefEven, tmpin, numElements - 1 );
//		dump( "output", outputRefEven, tmpout, numElements );
//		return false;
//	}
//}
//else
//{
//	std::fill( tmpout, tmpout + g_maxNumElements, 0.0f );
//	CUDA_SAFE_CALL( cudaMemcpy( &tmpout[0], &d_outputBuffer[0], sizeof(float) * numElements, cudaMemcpyDeviceToHost ) );
//	if( !equal( outputRefOdd, tmpout, numElements ) )
//	{
//		//fprintf( getLogFile(), "  cuda buffers changed since last call!\n" );
//		dump( "input", inputRefOdd, tmpin, numElements - 1 );
//		dump( "output", outputRefOdd, tmpout, numElements );
//		return false;
//	}
//}
//
//
//
//
//delete tmpin;
//delete tmpout;
//
//++count;










		// not needed when using single-pass fragment programs
		//if( maxCount != NULL )
		//{
			////fprintf( getLogFile(), "  cuda run max count\n" );

			//// Find maximum count
			//result = cudppScan( g_maxPlan, gd_maxElement, d_inputBuffer, numElements - 1 );

			//if( result != CUDPP_SUCCESS )
			//{
			//	//fprintf( getLogFile(), "  - error in cuda max count: %d\n", result );	
			//}

			//CUT_CHECK_ERROR( "Scan execution failed.\n" );

			//// Retrieve maximum count
			//CUDA_SAFE_CALL( cudaMemcpy( maxCount, &gd_maxElement[0], sizeof(float), cudaMemcpyDeviceToHost ) );
			//CUT_CHECK_ERROR( "cudaMemcpy failed.\n" );
		//}

		if( maxAccum != NULL )
		{
			//fprintf( getLogFile(), "  cuda get max accum\n" );

			// Retrieve maximum
			CUDA_SAFE_CALL( cudaMemcpy( maxAccum, &d_outputBuffer[numElements-1], sizeof(float), cudaMemcpyDeviceToHost ) );
			CUT_CHECK_ERROR( "cudaMemcpy failed.\n" );
		}

		//fprintf( getLogFile(), "  cuda unmap pbo's\n" );

		// Unmap in/out buffers
		CUDA_SAFE_CALL( cudaGLUnmapBufferObject( g_outputBuffer ) );
		CUDA_SAFE_CALL( cudaGLUnmapBufferObject( g_inputBuffer ) );

		CUT_CHECK_ERROR( "cudaGLUnmapBufferObject failed.\n" );

		return true;
	}

} // extern "C"
