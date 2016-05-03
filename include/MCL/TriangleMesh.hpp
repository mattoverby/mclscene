// Copyright 2016 Matthew Overby.
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
#include "BVH.hpp"

namespace mcl {

//
//	Special class for mesh bvh
//
class MeshBVH : public BVHNode {
public:
	MeshBVH() : BVHNode(), valid(false) {}
	void make_tree( const std::vector<trimesh::TriMesh::Face> &faces, const std::vector<trimesh::point> &vertices, int split_axis, int max_depth );
	std::vector<trimesh::TriMesh::Face> m_faces;
	bool valid;
};


//
//	Just a convenient wrapper to plug into the system
//
class TriangleMesh : public BaseObject {
public:
	TriangleMesh( std::shared_ptr<trimesh::TriMesh> tm, std::string mat="" ) : tris(tm), vertices(tm->vertices), normals(tm->normals), faces(tm->faces),
		filename(""), bvh(new MeshBVH), material(mat) {}

	// Mesh data
	std::vector<trimesh::point> &vertices;
	std::vector<trimesh::vec> &normals;
	std::vector<trimesh::TriMesh::Face> &faces;

	std::string get_type() const { return "trimesh"; }

	const std::shared_ptr<trimesh::TriMesh> get_TriMesh(){ return tris; }

	void apply_xform( const trimesh::xform &xf ){ trimesh::apply_xform( tris.get(), xf ); }

	std::string get_material() const { return material; }

	void get_aabb( trimesh::vec &bmin, trimesh::vec &bmax ){
		if( !bvh->valid ){
			bvh->valid = true;
			bvh->make_tree( faces, vertices, 0, 10 );
		}
		bmin = bvh->bounds()->min; bmax = bvh->bounds()->max;
	}

	void get_edges( std::vector<trimesh::vec> &edges ){ // return edges of BVH for debugging visuals
		trimesh::vec n,x; get_aabb(n,x); // build the bvh
		bvh->get_edges( edges );
	}

private:
	std::shared_ptr<MeshBVH> bvh;
	std::string filename;
	std::shared_ptr<trimesh::TriMesh> tris;
	std::string material;
};


} // end namespace mcl

#endif
