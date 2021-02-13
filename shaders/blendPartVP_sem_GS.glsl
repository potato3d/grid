#version 120
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect u_texTriangleVertices;

uniform float u_passIdx;

uniform vec3 u_minGrid;
uniform vec3 u_invCellSize;
uniform vec3 u_numCells;

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

vec3 getGridIndex3D( vec3 pos )
{
	vec3 cellCoord = ( pos - u_minGrid ) * u_invCellSize;
	return clamp( floor( cellCoord ), vec3( 0.0 ), u_numCells - vec3( 1.0 ) );
}

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

void main()
{
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

	// Compute total number of cells overlapped by current triangle
	vec3 v = endCell - startCell + vec3( 1.0 );
	float count = v.x * v.y * v.z;

	// Check valid passIdx
	if( u_passIdx >= count )
	{
		// "Discard" vertex
		gl_Position = vec4( 1.0, 0.0, 0.0, 0.0 );
		return;
	}

	// Compute local 3D offset from passIdx
	vec4 convert1Dto3D = vec4( v.x, 1.0 / v.x, v.x * v.y, 1.0 / ( v.x * v.y ) );
	vec3 localOffset = addrTranslation1Dto3D( u_passIdx, convert1Dto3D );

	// Compute current cell 3D coordinates
	vec3 cell = startCell + localOffset;

	// Get cell linear idx
	float cellIdxL = getGridIndex1D( cell );

	// Increment by 1 current frame buffer value (with blend)
	gl_FrontColor = vec4( 1.0, 0.0, 0.0, 1.0 );

	// Compute vertex position on screen
	gl_Position = vec4( ( gl_ModelViewProjectionMatrix * vec4( get2DCoord( cellIdxL, u_viewportW, u_invViewportW ), 0.0, 1.0 ) ).xy, 0.5, 1.0 );
}
