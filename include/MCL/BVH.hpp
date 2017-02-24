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

#ifndef MCLSCENE_BVH_H
#define MCLSCENE_BVH_H 1

#include "Object.hpp"
#include "AABB.hpp"
#include <memory>
#include <numeric>

namespace mcl {

typedef long morton_type;
typedef unsigned long long morton_encode_type;

//
//	BVH Initializer class for creating the tree and logging
//
class BVHInit {
public:
	BVHInit() : max_depth(10), n_nodes(0), avg_balance(0.f), runtime_s(0.f) {}
	BVHInit( int max_depth_ ) : max_depth(max_depth_), n_nodes(0), avg_balance(0.f), runtime_s(0.f) {}

	// Setup:
	int max_depth; // The max depth of the tree => 2^(max_depth) nodes

	// Logging (set by BVHBuilder):
	int n_nodes; // number of nodes in the created tree
	float avg_balance; // the "balance" of the last-created tree (lousy metric, but whatever)
	float runtime_s; // time it took to build the bvh (seconds)
};

//
//	BVH Node class that the tree is made up of
//
class BVHNode {
public:
	BVHNode() : aabb( new AABB ) { left_child=NULL; right_child=NULL; }
	~BVHNode() {
		// Should use a mempool this is slow...
		delete aabb;
		if( left_child != NULL ){ delete left_child; }
		if( right_child != NULL){ delete right_child; }
	}

	// Allocated in make_tree:
	BVHNode* left_child;
	BVHNode* right_child;
	AABB* aabb;
	mutable std::vector< std::shared_ptr<BaseObject> > m_objects; // empty unless a leaf node

	bool is_leaf() const { return m_objects.size()>0; }
	void get_edges( std::vector<Vec3f> &edges, bool add_children=true ); // for visual debugging
	void bounds( Vec3f &bmin, Vec3f &bmax ) const { bmin=aabb->min; bmax=aabb->max; }
};

//
//	BVH builder class for creating trees
//
class BVHBuilder {
public:
	// Create a tree using the BVHInit helper
//	static int make_tree( std::shared_ptr<BVHNode> &root, const std::vector< std::shared_ptr<BaseObject> > &objects

	// Parallel sorting construction (Lauterbach et al. 2009)
	// returns num nodes in tree
	static int make_tree_lbvh( BVHNode *root, const std::vector< std::shared_ptr<BaseObject> > &objects, int max_depth=10 );

	// Object Median split, round robin axis
	// returns num nodes in tree
	static int make_tree_spatial( BVHNode *root, const std::vector< std::shared_ptr<BaseObject> > &objects, int max_depth=10 );

	// Stats used for profiling:
	static int n_nodes; // number of nodes in the last-created tree
	static float avg_balance; // the "balance" of the last-created tree (lousy metric, but whatever)
	static float runtime_s; // time it took to build the bvh (seconds)

private:
	static void lbvh_split( BVHNode *node, const int bit, const std::vector< std::shared_ptr<BaseObject> > &prims,
		const std::vector< std::pair< morton_type, int > > &morton_codes, const int max_depth );
	static void spatial_split( BVHNode *node, const std::vector< std::shared_ptr<BaseObject> > &prims,
		const std::vector< int > &queue, const int split_axis, const int max_depth );

	static int num_avg_balance;
};

//
//	BVH Traversal class for stepping through tree
//
class BVHTraversal {
public:
	// Ray-Scene traversal for closest object (light rays)
	// Can also be used for selection rays if the last argument is used, as it sets the shared ptr of the object hit.
	static bool closest_hit( const BVHNode *node, const raycast::Ray *ray, raycast::Payload *payload, std::shared_ptr<BaseObject> *obj=0 );

	// Ray-Scene traversal for any object, early exit (shadow rays)
	// Remember to set your t_max in the payload!
	static bool any_hit( const BVHNode *node, const raycast::Ray *ray, raycast::Payload *payload );

	// Point-Scene traversal for closest object to a given point.
	// Projection is the point on the object surface, obj is the pointer to the object.
	// Returns false if a point was not in any AABB.
	static bool closest_object( const BVHNode *node, const Vec3f &point, Vec3f &projection, std::shared_ptr<BaseObject> *obj );
};


} // end namespace mcl



#endif
