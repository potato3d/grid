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

#ifndef _UTIL_H_
#define _UTIL_H_

#include <string>
#include <stdarg.h>
#include <time.h>

#include "coord.h"
#include "collision.h"
#include "cpugrid.h"
#include "gpugrid.h"

#define MPI        3.14159265358979323846   /* Pi Number */
#define TOL_ZERO   1.0e-08
#define GRAVITY    9.806

#ifndef RAND
#define RAND(X,Y) ((X)+((Y)-(X))*(rand()/(float)RAND_MAX))
#endif

#define RADIUS 5

#define TRACK_ALLOC 0

using std::string;

FILE* getLogFile();

float ScalarProd(const cCoord& u, const cCoord& v);
cCoord CrossProd(const cCoord& u, const cCoord& v);
cCoord Project(const cCoord& u, const cCoord& v);
bool FileDialog(string type, string title, string filter, string& name);
string DropFilename(string);

cCoord Min(cCoord a, cCoord b);
cCoord Max(cCoord a, cCoord b);

inline float Min(float a, float b) {return a < b ? a : b;}
inline float Max(float a, float b) {return a > b ? a : b;}
inline int Min(int a, int b) {return a < b ? a : b;}
inline int Max(int a, int b) {return a > b ? a : b;}

int printOglError(char* message, char *file, int line);

#define printOpenGLErrorM(message) printOglError(message, __FILE__, __LINE__)
#define printOpenGLError() printOglError("", __FILE__, __LINE__)

bool is_power_of_two(int x);

void convertTo2dSize( int linearSize, int maxWidth, int& width, int& height );

void setDefaultTexture2dParameters();

void checkGpuGrid( GpuGrid& gpuGridBuilder, const std::vector<CpuGrid::Cell>& cpuGrid );


#endif
