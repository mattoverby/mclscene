// Copyright (c) 2017 University of Minnesota
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


#ifndef MCLSCENE_KDTREE_H
#define MCLSCENE_KDTREE_H 1

#include "MCL/Vec.hpp"
#include "MCL/AABB.hpp"

namespace mcl {

//
// KDTree node
//
class KDTNode {
public:
	KDTNode(){ left_child=NULL; right_child=NULL; }
	~KDTNode() {
		if( left_child != NULL ){ delete left_child; }
		if( right_child != NULL){ delete right_child; }
	}

	// Allocated in make_tree:
	KDTNode* left_child;
	KDTNode* right_child;
	mutable std::vector< std::shared_ptr<BaseObject> > m_objects; // empty unless a leaf node
	bool is_leaf() const { return m_objects.size()>0; }

	unsigned short axis; // 0, 1, 2
	Vec3f point; // the point that splits the plane
};


//	
//
//
namespace kdtree {

	// Object Median split, round robin axis
	static inline void make_tree( KDTNode *root, const std::vector< std::shared_ptr<BaseObject> > &objects, int max_depth=10 );

	// object median split
	static inline void object_median_split( KDTNode *node, const std::vector< std::shared_ptr<BaseObject> > &prims,
		const std::vector< int > &queue, const int split_axis, const int max_depth );

	// Point-Scene traversal for closest object to a given point.
	// Projection is the point on the object surface, obj is the pointer to the object.
	// Returns true if a closest object was found (closer than distance between point and projection).
	static inline bool closest_object( const KDTNode *node, const Vec3f &point, Vec3f &projection, std::shared_ptr<BaseObject> *obj );

}; // end namespace kdtree

//
//	Implementation
//

static inline void kdtree::make_tree( KDTNode *root, const std::vector< std::shared_ptr<BaseObject> > &objects, int max_depth ){
	
	if( root == NULL ){ root = new KDTNode(); }
	else{
		if( root->left_child != NULL ){ delete root->left_child; root->left_child = NULL; }
		if( root->right_child != NULL){ delete root->right_child; root->right_child = NULL; }
		root->m_objects.clear();
	}

	// Get all the primitives in the domain and start construction
	std::vector< std::shared_ptr<BaseObject> > prims;
	for( int i=0; i<objects.size(); ++i ){ objects[i]->get_primitives( prims ); }
	if( prims.size()==0 ){ return; }

	// BEGIN!
	std::vector< int > queue( prims.size() );
	std::iota( std::begin(queue), std::end(queue), 0 );
	object_median_split( root, prims, queue, 0, max_depth );

} // end make kd tree


// object median split
static inline void kdtree::object_median_split( KDTNode *node, const std::vector< std::shared_ptr<BaseObject> > &prims,
	const std::vector< int > &queue, const int split_axis, const int max_depth ){

	AABB aabb; // Create an AABB to compute center
	std::vector< float > obj_min( queue.size() );
	std::vector< float > obj_max( queue.size() );
	for( int i=0; i<queue.size(); ++i ){
		Vec3f bmin, bmax;
		prims[ queue[i] ]->get_bounds( bmin, bmax );
		obj_min[i] = bmin[ split_axis ];
		obj_max[i] = bmax[ split_axis ];
		aabb += bmin; aabb += bmax;
	}

	// Set KDTree node details
	node->point = aabb.center();
	node->axis = split_axis;
	double mid_pt = node->point[ split_axis ];

	// If num faces == 1, we're done
	if( queue.size()==0 ){ return; }
	else if( queue.size()==1 || max_depth <= 0 ){
		node->m_objects.reserve( queue.size() );
		for( int i=0; i<queue.size(); ++i ){ node->m_objects.push_back( prims[ queue[i] ] ); }
		return;
	}

	// Split faces
	// Note that a triangle may be added to both sides of the KDTree!
	std::vector<int> left_queue, right_queue;
	for( int i=0; i<queue.size(); ++i ){
		if( obj_min[i] <= mid_pt ){ left_queue.push_back( queue[i] ); }
		if( obj_max[i] >= mid_pt ){ right_queue.push_back( queue[i] ); }
	}

	// Create the children
	node->left_child = new KDTNode();
	node->right_child = new KDTNode();
	object_median_split( node->left_child, prims, left_queue, ((split_axis+1)%3), max_depth-1 );
	object_median_split( node->right_child, prims, right_queue, ((split_axis+1)%3), max_depth-1 );
}


// Point-Scene traversal for closest object to a given point.
// Projection is the point on the object surface, obj is the pointer to the object.
static inline bool kdtree::closest_object( const KDTNode *node, const Vec3f &point, Vec3f &projection, std::shared_ptr<BaseObject> *obj ){

	// If we're a leaf, find closest projection
	if( node->is_leaf() ){
		double dist = (point-projection).squaredNorm();
		bool obj_hit = false;
		for( int i=0; i<node->m_objects.size(); ++i ){

			Vec3f p = node->m_objects[i]->projection( point );
			Vec3f n = point - p;

			// See if this projection is closer
			double curr_dist = n.squaredNorm();
			if( curr_dist < dist ){
				projection = p;
				obj_hit = true;
				obj=&(node->m_objects[i]);
				dist = curr_dist;
			}
		}
		return obj_hit;
	} // end is leaf node

	// Otherwise parse the tree
	bool left_hit = false; bool right_hit = false;
	if( node->left_child != NULL && point[node->axis] <= node->point[node->axis] ){
		left_hit = kdtree::closest_object( node->left_child, point, projection, obj );
	}
	if( node->right_child != NULL && point[node->axis] >= node->point[node->axis] ){
		right_hit = kdtree::closest_object( node->right_child, point, projection, obj );
	}

	if( left_hit || right_hit ){ return true; }
	return false;
}



} // end namespace mcl

#endif

