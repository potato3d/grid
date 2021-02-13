// ------------------------------------------------------------------------
// Particle System -- Tecgraf/PUC-Rio
// www.tecgraf.puc-rio.br/~lduarte
//
// $Source:  $
// $Revision: 1.0 $
// $Date: 2007/10/10 19:21:00 $
//
// $Author: lduarte $
// ------------------------------------------------------------------------

#ifdef WIN32
#include <windows.h>
#endif

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <vr/timer.h>

#include "util.h"

static FILE* s_logFile = NULL;

FILE* getLogFile()
{
	if( s_logFile == NULL )
		s_logFile = fopen( "log.txt", "wt" );

	return s_logFile;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
float ScalarProd(const cCoord& u, const cCoord& v)
{
 return (u.x * v.x) + (u.y * v.y) + (u.z * v.z);
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
cCoord Project(const cCoord& u, const cCoord& v)
{
 float scale = (ScalarProd(u, v)) / (ScalarProd(u, v));
 return cCoord(v.x * scale, v.y * scale, v.z * scale);
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
cCoord CrossProd(const cCoord& u, const cCoord& v)
{
 cCoord w;

 w.x = u.y * v.z - u.z * v.y;
 w.y = u.z * v.x - u.x * v.z;
 w.z = u.x * v.y - u.y * v.x;

 return w;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
cCoord Min(cCoord a, cCoord b)
{
 cCoord res;

 a.x < b.x ? res.x = a.x : res.x = b.x;
 a.y < b.y ? res.y = a.y : res.y = b.y;

 return res;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
cCoord Max(cCoord a, cCoord b)
{
 cCoord res;

 a.x > b.x ? res.x = a.x : res.x = b.x;
 a.y > b.y ? res.y = a.y : res.y = b.y;

 return res;
}

/* Returns 1 if an OpenGL error occurred, 0 otherwise. */
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
int printOglError(char* message, char *file, int line)
{
 GLenum glErr;
 int retCode = 0;

 glErr = glGetError();
 while(glErr != GL_NO_ERROR)
 {
  printf("glError in file %s @ line %d (%s): %s\n", file, line, message, gluErrorString(glErr));
  retCode = 1;
  glErr = glGetError();
 }
 return retCode;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
bool is_power_of_two(int x)
{
 if((x == 2)    ||
    (x == 4)    ||
    (x == 8)    ||
    (x == 16)   ||
    (x == 32)   ||
    (x == 64 )  ||
    (x == 128)  ||
    (x == 256)  ||
    (x == 512)  ||
    (x == 1024) ||
    (x == 2048) ||
    (x == 4096) ||
    (x == 8192))
  return true;
 else
  return false;
}

void convertTo2dSize( int linearSize, int maxWidth, int& width, int& height )
{
	width  = vr::min( linearSize, maxWidth );
	height = ceil( (float)linearSize / (float)maxWidth );
}

void setDefaultTexture2dParameters()
{
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

void checkGpuGrid( GpuGrid& gpuGridBuilder, const std::vector<CpuGrid::Cell>& cpuGrid )
{
	GLuint pboId;
	glGenBuffers( 1, &pboId );
	glBindBuffer( GL_PIXEL_PACK_BUFFER, pboId );
	glBufferData( GL_PIXEL_PACK_BUFFER, 8192 * 1024 * 2, NULL, GL_STATIC_COPY );

	GLuint fboId;
	glGenFramebuffersEXT( 1, &fboId );
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, fboId );
	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, gpuGridBuilder._texGridData, 0 );

	// Copy data from grid texture to pbo
	int w;
	int h;
	convertTo2dSize( gpuGridBuilder._maxAccum, gpuGridBuilder._gridDataW, w, h );

	glReadPixels( 0, 0, w, h, GL_LUMINANCE_ALPHA, GL_FLOAT, (char*)NULL );

	// Read data to CPU
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, pboId );
	float* buffer = (float*)glMapBuffer( GL_PIXEL_UNPACK_BUFFER, GL_READ_ONLY );

	int size = w * h * 2;
	std::vector<float> gpugrid( size, -1.0f );
	std::copy( buffer, buffer + size, gpugrid.begin() );

	glUnmapBuffer( GL_PIXEL_UNPACK_BUFFER );

	// Copy data from grid accum texture to pbo
	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, gpuGridBuilder._texCountAccum, 0 );

	glBindBuffer( GL_PIXEL_PACK_BUFFER, pboId );
	glReadPixels( 0, 0, gpuGridBuilder._gridCountAccumW, gpuGridBuilder._gridCountAccumH, GL_LUMINANCE, GL_FLOAT, (char*)NULL );

	// Read data to CPU
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, pboId );
	buffer = (float*)glMapBuffer( GL_PIXEL_UNPACK_BUFFER, GL_READ_ONLY );

	size = gpuGridBuilder._gridCountAccumW * gpuGridBuilder._gridCountAccumH;
	std::vector<float> gpuGridIdx( size, -1.0f );
	std::copy( buffer, buffer + size, gpuGridIdx.begin() );

	glUnmapBuffer( GL_PIXEL_UNPACK_BUFFER );
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );
	glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );

	//////////////////////////////////////////////////////////////////////////

	unsigned int count = 0;
	unsigned int gpugridCount = gpuGridBuilder._maxAccum*2;
	int gpugridIdxCount = gpuGridBuilder._numCells.x * gpuGridBuilder._numCells.y * gpuGridBuilder._numCells.z;
	vr::Timer checkTime;

	//////////////////////////////////////////////////////////////////////////

	printf( "Checking %d cell indices in gpu\n", gpugridIdxCount );
	checkTime.restart();

#ifdef NDEBUG
	#pragma omp parallel for shared( count, gpugridCount, gpugridIdxCount, gpugrid, cpuGrid, gpuGridIdx ) schedule( dynamic )
#endif
	for( int i = 0; i < gpugridIdxCount; ++i )
	{
		if( i % 100 == 0 )
		{
			printf( " checked %7d cells, errors %2d", i, count );
			printf( "\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r" );
		}

		int idx = gpuGridIdx[i];
		int size = gpuGridIdx[i+1] - idx;

		// Check size with cpu grid
		if( size != cpuGrid[i].size() )
		{
			getc( stdin );
			count++;
		}

		// gpu grid is in pairs [cellId, particleId]
		idx *= 2;

		bool foundInGpu = false;

		if( size == 0 )
		{
			// search cell in gpu, we don't want to find it
			for( unsigned int j = 0; j < gpugridCount; j+=2)
			{
				// found it
				if( i == gpugrid[j] )
				{
					foundInGpu = true;
					break;
				}
			}

			// shouldn't find cell in gpu because cell in cpu is empty
			if( foundInGpu )
			{
				getc( stdin );
				count++;
			}
		}
		else
		{
			// check valid idx
			if( i == gpugrid[idx] )
			{
				unsigned int j = idx;

				// check if gpu contains size elements
				for( int k = 0; k < size; ++k )
				{
					// cell ids are different
					if( i != gpugrid[j] )
					{
						getc( stdin );
						count++;
					}

					// check gpugrid bounds, shouldn't end now
					if( j >= gpugridCount )
					{
						getc( stdin );
						count++;
						break;
					}

					// goto next cell id in gpugrid
					j += 2;
				}

				// if i == gpugrid[j], there are still cells with same id in gpu (cell in gpu is larger than in cpu)
				if( j < gpugridCount && i == gpugrid[j] )
				{
					getc( stdin );
					count++;
				}

				foundInGpu = true;
			}

			if( !foundInGpu )
			{
				getc( stdin );
				count++;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////

	printf( "\nTotal error count: %d\n", count );
	printf( "Time: %.2f sec\n", checkTime.elapsed() );

	printf( "\n********************\n\n" );

	count = 0;

	printf( "Checking %7d cells in cpu, %7d elements in gpu\n", cpuGrid.size(), gpugridCount );
	checkTime.restart();

	// TODO: 
#ifdef NDEBUG
	#pragma omp parallel for shared( count, gpugridCount, gpugrid, cpuGrid ) schedule( dynamic )
#endif
	for( int i = 0; i < (int)cpuGrid.size(); ++i )
	{
		if( i % 100 == 0 )
		{
			printf( " checked %7d cells, errors %2d", i, count );
			printf( "\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r" );
		}

		const CpuGrid::Cell& cell = cpuGrid[i];
		bool foundInGpu = false;

		if( cell.empty() )
		{
			// search cell in gpu, we don't want to find it
			for( unsigned int j = 0; j < gpugridCount; j+=2)
			{
				// found it
				if( i == gpugrid[j] )
				{
					foundInGpu = true;
					break;
				}
			}

			// shouldn't find cell in gpu because cell in cpu is empty
			if( foundInGpu )
			{
				getc( stdin );
				count++;
			}
		}
		else
		{
			// search cell in gpu, we want to find it
			for( unsigned int j = 0; j < gpugridCount; j+=2)
			{
				// found it
				if( i == gpugrid[j] )
				{
					// check particle ids
					for( unsigned int k = 0; k < (int)cell.size(); ++k )
					{
						// particle ids are different
						if( cell[k] != gpugrid[j+1] )
						{
							getc( stdin );
							count++;
						}

						// check gpugrid bounds, shouldn't end now
						if( j >= gpugridCount )
						{
							getc( stdin );
							count++;
							break;
						}

						// goto next particle id in gpugrid
						j += 2;
					}

					// if i == gpugrid[j], there are still particles in the same cell in gpu (cell in gpu is larger than in cpu)
					if( j < gpugridCount && i == gpugrid[j] )
					{
						getc( stdin );
						count++;
					}

					foundInGpu = true;
					break;
				}
			}

			if( !foundInGpu )
			{
				getc( stdin );
				count++;
			}
		}
	}

	printf( "\nTotal error count: %d\n", count );
	printf( "Time: %.2f sec\n", checkTime.elapsed() );
}
