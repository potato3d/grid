#ifndef _GRIDSORT_H_
#define _GRIDSORT_H_

extern "C"
{
	void cudaRegisterGridBuffer0( GLuint bufferId );
	void cudaRegisterGridBuffer1( GLuint bufferId );

	float cudaRunSort( int numElements );

} // extern "C"

#endif // _GRIDCUDA_H_
