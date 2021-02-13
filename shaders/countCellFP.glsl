#version 120
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect u_texTriangleVertices;

uniform vec3 u_minGrid;
uniform vec3 u_invCellSize;
uniform vec3 u_numCells;

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

vec3 getGridIndex3D( vec3 pos )
{
	vec3 cellCoord = ( pos - u_minGrid ) * u_invCellSize;
	return clamp( floor( cellCoord ), vec3( 0.0 ), u_numCells - vec3( 1.0 ) );
}

float get1DCoord( vec2 coord2D, float w )
{
    return ( coord2D.x - 0.5 ) + ( coord2D.y - 0.5 ) * w;
}

void main()
{
    // Convert 2D fragment coordinates to 1D primitive coordinate
    float coord1D = get1DCoord( gl_FragCoord.xy, MAX_TEX_SIZE ) * 3.0;
    
    // Get triangle vertices
    vec3 v0 = getTexel2DRect( u_texTriangleVertices, coord1D ).rgb;
	vec3 v1 = getTexel2DRect( u_texTriangleVertices, coord1D + 1.0 ).rgb;
	vec3 v2 = getTexel2DRect( u_texTriangleVertices, coord1D + 2.0 ).rgb;

#if 1
    // If at least 2 vertices are equal, than we are beyond the vertex count (and thus are acessing the same final vertex several times)
    if( all( equal( v0, v1 ) ) )
    {
        gl_FragColor = vec4( 0.0, 0.0, 0.0, 0.0 );
        return;
    }
#endif
    
    // Compute triangle AABB
	vec3 boxMin = min( v0, min( v1, v2 ) );
	vec3 boxMax = max( v0, max( v1, v2 ) );

	// Find cell limits
	vec3 startCell = getGridIndex3D( boxMin );
	vec3 endCell = getGridIndex3D( boxMax );

	// Compute total number of cells overlapped by current triangle
	vec3 v = endCell - startCell + vec3( 1.0 );
	float count = v.x * v.y * v.z;
	float total = count;

	// Save total in frame buffer
	gl_FragColor = vec4( total, 0.0, 0.0, 0.0 );
}