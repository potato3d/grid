#version 120
#extension GL_EXT_geometry_shader4 : enable

uniform vec2 texGridAccumSize; // ( w, 1/w )

uniform vec3 minGrid;
uniform vec3 cellSize;
uniform vec2 _3Dto1Dconst; // ( ncellx, ncellx*ncelly )

varying in vec4 v_particleData[];
varying in vec3 v_startCell[];
varying in vec3 v_endCell[];

float myMod( float x, float y, float invY )
{
	return x - y * floor( x * invY );
}

float addrTranslation3Dto1D( vec3 address3D, vec2 mult )
{
	return address3D.x + address3D.y * mult.x + address3D.z * mult.y;
}

vec2 addrTranslation1Dto2D( float address1D, vec2 mult )
{
	return vec2( myMod( address1D, mult.x, mult.y ), floor( address1D * mult.y ) + 0.5 );
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
			// Find current cell index
			float indexL = addrTranslation3Dto1D( cell, _3Dto1Dconst );
			vec2 indexIJ = addrTranslation1Dto2D( indexL, texGridAccumSize );

			// Increment by 1 current frame buffer value (with blend)
			gl_FrontColor = vec4( 1.0, 0.0, 0.0, 1.0 );

			// Compute vertex position on screen
			gl_Position = vec4( ( gl_ModelViewProjectionMatrix * vec4( indexIJ, 1.0, 1.0 ) ).xy, 0.5, 1.0 );

			EmitVertex();
			EndPrimitive();

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

} // main
