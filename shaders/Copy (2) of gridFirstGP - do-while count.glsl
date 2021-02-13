#version 120
#extension GL_EXT_geometry_shader4 : enable

uniform vec2 texGridAccumSize;

uniform vec3 minGrid;
uniform vec3 cellSize;
uniform vec2 _3Dto1Dconst; // ( width, width*height )
uniform vec4 _1Dto3Dconst; // ( x, 1/x, x*y, 1/x*y )

uniform sampler2D particles;

varying in vec4 v_particleData[];
varying in vec3 v_startCell[];
varying in vec3 v_endCell[];
//varying in float v_startIndex[];
//varying in float v_endIndex[];

float myMod( float x, float y, float invY )
{
	return x - y * floor( x * invY );
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

vec2 addrTranslation1Dto2D( float address1D, vec2 mult )
{
	return vec2( myMod( address1D, mult.x, mult.y ), address1D * mult.y );
}

// Check to see if the sphere overlaps the AABB
bool sphereOverlapsAABB( vec3 boxMin, vec3 boxMax, vec3 center, float radius )
{
	vec3 boxCenter    = ( boxMax + boxMin ) * vec3( 0.5 );
	vec3 boxHalfSizes = ( boxMax - boxMin ) * vec3( 0.5 );

	vec3 v = center - boxCenter - vec3( radius );

	return ( v.x <= boxHalfSizes.x && v.y <= boxHalfSizes.y && v.z <= boxHalfSizes.z );
}

void main()
{
	// For each cell that AABB overlaps, compute exact particle overlap
	vec4 particleData = v_particleData[0];
	vec3 startCell = v_startCell[0];
	vec3 endCell = v_endCell[0];

	vec3 currCell = startCell;

	vec3 v = endCell - startCell;
	float i = 0.0;
	float count = 0.0;
	count = v.x * v.y * v.z;
	

	//if(i == count)
	//{
	//	// Increment by 1 current frame buffer value (with blend)
	//	gl_FrontColor = vec4( 15.0, 0.0, 0.0, 1.0 );

	//	// Compute vertex position on screen
	//	gl_Position = gl_ModelViewProjectionMatrix * vec4( 0.0, 0.0, 0.0, 1.0 );

	//	EmitVertex();
	//	EndPrimitive();
	//}
	//return;

	do
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

		++i;

		++currCell.x;

		if(currCell.x > endCell.x)
		{
			currCell.x = startCell.x;
			++currCell.y;
		}
		if(currCell.y > endCell.y)
		{
			currCell.y = startCell.y;
			++currCell.z;
		}
	}
	while( i <= count );

	//do
	//{
	//	currCell.y = startCell.y;

	//	do
	//	{
	//		currCell.x = startCell.x;

	//		do
	//		{
	//			// Compute current cell's AABB
	//			vec3 boxMin = minGrid + currCell * cellSize;
	//			vec3 boxMax = boxMin + cellSize;

	//			// If overlaps, increment that cell's counter
	//			// The increment is done by emitting a vertex to that cell's position on screen
	//			if( sphereOverlapsAABB( boxMin, boxMax, particleData.xyz, particleData.w ) )
	//			{
	//				// Find current cell index
	//				float indexL = addrTranslation3Dto1D( currCell, _3Dto1Dconst );
	//				vec2 indexIJ = addrTranslation1Dto2D( indexL, texGridAccumSize );

	//				// Increment by 1 current frame buffer value (with blend)
	//				gl_FrontColor = vec4( 1.0, 0.0, 0.0, 1.0 );

	//				// Compute vertex position on screen
	//				gl_Position = gl_ModelViewProjectionMatrix * vec4( indexIJ, 0.0, 1.0 );

	//				EmitVertex();
	//				EndPrimitive();
	//			}

	//			++currCell.x;
	//		}
	//		while( currCell.x <= endCell.x );

	//		++currCell.y;
	//	}
	//	while( currCell.y <= endCell.y );

	//	++currCell.z;
	//}
	//while( currCell.z <= endCell.z );


				



	//for( currCell.z = startCell.z; currCell.z <= endCell.z; ++currCell.z )
	//{
	//	for( currCell.y = startCell.y; currCell.y <= endCell.y; ++currCell.y )
	//	{
	//		for( currCell.x = startCell.x; currCell.x <= endCell.x; ++currCell.x )
	//		{
	//			// Compute current cell's AABB
	//			vec3 boxMin = minGrid + currCell * cellSize;
	//			vec3 boxMax = boxMin + cellSize;

	//			// If overlaps, increment that cell's counter
	//			// The increment is done by emitting a vertex to that cell's position on screen
	//			if( sphereOverlapsAABB( boxMin, boxMax, particleData.xyz, particleData.w ) )
	//			{
	//				// Find current cell index
	//				float indexL = addrTranslation3Dto1D( currCell, _3Dto1Dconst );
	//				vec2 indexIJ = addrTranslation1Dto2D( indexL, texGridAccumSize );

	//				// Increment by 1 current frame buffer value (with blend)
	//				gl_FrontColor = vec4( 1.0, 0.0, 0.0, 1.0 );

	//				// Compute vertex position on screen
	//				gl_Position = gl_ModelViewProjectionMatrix * vec4( indexIJ, 0.0, 1.0 );

	//				EmitVertex();
	//				EndPrimitive();
	//			}
	//		}
	//	}
	//}
}
