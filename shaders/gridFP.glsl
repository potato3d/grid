#version 120
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect u_texCellAccum;
uniform sampler2DRect u_texTriangleVertices;

uniform float u_texGridDataSize;
uniform float u_texCellAccumSize;

uniform vec3 u_minGrid;
uniform vec3 u_invCellSize;
uniform vec3 u_numCells;

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

float get1DCoord( vec2 coord2D, float w )
{
    return ( coord2D.x - 0.5 ) + ( coord2D.y - 0.5 ) * w;
}

// TODO: optimize!
vec2 binarySearchLowerBound( float target )
{
    float low = 0.0;
	float high = u_texCellAccumSize - 1.0;
	float mid = 0.0;
	float midVal = 0.0;

	while( low <= high )
	{
		mid = low + floor( ( high - low ) * 0.5 );
		midVal = getTexel2DRect( u_texCellAccum, mid ).r;

		if( midVal < target )
			low = mid + 1;
		else if( midVal > target )
			high = mid - 1;
        else
            return vec2( mid, midVal );
	}
	
	if( midVal > target )
		return vec2( mid-1, getTexel2DRect( u_texCellAccum, mid-1 ).r );
	else
		return vec2( mid, midVal );
}

void main()
{
    // 1- Convert 2D fragment coordinates to 1D coordinate
    float coord1D = get1DCoord( gl_FragCoord.xy, MAX_TEX_SIZE );
    
    // Discard fragments beyond actual grid data
    if( coord1D >= u_texGridDataSize )
        discard;
    
    // 2- Binary search for value immediatelly smaller than coord1D inside accum texture.
    // Save index as primitive ID and value as cell start.
	vec2 primIDcellStart = binarySearchLowerBound( coord1D );
	
    // 3- Use primitive ID to read vertices and re-compute overlapped cells
    
    // Get triangle vertices
	vec3 v0 = getTexel2DRect( u_texTriangleVertices, primIDcellStart.x * 3.0 ).rgb;
	vec3 v1 = getTexel2DRect( u_texTriangleVertices, primIDcellStart.x * 3.0 + 1.0 ).rgb;
	vec3 v2 = getTexel2DRect( u_texTriangleVertices, primIDcellStart.x * 3.0 + 2.0 ).rgb;

	// Compute triangle AABB
	vec3 boxMin = min( v0, min( v1, v2 ) );
	vec3 boxMax = max( v0, max( v1, v2 ) );

	// Find cell limits
	vec3 startCell = getGridIndex3D( boxMin );
	vec3 endCell = getGridIndex3D( boxMax );

	// Overlapped cell volume (in number of cells)
	vec3 v = endCell - startCell + vec3( 1.0 );
    
    // 4- Determine cell ID from difference between 1D coord and cell start

	// Compute local 3D offset from cell start
	vec4 convert1Dto3D = vec4( v.x, 1.0 / v.x, v.x * v.y, 1.0 / ( v.x * v.y ) );
	vec3 localOffset = addrTranslation1Dto3D( coord1D - primIDcellStart.y, convert1Dto3D );

	// Compute current cell 3D coordinates
	vec3 cell = startCell + localOffset;

	// Get cell linear idx (to be written in texture)
	float cellIdxL = getGridIndex1D( cell );
    
    // 5- Write the pair ( cellIdxL, triangleId ) in LUMINANCE_ALPHA -> red and alpha components
	gl_FragColor = vec4( cellIdxL, 0.0, 0.0, primIDcellStart.x );
}