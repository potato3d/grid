#ifndef _GPUGRID_H_
#define _GPUGRID_H_

#ifdef WIN32
#include <windows.h>
#endif

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include "shader.h"

#include <vr/vec3.h>

class GpuGrid
{
public:
	static const unsigned int MAX_TEXTURE_WIDTH  = 8192;
	static const unsigned int MAX_TEXTURE_HEIGHT = 1024;

	GpuGrid();

	void clear();

	void init( int numTriangles, const vr::vec3f& boxMin, const vr::vec3f& boxMax, bool fixedSize = false, bool recomputeResolution = true );

	void rebuildGrid();

	void updateCellSize();

//private:
	int   _numTriangles;
	float _maxCount;
	float _maxAccum;

	vr::vec3f _boxMin;
	vr::vec3f _boxMax;
	vr::vec3<int> _numCells;
	vr::vec3f _cellSize;
	float _k;

	int _cellCountAccumW;
	int _cellCountAccumH;

	int _gridCountAccumW;
	int _gridCountAccumH;

	int _gridDataW;
	int _gridDataH;

	int _maxW; // max( _gridDataW, maxAccumW );
	int _maxH; // max( _gridDataW, maxAccumW );

	GLuint _vboTrigger;
	GLuint _texCountAccum;
	GLuint _texGridData;
	GLuint _texGridAccum;
	GLuint _fboRenderTarget;
	GLuint _pboCudaInput;
	GLuint _pboCudaOutput;

	cShader* _shaderBlendNumParticles;
	cShader* _shaderGrid;  
	cShader* _shaderCountCells;
	cShader* _shaderGridAccum;
};

#endif // _GPUGRID_H_
