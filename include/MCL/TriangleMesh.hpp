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

#ifndef MCLSCENE_TRIANGLEMESH_H
#define MCLSCENE_TRIANGLEMESH_H 1

#include "Object.hpp"
#include "AABB.hpp"

namespace mcl {

//
//	Just a convenient wrapper to plug into the system
//
class TriangleMesh : public BaseObject {
public:
	static inline std::shared_ptr<TriangleMesh> create(){
		return std::shared_ptr<TriangleMesh>( new TriangleMesh() );
	}

	// Data
	std::vector< Vec3f > vertices; // all vertices in the mesh
	std::vector< Vec3f > normals; // zero length for all non-surface normals
	std::vector< Vec3i > faces; // surface triangles
	std::vector< Vec2f > texcoords; // per vertex uv coords
	std::vector< Vec2i > edges;

	// Get per-vertex data
	bool get_vertices(
		float* &vertices, int &num_vertices,
		float* &normals, int &num_normals,
		float* &texcoords, int &num_texcoords );

	// Get primitive data
	bool get_primitives( const Prim &type, int* &indices, int &num_prims );

	// Returns true on success
	bool load( std::string filename );

	// Saves the triangle mesh to a file (obj)
	void save( std::string filename );

	std::string get_xml( int mode=0 );

	void apply_xform( const trimesh::xform &xf );

	void get_bounds( Vec3f &bmin, Vec3f &bmax );

	void need_normals( bool recompute=false );

	// Creates edges for rendering wireframe
	void need_edges( bool recompute=false );

//	void get_primitives( std::vector< std::shared_ptr<BaseObject> > &prims ){
//		if( tri_refs.size() != faces.size() ){ make_tri_refs(); }
//		prims.insert( prims.end(), tri_refs.begin(), tri_refs.end() );
//	}

	// Normal of face f
	Vec3f trinorm( unsigned int f );

	// Clear all mesh data
	void clear();

	// When stitching meshes together, this function will
	// join two vertices as one if they are within dist.
	void collapse_points( float distance );

	void make_ccw();

private:
	AABB aabb;

	// Triangle refs are used for BVH hook-in.
//	void make_tri_refs();
//	std::vector< std::shared_ptr<BaseObject> > tri_refs;

	bool load_obj( std::string filename );
};


} // end namespace mcl

#endif
