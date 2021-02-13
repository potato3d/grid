#include "GpuGrid.h"
#include <cmath>
#include <vector>
#include <cassert>

#include "cuda/gridcuda.h"
#include "cuda/gridsort.h"
#include "util.h"

#include <vr/timer.h>

static void addrTranslation1Dto2DNonSquareTex( float address1D, float w, float h, float& addr2dx, float& addr2dy )
{  
	addr2dx = fmod( address1D, w );
	addr2dy = floor( address1D / w );
}

static void checkFramebufferStatus()
{
	GLenum status;
	status = (GLenum)glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	switch(status)
	{
	case GL_FRAMEBUFFER_COMPLETE_EXT:
		//fprintf( getLogFile(), "Framebuffer complete\n");
		break;
	case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
		//fprintf( getLogFile(), "Unsupported framebuffer format\n");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
		//fprintf( getLogFile(), "Framebuffer incomplete, missing attachment\n");
		break;
		/*  case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT:
		log("Framebuffer incomplete, duplicate attachment\n");
		break;*/
	case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
		//fprintf( getLogFile(), "Framebuffer incomplete, attached images must have same dimensions\n");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
		//fprintf( getLogFile(), "Framebuffer incomplete, attached images must have same format\n");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
		//fprintf( getLogFile(), "Framebuffer incomplete, missing draw buffer\n");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
		//fprintf( getLogFile(), "Framebuffer incomplete, missing read buffer\n");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
		//fprintf( getLogFile(), "Framebuffer incomplete, incomplete attachment\n");
		break;
	default:
		assert(0);
	}
}

GpuGrid::GpuGrid()
{
	_k = 5;
}

void GpuGrid::clear()
{
	cudaDeleteScan();
	cudaUnregisterBuffers();

	glDeleteTextures( 1, &_texCountAccum );
	glDeleteTextures( 1, &_texGridData );
	glDeleteTextures( 1, &_texGridAccum );
	
	glDeleteBuffers( 1, &_vboTrigger );
	glDeleteBuffers( 1, &_pboCudaInput );
	glDeleteBuffers( 1, &_pboCudaOutput );
}

void GpuGrid::updateCellSize()
{
	vr::vec3f diagonal = _boxMax - _boxMin;

	_cellSize.x = diagonal.x / (float)_numCells.x;
	_cellSize.y = diagonal.y / (float)_numCells.y;
	_cellSize.z = diagonal.z / (float)_numCells.z;
}

void GpuGrid::init( int numTriangles, const vr::vec3f& boxMin, const vr::vec3f& boxMax, bool fixedSize, bool recomputeResolution )
{
	//////////////////////////////////////////////////////////////////////////
	// Initial OpenGL setup
	//////////////////////////////////////////////////////////////////////////
	glewInit();

	// General state
	glEnableClientState( GL_VERTEX_ARRAY );

	glPointSize( 1.0f );

	glEnable(GL_TEXTURE_RECTANGLE_ARB);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Disable color buffer clamping, so we can write values above 1.0!
	glClampColorARB( GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE );

	//////////////////////////////////////////////////////////////////////////
	// Setup Grid
	//////////////////////////////////////////////////////////////////////////
	_numTriangles = numTriangles;
	_boxMin = boxMin;
	_boxMax = boxMax;

	if( recomputeResolution )
	{
		vr::vec3f diagonal = _boxMax - _boxMin;

		if( fixedSize )
		{
			_numCells.x = 100;
			_numCells.y = 100;
			_numCells.z = 100;
		}
		else
		{
			float V = ( diagonal.x * diagonal.y * diagonal.z );
			float factor = powf( (float)( _k * _numTriangles ) / V, 1.0f / 3.0f );

			// Compute actual number of cells in each dimension
			_numCells.x = (int)ceilf( diagonal.x * factor );
			_numCells.y = (int)ceilf( diagonal.y * factor );
			_numCells.z = (int)ceilf( diagonal.z * factor );
		}

		_cellSize.x = diagonal.x / (float)_numCells.x;
		_cellSize.y = diagonal.y / (float)_numCells.y;
		_cellSize.z = diagonal.z / (float)_numCells.z;
	}

	int numCells = _numCells.x * _numCells.y * _numCells.z;

	// Reserve space in textures for an additional hypothetical cell
	// This space will store information such as starting point for this cell's block
	// It will be used by last actual cell to determine its block size
	convertTo2dSize( numCells + 1, MAX_TEXTURE_WIDTH, _gridCountAccumW, _gridCountAccumH );

	// Reserve space in textures for an additional hypothetical triangle
	// This space will store information such as starting point for this triangle's cell block
	// It will be used by last actual triangle to determine its block size
	convertTo2dSize( _numTriangles + 1, MAX_TEXTURE_WIDTH, _cellCountAccumW, _cellCountAccumH );

	int maxAccumW = vr::max( _cellCountAccumW, _gridCountAccumW );
	int maxAccumH = vr::max( _cellCountAccumH, _gridCountAccumH );

	// conteudo: cell count, cell accum, grid count, grid accum
	// tamanho: num_cells em 2D
	//fprintf( getLogFile(),  "allocating texture in GPU (%.1f MB)... ", maxAccumW * maxAccumH * 1 * sizeof(float) * 0.00000095367431640625 );
#if TRACK_ALLOC
	fgetc(stdin);
#endif

	glGenTextures( 1, &_texCountAccum );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, _texCountAccum );
	setDefaultTexture2dParameters();
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_LUMINANCE32F_ARB, maxAccumW, maxAccumH, 0, GL_LUMINANCE, GL_FLOAT, NULL );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );

	//fprintf( getLogFile(),  "done\n" );

	// Since we don't know a priori what size the grid data will have, we use a hard-coded upper-bound
	_gridDataW = MAX_TEXTURE_WIDTH;
	_gridDataH = MAX_TEXTURE_HEIGHT;

	// conteudo: grid data (cellId, triId) e grid unsorted (em fbo 1)
	// tamanho: arbitrario (8192 x ?, max 4M texels por causa do limite do sort? tentar mais)
	//fprintf( getLogFile(),  "allocating texture in GPU (%.1f MB)... ", _gridDataW * _gridDataH * 2 * sizeof(float) * 0.00000095367431640625 );
#if TRACK_ALLOC
	fgetc(stdin);
#endif

	glGenTextures( 1, &_texGridData );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, _texGridData );
	setDefaultTexture2dParameters();
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_LUMINANCE_ALPHA32F_ARB, _gridDataW, _gridDataH, 0, GL_LUMINANCE_ALPHA, GL_FLOAT, NULL );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );

	//fprintf( getLogFile(),  "done\n" );

	// conteudo: grid accum com 2 floats por texel ( cell start, cell size )
	// tamanho: num_cells em 2D
	//fprintf( getLogFile(),  "allocating texture in GPU (%.1f MB)... ", _gridCountAccumW * _gridCountAccumH * 2 * sizeof(float) * 0.00000095367431640625 );
#if TRACK_ALLOC
	fgetc(stdin);
#endif

	glGenTextures( 1, &_texGridAccum );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, _texGridAccum );
	setDefaultTexture2dParameters();
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_LUMINANCE_ALPHA32F_ARB, _gridCountAccumW, _gridCountAccumH, 0, GL_LUMINANCE_ALPHA, GL_FLOAT, NULL );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );

	//fprintf( getLogFile(),  "done\n" );

	//////////////////////////////////////////////////////////////////////////
	// Setup basic VBO for triggering vertex and fragment processors
	//////////////////////////////////////////////////////////////////////////
	
	// conteudo: (id, 0)
	// tamanho: max( num_triangles, num_cells )
	int size = vr::max( numTriangles, numCells );
	std::vector<float> data( size * 2 );
	for( int i = 0; i < size; ++i )
	{
		data[i*2]   = i;
		data[i*2+1] = 0;
	}

	//fprintf( getLogFile(),  "allocating VBO in GPU (%.1f MB)... ", data.size() * sizeof(float) * 0.00000095367431640625 );
#if TRACK_ALLOC
	fgetc(stdin);
#endif

	glGenBuffers( 1, &_vboTrigger );
	glBindBuffer( GL_ARRAY_BUFFER, _vboTrigger );
	glBufferData( GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	//fprintf( getLogFile(), "done\n" );

	//////////////////////////////////////////////////////////////////////////
	// Setup auxiliary OpenGL containers
	//////////////////////////////////////////////////////////////////////////

	// Basic FBO for all renderings
	glGenFramebuffersEXT( 1, &_fboRenderTarget );

	_maxW = vr::max( _gridDataW, maxAccumW );
	_maxH = vr::max( _gridDataH, maxAccumH );

	// conteudo: input cell count, cell accum e grid data
	//fprintf( getLogFile(), "allocating PBO in GPU (%.1f MB)... ", _maxW * _maxH * 2 * sizeof(float) * 0.00000095367431640625 );
#if TRACK_ALLOC
	fgetc(stdin);
#endif

	glGenBuffers( 1, &_pboCudaInput );
	glBindBuffer( GL_PIXEL_PACK_BUFFER, _pboCudaInput );
	glBufferData( GL_PIXEL_PACK_BUFFER, _maxW * _maxH * 2 * sizeof(float), NULL, GL_STREAM_COPY );
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );

	//fprintf( getLogFile(), "done\n" );

	cudaRegisterGridBuffer0( _pboCudaInput );
	cudaRegisterInputBuffer( _pboCudaInput );

	// conteudo: output cell count, cell accum e grid data
	//fprintf( getLogFile(), "allocating PBO in GPU (%.1f MB)... ", _maxW * _maxH * 2 * sizeof(float) * 0.00000095367431640625 );
#if TRACK_ALLOC
	fgetc(stdin);
#endif

	glGenBuffers( 1, &_pboCudaOutput );
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _pboCudaOutput );
	glBufferData( GL_PIXEL_UNPACK_BUFFER, _maxW * _maxH * 2 * sizeof(float), NULL, GL_STREAM_COPY );
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );

	//fprintf( getLogFile(), "done\n" );

	cudaRegisterGridBuffer1( _pboCudaOutput );
	cudaRegisterOutputBuffer( _pboCudaOutput );

	cudaInitScan( maxAccumW * maxAccumH );

	//////////////////////////////////////////////////////////////////////////
	// Setup shaders
	//////////////////////////////////////////////////////////////////////////
	//std::string vpFilename = "../shaders/countCellVP.glsl";
	//std::string fpFilename = "../shaders/defaultFP.glsl";
	std::string vpFilename = "../shaders/defaultVP.glsl";
	std::string fpFilename = "../shaders/countCellFP.glsl";

	////////////////////////////////////////////////
	// SHADER 1
	////////////////////////////////////////////////
	_shaderCountCells = new cShader();
	_shaderCountCells->Compile(cShader::VP, vpFilename.c_str());
	_shaderCountCells->Compile(cShader::FP, fpFilename.c_str());
	_shaderCountCells->Link(GL_POINTS);
	_shaderCountCells->Load();

	_shaderCountCells->BindTexture( "u_texTriangleVertices", 0 );
	_shaderCountCells->SetConstant( "u_minGrid", _boxMin.x, _boxMin.y, _boxMin.z );
	_shaderCountCells->SetConstant( "u_invCellSize", 1.0f / _cellSize.x, 1.0f / _cellSize.y, 1.0f / _cellSize.z );
	_shaderCountCells->SetConstant( "u_numCells", _numCells.x, _numCells.y, _numCells.z );
	//_shaderCountCells->SetConstant( "u_viewportW", _cellCountAccumW );
	//_shaderCountCells->SetConstant( "u_invViewportW", 1.0f / (float)_cellCountAccumW );

	_shaderCountCells->Unload();


	////////////////////////////////////////////////
	// SHADER 2
	////////////////////////////////////////////////
	std::string gpFilename = "../shaders/blendPartGP.glsl";
	fpFilename = "../shaders/blendPartFP.glsl";

#ifdef USE_GEOMETRY_BLEND
	vpFilename = "../shaders/blendPartVP.glsl";
#else
	//vpFilename = "../shaders/blendPartVP_sem_GS.glsl";
	vpFilename = "../shaders/defaultVP.glsl";
#endif

	_shaderBlendNumParticles = new cShader();
#ifdef USE_GEOMETRY_BLEND
	_shaderBlendNumParticles->Compile(cShader::GP, gpFilename.c_str());
#endif
	_shaderBlendNumParticles->Compile(cShader::VP,vpFilename.c_str());
	_shaderBlendNumParticles->Compile(cShader::FP,fpFilename.c_str());
	_shaderBlendNumParticles->Link(GL_POINTS, 8);
	_shaderBlendNumParticles->Load();

	_shaderBlendNumParticles->BindTexture( "u_texGridData", 1 );
	_shaderBlendNumParticles->SetConstant( "u_texGridDataSize", _gridDataW * _gridDataH );
	//_shaderBlendNumParticles->SetConstant( "u_minGrid", _boxMin.x, _boxMin.y, _boxMin.z );
	//_shaderBlendNumParticles->SetConstant( "u_invCellSize", 1.0f / _cellSize.x, 1.0f / _cellSize.y, 1.0f / _cellSize.z );
	//_shaderBlendNumParticles->SetConstant( "u_numCells", _numCells.x, _numCells.y, _numCells.z );
	//_shaderBlendNumParticles->SetConstant( "u_viewportW", _gridCountAccumW );
	//_shaderBlendNumParticles->SetConstant( "u_invViewportW", 1.0f / (float)_gridCountAccumW );

	_shaderBlendNumParticles->Unload();


	////////////////////////////////////////////////
	// SHADER 3
	////////////////////////////////////////////////
	gpFilename = "../shaders/gridGP.glsl";
	fpFilename = "../shaders/gridFP.glsl";
	//fpFilename = "../shaders/defaultFP.glsl";

#ifdef USE_GEOMETRY_GRID
	vpFilename = "../shaders/gridVP.glsl";
#else
	//vpFilename = "../shaders/gridVP_sem_GS.glsl";
	vpFilename = "../shaders/defaultVP.glsl";
#endif

	_shaderGrid = new cShader();
#ifdef USE_GEOMETRY_GRID
	_shaderGrid->Compile(cShader::GP, gpFilename.c_str());
#endif
	_shaderGrid->Compile(cShader::VP, vpFilename.c_str());
	_shaderGrid->Compile(cShader::FP, fpFilename.c_str());
	_shaderGrid->Link(GL_POINTS, 8);
	_shaderGrid->Load();

	_shaderGrid->BindTexture( "u_texTriangleVertices", 0 );
	_shaderGrid->BindTexture( "u_texCellAccum", 1 );
	_shaderGrid->SetConstant( "u_minGrid", _boxMin.x, _boxMin.y, _boxMin.z );
	_shaderGrid->SetConstant( "u_invCellSize", 1.0f / _cellSize.x, 1.0f / _cellSize.y, 1.0f / _cellSize.z );
	_shaderGrid->SetConstant( "u_numCells", _numCells.x, _numCells.y, _numCells.z );
//_shaderGrid->SetConstant( "u_viewportW", _gridDataW );
//_shaderGrid->SetConstant( "u_invViewportW", 1.0f / (float)_gridDataW );
	_shaderGrid->SetConstant( "u_texCellAccumSize", _numTriangles + 1 );

	_shaderGrid->Unload();


	//////////////////////////////////////////////////////////////////////////
	// Shader 4
	//////////////////////////////////////////////////////////////////////////
	vpFilename = "../shaders/gridAccumVP_sem_GS.glsl";
	fpFilename = "../shaders/defaultFP.glsl";

	_shaderGridAccum = new cShader();
	_shaderGridAccum->Compile(cShader::VP, vpFilename.c_str());
	_shaderGridAccum->Compile(cShader::FP, fpFilename.c_str());
	_shaderGridAccum->Link(GL_POINTS);
	_shaderGridAccum->Load();

	_shaderGridAccum->BindTexture( "u_texGridAccum", 1 );
	_shaderGridAccum->SetConstant( "u_viewportW", _gridCountAccumW );
	_shaderGridAccum->SetConstant( "u_invViewportW", 1.0f / (float)_gridCountAccumW );

	_shaderGridAccum->Unload();
}

static void cleanup()
{
	glUseProgram( 0 );
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
	glPopMatrix();
}

void GpuGrid::rebuildGrid()
{
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Pass 1: count de quantas celulas cada triangulo faz overlap
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Setup view to get 1:1 pixel<->texel correspondence
	glViewport( 0, 0, _cellCountAccumW, _cellCountAccumH );

	//glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D( 0.0f, _cellCountAccumW, 0.0f, _cellCountAccumH );

	// Set FBO
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, _fboRenderTarget );
	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, _texCountAccum, 0 );

	_shaderCountCells->Load();

	glBegin( GL_QUADS );
		glVertex2f( 0, 0 );
		glVertex2f( _cellCountAccumW, 0 );
		glVertex2f( _cellCountAccumW, _cellCountAccumH );
		glVertex2f( 0, _cellCountAccumH );	
	glEnd();

#ifdef _DEBUG
	std::vector<float> countcells( _cellCountAccumW * _cellCountAccumH );
	glReadPixels( 0, 0, _cellCountAccumW, _cellCountAccumH, GL_LUMINANCE, GL_FLOAT, &countcells[0] );
#endif

//cleanup();
//return;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Pass 2: accum do passo 1 no CUDA
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Copy data from frame buffer texture to _cuda_input_pbo_id
	glBindBuffer( GL_PIXEL_PACK_BUFFER, _pboCudaInput );
	glReadPixels( 0, 0, _cellCountAccumW, _cellCountAccumH, GL_LUMINANCE, GL_FLOAT, (char*)NULL );
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );

glFinish();

	// Run CUDA to accumulate cell_count data
	cudaRunScan( _numTriangles + 1, &_maxCount, &_maxAccum );

#ifdef _DEBUG
	if( _maxAccum > 4000000 )
	{
		printf( "maxAccum: %6f, aborting...\n", _maxAccum );
		exit( 1 );
	}
#endif
	///printf( "maxCount: %6f\n", _maxCount );
	///printf( "maxAccum: %6f\n", _maxAccum );

	// Copy data from _cuda_output_pbo_id to grid_accum texture
	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, _texCountAccum );
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _pboCudaOutput );
	glTexSubImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, _cellCountAccumW, _cellCountAccumH, GL_LUMINANCE, GL_FLOAT, (char*)NULL );
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );

#ifdef _DEBUG
	std::vector<float> cellaccum( _numTriangles );

	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _pboCudaOutput );
	float* mapb1 = (float*)glMapBuffer( GL_PIXEL_UNPACK_BUFFER, GL_READ_ONLY );
	std::copy( mapb1, mapb1 + _numTriangles, &cellaccum[0] );
	glUnmapBuffer( GL_PIXEL_UNPACK_BUFFER );
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );
#endif

//cleanup();
//return;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Pass 3: construcao do grid unsorted
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	int w;
	int h;
	convertTo2dSize( _maxAccum, _gridDataW, w, h );

	// Setup view to get 1:1 pixel<->texel correspondence
	glViewport( 0, 0, w, h );
	glLoadIdentity();
	gluOrtho2D( 0.0f, w, 0.0f, h );

	// Set FBO
	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, _texGridData, 0 );

	_shaderGrid->Load();
	_shaderGrid->SetConstant( "u_texGridDataSize", _maxAccum );

	glBegin( GL_QUADS );
		glVertex2f( 0, 0 );
		glVertex2f( w, 0 );
		glVertex2f( w, h );
		glVertex2f( 0, h );
	glEnd();


#ifdef _DEBUG
	std::vector<float> gridunsorted( _gridDataW * _gridDataH * 2 );
	glReadPixels( 0, 0, _gridDataW, _gridDataH, GL_LUMINANCE_ALPHA, GL_FLOAT, &gridunsorted[0] );
#endif

//cleanup();
//return;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Pass 4: sort do grid de acordo com cellIDs
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Copy data from frame buffer texture to cuda_sort_pbo_id
	glBindBuffer( GL_PIXEL_PACK_BUFFER, _pboCudaInput );
	glReadPixels( 0, 0, w, h, GL_LUMINANCE_ALPHA, GL_FLOAT, (char*)NULL );
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );

	// Run CUDA to sort the grid
	cudaRunSort( _maxAccum );

	// Copy data from _cuda_sortgrid0_pbo_id to sortgrid texture
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, _texGridData );
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _pboCudaInput ); // sort result is in inputPbo
	glTexSubImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, w, h, GL_LUMINANCE_ALPHA, GL_FLOAT, (char*)NULL );
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );

#ifdef _DEBUG
	std::vector<float> gridsorted( w * h * 2 );

	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _pboCudaInput ); // sort result is in inputPbo
	float* mapb = (float*)glMapBuffer( GL_PIXEL_UNPACK_BUFFER, GL_READ_ONLY );
	std::copy( mapb, mapb + w * h * 2, &gridsorted[0] );
	glUnmapBuffer( GL_PIXEL_UNPACK_BUFFER );
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );
#endif

//cleanup();
//return;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Pass 5: count numero de triangulos por celula
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Setup view to get 1:1 pixel<->texel correspondence
	glViewport( 0, 0, _gridCountAccumW, _gridCountAccumH );
	glLoadIdentity();
	gluOrtho2D( 0.0f, _gridCountAccumW, 0.0f, _gridCountAccumH );

	// Set FBO
	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, _texGridAccum, 0 );

	_shaderBlendNumParticles->Load();
	_shaderBlendNumParticles->SetConstant( "u_texGridDataSize", _maxAccum );

	glBegin( GL_QUADS );
		glVertex2f( 0, 0 );
		glVertex2f( _gridCountAccumW, 0 );
		glVertex2f( _gridCountAccumW, _gridCountAccumH );
		glVertex2f( 0, _gridCountAccumH );	
	glEnd();

#ifdef _DEBUG
	std::vector<float> countpixels( _gridCountAccumW * _gridCountAccumH * 2 );
	glReadPixels( 0, 0, _gridCountAccumW, _gridCountAccumH, GL_LUMINANCE_ALPHA, GL_FLOAT, &countpixels[0] );
#endif

	// Cleanup state
	cleanup();
}
