#version 120
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect u_texGridAccum;

uniform float u_viewportW;
uniform float u_invViewportW;

const float MAX_TEX_SIZE = 8192.0;
const float INV_MAX_TEX_SIZE = 0.0001220703125;

float myMod( float x, float y, float invY )
{
	return x - y * floor( x * invY );
}

vec2 get2DCoord( float linearCoord, float w, float invW )
{
	return vec2( myMod( linearCoord, w, invW ), floor( linearCoord * invW ) );
}

vec4 getTexel2DRect( sampler2DRect texSampler, float linearCoord )
{
	return texture2DRect( texSampler, get2DCoord( linearCoord, MAX_TEX_SIZE, INV_MAX_TEX_SIZE ) );
}

void main()
{
	// Get current and next cell's accum value
    float currValue = getTexel2DRect( u_texGridAccum, gl_Vertex.x ).r;
    float nextValue = getTexel2DRect( u_texGridAccum, gl_Vertex.x + 1.0 ).r;
    
    // Write the pair (currValue, nextValue - currValue) in LUMINANCE_ALPHA -> red and alpha components
    gl_FrontColor = vec4( currValue, 0.0, 0.0, nextValue - currValue );

	// Compute vertex position on screen
	gl_Position = vec4( ( gl_ModelViewProjectionMatrix * vec4( get2DCoord( gl_Vertex.x, u_viewportW, u_invViewportW ), 0.0, 1.0 ) ).xy, 0.5, 1.0 );
}
