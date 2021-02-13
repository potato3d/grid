#ifndef _GRIDCUDA_H_
#define _GRIDCUDA_H_

extern "C"
{
	void cudaDeleteScan();

	void cudaInitScan( int maxNumElements );

	void cudaRegisterInputBuffer( GLuint bufferId );
	void cudaRegisterOutputBuffer( GLuint bufferId );

	void cudaUnregisterBuffers();

	bool cudaRunScan( int numElements, float* maxCount, float* maxAccum );

} // extern "C"

#endif // _GRIDCUDA_H_
