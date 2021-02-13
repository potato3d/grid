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

#ifndef _COORD_H_
#define _COORD_H_

#include <math.h>

class cCoord
{
public:
 float x, y, z;

 cCoord(float X = 0, float Y = 0, float Z = 0) : x(X), y(Y), z(Z) {}
 cCoord(const cCoord &coord) { *this = coord; }

 void operator = (const cCoord &coord)
 { x = coord.x; y = coord.y; z = coord.z; }
 void operator = (const float c)
 { x = c; y = c; z = c; }
 void operator += (const cCoord &coord)
 { x += coord.x; y += coord.y; z += coord.z; }
 void operator -= (const cCoord &coord)
 { x -= coord.x; y -= coord.y; z -= coord.z; }
 void operator *= (const cCoord &coord)
 { x *= coord.x; y *= coord.y; z *= coord.z; }
 void operator /= (const cCoord &coord)
 { x /= coord.x; y /= coord.y; z /= coord.z; }
 float modulo(void)
 { return sqrt((x*x)+(y*y)+(z*z)); }
 void unit(void)
 { x /= modulo(); y /= modulo(); z /= modulo(); }

 friend cCoord operator + (const cCoord &p1, const cCoord &p2)
 { cCoord p; p.x = p1.x + p2.x; p.y = p1.y + p2.y; p.z = p1.z + p2.z; return p; }
 friend cCoord operator + (const cCoord &p1, float c)
 { cCoord p; p.x = p1.x + c; p.y = p1.y + c; p.z = p1.z + c; return p; }
 friend cCoord operator - (const cCoord &p1, const cCoord &p2)
 { cCoord p; p.x = p1.x - p2.x; p.y = p1.y - p2.y; p.z = p1.z - p2.z; return p; }
 friend cCoord operator - (const cCoord &p1, float c)
 { cCoord p; p.x = p1.x - c; p.y = p1.y - c; p.z = p1.z - c; return p; }
 friend cCoord operator * (const cCoord &p1, float c)
 { cCoord p; p.x = p1.x * c; p.y = p1.y * c; p.z = p1.z * c; return p; }
 friend cCoord operator / (const cCoord &p1, float c)
 { cCoord p; p.x = p1.x / c; p.y = p1.y / c; p.z = p1.z / c; return p; }

};

#endif
