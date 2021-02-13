#version 120
#extension GL_EXT_geometry_shader4 : enable

uniform vec2 texGridSize; // ( w, 1/w )

varying in float v_currentFirstIdxL[];
varying in float v_nextFirstIdxL[];

float myMod( float x, float y, float invY )
{
	return x - y * floor( x * invY );
}

vec2 addrTranslation1Dto2D( float address1D, vec2 mult )
{
	return vec2( myMod( address1D, mult.x, mult.y ), floor( address1D * mult.y ) + 0.5 );
}

void main()
{
	// Loop over cell block, starting from 2o element
	float count = v_nextFirstIdxL[0] - v_currentFirstIdxL[0];

	// First element receives no fragments, second receives 1 frag, third receives 2 frag, etc (to increment stencil accordingly)
	float idx = 1.0;
	float addr = v_currentFirstIdxL[0] + 1.0;

	while( idx < count )
	{
		vec2 ij = addrTranslation1Dto2D( addr, texGridSize );
		
		for( float k = 0; k < idx; ++k )
		{
			gl_Position = vec4( ( gl_ModelViewProjectionMatrix * vec4( ij, 1.0, 1.0 ) ).xy, 0.5, 1.0 );
			EmitVertex();
			EndPrimitive();
		}

		++idx;
		++addr;
	}
}
