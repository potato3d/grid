#ifndef _CPUGRID_H_
#define _CPUGRID_H_

#include <vr/vec3.h>
#include <vector>

class CpuGrid
{
public:
	typedef std::vector<int> Cell;

	void setBoxMin( float x, float y, float z );
	void setBoxMax( float x, float y, float z );

	// Compute cell size from box size and cell count
	void setNumCells( int nx, int ny, int nz );
	void setNumTriangles( int numTri, bool fixedGrid );

	void worldToVoxel( const vr::vec3f& point, vr::vec3<int>& cell );

	Cell& getCell( const vr::vec3<int>& cellCoords );

	void rebuild( float* triangleVertices, int numTriangles );

	const std::vector<Cell>& getGridData() const;

private:
	vr::vec3f _boxMin;
	vr::vec3f _boxMax;

	vr::vec3<int> _numCells;

	vr::vec3f _cellSize;
	vr::vec3f _invCellSize;

	std::vector<Cell> _data;
};

#endif // _CPUGRID_H_
