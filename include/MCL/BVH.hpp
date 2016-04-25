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

#ifndef MCLSCENE_BVH_H
#define MCLSCENE_BVH_H 1

#include "TriMesh.h"
#include <memory>
#include <cassert>

namespace mcl {

namespace helper {
	static inline trimesh::point face_center( const trimesh::TriMesh::Face &f, const std::vector<trimesh::point> &vertices ){
		return (vertices[ f[0] ]+vertices[ f[1] ]+vertices[ f[2] ])/3.f;
	}
}

class AABB { // axis aligned bounding box
public:
	AABB() : valid(false) {}
	AABB( trimesh::vec min_, trimesh::vec max_ ) : min(min_), max(max_), valid(false) {}
	trimesh::vec min, max;

	// Fills the vector with points that make the edge lines.
	// Used for visual debugging in OpenGL.
	void get_edges( std::vector<trimesh::vec> &edges );

	trimesh::vec center(){ return (min+max)*0.5f; }

	AABB& operator+=(const AABB& aabb){
		if( valid ){ min.min( aabb.min ); max.max( aabb.max ); }
		else{ min = aabb.min; max = aabb.max; }
		valid = true;
	}

	AABB& operator+=(const trimesh::vec& p){
		if( valid ){ min.min(p); max.max(p); }
		else{ min = p; max = p; }
		valid = true;
	}

	bool valid;
};


class BVHNode {
public:
	BVHNode( const std::vector<trimesh::TriMesh::Face> &faces, const std::vector<trimesh::point> &vertices, int split_axis, int max_depth=10 );
	BVHNode( std::vector< std::shared_ptr<BVHNode> > bvhnodes, int split_axis );
	void get_faces( const std::vector<trimesh::TriMesh::Face> *faces ){ faces = &m_faces; }
	void get_edges( std::vector<trimesh::vec> &edges );
	const std::shared_ptr<AABB> bounds(){ return aabb; }

private:
	std::shared_ptr<BVHNode> left_child;
	std::shared_ptr<BVHNode> right_child;
	std::shared_ptr<AABB> aabb;

	int m_split; // split axis
	std::vector<trimesh::TriMesh::Face> m_faces;

};

// Makes a BVH from a trimesh
static inline std::shared_ptr<BVHNode> make_tree( trimesh::TriMesh *mesh ){
	mesh->need_faces();
	std::vector<trimesh::point> vertices = mesh->vertices;
	std::vector<trimesh::TriMesh::Face> faces = mesh->faces;
	std::shared_ptr<BVHNode> root( new BVHNode( faces, vertices, 0 ) );
	return root;
}

// Makes a BVH from a vector of other BVHs
static inline std::shared_ptr<BVHNode> make_tree( std::vector< std::shared_ptr<BVHNode> > bvhs ){
	std::shared_ptr<BVHNode> root( new BVHNode( bvhs, 0 ) );
	return root;
}


} // end namespace mcl



#endif
