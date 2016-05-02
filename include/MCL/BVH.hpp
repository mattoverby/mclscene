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
#include "MCL/Object.hpp"
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
	BVHNode() : aabb( new AABB ) { left_child=NULL; right_child=NULL; }

	void get_edges( std::vector<trimesh::vec> &edges );
	const std::shared_ptr<AABB> bounds(){ return aabb; }

	std::shared_ptr<BVHNode> left_child;
	std::shared_ptr<BVHNode> right_child;
	std::shared_ptr<AABB> aabb;

	int m_split; // split axis
	std::vector< std::shared_ptr<BaseObject> > m_objects;

};


static inline std::shared_ptr<BVHNode> make_tree( const std::vector< std::shared_ptr<BaseObject> > objects, int split_axis, int max_depth ) {

	using namespace trimesh;	
	std::shared_ptr<BVHNode> node( new BVHNode() );

	node->left_child = NULL;
	node->right_child = NULL;
	split_axis = (split_axis+1)%3;
	node->m_split = split_axis;
	max_depth--;

	// Create the aabb
	std::vector< point > obj_centers; // store the centers for later lookup
	for( int i=0; i<objects.size(); ++i ){
		vec bmin, bmax; objects[i]->get_aabb( bmin, bmax );
		*node->aabb += bmin;
		*node->aabb += bmax;
		obj_centers.push_back( (bmin+bmax)*0.5f );
	}
	point center = node->aabb->center();

	// If num faces == 1, we're done
	if( objects.size()==0 ){ return node; }
	else if( objects.size()==1 || max_depth <= 0 ){
		node->m_objects = objects;
		return node;
	}
	else if( objects.size()==2 ){
		std::vector< std::shared_ptr<BaseObject> > left_objs(1,objects[0]), right_objs(1,objects[1]);
		node->left_child = make_tree( left_objs, split_axis, max_depth );
		node->right_child = make_tree( right_objs, split_axis, max_depth );
		return node;
	}

	// Split faces
	std::vector< std::shared_ptr<BaseObject> > left_objs, right_objs;
	for( int i=0; i<objects.size(); ++i ){
		double oc = obj_centers[i][split_axis];
		if( oc <= center[ split_axis ] ){ left_objs.push_back( objects[i] ); }
		else if( oc > center[ split_axis ] ){ right_objs.push_back( objects[i] ); }
	}

	// Check to make sure things got sorted. Sometimes small meshes fail.
	if( left_objs.size()==0 ){ left_objs.push_back( right_objs.back() ); right_objs.pop_back(); }
	if( right_objs.size()==0 ){ right_objs.push_back( left_objs.back() ); left_objs.pop_back(); }

	// Create the children
	node->left_child = make_tree( left_objs, split_axis, max_depth );
	node->right_child = make_tree( right_objs, split_axis, max_depth );

	return node;
}





} // end namespace mcl



#endif
