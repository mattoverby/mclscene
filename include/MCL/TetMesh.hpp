// Copyright (c) 2016 University of Minnesota
// 
// MCLSCENE Uses the BSD 2-Clause License (http://www.opensource.org/licenses/BSD-2-Clause)
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other materials
//    provided with the distribution.
// THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR  A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE UNIVERSITY OF MINNESOTA, DULUTH OR CONTRIBUTORS BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// By Matt Overby (http://www.mattoverby.net)

#ifndef MCLSCENE_TETMESH_H
#define MCLSCENE_TETMESH_H 1

#include <vector>
#include "Object.hpp"
#include "AABB.hpp"

namespace mcl {

//
//	Tetrahedral Mesh
//
class TetMesh : public BaseObject {
public:
	static inline std::shared_ptr<TetMesh> create(){
		return std::shared_ptr<TetMesh>( new TetMesh() );
	}

	// Data
	std::vector< Vec4i > tets; // all elements
	std::vector< Vec3f > vertices; // all vertices in the tet mesh
	std::vector< Vec3f > normals; // zero length for all non-surface normals
	std::vector< Vec3i > faces; // surface triangles
	std::vector< Vec2f > texcoords; // per vertex uv coords
	std::vector< Vec2i > edges; // contains surface edges only

	// Get per-vertex data
	bool get_vertices(
		float* &vertices, int &num_vertices,
		float* &normals, int &num_normals,
		float* &texcoords, int &num_texcoords );

	// Get primitive data
	bool get_primitives( const Prim &type, int* &indices, int &num_prims );

	// Filename is the first part of a tetmesh which must contain an .ele and .node file.
	// Can also load a ".tet" file which is a list of vertices and eles in the same file.
	// I'll put some docs on filetypes some day.
	// If a ply file is supplied, tetgen will be used to tetrahedralize the mesh (however,
	// the ply must be ascii, not binary).
	// Returns true on success
	bool load( std::string filename );

	// Saves the tet mesh to .ele and .node files.
	// If extention is .tet, saves as a .tet file.
	// Otherwise do not include extensions on the filename argument.
	void save( std::string filename );

	std::string get_xml( int mode );

	// Compute the normals for surface vertices. The inner normals are length zero.
	void need_normals( bool recompute=false );

	// Transform the mesh by the given matrix
	void apply_xform( const trimesh::xform &xf );

	// Compute surface edges
	void need_edges();

	// Get bounds
	void get_bounds( Vec3f &bmin, Vec3f &bmax );

	// Get the vertices that make up the surface faces
	void get_surface_vertices( std::vector<int> *indices );

	// When stitching meshes together, this function will
	// join two vertices as one if they are within dist.
	void collapse_points( float distance );

private:
	AABB aabb;
	bool load_node( std::string filename ); // .node file
	bool load_ele( std::string filename ); // .ele file
	bool load_tet( std::string filename ); // .tet file
	bool load_mesh( std::string filename ); // .mesh file

	// Computes a surface mesh, called by load
	bool need_surface();

	// Uses tetgen to tetrahedralize a mesh, returning
	// the filename of the new files (node and ele)
	// which are generated and dumped in the same directory
	// as the original ply.
	// Returns an empty string on failure.
	std::string make_tetmesh( std::string filename );

}; // end class TetMesh

} // end namespace mcl

#endif
