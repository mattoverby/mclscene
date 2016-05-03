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

#include "Object.hpp"
#include "AABB.hpp"
#include <memory>

namespace mcl {

namespace helper {
	static inline trimesh::point face_center( const trimesh::TriMesh::Face &f, const std::vector<trimesh::point> &vertices ){
		return (vertices[ f[0] ]+vertices[ f[1] ]+vertices[ f[2] ])/3.f;
	}
}


class BVHNode {
public:
	BVHNode() : aabb( new AABB ) { left_child=NULL; right_child=NULL; }
	virtual ~BVHNode(){}

	void get_edges( std::vector<trimesh::vec> &edges );
	const std::shared_ptr<AABB> bounds(){ return aabb; }

	std::shared_ptr<BVHNode> left_child;
	std::shared_ptr<BVHNode> right_child;
	std::shared_ptr<AABB> aabb;

	int m_split; // split axis
	std::vector< std::shared_ptr<BaseObject> > m_objects;

	void make_tree(const std::vector< std::shared_ptr<BaseObject> > objects ){ make_tree_spatial(objects,0,10); }

	// Spatial split, round robin axis
	void make_tree_spatial( const std::vector< std::shared_ptr<BaseObject> > objects, int split_axis, int max_depth );

	// Use the parallel sorting construction (Lauterbach et al. 2009)
	void make_tree_lbvh( const std::vector< std::shared_ptr<BaseObject> > objects );
};


class BVHTraversal {
public:
	static bool ray_intersect( std::shared_ptr<BVHNode> node, intersect::Ray &ray, intersect::Payload &payload );
};


} // end namespace mcl



#endif
