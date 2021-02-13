#version 120
#extension GL_EXT_geometry_shader4 : enable

uniform vec2 texGridSize;
uniform vec2 _1Dto2Dconst; // ( 1.0/width, 1.0/(width*height) )

uniform vec3 minGrid;
uniform vec3 ncells;
uniform vec3 cellSize;
uniform vec3 _3Dto1Dconst; // ( 1.0, width, width*height )

uniform sampler2D particles;

float addrTranslation3Dto1D( vec3 address3D, vec3 mult )
{
	return dot( address3D, mult );
}

vec2 addrTranslation1Dto2D( float address1D, vec2 mult )
{
	return vec2( mod( address1D, mult.x ), address1D / mult.x );
}

float addrTranslation2Dto1D( vec2 address2D, vec2 mult )
{
	return ( address2D.y / mult.x ) + ( address2D.x / mult.x );
}

vec3 getGridIndex3D(vec3 particle)
{
	vec3 position = ( particle.xyz - minGrid ) / cellSize;
	return clamp( floor( position ), vec3( 0.0 ), ncells - vec3( 1.0 ) );
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
	// Get original particle information (position = xyz, radius = w), from vertex attributes ( id, texcoord s, texcoord t )
	vec4 particleData = texture2D( particles, gl_PositionIn[0].yz );

	// TODO: testing
gl_FrontColor = vec4( 1.0, 0.0, 0.0, 1.0 );

vec3 cell = vec3( 0,1,0 );

float indexL = addrTranslation3Dto1D( cell, _3Dto1Dconst );
vec2 indexIJ = addrTranslation1Dto2D( indexL, texGridSize );

if( _3Dto1Dconst.y == 1.0 )
{
	gl_FrontColor = vec4( 1.0, 0.0, 0.0, 1.0 );
	gl_Position = gl_ModelViewProjectionMatrix * vec4( 0.0, 0.0, 0.0, 1.0 );
	EmitVertex();
	EndPrimitive();
}
return;

gl_Position = gl_ModelViewProjectionMatrix * vec4( indexIJ, 0.0, 1.0 );
EmitVertex();
EndPrimitive();
return;

	// Compute particle AABB
	vec3 particleBoxMin = particleData.xyz - vec3( particleData.w );
	vec3 particleBoxMax = particleData.xyz + vec3( particleData.w );

	// Find cell limits
	vec3 startCell = getGridIndex3D( particleBoxMin );
	vec3 endCell   = getGridIndex3D( particleBoxMax );

	// For each cell that AABB overlaps, compute exact particle overlap
	vec3 currCell;

	for( currCell.z = startCell.z; currCell.z <= endCell.z; ++currCell.z )
	{
		for( currCell.y = startCell.y; currCell.y <= endCell.y; ++currCell.y )
		{
			for( currCell.x = startCell.x; currCell.x <= endCell.x; ++currCell.x )
			{
				// Compute current cell's AABB
				vec3 boxMin = minGrid + currCell * cellSize;
				vec3 boxMax = boxMin + cellSize;

				// If overlaps, increment that cell's counter
				// The increment is done by emitting a vertex to that cell's position on screen
				if( sphereOverlapsAABB( boxMin, boxMax, particleData.xyz, particleData.w ) )
				{
					// Find current cell normalized index
					float indexL = addrTranslation3Dto1D( currCell, _3Dto1Dconst );
					vec2 indexIJ = addrTranslation1Dto2D( indexL, texGridSize );

					// Increment by 1 current frame buffer value (with blend)
					gl_FrontColor = vec4( 1.0, 0.0, 0.0, 1.0 );

					// Compute vertex position on screen
					gl_Position = vec4( ( gl_ModelViewProjectionMatrix * vec4( indexIJ, 1.0, 1.0 ) ).xy, 0.5, 1.0 );

					EmitVertex();
					EndPrimitive();
				}
			}
		}
	}
}
