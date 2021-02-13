#version 120
#extension GL_EXT_geometry_shader4 : enable

uniform vec3 minGrid;
uniform vec3 cellSize;
uniform vec2 texGridSize; // ( w, 1/w )
uniform vec2 _3Dto1Dconst; // ( ncellx, ncellx*ncelly )

varying in float v_currentFirstIdxL[];
//varying in float v_nextFirstIdxL[];
varying in vec3 v_startCell[];
varying in vec3 v_endCell[];
varying in vec4 v_particleData[];

float myMod( float x, float y, float invY )
{
	return x - y * floor( x * invY );
}

vec2 addrTranslation1Dto2D( float address1D, vec2 mult )
{
	return vec2( myMod( address1D, mult.x, mult.y ), floor( address1D * mult.y ) + 0.5 );
}

vec3 addrTranslation1Dto3D( float address1D, vec4 mult )
{
	vec3 cellCoords;

	cellCoords.z = floor( address1D * mult.w );
	cellCoords.y = floor( (address1D - cellCoords.z * mult.z) * mult.y );
	cellCoords.x = floor( (address1D - cellCoords.z * mult.z - cellCoords.y * mult.x) );

	return cellCoords;
}

float addrTranslation3Dto1D( vec3 address3D, vec2 mult )
{
	return address3D.x + address3D.y * mult.x + address3D.z * mult.y;
}

// Check to see if the sphere overlaps the AABB
bool sphereOverlapsAABB( vec3 boxMin, vec3 boxMax, vec3 center, float radius )
{
	float s;
	float d = 0.0;

	// Find the square of the distance from the sphere to the box

	//////////////////////
	// X AXIS
	//////////////////////
	if( center.x < boxMin.x )
	{
		s = center.x - boxMin.x;
		d += s*s;
	}
	else if( center.x > boxMax.x )
	{
		s = center.x - boxMax.x;
		d += s*s;
	}

	//////////////////////
	// Y AXIS
	//////////////////////
	if( center.y < boxMin.y )
	{
		s = center.y - boxMin.y;
		d += s*s;
	}
	else if( center.y > boxMax.y )
	{
		s = center.y - boxMax.y;
		d += s*s;
	}

	//////////////////////
	// Z AXIS
	//////////////////////
	if( center.z < boxMin.z )
	{
		s = center.z - boxMin.z;
		d += s*s;
	}
	else if( center.z > boxMax.z )
	{
		s = center.z - boxMax.z;
		d += s*s;
	}

	return d <= radius*radius;
}

void main()
{
	// For each cell that AABB overlaps, compute exact particle overlap
	vec3 currCell = v_startCell[0];

	vec3 v = v_endCell[0] - v_startCell[0] + vec3( 1.0 );
	float i = 0.0;
	float count = v.x * v.y * v.z;

	vec3 cell;
	float writeIdx = v_currentFirstIdxL[0];

	do
	{
		// Get current cell
		cell = currCell;

		// Begin computing next cell coords (apparently step() is expensive)
		++currCell.x;

		// Compute current cell's AABB
		vec3 boxMin = minGrid + cell * cellSize;
		vec3 boxMax = boxMin + cellSize;

		// If overlaps, increment that cell's counter
		// The increment is done by emitting a vertex to that cell's position on screen
		if( sphereOverlapsAABB( boxMin, boxMax, v_particleData[0].xyz, v_particleData[0].w ) )
		{
			// Get cell linear idx (to be written in texture)
			float cellIdxL = addrTranslation3Dto1D( cell, _3Dto1Dconst );

			// Get texel coordinates
			vec2 ij = addrTranslation1Dto2D( writeIdx, texGridSize );

			// Write the pair ( cellIdxL, partId ) in LUMINANCE_ALPHA -> blue and alpha components
			gl_FrontColor = vec4( cellIdxL, 0.0, 0.0, gl_PositionIn[0].x );
			gl_Position   = vec4( ( gl_ModelViewProjectionMatrix * vec4( ij, 0.0, 1.0 ) ).xy, 0.5, 1.0 );

			EmitVertex();
			EndPrimitive();

			++writeIdx;

		} // if overlap

		// Increment counter
		++i;

		if( currCell.x > v_endCell[0].x )
		{
			currCell.x = v_startCell[0].x;
			++currCell.y;
		}
		if( currCell.y > v_endCell[0].y )
		{
			currCell.y = v_startCell[0].y;
			++currCell.z;
		}
	}
	while( i < count );














	//// Begin version without overlap test /////

	//// Particle volume
	//vec3 v = v_endCell[0] - v_startCell[0] + vec3( 1.0 );

	//// Size of particle block
	//float count = v_nextFirstIdxL[0] - v_currentFirstIdxL[0];
	//float idx = 0.0;

	//// Constant to compute linear address from 3D address
	//vec4 _1Dto3Dconst = vec4( v.x, 1.0 / v.x, v.x * v.y, 1.0 / ( v.x * v.y ) );

	//while( idx < count )
	//{
	//	vec3 localOffset = addrTranslation1Dto3D( idx, _1Dto3Dconst );

	//	// Compute current cell
	//	vec3 cell = v_startCell[0] + localOffset;

	//	// Get cell linear idx (to be written in texture)
	//	float cellIdxL = addrTranslation3Dto1D( cell, _3Dto1Dconst );

	//	// Get texel coordinates
	//	vec2 ij = addrTranslation1Dto2D( v_currentFirstIdxL[0] + idx, texGridSize );

	//	// Write the pair ( cellIdxL, partId ) in LUMINANCE_ALPHA -> blue and alpha components
	//	gl_FrontColor = vec4( cellIdxL, 0.0, 0.0, gl_PositionIn[0].x );
	//	gl_Position   = vec4( ( gl_ModelViewProjectionMatrix * vec4( ij, 0.0, 1.0 ) ).xy, 0.5, 1.0 );

	//	EmitVertex();
	//	EndPrimitive();

	//	++idx;
	//}

	//// End version without overlap test /////
}
