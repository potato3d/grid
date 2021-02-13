#version 120
#extension GL_EXT_geometry_shader4 : enable

uniform vec2 texGridSize; // ( x, 1/x )

varying in float v_currentFirstIdxL[];
varying in float v_nextFirstIdxL[];

float myMod( float x, float y, float invY )
{
	return x - y * floor( x * invY );
}

vec2 addrTranslation1Dto2D( float address1D, vec2 mult )
{
	return vec2( myMod( address1D, mult.x, mult.y ), address1D * mult.y );
}

void main()
{
	//if( gl_PositionIn[0].x > 6.0 )
		//return;

	// Compute first and last indices in 2D
	float firstIdxL = v_currentFirstIdxL[0];
	float nextIdxL  = v_nextFirstIdxL[0];

	//firstIdxL = ( firstIdxL == texGridSize.x )? texGridSize.x - 1.0 : firstIdxL;
	//lastIdxL  = ( lastIdxL < 0.0 )? 0.0 : lastIdxL;

	vec2 firstIJ = floor( addrTranslation1Dto2D( firstIdxL, texGridSize ) );
	vec2 nextIJ  = floor( addrTranslation1Dto2D( nextIdxL,  texGridSize ) );



	//if( firstIJ.x == 1.0 && lastIJ.x == 0.0 && firstIJ.y == 0.0 && lastIJ.y == 0.0 )
	//{
	//	gl_FrontColor = vec4( firstIdxL, 20.0, 30.0, 1.0 );
	//	gl_Position = gl_ModelViewProjectionMatrix * vec4( 0.0, 0.0, 0.0, 1.0 );
	//	EmitVertex();

	//	gl_FrontColor = vec4( firstIdxL, 20.0, 30.0, 1.0 );
	//	gl_Position = gl_ModelViewProjectionMatrix * vec4( 1.0, 0.0, 0.0, 1.0 );
	//	EmitVertex();

	//	EndPrimitive();
	//}
	//return;



	// If cell block occupies two lines, need to fill both of them
	if( nextIJ.y > firstIJ.y )
	{
		// Add one to the end because the GPU discards last fragment in strip
		vec2 endTex = vec2( 8192.0, firstIJ.y );

		// Cell block in first line
		for( float i = firstIJ.x + 1; i < endTex.x; ++i )
		{
			gl_Position = gl_ModelViewProjectionMatrix * vec4( i, firstIJ.y, 0.0, 1.0 );
			EmitVertex();

			gl_Position = gl_ModelViewProjectionMatrix * vec4( endTex, 0.0, 1.0 );
			EmitVertex();

			EndPrimitive();
		}

		if( nextIJ.x > 0 )
		{
			// Remaining cell block in second line
			// Starting size depends on above iteration
			float count = endTex.x - ( firstIJ.x + 1 );

			for( float i = 0.0; i < count; ++i )
			{
				gl_Position = gl_ModelViewProjectionMatrix * vec4( 0.0, nextIJ.y, 0.0, 1.0 );
				EmitVertex();

				// Add one to the end because the GPU discards last fragment in strip
				gl_Position = gl_ModelViewProjectionMatrix * vec4( nextIJ, 0.0, 1.0 );
				EmitVertex();

				EndPrimitive();
			}

			// Remaining cell block in second line
			for( float i = 0.0; i < nextIJ.x; ++i )
			{
				gl_Position = gl_ModelViewProjectionMatrix * vec4( i, nextIJ.y, 0.0, 1.0 );
				EmitVertex();

				// Add one to the end because the GPU discards last fragment in strip
				gl_Position = gl_ModelViewProjectionMatrix * vec4( nextIJ, 0.0, 1.0 );
				EmitVertex();

				EndPrimitive();
			}
		}
	}
	else
	{
		// Entire cell block in first line
		for( float i = firstIJ.x + 1; i < nextIJ.x; ++i )
		{
			gl_Position = gl_ModelViewProjectionMatrix * vec4( i, firstIJ.y, 0.0, 1.0 );
			EmitVertex();

			// Add one to the end because the GPU discards last fragment in strip
			gl_Position = gl_ModelViewProjectionMatrix * vec4( nextIJ, 0.0, 1.0 );
			EmitVertex();

			EndPrimitive();
		}
	}


	// First element receives no fragments, second receives 1 frag, third receives 2 frag, etc (to increment stencil accordingly)
	

	//float currentFirstIdxL = v_currentFirstIdxL[0];

	//// Compute sum of vertices to be emitted (limit of loop)
	//// Arithmetic progression: Sum = ( n * ( a1 + an ) ) / 2
	//float n = v_nextFirstIdxL[0] - currentFirstIdxL;
	//float sum = ( n * ( n - 1.0 ) ) * 0.5;

	//// Begin with second element (first receives zero vertices)
	//float i = 1.0;
	//++currentFirstIdxL;

	//// K is the comparison value with I to determine when to go to next position
	//float k = 1.0;

	//// J keeps track of the current element in the arithmetic progression
	//float j = 1.0;

	//vec2 ij = addrTranslation1Dto2D( currentFirstIdxL, texGridSize );

	//while( i <= sum )
	//{
	//	ij = addrTranslation1Dto2D( currentFirstIdxL, texGridSize );
	//	
	//	gl_Position = gl_ModelViewProjectionMatrix * vec4( ij, 0.0, 1.0 );
	//	EmitVertex();
	//	EndPrimitive();

	//	if( i == k )
	//	{
	//		++currentFirstIdxL;
	//		++j;
	//		k += j;
	//	}

	//	++i;
	//}
}
