// ------------------------------------------------------------------------
// Particle System -- Tecgraf/PUC-Rio
// www.tecgraf.puc-rio.br/~lduarte
//
// $Source:  $
// $Revision: 1.0 $
// $Date: 2007/10/21 16:02:00 $
//
// $Author: lduarte $
// ------------------------------------------------------------------------

#ifndef _COLLISION_H_
#define _COLLISION_H_

#ifdef WIN32
#include <windows.h>
#endif

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>

#include "shader.h"
#include "coord.h"

#define MAX_TEX_WIDTH  8192
#define MAX_TEX_HEIGHT 512

class cCollision
{
public:
	int _numparticles;

	// DOMAIN Info
	float _MIN_X;
	float _MIN_Y;
	float _MIN_Z;
	float _MAX_X;
	float _MAX_Y;
	float _MAX_Z;
	float _max_accum;
	float _max_count;

	// GRID Info
	int _ncellsX;
	int _ncellsY;
	int _ncellsZ;
	int _ncells;
	int _gridCountW;
	int _gridCountH;
	int _partGridSizeW;
	int _partGridSizeH;
	cCoord _cellSize;
	int _gridW;
	int _gridH;

	// TEXTURES
	GLuint _part_id;
	GLuint _grid_count_id;
	GLuint _grid_accum_id;
	GLuint _grid_id0;
	GLuint _grid_id1;
	GLuint _ncell_id;
	GLuint _cell_accum_id; 
	GLuint _sortgrid_id; 

	// Accum PBOs
	GLuint _cuda_input_pbo_id;
	GLuint _cuda_output_pbo_id;

	// Sort PBOs
	GLuint _cuda_sortgrid0_pbo_id;
	GLuint _cuda_sortgrid1_pbo_id;

	// FRAME BUFFERS
	GLuint _gridCount_fbo;
	GLuint _grid_fbo;
	GLuint _cellCount_fbo;

	// SHADERS
	cShader* _shaderBlendNumParticles;
	cShader* _shaderGrid;  
	cShader* _shaderCountCells;

	GLuint _partVboId;
	GLuint _cellVboId;

	void CheckFramebufferStatus();

public:
	cCollision();

	void initGL();
	void update( float minX, float minY, float minZ, float maxX, float maxY, float maxZ, int nParticles, float *particles, bool fixedSizeGrid = true );

	void updateParticles( float nParticles, float* particles );
	void updateGrid( float minX, float minY, float minZ, float maxX, float maxY, float maxZ, bool fixedSize );
	void updateTextures();
	void updateShaders();

	void constructGrid();

	void cleanup();
};

#endif
