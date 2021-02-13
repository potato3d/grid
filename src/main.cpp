/**
*	Exemplo grid geometry shader
*/

#include <vr/random.h>
#include <vr/timer.h>
#include <vr/string.h>
#include <vr/mat4.h>
#include <vr/vec2.h>
#include <vr/stl_utils.h>

#include <gl/glew.h>
#include <gl/glut.h>
#include <gl/wglew.h>

#include <iostream>
#include <fstream>

#include "collision.h"
#include "CpuGrid.h"
#include "GpuGrid.h"
#include "util.h"
//#include "GpuObjLoader.h"

// TODO: 
#include <omp.h>

/************************************************************************/
/* Global variables                                                     */
/************************************************************************/

// Global objects
static unsigned int s_numTriangles = 500e3;//300000;//300000;//250000;
static std::vector<float> s_triangleVertices;
static bool s_fixedGrid = false;
static float s_max_rd = 1.0;//0.2;//1.0;
#ifdef _DEBUG
static int s_numIterations = 1;
#else
static int s_numIterations = 1;//100;
#endif

static cCollision* s_collision = NULL;
static CpuGrid s_cpuGrid;
static GpuGrid s_gpuGrid;

// Camera manipulation
static double s_walkSpeed = 0.5;
static double s_lookSpeed = 0.005;

static vr::mat4f s_viewMatrix;
static vr::mat4f s_projMatrix;

static vr::vec2f s_prevMousePos = vr::vec2f( 0.0, 0.0 );
static bool s_mouseDragActive = false;

// Measuring fps
static int s_frameCounter = 0;
static vr::Timer s_fpsTimer;
static vr::String s_fpsMsg;

// Miscellaneous
static bool s_wireframeActive = false;
static int s_screenWidth  = 640;
static int s_screenHeight = 480;

static GLuint s_texTriangleVertices = 0;

/************************************************************************/
/* Utility functions                                                    */
/************************************************************************/

static void resetCamera()
{
	s_viewMatrix.makeLookAt( vr::vec3f( 0.0, 0.0, 20.0 ), vr::vec3f( 0.0, 0.0, 0.0 ), vr::vec3f( 0.0, 1.0, 0.0 ) );
	glMatrixMode( GL_MODELVIEW );
	glLoadMatrixf( s_viewMatrix.ptr() );
}

static void moveCamera( const vr::mat4f& transform )
{
	s_viewMatrix.product( s_viewMatrix, transform );
	glMatrixMode( GL_MODELVIEW );
	glLoadMatrixf( s_viewMatrix.ptr() );
}

static void displayTextLine( const char* s, float x, float y )
{
	glRasterPos3d( x, y, 0.8 );

	for( unsigned int i = 0; s[i]; ++i )
		glutBitmapCharacter( GLUT_BITMAP_HELVETICA_12, s[i] );
}

static void displayScreenMessage()
{
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();
	//glDisable( GL_LIGHTING );
	glDisable( GL_DEPTH_TEST );

	glColor3f( 1.0f, 1.0f, 1.0f );

	// Show FPS
	displayTextLine( s_fpsMsg.toCharArray(), -0.95f, 0.90f );

	glEnable( GL_DEPTH_TEST );
	//glEnable( GL_LIGHTING );
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();
}

/************************************************************************/
/* Main functions                                                       */
/************************************************************************/

//static ObjAnimation s_scene;

static void createScene()
{
	//vr::StringList sl;
	//sl.push_back( "Toasters004.obj" );

	//s_scene.loadAnimation( sl );

	//// Create GpuGrid object
	//s_gpuGrid.init( s_scene.frameObjs[0]->vertices.size() / 3, s_scene.bbox.minv, s_scene.bbox.maxv, s_fixedGrid );

	//// Setup CpuGrid
	//s_cpuGrid.setBoxMin( s_scene.bbox.minv.x, s_scene.bbox.minv.y, s_scene.bbox.minv.z );
	//s_cpuGrid.setBoxMax( s_scene.bbox.maxv.x, s_scene.bbox.maxv.y, s_scene.bbox.maxv.z );
	//s_cpuGrid.setNumTriangles( s_numTriangles, s_fixedGrid );

	//// Create texture
	//glEnable( GL_TEXTURE_2D );
	//glEnable( GL_TEXTURE_RECTANGLE_ARB );

	//int w;
	//int h;
	//convertTo2dSize( s_scene.frameObjs[0]->vertices.size(), 8192, w, h );

	//vr::vectorExactResize( s_scene.frameObjs[0]->vertices, w * h, vr::vec3f( 0.0f, 0.0f , 0.0f ) );

	//glGenTextures( 1, &s_texTriangleVertices );
	//glBindTexture( GL_TEXTURE_RECTANGLE_ARB, s_texTriangleVertices );
	//setDefaultTexture2dParameters();
	//glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB32F_ARB, w, h, 0, GL_RGB, GL_FLOAT, &s_scene.frameObjs[0]->vertices[0] );
	//glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );




	//s_numTriangles = 78e3;

	// Create particles
	vr::vectorExactResize( s_triangleVertices, s_numTriangles * 3 * 3 );

	vr::vec3f boxMin( -50, -50, -50 );
	vr::vec3f boxMax( 50, 50, 50 );

	for( unsigned int i = 0; i < s_triangleVertices.size(); i+=9 )
	{
		// v0
		s_triangleVertices[i+0] = vr::Random::real( -50.0 + s_max_rd, 50.0 - s_max_rd );
		s_triangleVertices[i+1] = vr::Random::real( -50.0 + s_max_rd, 50.0 - s_max_rd );
		s_triangleVertices[i+2] = vr::Random::real( -50.0 + s_max_rd, 50.0 - s_max_rd );

		vr::vec3f dir;
		dir.x = vr::Random::real(-1,1);
		dir.y = vr::Random::real(-1,1);
		dir.z = vr::Random::real(-1,1);
		dir.normalize();

		// v1: offset diameter from v0
		s_triangleVertices[i+3] = s_triangleVertices[i+0] + dir.x * vr::Random::real( 0.2, s_max_rd ) * 2.0f;
		s_triangleVertices[i+4] = s_triangleVertices[i+1] + dir.y * vr::Random::real( 0.2, s_max_rd ) * 2.0f;
		s_triangleVertices[i+5] = s_triangleVertices[i+2] + dir.z * vr::Random::real( 0.2, s_max_rd ) * 2.0f;

		vr::vec3f mid;
		mid.x = ( s_triangleVertices[i+0] + s_triangleVertices[i+3] ) * 0.5f;
		mid.y = ( s_triangleVertices[i+1] + s_triangleVertices[i+4] ) * 0.5f;
		mid.z = ( s_triangleVertices[i+2] + s_triangleVertices[i+5] ) * 0.5f;

		dir.x = vr::Random::real(-1,1);
		dir.y = vr::Random::real(-1,1);
		dir.z = vr::Random::real(-1,1);
		dir.normalize();

		// v2: offset radius from midpoint
		s_triangleVertices[i+6] = mid.x + dir.x * vr::Random::real( 0.2, s_max_rd );
		s_triangleVertices[i+7] = mid.y + dir.y * vr::Random::real( 0.2, s_max_rd );
		s_triangleVertices[i+8] = mid.z + dir.z * vr::Random::real( 0.2, s_max_rd );

		//////////////////////////////////////////////////////////////////////////
		// old method (wrong, but used as comparison)
		//////////////////////////////////////////////////////////////////////////
		//// v1: offset from v0
		//s_triangleVertices[i+3] = s_triangleVertices[i+0] + vr::Random::real( 0.2, s_max_rd );
		//s_triangleVertices[i+4] = s_triangleVertices[i+1] + vr::Random::real( 0.2, s_max_rd );
		//s_triangleVertices[i+5] = s_triangleVertices[i+2] + vr::Random::real( 0.2, s_max_rd );

		//// v2: offset from v0
		//s_triangleVertices[i+6] = s_triangleVertices[i+0] + vr::Random::real( 0.2, s_max_rd );
		//s_triangleVertices[i+7] = s_triangleVertices[i+1] + vr::Random::real( 0.2, s_max_rd );
		//s_triangleVertices[i+8] = s_triangleVertices[i+2] + vr::Random::real( 0.2, s_max_rd );
	}

	// Create GpuGrid object
	s_gpuGrid.init( s_numTriangles, boxMin, boxMax, s_fixedGrid );

	// Setup CpuGrid
	s_cpuGrid.setBoxMin( boxMin.x, boxMin.y, boxMin.z );
	s_cpuGrid.setBoxMax( boxMax.x, boxMax.y, boxMax.z );
	s_cpuGrid.setNumTriangles( s_numTriangles, s_fixedGrid );


	// Create texture
	glEnable( GL_TEXTURE_RECTANGLE_ARB );

	int w;
	int h;
	convertTo2dSize( s_numTriangles * 3, 8192, w, h );

	vr::vectorExactResize( s_triangleVertices, w * h * 3, 0.0f );

	glGenTextures( 1, &s_texTriangleVertices );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, s_texTriangleVertices );
	setDefaultTexture2dParameters();
	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB32F_ARB, w, h, 0, GL_RGB, GL_FLOAT, &s_triangleVertices[0] );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );
}

// Update frames per second
static void updateFps()
{
	double elapsedTime = s_fpsTimer.elapsed();
	++s_frameCounter;

	if( elapsedTime >= 1.0 )
	{
		double fps = s_frameCounter / elapsedTime;
		s_fpsMsg.format( "%.2f fps    %.2f ms", fps, ( elapsedTime * 1000.0 ) / s_frameCounter );

		s_frameCounter = 0;
		s_fpsTimer.restart();
	}
}

// Draw scene
static void drawScene()
{
	if( s_texTriangleVertices == 0 )
		return;

	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, s_texTriangleVertices );

	for( int i = 0; i < s_numIterations; ++i )
	{
		s_gpuGrid.rebuildGrid();
		//s_cpuGrid.rebuild( &s_triangleVertices[0], s_numTriangles );
	}

	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();

	//exit( 1 );

//#ifdef _DEBUG

	//s_cpuGrid.rebuild( &s_triangleVertices[0], s_numTriangles );
	//checkGpuGrid( s_gpuGrid, s_cpuGrid.getGridData() );
	

	//////////////////////////////////////////////////////////////////////////

	//std::ofstream outcpu( "outcpu.txt" );
	//for( unsigned int i = 0; i < cpuGrid.size(); ++i )
	//{
	//	if( cpuGrid[i].empty() )
	//		continue;

	//	const CpuGrid::Cell& cell = cpuGrid[i];

	//	for( unsigned int k = 0; k < cell.size(); ++k )
	//		outcpu << cell[k] << std::endl;
	//}
	
	//getc(stdin);
	//exit( 0 );
//#endif
}

// Display callback
static void display()
{
	// Clear buffers
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Set view matrix
	glMatrixMode( GL_MODELVIEW );
	glLoadMatrixf( s_viewMatrix.ptr() );

	// Draw scene
	drawScene();

	// na mao
	glViewport( 0, 0, s_screenWidth, s_screenHeight );

	// Update on-screen user information
	updateFps();
	displayScreenMessage();

	glutSwapBuffers();
}

// Idle callback for performance measurements
static void idle()
{
	display();
}

// Reshape callback
static void reshape( int w, int h )
{
	static bool first = true;
	if( first )
	{
		glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
		glEnable( GL_DEPTH_TEST );

		glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
		glEnable( GL_COLOR_MATERIAL );

		glEnable( GL_NORMALIZE );

		// Setup headlight
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();

		//glEnable( GL_LIGHTING );
		//glEnable( GL_LIGHT0 );
		//float lightPos[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		//glLightfv( GL_LIGHT0, GL_POSITION, lightPos );

		// Reset camera
		resetCamera();

		// Create scene with valid OpenGL context
		createScene();

		// Disable V-Sync for benchmarking
		// Attention! Only works under Windows
		wglSwapIntervalEXT( 0 );

		// Reset global timer
		s_fpsTimer.restart();

		first = false;
	}

	glMatrixMode( GL_PROJECTION );
	s_projMatrix.makePerspective( 65.0, (double)w/(double)h, 0.1, 1000.0 );
	glLoadMatrixf( s_projMatrix.ptr() );

	glViewport( 0, 0, w, h );
	s_screenWidth  = w;
	s_screenHeight = h;
}

// Keyboard callback
static void keyboard( unsigned char key, int x, int y )
{
	switch( key )
	{
	// Camera movement
	case 'w':
		{
			vr::mat4f aux;
			aux.makeTranslation( 0.0, 0.0, s_walkSpeed );
			moveCamera( aux );
			break;
		}
	case 'a':
		{
			vr::mat4f aux;
			aux.makeTranslation( s_walkSpeed, 0.0, 0.0 );
			moveCamera( aux );
			break;
		}
	case 's':
		{
			vr::mat4f aux;
			aux.makeTranslation( 0.0, 0.0, -s_walkSpeed );
			moveCamera( aux );
			break;
		}
	case 'd':
		{
			vr::mat4f aux;
			aux.makeTranslation( -s_walkSpeed, 0.0, 0.0 );
			moveCamera( aux );
			break;
		}
	case 'r':
		{
			vr::mat4f aux;
			aux.makeTranslation( 0.0, -s_walkSpeed, 0.0 );
			moveCamera( aux );
			break;
		}
	case 'f':
		{
			vr::mat4f aux;
			aux.makeTranslation( 0.0, s_walkSpeed, 0.0 );
			moveCamera( aux );
			break;
		}
	case 'q':
		{
			vr::mat4f aux;
			aux.makeRotation( -10.0*s_lookSpeed, 0.0, 0.0, 1.0 );
			moveCamera( aux );
			break;
		}
	case 'e':
		{
			vr::mat4f aux;
			aux.makeRotation( 10.0*s_lookSpeed, 0.0, 0.0, 1.0 );
			moveCamera( aux );
			break;
		}

	// Space key: reset viewer
	case 32:
		resetCamera();
		break;

	// Esc key: exit application
	case 27:
		exit( 0 );
		break;

	// Wireframe
	case 'z':
		s_wireframeActive ^= true;
		if( s_wireframeActive )
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		else
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		break;

	default:
	    return;
	}

	glutPostRedisplay();
}

// Mouse callback
void mouse( int button, int state, int x, int y )
{
	if( button == GLUT_LEFT_BUTTON )
	{
		if( state == GLUT_DOWN )
		{
			s_prevMousePos.set( x, y );
			s_mouseDragActive = true;
		}
		else
		{
			s_mouseDragActive = false;
		}
	}
}

// Mouse drag callback
void drag( int x, int y )
{
	if( !s_mouseDragActive )
		return;

	vr::vec2f currentMousePos( x, y );
	vr::vec2f delta = currentMousePos - s_prevMousePos;
	s_prevMousePos = currentMousePos;

	vr::mat4f rotateX;
	rotateX.makeRotation( delta.x * s_lookSpeed, 0.0, 1.0, 0.0 );

	vr::mat4f rotateY;
	rotateY.makeRotation( delta.y * s_lookSpeed, 1.0, 0.0, 0.0 );

	rotateX.product( rotateX, rotateY );

	moveCamera( rotateX );

	glutPostRedisplay();
}

//////////////////////////////////////////////////////////////////////////
// Main entry point
//////////////////////////////////////////////////////////////////////////
int main( int argc, char* argv[] )
{
	// Open GLUT
	glutInit( &argc, argv );
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB );
	glutInitWindowSize( s_screenWidth, s_screenHeight );

#ifdef _DEBUG
	int windowId = glutCreateWindow( "Grid - Debug Mode" );
#else
	int windowId = glutCreateWindow( "Grid" );
#endif

	glutReshapeFunc( reshape );
	glutDisplayFunc( display );
	glutIdleFunc( idle );
	glutKeyboardFunc( keyboard );
	glutMouseFunc( mouse );
	glutMotionFunc( drag );

	// Interact
	glutMainLoop();
	return 0;
}
