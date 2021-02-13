#version 120

uniform vec3 texGridAccumSize; // ( w, 1/w, 1/h )
uniform sampler2D gridAccumIn;

varying out float v_currentFirstIdxL;
varying out float v_nextFirstIdxL;

float myMod( float x, float y, float invY )
{
	return x - y * floor( x * invY );
}

vec2 addrTranslation1Dto2D( float address1D, vec2 mult )
{
	return vec2( myMod( address1D, mult.x, mult.y ), floor( address1D * mult.y ) + 0.5 );
}

void main ()
{
	// Get cell index in grid texture
	v_currentFirstIdxL = texture2D( gridAccumIn, gl_Vertex.yz ).r; // First linear idx in cell block

	// Get next cell index in grid texture
	vec2 nextAccumIdxIJ = addrTranslation1Dto2D( gl_Vertex.x + 1, texGridAccumSize.xy );

	// Get last index of current cell in grid texture
	v_nextFirstIdxL = texture2D( gridAccumIn, nextAccumIdxIJ * texGridAccumSize.yz ).r; // Next cell linear idx

	gl_Position = gl_Vertex;
}
