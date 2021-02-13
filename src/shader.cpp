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

#ifdef WIN32
#include <io.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>

#include "shader.h"
#include "util.h"

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
cShader::cShader()
{
	geometryShader = glCreateShader(GL_GEOMETRY_SHADER_EXT);
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
 fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

 geometryCompiled = 0;
 vertexCompiled = 0;
 fragmentCompiled = 0;

 geometrySrc = NULL;
 vertexSrc = NULL;
 fragmentSrc = NULL;

 program = 0;
 programLinked = 0;
 programValidate = 0;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
bool cShader::Compile(int type, const char* filename)
{
 int fd, fileSize;
 GLint res;
 FILE* fp = NULL;

// Open the file, seek to the end to find its length
#ifdef WIN32
 fd = _open(filename, _O_RDONLY);
 if(fd != -1)
 {
  fileSize = _lseek(fd, 0, SEEK_END) + 1;
  _close(fd);
 }
#else
  fd = open(name, O_RDONLY);
  if(fd != -1)
  {
   fileSize = lseek(fd, 0, SEEK_END) + 1;
   close(fd);
  }
#endif

 //open shader src
 fp = fopen(filename, "r");
 if(!fp)
  return false;

 if(type == VP)
 {
  //vertex src memory allocation
  vertexSrc = (GLchar*)malloc(fileSize);

  //vertex src string
  fseek(fp, 0, SEEK_SET);
  fileSize = (int)fread(vertexSrc, 1, fileSize, fp);
  vertexSrc[fileSize] = '\0';

  //load vertex src into shader
  glShaderSource(vertexShader, 1, (const GLchar**)&vertexSrc, NULL);

  //compile vertex shader
  glCompileShader(vertexShader);

  //check OpenGL error
  printOpenGLError();

  //get compile status
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vertexCompiled);

  //print log of vertex object
  printShaderInfoLog(vertexShader);

  res = vertexCompiled;
 }
 else if( type == FP )
 {
  //fragment src memory allocation 
  fragmentSrc = (GLchar*)malloc(fileSize);

  //fragment src string
  fseek(fp, 0, SEEK_SET);
  fileSize = (int)fread(fragmentSrc, 1, fileSize, fp);
  fragmentSrc[fileSize] = '\0';

  //load fragment shader
  glShaderSource(fragmentShader, 1, (const GLchar**)&fragmentSrc, NULL);

  //compile fragment shader
  glCompileShader(fragmentShader);

  //check OpenGL error
  printOpenGLError();

  //get compile status
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fragmentCompiled);

  //print log of fragment object
  printShaderInfoLog(fragmentShader);

  res = fragmentCompiled;
 }
 else if( type == GP )
 {
	 //geometry src memory allocation 
	 geometrySrc = (GLchar*)malloc(fileSize);

	 //geometry src string
	 fseek(fp, 0, SEEK_SET);
	 fileSize = (int)fread(geometrySrc, 1, fileSize, fp);
	 geometrySrc[fileSize] = '\0';

	 //load geometry shader
	 glShaderSource(geometryShader, 1, (const GLchar**)&geometrySrc, NULL);

	 //compile geometry shader
	 glCompileShader(geometryShader);

	 //check OpenGL error
	 printOpenGLErrorM( "alo" );

	 //get compile status
	 glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &geometryCompiled);

	 //print log of geometry object
	 printShaderInfoLog(geometryShader);

	 res = geometryCompiled;
 }
 else
 {
	 printf( "Invalid shader type!\n" );
 }

 fclose(fp);

 if(!res)
  return false;
 else
  return true;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
bool cShader::Link( GLuint gs_output_type, GLuint gs_num_output_vertices )
{
 //create a program object
 program = glCreateProgram();

 //attach vertex shader
 if(vertexCompiled)
  glAttachShader(program, vertexShader);

 //attach fragment shader
 if(fragmentCompiled)
  glAttachShader(program, fragmentShader);

 //attach geometry shader
 if(geometryCompiled)
 {
	 glAttachShader(program, geometryShader);

	 GLint outputVertexLimit;
	 glGetIntegerv( GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT, &outputVertexLimit );
	 printf( "Max GS output vertices: %d\n", outputVertexLimit );

	 GLint outputComponentLimit;  
	 glGetIntegerv( GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_EXT, &outputComponentLimit ); 
	 printf( "Max GS output components: %d\n", outputComponentLimit );

	 if( (int)gs_num_output_vertices > outputComponentLimit * 3 )
		 printf( "Warning: GS output vertices is greater than MAX output components x3!\n" );

	 // one of: GL_POINTS, GL_LINES, GL_LINES_ADJACENCY_EXT, GL_TRIANGLES, GL_TRIANGLES_ADJACENCY_EXT
	 //Set GL_TRIANGLES primitives as INPUT
	 glProgramParameteriEXT( program, GL_GEOMETRY_INPUT_TYPE_EXT, GL_POINTS );

	 // one of: GL_POINTS, GL_LINE_STRIP, GL_TRIANGLE_STRIP
	 //Set TRIANGLE STRIP as OUTPUT
	 glProgramParameteriEXT( program, GL_GEOMETRY_OUTPUT_TYPE_EXT, gs_output_type );

	 // This parameter is very important and have an important impact on Shader performances
	 // Its value must be chosen closer as possible to real maximum number of vertices
	 glProgramParameteriEXT( program, GL_GEOMETRY_VERTICES_OUT_EXT, gs_num_output_vertices );
	 printf( "GS output set to: %d\n", gs_num_output_vertices );
 }

 //link the program object
 glLinkProgram(program);

 //check OpenGL error
 printOpenGLError();

 //get link status
 glGetProgramiv(program, GL_LINK_STATUS, &programLinked);

 //print log of program object
 printProgramInfoLog(program);

 //get link validate
	glValidateProgram(program);
	glGetProgramiv(program, GL_VALIDATE_STATUS, &programValidate);


 if(!programLinked)
  return false;
 else
  return true;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
bool cShader::Load(void)
{
 //if(!program)
 // return false;

 glUseProgram(program);
 return true;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
void cShader::Unload(void)
{
 glUseProgram(0);
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
bool cShader::BindTexture(const char *name, int id)
{
 GLint loc = glGetUniformLocation(program, name);

 if(loc == -1)
 {
  printf("No such uniform named \"%s\"\n", name);
  return false;
 }

 ////check OpenGL error
 //printOpenGLError();

 glUniform1i(loc, id);

 return true;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
bool cShader::SetConstant(const char *name, int x, int y)
{
 GLint loc = glGetUniformLocation(program, name);

 if(loc == -1)
 {
  printf("No such uniform named \"%s\"\n", name);
  return false;
 }

 ////check OpenGL error
 //printOpenGLError();

 glUniform2i(loc, x, y);

 return true;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
bool cShader::SetConstant(const char *name, float x)
{
 GLint loc = glGetUniformLocation(program, name);

 if(loc == -1)
 {
  printf("No such uniform named \"%s\"\n", name);
  return false;
 }

 ////check OpenGL error
 //printOpenGLError();

 glUniform1f(loc, x);

 return true;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
bool cShader::SetConstant(const char *name, float x, float y)
{
 GLint loc = glGetUniformLocation(program, name);

 if(loc == -1)
 {
  printf("No such uniform named \"%s\"\n", name);
  return false;
 }

 ////check OpenGL error
 //printOpenGLError();

 glUniform2f(loc, x, y);

 return true;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
bool cShader::SetConstant(const char *name, float x, float y, float z)
{
 GLint loc = glGetUniformLocation(program, name);

 if(loc == -1)
 {
  printf("No such uniform named \"%s\"\n", name);
  return false;
 }

 ////check OpenGL error
 //printOpenGLError();

 glUniform3f(loc, x, y, z);

 return true;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
bool cShader::SetConstant(const char *name, float x, float y, float z, float w)
{
 GLint loc = glGetUniformLocation(program, name);

 if(loc == -1)
 {
  printf("No such uniform named \"%s\"\n", name);
  return false;
 }

 ////check OpenGL error
 //printOpenGLError();

 glUniform4f(loc, x, y, z, w);

 return true;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
bool cShader::SetConstant(const char* name, int rows, int columns, float* m)
{
 //if (rows < 2 || rows > 4 || rows != columns)
 //{
 // printf("UTL: GLSL does not accept matrices other than 2x2, 3x3 or 4x4. Tried to set a %d rows x %d columns matrix.\n",rows,columns);
 // return false;
 //}

 GLint loc = glGetUniformLocation(program, name);
 if(loc == -1)
 {
  printf("No such uniform named \"%s\"\n", name);
  return false;
 }

 if(rows == 2) 
  glUniformMatrix2fv(loc,1,GL_FALSE,m);
 else if(rows == 3)
  glUniformMatrix3fv(loc,1,GL_FALSE,m);
 else if(rows == 4)
  glUniformMatrix4fv(loc,1,GL_FALSE,m);

 return true;
}

/* Print out the information log for a shader object */
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
void cShader::printShaderInfoLog(GLuint shader)
{
 int infologLength = 0;
 int charsWritten  = 0;
 GLchar *infoLog;

 //check OpenGL error
 printOpenGLError();

 glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength);

 //check OpenGL error
 printOpenGLError();

 if(infologLength > 0)
 {
  infoLog = (GLchar*)malloc(infologLength);
  if(infoLog == NULL)
  {
   printf("ERROR: Could not allocate InfoLog buffer\n");
   exit(1);
  }
  glGetShaderInfoLog(shader, infologLength, &charsWritten, infoLog);
  printf("Shader InfoLog:\n%s\n\n", infoLog);
  free(infoLog);
 }

 //check OpenGL error
 printOpenGLError();
}

/* Print out the information log for a program object */
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
void cShader::printProgramInfoLog(GLuint program)
{
 int infologLength = 0;
 int charsWritten  = 0;
 GLchar *infoLog;

 //check OpenGL error
 printOpenGLError();

 glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLength);

 //check OpenGL error
 printOpenGLError();

 if(infologLength > 0)
 {
  infoLog = (GLchar*)malloc(infologLength);
  if(infoLog == NULL)
  {
   printf("ERROR: Could not allocate InfoLog buffer\n");
   exit(1);
  }
  glGetProgramInfoLog(program, infologLength, &charsWritten, infoLog);
  printf("Program InfoLog:\n%s\n\n", infoLog);
  free(infoLog);
 }

 //check OpenGL error
 printOpenGLError();
}
