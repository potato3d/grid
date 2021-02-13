#include "CpuGrid.h"
#include <vr/stl_utils.h>

bool sphereOverlapsAabb( const vr::vec3f& boxMin, const vr::vec3f& boxMax, const vr::vec3f& center, float radius )
{
	float s;
	float d = 0.0f;

	//find the square of the distance
	//from the sphere to the box
	for( int i = 0 ; i < 3 ; ++i )
	{ 
		if( center[i] < boxMin[i] )
		{
			s = center[i] - boxMin[i];
			d += s*s; 
		}
		else if( center[i] > boxMax[i] )
		{ 
			s = center[i] - boxMax[i];
			d += s*s;
		}
	}

	return d <= radius*radius;
}

void CpuGrid::setBoxMin( float x, float y, float z )
{
	_boxMin.set( x, y, z );

	_boxMin *= 0.99f;
}

void CpuGrid::setBoxMax( float x, float y, float z )
{
	_boxMax.set( x, y, z );

	_boxMax *= 1.01f;
}

void CpuGrid::setNumTriangles( int numTri, bool fixedGrid )
{
	float diagonalX = _boxMax.x - _boxMin.x;
	float diagonalY = _boxMax.y - _boxMin.y;
	float diagonalZ = _boxMax.z - _boxMin.z;

	float V = ( diagonalX * diagonalY * diagonalZ );

	// TODO: magic user-supplied number
	int k = 6;
	float factor = powf( (float)( k * numTri) / V, 1.0f / 3.0f );

	int nx;
	int ny;
	int nz;

	// Compute actual number of cells in each dimension
	if( fixedGrid )
	{
		nx = 100;
		ny = 100;
		nz = 100;
	}
	else
	{
		nx = (int)ceilf( diagonalX * factor );
		ny = (int)ceilf( diagonalY * factor );
		nz = (int)ceilf( diagonalZ * factor );
	}

	setNumCells( nx, ny, nz );
}

void CpuGrid::setNumCells( int nx, int ny, int nz )
{
	_numCells.set( nx, ny, nz );

	// Allocate memory
	vr::vectorExactResize( _data, nx*ny*nz );

	// Compute cell size
	_cellSize.x = ( _boxMax.x - _boxMin.x ) / (float)nx;
	_cellSize.y = ( _boxMax.y - _boxMin.y ) / (float)ny;
	_cellSize.z = ( _boxMax.z - _boxMin.z ) / (float)nz;

	_invCellSize.x = 1.0f / _cellSize.x;
	_invCellSize.y = 1.0f / _cellSize.y;
	_invCellSize.z = 1.0f / _cellSize.z;
}

float roundIfClose( float number, float tolerance = 1e-6 )
{
	int intnumber = vr::round( number );
	if( abs( ( number - (float)intnumber ) ) < tolerance )
		return (float)intnumber;
	else
		return number;
}

void CpuGrid::worldToVoxel( const vr::vec3f& point, vr::vec3<int>& cell )
{
	//// Simulate GPU implementation by forcing all intermediary results to 32-bit precision
	//cell.x = (float)( (float)( point.x - _boxMin.x ) * _invCellSize.x );
	//cell.y = (float)( (float)( point.y - _boxMin.y ) * _invCellSize.y );
	//cell.z = (float)( (float)( point.z - _boxMin.z ) * _invCellSize.z );

	cell.x = ( point.x - _boxMin.x ) * _invCellSize.x;
	cell.y = ( point.y - _boxMin.y ) * _invCellSize.y;
	cell.z = ( point.z - _boxMin.z ) * _invCellSize.z;
}

CpuGrid::Cell& CpuGrid::getCell( const vr::vec3<int>& cellCoords )
{
	return _data[cellCoords.x + cellCoords.y*_numCells.x + cellCoords.z*_numCells.x*_numCells.y];
}

void CpuGrid::rebuild( float* triangleVertices, int numTriangles )
{
	// Clear previous grid data
	for( unsigned int i = 0; i < _data.size(); ++i )
		_data[i].clear();

	int triangleId = 0;

	for( int t = 0; t < numTriangles*9; t+=9 )
	{
		vr::vec3f v0( &triangleVertices[t+0] );
		vr::vec3f v1( &triangleVertices[t+3] );
		vr::vec3f v2( &triangleVertices[t+6] );

		vr::vec3f boxMin;
		vr::vec3f boxMax;

		boxMin.x = vr::min( v0.x, vr::min( v1.x, v2.x ) );
		boxMin.y = vr::min( v0.y, vr::min( v1.y, v2.y ) );
		boxMin.z = vr::min( v0.z, vr::min( v1.z, v2.z ) );

		boxMax.x = vr::max( v0.x, vr::max( v1.x, v2.x ) );
		boxMax.y = vr::max( v0.y, vr::max( v1.y, v2.y ) );
		boxMax.z = vr::max( v0.z, vr::max( v1.z, v2.z ) );

		// Find cell limits
		vr::vec3<int> startCell;
		vr::vec3<int> endCell;

		worldToVoxel( boxMin, startCell );
		worldToVoxel( boxMax, endCell );

		// For each cell that AABB overlaps, compute exact triangle overlap
		vr::vec3<int> currCell;

		for( currCell.z = startCell.z; currCell.z <= endCell.z; ++currCell.z )
		{
			for( currCell.y = startCell.y; currCell.y <= endCell.y; ++currCell.y )
			{
				for( currCell.x = startCell.x; currCell.x <= endCell.x; ++currCell.x )
				{
					// Compute current cell's AABB
					vr::vec3f cellBoxMin = _boxMin + vr::vec3f( currCell.x, currCell.y, currCell.z ) * _cellSize;
					vr::vec3f cellBoxMax = cellBoxMin + _cellSize;

					//if( triangleOverlapsAabb( v0, v1, v2, cellBoxMin, cellBoxMax ) )
					getCell( currCell ).push_back( triangleId );
				}
			}
		}

		++triangleId;
	}
}

const std::vector<CpuGrid::Cell>& CpuGrid::getGridData() const
{
	return _data;
}
