#version 120
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect u_texCellAccum;
uniform sampler2DRect u_texTriangleVertices;

uniform float u_passIdx;

uniform vec3 u_minGrid;
uniform vec3 u_invCellSize;
uniform vec3 u_numCells;

uniform float u_viewportW;
uniform float u_invViewportW;

const float MAX_TEX_SIZE = 8192.0;
const float INV_MAX_TEX_SIZE = 0.0001220703125;

float getGridIndex1D( vec3 address3D )
{
	return address3D.x + address3D.y * u_numCells.x + address3D.z * u_numCells.x * u_numCells.y;
}

vec3 addrTranslation1Dto3D( float address1D, vec4 mult )
{
	vec3 cellCoords;

	cellCoords.z = floor( address1D * mult.w );
	cellCoords.y = floor( (address1D - cellCoords.z * mult.z) * mult.y );
	cellCoords.x = floor( (address1D - cellCoords.z * mult.z - cellCoords.y * mult.x) );

	return cellCoords;
}

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

void main()
{
	// Get current cell start in grid data texture
	float currStartIdxLinear = getTexel2DRect( u_texCellAccum, gl_Vertex.x ).r;

	// Get next cell start in grid data texture
	float nextStartIdxLinear = getTexel2DRect( u_texCellAccum, gl_Vertex.x + 1.0 ).r;

	// Compute size of particle block
	float count = nextStartIdxLinear - currStartIdxLinear;

	// Check if block is full
	if( u_passIdx >= count )
	{
		// "Discard" vertex
		gl_Position = vec4( 1.0, 0.0, 0.0, 0.0 );
		return;
	}

	// Get triangle vertices
	vec3 v0 = getTexel2DRect( u_texTriangleVertices, gl_Vertex.x * 3.0 ).rgb;
	vec3 v1 = getTexel2DRect( u_texTriangleVertices, gl_Vertex.x * 3.0 + 1.0 ).rgb;
	vec3 v2 = getTexel2DRect( u_texTriangleVertices, gl_Vertex.x * 3.0 + 2.0 ).rgb;

	// Compute triangle AABB
	vec3 boxMin = min( v0, min( v1, v2 ) );
	vec3 boxMax = max( v0, max( v1, v2 ) );

	// Find cell limits
	vec3 startCell = getGridIndex3D( boxMin );
	vec3 endCell = getGridIndex3D( boxMax );

	// Overlapped cell volume (in number of cells)
	vec3 v = endCell - startCell + vec3( 1.0 );

	// Compute local 3D offset from passIdx
	vec4 convert1Dto3D = vec4( v.x, 1.0 / v.x, v.x * v.y, 1.0 / ( v.x * v.y ) );
	vec3 localOffset = addrTranslation1Dto3D( u_passIdx, convert1Dto3D );

	// Compute current cell 3D coordinates
	vec3 cell = startCell + localOffset;

	// Get cell linear idx (to be written in texture)
	float cellIdxL = getGridIndex1D( cell );

	// Write the pair ( cellIdxL, triangleId ) in LUMINANCE_ALPHA -> red and alpha components
	gl_FrontColor = vec4( cellIdxL, 0.0, 0.0, gl_Vertex.x );

	// Vertex position on screen depends on offset in current cell block
	gl_Position   = vec4( ( gl_ModelViewProjectionMatrix * vec4( get2DCoord( currStartIdxLinear + u_passIdx, u_viewportW, u_invViewportW ), 0.0, 1.0 ) ).xy, 0.5, 1.0 );
}
