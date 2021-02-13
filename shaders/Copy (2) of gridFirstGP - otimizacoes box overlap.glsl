#version 120
#extension GL_EXT_geometry_shader4 : enable

uniform vec2 texGridAccumSize;

uniform vec3 minGrid;
uniform vec3 ncells;
uniform vec3 cellSize;
uniform vec3 invCellSize;
uniform vec2 _3Dto1Dconst; // ( width, width*height )

uniform sampler2D particles;

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
	return vec2( myMod( address1D, mult.x, mult.y ), address1D * mult.y );
}

vec3 getGridIndex3D( vec3 particlePos )
{
	vec3 cellCoord = ( particlePos - minGrid ) * invCellSize;
	return clamp( floor( cellCoord ), vec3( 0.0 ), ncells - vec3( 1.0 ) );
}

// Check to see if the sphere overlaps the AABB
bool sphereOverlapsAABB( vec3 boxMin, vec3 boxMax, vec3 center, float radius )
{
	vec3 boxCenter    = ( boxMax + boxMin ) * vec3( 0.5 );
	vec3 boxHalfSizes = ( boxMax - boxMin ) * vec3( 0.5 );

	vec3 v = center - boxCenter - vec3( radius );

	return ( v.x <= boxHalfSizes.x && v.y <= boxHalfSizes.y && v.z <= boxHalfSizes.z );
	
	//float s;
	//float d = 0.0;

	// Find the square of the distance from the sphere to the box
	
	//// ALOGORITHM 1, 2nd FASTEST
	//s = ( center.x < boxMin.x )? (center.x - boxMin.x) : ( ( center.x > boxMax.x )? (center.x - boxMax.x) : 0.0 );
	//d += s*s;

	//if( d <= radius*radius )
	//	return true;

	//s = ( center.y < boxMin.y )? (center.y - boxMin.y) : ( ( center.y > boxMax.y )? (center.y - boxMax.y) : 0.0 );
	//d += s*s;

	//if( d <= radius*radius )
	//	return true;

	//s = ( center.z < boxMin.z )? (center.z - boxMin.z) : ( ( center.z > boxMax.z )? (center.z - boxMax.z) : 0.0 );
	//d += s*s;

	//// ALOGORITHM 2, FASTEST
		//float comp = max( center.x - boxMin.x, center.x - boxMax.x );
	//s = comp * step( comp, 0.0 );
	//d += s*s;

	//if( d <= radius*radius )
	//	return true;

	//comp = max( center.y - boxMin.y, center.y - boxMax.y );
	//s = comp * step( comp, 0.0 );
	//d += s*s;

	//if( d <= radius*radius )
	//	return true;

	//comp = max( center.z - boxMin.z, center.z - boxMax.z );
	//s = comp * step( comp, 0.0 );
	//d += s*s;

	//// ALOGORITHM 3, original version
	////////////////////////
	//// X AXIS
	////////////////////////
	//if( center.x < boxMin.x )
	//{
	//	s = center.x - boxMin.x;
	//	d += s*s;
	//}
	//else if( center.x > boxMax.x )
	//{
	//	s = center.x - boxMax.x;
	//	d += s*s;
	//}

	//if( d <= radius*radius )
	//	return true;

	////////////////////////
	//// Y AXIS
	////////////////////////
	//if( center.y < boxMin.y )
	//{
	//	s = center.y - boxMin.y;
	//	d += s*s;
	//}
	//else if( center.y > boxMax.y )
	//{
	//	s = center.y - boxMax.y;
	//	d += s*s;
	//}

	//if( d <= radius*radius )
	//	return true;

	////////////////////////
	//// Z AXIS
	////////////////////////
	//if( center.z < boxMin.z )
	//{
	//	s = center.z - boxMin.z;
	//	d += s*s;
	//}
	//else if( center.z > boxMax.z )
	//{
	//	s = center.z - boxMax.z;
	//	d += s*s;
	//}

	//return d <= radius*radius;
}

void main()
{
	// Get original particle information (position = xyz, radius = w), from vertex attributes ( id, texcoord s, texcoord t )
	vec4 particleData = texture2D( particles, gl_PositionIn[0].yz );

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
					// Find current cell index
					float indexL = addrTranslation3Dto1D( currCell, _3Dto1Dconst );
					vec2 indexIJ = addrTranslation1Dto2D( indexL, texGridAccumSize );

					// Increment by 1 current frame buffer value (with blend)
					gl_FrontColor = vec4( 1.0, 0.0, 0.0, 1.0 );

					// Compute vertex position on screen
					gl_Position = gl_ModelViewProjectionMatrix * vec4( indexIJ, 0.0, 1.0 );

					EmitVertex();
					EndPrimitive();
				}
			}
		}
	}
}



	//for( i = start; i < = end; ++i )
	//{
	//	vec3 currCell = addrTranslation1Dto3D( i, _1Dto3Dconst );

	//	// Compute current cell's AABB
	//	vec3 boxMin = minGrid + currCell * cellSize;
	//	vec3 boxMax = boxMin + cellSize;

	//	// If overlaps, increment that cell's counter
	//	// The increment is done by emitting a vertex to that cell's position on screen
	//	if( sphereOverlapsAABB( boxMin, boxMax, particleData.xyz, particleData.w ) )
	//	{
	//		// Find current cell index
	//		float indexL = addrTranslation3Dto1D( currCell, _3Dto1Dconst );
	//		vec2 indexIJ = addrTranslation1Dto2D( indexL, texGridAccumSize );

	//		// Increment by 1 current frame buffer value (with blend)
	//		gl_FrontColor = vec4( 1.0, 0.0, 0.0, 1.0 );

	//		// Compute vertex position on screen
	//		gl_Position = gl_ModelViewProjectionMatrix * vec4( indexIJ, 0.0, 1.0 );

	//		EmitVertex();
	//		EndPrimitive();
	//	}
	//}