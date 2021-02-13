#version 120

//uniform vec3 texCellAccumSize; // ( w, 1/w, 1/h )
uniform vec3 minGrid;
uniform vec3 ncells;
uniform vec3 invCellSize;

uniform sampler2D particles;
uniform sampler2D cellAccumIn;

varying out float v_currentFirstIdxL;
//varying out float v_nextFirstIdxL;
varying out vec3 v_startCell;
varying out vec3 v_endCell;
varying out vec4 v_particleData;

float myMod( float x, float y, float invY )
{
	return x - y * floor( x * invY );
}

vec2 addrTranslation1Dto2D( float address1D, vec2 mult )
{
	return vec2( myMod( address1D, mult.x, mult.y ) + 0.5, floor( address1D * mult.y ) + 0.5 ); // why we need to add 0.5 to x and y here, but not on the other shaders?!?!?
}

vec3 getGridIndex3D( vec3 particlePos )
{
	vec3 cellCoord = ( particlePos - minGrid ) * invCellSize;
	return clamp( floor( cellCoord ), vec3( 0.0 ), ncells - vec3( 1.0 ) );
}

void main ()
{
	// Get cell index in grid texture
	v_currentFirstIdxL = texture2D( cellAccumIn, gl_Vertex.yz ).r; // First linear idx in cell block

	//// Get next cell index in grid texture
	//vec2 nextAccumIdxIJ = addrTranslation1Dto2D( gl_Vertex.x + 1.0, texCellAccumSize.xy );

	//// Get last index of current cell in grid texture
	//v_nextFirstIdxL = texture2D( cellAccumIn, nextAccumIdxIJ * texCellAccumSize.yz ).r; // Next cell linear idx

	// Get original particle information (position = xyz, radius = w), from vertex attributes ( id, texcoord s, texcoord t )
	v_particleData = texture2D( particles, gl_Vertex.yz );

	// Compute particle AABB
	vec3 particleBoxMin = v_particleData.xyz - vec3( v_particleData.w );
	vec3 particleBoxMax = v_particleData.xyz + vec3( v_particleData.w );

	// Find cell limits
	v_startCell = getGridIndex3D( particleBoxMin );
	v_endCell   = getGridIndex3D( particleBoxMax );

	gl_Position = gl_Vertex;
}
