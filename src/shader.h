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

#ifndef _SHADER_H_
#define _SHADER_H_

#ifdef WIN32
#include <windows.h>
#endif

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

class cShader
{
private:
 GLuint program;
 GLint  programLinked;
 GLint  programValidate;

 GLuint geometryShader;
 GLuint vertexShader;
 GLuint fragmentShader;

 GLint geometryCompiled;
 GLint vertexCompiled;
 GLint fragmentCompiled;

 GLchar* geometrySrc;
 GLchar* vertexSrc;
 GLchar* fragmentSrc;

 //info functions
 void printShaderInfoLog(GLuint shader);
 void printProgramInfoLog(GLuint program);

public:
 cShader();
 ~cShader() {}

 bool Link( GLuint gs_output_type, GLuint gs_num_output_vertices = 381 );
 bool Compile(int type, const char* filename);

 bool Load(void);
 void Unload(void);
 
 bool BindTexture(const char* name, int id);

 bool SetConstant(const char* name, int x, int y);

 bool SetConstant(const char* name, float x);
 bool SetConstant(const char* name, float x, float y);
 bool SetConstant(const char* name, float x, float y, float z);
 bool SetConstant(const char* name, float x, float y, float z, float w);
 bool SetConstant(const char* name, int rows, int columns, float* m);

 enum { VP = 0, FP, GP };
};

#endif
