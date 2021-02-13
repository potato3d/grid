#version 120

uniform vec3 texGridAccumSize; // ( w, 1/w, 1/h )
uniform vec2 texGridSize; // ( w, 1/w )
uniform sampler2D gridAccumIn;

uniform float passIdx;

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
	float currentFirstIdxL = texture2D( gridAccumIn, gl_Vertex.yz ).r; // First linear idx in cell block

	// Get next cell index in grid texture
	vec2 nextAccumIdxIJ = addrTranslation1Dto2D( gl_Vertex.x + 1, texGridAccumSize.xy );

	// Get last index of current cell in grid texture
	float nextFirstIdxL = texture2D( gridAccumIn, nextAccumIdxIJ * texGridAccumSize.yz ).r; // Next cell linear idx

	// Check if need to discard vertex
	float count = nextFirstIdxL - currentFirstIdxL;

	if( passIdx >= count )
	{
		// "Discard" vertex
		gl_Position = vec4( 1.0, 0.0, 0.0, 0.0 );
		return;
	}

	// Move vertex to ith position
	vec2 ij = addrTranslation1Dto2D( currentFirstIdxL + passIdx, texGridSize );
	gl_Position = vec4( ( gl_ModelViewProjectionMatrix * vec4( ij, 1.0, 1.0 ) ).xy, 0.5, 1.0 );
}
