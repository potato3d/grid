#version 120
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect u_texGridData;
uniform float u_texGridDataSize;

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

float get1DCoord( vec2 coord2D, float w )
{
    return ( coord2D.x - 0.5 ) + ( coord2D.y - 0.5 ) * w;
}

float binarySearch( float target )
{
    float low = 0.0;
	float high = u_texGridDataSize - 1.0;
	float mid = 0.0;
	float midVal = 0.0;

	while( low <= high )
	{
		mid = low + floor( ( high - low ) * 0.5 );
		midVal = getTexel2DRect( u_texGridData, mid ).b;

		if( midVal < target )
			low = mid + 1;
		else if( midVal > target )
			high = mid - 1;
        else
            return mid;
	}

    return -1.0;
}

void main()
{
    // 1- Convert 2D fragment coordinates to cell ID
    float cellID = get1DCoord( gl_FragCoord.xy, MAX_TEX_SIZE );
    
    // 2- Binary search cell ID in ordered grid pairs (cellID, primID)
    float pairIdx = binarySearch( cellID );
    
    // If cell ID not found, cell is empty and we need to write 0 as start and size
    if( pairIdx < 0.0 )
    {
        gl_FragColor = vec4( 0.0, 0.0, 0.0, 0.0 );
        return;
    }
    
    // 3- Count how many pairs to the left and to the right have the same cell ID
    float cellStart = pairIdx;
    float cellSize = 1.0;
    
    for( float i = pairIdx-1.0; i >= 0.0; --i )
    {
        float id = getTexel2DRect( u_texGridData, i ).b;
        
        if( id != cellID )
            break;
            
        ++cellSize;
        --cellStart;
    }
    
    for( float i = pairIdx+1.0; i < u_texGridDataSize; ++i )
    {
        float id = getTexel2DRect( u_texGridData, i ).b;
        
        if( id != cellID )
            break;
            
        ++cellSize;
    }
    
    // Write the pair (cellStart, cellSize) in LUMINANCE_ALPHA -> red and alpha components
    gl_FragColor = vec4( cellStart, 0.0, 0.0, cellSize );
}