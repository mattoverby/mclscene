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

#include "MCL/BVH.hpp"

using namespace mcl;


void BVHNode::get_edges( std::vector<trimesh::vec> &edges ){
	aabb->get_edges( edges );
	if( left_child != NULL ){ left_child->get_edges( edges ); }
	if( right_child != NULL ){ right_child->get_edges( edges ); }

	for( int i=0; i<m_objects.size(); ++i ){
		m_objects[i]->get_edges( edges );
	}
}


void BVHNode::make_tree_spatial( const std::vector< std::shared_ptr<BaseObject> > objects, int split_axis, int max_depth ) {

	using namespace trimesh;	

	split_axis = (split_axis+1)%3;
	m_split = split_axis;
	max_depth--;

	// Create the aabb
	std::vector< point > obj_centers; // store the centers for later lookup
	for( int i=0; i<objects.size(); ++i ){
		vec bmin, bmax; objects[i]->get_aabb( bmin, bmax );
		*aabb += bmin;
		*aabb += bmax;
		obj_centers.push_back( (bmin+bmax)*0.5f );
	}
	point center = aabb->center();

	// If num faces == 1, we're done
	if( objects.size()==0 ){ return; }
	else if( objects.size()==1 || max_depth <= 0 ){
		m_objects = objects;
		return;
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
	left_child = std::shared_ptr<BVHNode>( new BVHNode() );
	right_child = std::shared_ptr<BVHNode>( new BVHNode() );
	left_child->make_tree_spatial( left_objs, split_axis, max_depth );
	right_child->make_tree_spatial( right_objs, split_axis, max_depth );
}


void BVHNode::make_tree_lbvh( const std::vector< std::shared_ptr<BaseObject> > objects ){

	

}


bool BVHTraversal::ray_intersect( std::shared_ptr<BVHNode> node, intersect::Ray &ray, intersect::Payload &payload ) {

	// See if we even hit the box
	if( !node->aabb->ray_intersect( ray.origin, ray.direction, payload.t_min, payload.t_max ) ){ return false; }

	// If we have children, progress down the tree
	if( node->left_child != NULL || node->right_child != NULL ){

		intersect::Payload payload_l=payload; intersect::Payload payload_r=payload;
		bool left_hit=false, right_hit=false;
		if( node->left_child != NULL ){ left_hit = ray_intersect( node->left_child, ray, payload_l ); }
		if( node->right_child != NULL ){ right_hit = ray_intersect( node->right_child, ray, payload_r ); }

		// See which child is closer
		if( left_hit && right_hit ){
			if( payload_r.t_max < payload_l.t_max ){
				payload = payload_r;
				return true;
			}
			payload = payload_l;
			return true;
		}
		else if( right_hit ){
			payload = payload_r;
			return true;
		}
		else if( left_hit ){
			payload = payload_l;
			return true;
		}

	} // end ray_intersect children

	// Otherwise it's a leaf node, check objects
	else{
		bool obj_hit = false;
		for( int i=0; i<node->m_objects.size(); ++i ){
			if( node->m_objects[i]->ray_intersect( ray, payload ) ){ obj_hit=true; }
		}
		return obj_hit;
	} // end ray_intersect objects

	return false;

} // end ray intersect


