// ------------------------------------------------------------------------
// Particle System -- Tecgraf/PUC-Rio
// www.tecgraf.puc-rio.br/~lduarte
//
// $Source:  $
// $Revision: 1.0 $
// $Date: 2007/10/21 16:57:00 $
//
// $Author: lduarte $
// ------------------------------------------------------------------------

#include <algorithm>
#include <float.h>
#include <math.h>
#include <assert.h>

#include <vector>
#include <fstream>

#include "collision.h"
#include "util.h"
#include "cuda/gridcuda.h"

#include "cuda/gridsort.h"
//#include "cuda/radixsort.cuh"

//#define USE_GEOMETRY_GRID
//#define USE_GEOMETRY_BLEND

void addrTranslation1Dto2DNonSquareTex(float address1D, float w, float h, float* addr2dx, float* addr2dy)
{  
	*addr2dx = ( fmod( address1D, w ) + 0.5 ) / w;
	*addr2dy = ( floor( address1D / w ) + 0.5 ) / h;
}

void drawTestQuad( float w, float h )
{
	glColor3f( 0.2f, 1.0f, 1.0f );
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(w, 0.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(w, h);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, h);
	glEnd();
}

void cCollision::CheckFramebufferStatus()
{
	GLenum status;
	status = (GLenum)glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	switch(status)
	{
	case GL_FRAMEBUFFER_COMPLETE_EXT:
		printf("Framebuffer complete\n");
		break;
	case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
		printf("Unsupported framebuffer format\n");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
		printf("Framebuffer incomplete, missing attachment\n");
		break;
		/*  case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT:
		printf("Framebuffer incomplete, duplicate attachment\n");
		break;*/
	case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
		printf("Framebuffer incomplete, attached images must have same dimensions\n");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
		printf("Framebuffer incomplete, attached images must have same format\n");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
		printf("Framebuffer incomplete, missing draw buffer\n");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
		printf("Framebuffer incomplete, missing read buffer\n");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
		printf("Framebuffer incomplete, incomplete attachment\n");
		break;
	default:
		assert(0);
	}
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
cCollision::cCollision()

{
	_part_id  = 0;
	_grid_id0  = 0;
	_gridCount_fbo = 0;
	_cuda_input_pbo_id = 0;
	_cuda_output_pbo_id = 0;

	// Default value just to be safe
	_max_count = 8;
}

void cCollision::initGL()
{
	glewInit();

	// General state
	glEnableClientState( GL_VERTEX_ARRAY );

	glPointSize( 1.0f );

	glEnable(GL_TEXTURE_2D);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Disable color buffer clamping, so we can write values above 1.0!
	glClampColorARB( GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE );

	glGenBuffers( 1, &_partVboId );
	glGenBuffers( 1, &_cellVboId );

	glGenTextures(1, &_part_id);
	glBindTexture(GL_TEXTURE_2D, _part_id);
	setDefaultTexture2dParameters();
	glBindTexture(GL_TEXTURE_2D, 0);

	// CUDA input texture, output of first pass
	glGenTextures(1, &_grid_count_id);
	glBindTexture(GL_TEXTURE_2D, _grid_count_id);
	setDefaultTexture2dParameters();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE32F_ARB, 128, 128, 0, GL_LUMINANCE, GL_FLOAT, NULL); // arbitrary default value
	glBindTexture(GL_TEXTURE_2D, 0);

	// FBO for first pass
	glGenFramebuffersEXT(1, &_gridCount_fbo);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _gridCount_fbo);

	// Attach texture to frame buffer
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, _grid_count_id, 0);

	CheckFramebufferStatus();
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	// PBO for CUDA communication (need additional offset at the beginning)
	glGenBuffers( 1, &_cuda_input_pbo_id );
	glBindBuffer( GL_PIXEL_PACK_BUFFER, _cuda_input_pbo_id );
	glBufferData( GL_PIXEL_PACK_BUFFER, 1024, NULL, GL_STREAM_COPY ); // arbitrary default value
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );

	// PBO for CUDA communication (need additional offset at the beginning)
	glGenBuffers( 1, &_cuda_output_pbo_id );
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _cuda_output_pbo_id );
	glBufferData( GL_PIXEL_UNPACK_BUFFER, 1024, NULL, GL_STREAM_COPY ); // arbitrary default value
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );

	// Initialize CUDA scan (need additional offset at the beginning)
	cudaInitScan( MAX_TEX_WIDTH * MAX_TEX_HEIGHT );

	// CUDA output texture, input for second pass
	glGenTextures(1, &_grid_accum_id);
	glBindTexture(GL_TEXTURE_2D, _grid_accum_id);
	setDefaultTexture2dParameters();
	glBindTexture(GL_TEXTURE_2D, 0);

	// CUDA output texture, input for second pass
	glGenTextures(1, &_cell_accum_id);
	glBindTexture(GL_TEXTURE_2D, _cell_accum_id);
	setDefaultTexture2dParameters();
	glBindTexture(GL_TEXTURE_2D, 0);

	// Hard-coded size
	_gridW = MAX_TEX_WIDTH;
	_gridH = MAX_TEX_HEIGHT;

	// CUDA buffers to radixsort (_grid0 is the input and output)

	// PBO for SORT CUDA communication
	glGenBuffers( 1, &_cuda_sortgrid0_pbo_id );
	glBindBuffer( GL_PIXEL_PACK_BUFFER, _cuda_sortgrid0_pbo_id );
	glBufferData( GL_PIXEL_PACK_BUFFER, _gridW * _gridH * 2 * sizeof(float), NULL, GL_STREAM_COPY );
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );

	// Register PBO to SORT CUDA for later read/write
	cudaRegisterGridBuffer0( _cuda_sortgrid0_pbo_id );

	// PBO for SORT CUDA communication
	glGenBuffers( 1, &_cuda_sortgrid1_pbo_id );
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _cuda_sortgrid1_pbo_id );
	glBufferData( GL_PIXEL_UNPACK_BUFFER, _gridW * _gridH * 2 * sizeof(float), NULL, GL_STREAM_COPY );
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );

	// Register PBO to SORT CUDA for later read/write
	cudaRegisterGridBuffer1( _cuda_sortgrid1_pbo_id );

	// SORT CUDA output texture, grid result
	glGenTextures(1, &_sortgrid_id);
	glBindTexture(GL_TEXTURE_2D, _sortgrid_id);
	setDefaultTexture2dParameters();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA32F_ARB, _gridW, _gridH, 0, GL_LUMINANCE_ALPHA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Output texture from second pass
	glGenTextures(1, &_grid_id0);
	glBindTexture(GL_TEXTURE_2D, _grid_id0);
	setDefaultTexture2dParameters();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA32F_ARB, _gridW, _gridH, 0, GL_LUMINANCE_ALPHA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	// FBO for second pass
	glGenFramebuffersEXT(1, &_grid_fbo);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _grid_fbo);

	// Attach texture to frame buffer
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, _grid_id0, 0);

	CheckFramebufferStatus();
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	// Stores number of cells per particle
	glGenTextures(1, &_ncell_id);
	glBindTexture(GL_TEXTURE_2D, _ncell_id);
	setDefaultTexture2dParameters();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE32F_ARB, 128, 128, 0, GL_LUMINANCE, GL_FLOAT, NULL); // arbitrary default value
	glBindTexture(GL_TEXTURE_2D, 0);

	// FBO for counting number of cells
	glGenFramebuffersEXT(1, &_cellCount_fbo);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _cellCount_fbo);

	// Attach texture to frame buffer
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, _ncell_id, 0);

	CheckFramebufferStatus();
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	//////////////////////////////////////////////////////////////////////////
	// Shaders
	//////////////////////////////////////////////////////////////////////////
	//INIT SHADER
	string vpFilename = "../shaders/countCellVP.glsl";
	string fpFilename = "../shaders/defaultFP.glsl";

	////////////////////////////////////////////////
	// SHADER 1
	////////////////////////////////////////////////
	_shaderCountCells = new cShader();
	_shaderCountCells->Compile(cShader::VP, vpFilename.c_str());
	_shaderCountCells->Compile(cShader::FP, fpFilename.c_str());
	_shaderCountCells->Link(GL_POINTS);
	_shaderCountCells->Load();
	_shaderCountCells->BindTexture("particles",0);
	_shaderCountCells->Unload();


	////////////////////////////////////////////////
	// SHADER 2
	////////////////////////////////////////////////
	string gpFilename = "../shaders/blendPartGP.glsl";
#ifdef USE_GEOMETRY_BLEND
	vpFilename = "../shaders/blendPartVP.glsl";
#else
	vpFilename = "../shaders/blendPartVP_sem_GS.glsl";
#endif

	_shaderBlendNumParticles = new cShader();
#ifdef USE_GEOMETRY_BLEND
	_shaderBlendNumParticles->Compile(cShader::GP, gpFilename.c_str());
#endif
	_shaderBlendNumParticles->Compile(cShader::VP,vpFilename.c_str());
	_shaderBlendNumParticles->Compile(cShader::FP,fpFilename.c_str());
	_shaderBlendNumParticles->Link(GL_POINTS, 8);
	_shaderBlendNumParticles->Load();
	_shaderBlendNumParticles->BindTexture("particles",0);
	_shaderBlendNumParticles->Unload();


	////////////////////////////////////////////////
	// SHADER 3
	////////////////////////////////////////////////
	gpFilename = "../shaders/gridGP.glsl";
#ifdef USE_GEOMETRY_GRID
	vpFilename = "../shaders/gridVP.glsl";
#else
	vpFilename = "../shaders/gridVP_sem_GS.glsl";
#endif

	_shaderGrid = new cShader();
#ifdef USE_GEOMETRY_GRID
	_shaderGrid->Compile(cShader::GP, gpFilename.c_str());
#endif
	_shaderGrid->Compile(cShader::VP, vpFilename.c_str());
	_shaderGrid->Compile(cShader::FP, fpFilename.c_str());
	_shaderGrid->Link(GL_POINTS, 8);
	_shaderGrid->Load();
	_shaderGrid->BindTexture("particles",0);
	_shaderGrid->BindTexture("cellAccumIn",1);
	_shaderGrid->Unload();
}

void cCollision::update( float minX, float minY, float minZ, float maxX, float maxY, float maxZ, int nParticles, float* particles, bool fixedSizeGrid )
{
	updateParticles( nParticles, particles );
	updateGrid( minX, minY, minZ, maxX, maxY, maxZ, fixedSizeGrid );
	updateTextures();
	updateShaders();
}

void cCollision::updateParticles( float nParticles, float* particles )
{
	_numparticles = nParticles;

	// Reserve space in textures for an additional hypothetical particle
	// This space will store information such as starting point for this particle's cell block
	// It will be used by last actual particle
	int np = _numparticles + 1;

	_partGridSizeW = ( np < MAX_TEX_WIDTH )? np : MAX_TEX_WIDTH;
	_partGridSizeH = floorf( np / MAX_TEX_WIDTH ) + 1;

	std::vector<float> ids( _numparticles*3 );
	float ad2x, ad2y;

	for( int i = 0; i < _numparticles; i++ )
	{
		addrTranslation1Dto2DNonSquareTex( i, _partGridSizeW, _partGridSizeH, &ad2x, &ad2y );

		ids[3*i+0] = i;
		ids[3*i+1] = ad2x;
		ids[3*i+2] = ad2y;
	}

	// Setup VBO
	glBindBuffer( GL_ARRAY_BUFFER, _partVboId );
	glBufferData( GL_ARRAY_BUFFER, 3*_numparticles * sizeof(float), &ids[0], GL_STATIC_DRAW );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	// Reserve space for all texels, given computed texture size
	std::vector<float> particlesTexels( _partGridSizeW * _partGridSizeH * 4, 0.0f );
	std::copy( particles, particles + _numparticles * 4, particlesTexels.begin() );

	// Init particles texture
	glBindTexture(GL_TEXTURE_2D, _part_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, _partGridSizeW, _partGridSizeH, 0, GL_RGBA, GL_FLOAT, &particlesTexels[0] );
	glBindTexture(GL_TEXTURE_2D, 0);
}

void cCollision::updateGrid( float minX, float minY, float minZ, float maxX, float maxY, float maxZ, bool fixedSize )
{
	_MIN_X = minX;
	_MIN_Y = minY;
	_MAX_X = maxX;
	_MAX_Y = maxY;
	_MIN_Z = minZ;
	_MAX_Z = maxZ;

	float diagonalX = _MAX_X - _MIN_X;
	float diagonalY = _MAX_Y - _MIN_Y;
	float diagonalZ = _MAX_Z - _MIN_Z;

	if( fixedSize )
	{
		// Fixed size grid
		_ncellsX = 100;
		_ncellsY = 100;
		_ncellsZ = 100;
	}
	else
	{
		// TODO: magic user-supplied number
		int k = 6;
		float V = ( diagonalX * diagonalY * diagonalZ );
		float factor = powf( (float)( k * _numparticles ) / V, 1.0f / 3.0f );

		// Compute actual number of cells in each dimension
		_ncellsX = (int)ceilf( diagonalX * factor );
		_ncellsY = (int)ceilf( diagonalY * factor );
		_ncellsZ = (int)ceilf( diagonalZ * factor );
	}

	_cellSize.x = diagonalX / (float)_ncellsX;
	_cellSize.y = diagonalY / (float)_ncellsY;
	_cellSize.z = diagonalZ / (float)_ncellsZ;

	//printf("nx: %d ny: %d nz: %d \n", _ncellsX, _ncellsY, _ncellsZ);

	_ncells = _ncellsX*_ncellsY*_ncellsZ;

	_gridCountW = MAX_TEX_WIDTH;
	_gridCountH = 1 + _ncells / MAX_TEX_WIDTH;

	std::vector<float> cellIds( _ncells * 3 );

	float ad2x, ad2y;
	for(int n = 0; n < _ncells; n++)
	{
		addrTranslation1Dto2DNonSquareTex( n, _gridCountW, _gridCountH, &ad2x, &ad2y );
		cellIds[3*n+0] = n;
		cellIds[3*n+1] = ad2x;
		cellIds[3*n+2] = ad2y;
	}

	glBindBuffer( GL_ARRAY_BUFFER, _cellVboId );
	glBufferData( GL_ARRAY_BUFFER, 3*_ncells * sizeof(float), &cellIds[0], GL_STATIC_DRAW );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void cCollision::updateTextures()
{
	// Initial PBO memory
	float maxNumElements = Max( _gridCountW*_gridCountH, _partGridSizeW*_partGridSizeH );

	// PBO for CUDA communication (need additional offset at the beginning)
	glBindBuffer( GL_PIXEL_PACK_BUFFER, _cuda_input_pbo_id );
	glBufferData( GL_PIXEL_PACK_BUFFER, maxNumElements * sizeof(float), NULL, GL_STREAM_COPY );
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );

	cudaRegisterInputBuffer( _cuda_input_pbo_id );

	// PBO for CUDA communication (need additional offset at the beginning)
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _cuda_output_pbo_id );
	glBufferData( GL_PIXEL_UNPACK_BUFFER, maxNumElements * sizeof(float), NULL, GL_STREAM_COPY );
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );

	cudaRegisterOutputBuffer( _cuda_output_pbo_id );

	// CUDA input texture, output of first pass
	glBindTexture(GL_TEXTURE_2D, _grid_count_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE32F_ARB, _gridCountW, _gridCountH, 0, GL_LUMINANCE, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	// CUDA output texture, input for second pass
	glBindTexture(GL_TEXTURE_2D, _grid_accum_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE32F_ARB, _gridCountW, _gridCountH, 0, GL_LUMINANCE, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	// CUDA output texture, input for second pass
	glBindTexture(GL_TEXTURE_2D, _cell_accum_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE32F_ARB, _partGridSizeW, _partGridSizeH, 0, GL_LUMINANCE, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Stores number of cells per particle
	glBindTexture(GL_TEXTURE_2D, _ncell_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE32F_ARB, _partGridSizeW, _partGridSizeH, 0, GL_LUMINANCE, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void cCollision::updateShaders()
{
	//////////////////////////////////////////////////////////////////////////
	// Shader 1
	//////////////////////////////////////////////////////////////////////////
	_shaderCountCells->Load();

	_shaderCountCells->SetConstant("cellSize", _cellSize.x, _cellSize.y, _cellSize.z);
	_shaderCountCells->SetConstant("texPartSizeW" , _partGridSizeW);
	_shaderCountCells->SetConstant("texPartSizeH" , _partGridSizeH);
	_shaderCountCells->SetConstant("minGrid" , _MIN_X, _MIN_Y, _MIN_Z);
	_shaderCountCells->SetConstant("ncells"  , (float)_ncellsX, (float)_ncellsY, (float)_ncellsZ);
	_shaderCountCells->SetConstant("invCellSize", 1.0f / _cellSize.x, 1.0f / _cellSize.y, 1.0f / _cellSize.z);

	_shaderCountCells->Unload();


	//////////////////////////////////////////////////////////////////////////
	// Shader 2
	//////////////////////////////////////////////////////////////////////////
	_shaderBlendNumParticles->Load();

	_shaderBlendNumParticles->SetConstant("cellSize", _cellSize.x, _cellSize.y, _cellSize.z);
	_shaderBlendNumParticles->SetConstant("invCellSize", 1.0f / _cellSize.x, 1.0f / _cellSize.y, 1.0f / _cellSize.z);
	_shaderBlendNumParticles->SetConstant("texGridAccumSize", (float)_gridCountW, 1.0f / (float)_gridCountW);
	_shaderBlendNumParticles->SetConstant("minGrid" , _MIN_X, _MIN_Y, _MIN_Z);
	_shaderBlendNumParticles->SetConstant("ncells"  , (float)_ncellsX, (float)_ncellsY, (float)_ncellsZ);
	_shaderBlendNumParticles->SetConstant("_3Dto1Dconst", (float)_ncellsX, (float)_ncellsX*_ncellsY);

	_shaderBlendNumParticles->Unload();


	//////////////////////////////////////////////////////////////////////////
	// Shader 3
	//////////////////////////////////////////////////////////////////////////
	_shaderGrid->Load();

	_shaderGrid->SetConstant("cellSize", _cellSize.x, _cellSize.y, _cellSize.z);
	_shaderGrid->SetConstant("invCellSize", 1.0f / _cellSize.x, 1.0f / _cellSize.y, 1.0f / _cellSize.z);
	_shaderGrid->SetConstant("texCellAccumSize", (float)_partGridSizeW, 1.0f / (float)_partGridSizeW, 1.0f / (float)_partGridSizeH );
	_shaderGrid->SetConstant("texGridSize", (float)_gridW, 1.0f / (float)_gridW );
	_shaderGrid->SetConstant("minGrid" , _MIN_X, _MIN_Y, _MIN_Z);
	_shaderGrid->SetConstant("ncells"  , (float)_ncellsX, (float)_ncellsY, (float)_ncellsZ);
	_shaderGrid->SetConstant("_3Dto1Dconst", (float)_ncellsX, (float)_ncellsX*_ncellsY);

	_shaderGrid->Unload();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void cCollision::cleanup(void)
{
	// Cleanup
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glDrawBuffer( GL_BACK );
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, 0 );
	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_2D, 0 );
	glActiveTexture( GL_TEXTURE2 );
	glBindTexture( GL_TEXTURE_2D, 0 );

	// Pop modelview
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	//glPopAttrib();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void cCollision::constructGrid(void)
{
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	// Particle data (position, radius)
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _part_id);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Pass 1
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Setup view to get 1:1 pixel<->texel correspondence
	glViewport(0, 0, (GLsizei)_partGridSizeW, (GLsizei)_partGridSizeH);

	//glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0f, _partGridSizeW, 0.0f, _partGridSizeH);

	// Set FBO for second pass
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _cellCount_fbo);

	_shaderCountCells->Load();

	// Draw a vertex for each particle
	glBindBuffer( GL_ARRAY_BUFFER, _partVboId );
	glVertexPointer( 3, GL_FLOAT, 0, NULL );
	glDrawArrays( GL_POINTS, 0, _numparticles );

	//_shaderCountCells->Unload();
	//cleanup();
	//return;

	// TODO: testing
#ifdef _DEBUG
	std::vector<float> countcells( _partGridSizeW * _partGridSizeH );
	glReadPixels( 0, 0, _partGridSizeW, _partGridSizeH, GL_LUMINANCE, GL_FLOAT, &countcells[0] );
#endif

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Pass 2
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Copy data from frame buffer texture to _cuda_input_pbo_id
	glBindBuffer( GL_PIXEL_PACK_BUFFER, _cuda_input_pbo_id );
	glReadPixels( 0, 0, _partGridSizeW, _partGridSizeH, GL_LUMINANCE, GL_FLOAT, (char*)NULL );
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );

	// Run CUDA to accumulate cell_count data
	cudaRunScan( _numparticles + 1, &_max_count, &_max_accum );

	// Copy data from _cuda_output_pbo_id to grid_accum texture
	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_2D, _cell_accum_id );
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _cuda_output_pbo_id );
	glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, _partGridSizeW, _partGridSizeH, GL_LUMINANCE, GL_FLOAT, (char*)NULL );
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );

	//cleanup();
	//return;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Pass 3
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Setup view to get 1:1 pixel<->texel correspondence
	glViewport(0, 0, (GLsizei)_gridW, (GLsizei)_gridH);

	//glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0f, _gridW, 0.0f, _gridH);

	// Set FBO for second pass
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _grid_fbo);

#ifdef _DEBUG
	glClearColor( 0,0,0,0 );
	glClear( GL_COLOR_BUFFER_BIT );
	glClearColor( 0,0,0,1 );
#endif

	_shaderGrid->Load();

	// Draw a vertex for each particle
	glBindBuffer( GL_ARRAY_BUFFER, _partVboId );
	glVertexPointer( 3, GL_FLOAT, 0, NULL );

#ifdef USE_GEOMETRY_GRID
	glDrawArrays( GL_POINTS, 0, _numparticles );
#else
	for( int i = 0; i < _max_count; ++i )
	{
		_shaderGrid->SetConstant( "passIdx", i );
		glDrawArrays( GL_POINTS, 0, _numparticles );
	}
#endif

	//_shaderGrid->Unload();
	//cleanup();
	//return;

	// TODO: testing
#ifdef _DEBUG
	std::vector<float> gridunsorted( _gridW * _gridH * 2 );
	glReadPixels( 0, 0, _gridW, _gridH, GL_LUMINANCE_ALPHA, GL_FLOAT, &gridunsorted[0] );
#endif


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Pass 4
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	int w = ( _max_accum >= _gridW )? _gridW : _max_accum;
	int h = floor( _max_accum / _gridW ) + 1;

	// Copy data from frame buffer texture to cuda_sort_pbo_id
	glBindBuffer( GL_PIXEL_PACK_BUFFER, _cuda_sortgrid0_pbo_id );
	glReadPixels( 0, 0, w, h, GL_LUMINANCE_ALPHA, GL_FLOAT, (char*)NULL );
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );

	// Run CUDA to sort the grid
	cudaRunSort( _max_accum );

	// Copy data from _cuda_sortgrid0_pbo_id to sortgrid texture
	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_2D, _sortgrid_id );
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _cuda_sortgrid0_pbo_id );
	glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, w, h, GL_LUMINANCE_ALPHA, GL_FLOAT, (char*)NULL );
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );



	//cleanup();
	//return;




	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Pass 5
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Setup view to get 1:1 pixel<->texel correspondence
	glViewport(0, 0, (GLsizei)_gridCountW, (GLsizei)_gridCountH);

	//glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0f, _gridCountW, 0.0f, _gridCountH);

	// Blend to count number of particles per cell
	glEnable( GL_BLEND );
	glBlendFunc( GL_ONE, GL_ONE );

	// Disable depth test to accumulate multiple vertices within the same position
	glDisable( GL_DEPTH_TEST );

	// First pass FBO
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _gridCount_fbo);

	// First pass shaders
	_shaderBlendNumParticles->Load();

	// Clear FBO contents
	glClear( GL_COLOR_BUFFER_BIT );

	// Render each particle as a single vertex and process them in geometry shader
	glBindBuffer( GL_ARRAY_BUFFER, _partVboId );
	glVertexPointer( 3, GL_FLOAT, 0, NULL );

#ifdef USE_GEOMETRY_BLEND
	glDrawArrays( GL_POINTS, 0, _numparticles );
#else
	for( int i = 0; i < _max_count; ++i )
	{
		_shaderBlendNumParticles->SetConstant( "passIdx", i );
		glDrawArrays( GL_POINTS, 0, _numparticles );
	}
#endif

	// Cleanup
	_shaderBlendNumParticles->Unload();
	glDisable( GL_BLEND );

	//cleanup();
	//return;

	// TODO: testing
#ifdef _DEBUG
	std::vector<float> countpixels( _gridCountW * _gridCountH );
	glReadPixels( 0, 0, _gridCountW, _gridCountH, GL_LUMINANCE, GL_FLOAT, &countpixels[0] );
#endif


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Pass 6
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Copy data from frame buffer texture to _cuda_input_pbo_id
	glBindBuffer( GL_PIXEL_PACK_BUFFER, _cuda_input_pbo_id );
	glReadPixels( 0, 0, _gridCountW, _gridCountH, GL_LUMINANCE, GL_FLOAT, (char*)NULL );
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );

	// Run CUDA to accumulate grid_count data
	cudaRunScan( _gridCountW*_gridCountH, NULL, NULL );

	// Copy data from _cuda_output_pbo_id to grid_accum texture
	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_2D, _grid_accum_id );
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _cuda_output_pbo_id );
	glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, _gridCountW, _gridCountH, GL_LUMINANCE, GL_FLOAT, (char*)NULL );
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );





	cleanup();
	return;
}
