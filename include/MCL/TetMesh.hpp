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

#include "MCL/TriangleMesh.hpp"

namespace mcl {

//
//	Tetrahedral Mesh
//
class TetMesh : public BaseObject {
public:
	std::vector< Vec4i > tets; // all elements
	std::vector< Vec3d > vertices; // all vertices in the tet mesh
	std::vector< Vec3d > normals; // zero length for all non-surface normals
	std::vector< Vec3d > colors; // per vertex colors
	std::vector< Vec3i > faces; // surface triangles
	std::vector< Vec2d > texcoords; // per vertex uv coords

	// Filename is the first part of a tetmesh which must contain an .ele and .node file.
	// If a ply file is supplied, tetgen will be used to tetrahedralize the mesh (however,
	// the ply must be ascii, not binary).
	// Returns true on success
	bool load( std::string filename );

	// Saves the tet mesh to .ele and .node files.
	// Do not include extensions on the filename argument.
	void save( std::string filename );

	std::string get_xml( int mode );

	// Compute the normals for surface vertices. The inner normals are length zero.
	void need_normals( bool recompute=true );

	// Transform the mesh by the given matrix
	void apply_xform( const trimesh::xform &xf );

	void bounds( Vec3d &bmin, Vec3d &bmax );

	void update_appdata();

	void get_primitives( std::vector< std::shared_ptr<BaseObject> > &prims ){
		if( tri_refs.size() != faces.size() ){ make_tri_refs(); }
		prims.insert( prims.end(), tri_refs.begin(), tri_refs.end() );
	}

	void get_surface_vertices( std::vector<int> *indices );

private:
	AABB aabb;

	bool load_node( std::string filename );

	bool load_ele( std::string filename );

	// Computes a surface mesh, called by load
	bool need_surface();

	// Uses tetgen to tetrahedralize a mesh, returning
	// the filename of the new files (node and ele)
	// which are generated and dumped in the same directory
	// as the original ply.
	// Returns an empty string on failure.
	std::string make_tetmesh( std::string filename );

	// Triangle refs are used for BVH hook-in.
	void make_tri_refs();
	std::vector< std::shared_ptr<BaseObject> > tri_refs;

}; // end class TetMesh

} // end namespace mcl

#endif
