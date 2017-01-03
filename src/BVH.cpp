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

#include "MCL/BVH.hpp"
#include <chrono>
#include <bitset>

using namespace mcl;

namespace helper {
	// use: bool is_one = helper::check_bit( myInt, bit_position );
	static inline bool check_bit( morton_type variable, int bit ){
		std::bitset<sizeof(morton_type)*8> bs(variable);
		return ( bs[bit]==1 );
	}
}

static inline morton_type morton_encode(const morton_encode_type x, const morton_encode_type y, const morton_encode_type z){
	int n_iters = sizeof(morton_type)*8;
	// Step through the bits and assign them. The x2 for i is required to round-robinish interleaving
	// and differs from typical morton encoding. Without them, I got thin slices along the x and z axes.
	morton_type result = 0;
	for( morton_type i = 0; i<n_iters; ++i ){
		result |= (x & (morton_type(1) << i)) << i*2
			| (y & (morton_type(1) << i)) << (i*2 + 1)
			| (z & (morton_type(1) << i)) << (i*2 + 2);
	}
	return result;
}


// Used for stats:
int BVHBuilder::n_nodes = 0;
float BVHBuilder::avg_balance = 0.f;
int BVHBuilder::num_avg_balance = 0;
float BVHBuilder::runtime_s = 0.f;


int BVHBuilder::make_tree_lbvh( BVHNode *root, const std::vector< std::shared_ptr<BaseObject> > &objects, int max_depth ){

	std::chrono::time_point<std::chrono::system_clock> start, end;
	start = std::chrono::system_clock::now();
	if( root == NULL ){ root = new BVHNode(); }

	using namespace trimesh;

	// Get all the primitives in the domain
	std::vector< std::shared_ptr<BaseObject> > prims;
	prims.reserve( n_nodes ); // only helpful if we're recreating

	n_nodes = 1;
	avg_balance = 0.f;
	num_avg_balance = 0;

	for( int i=0; i<objects.size(); ++i ){ objects[i]->get_primitives( prims ); }
	if( prims.size()==0 ){ return 1; }

	// Compute centroids
	std::vector< Eigen::Vector3d > centroids( prims.size() );
	AABB world_aabb;
	for( int i=0; i<prims.size(); ++i ){
		Eigen::Vector3d bmin, bmax; prims[i]->bounds( bmin, bmax );
		world_aabb += bmin; world_aabb += bmax;
		centroids[i]=( (bmin+bmax)*0.5f );
	}

	double max_scaled = 1024.f;
	Eigen::Vector3d world_min( world_aabb.min );
	Eigen::Vector3d world_max( world_aabb.max );
	Eigen::Vector3d world_len = (world_max-world_min) * (1.0/max_scaled);

	// Assign morton codes
	std::vector< std::pair< morton_type, int > > morton_codes( prims.size() );
#pragma omp parallel for
	for( int i=0; i<prims.size(); ++i ){

		// Scale the centroid to a value between 0 and max_scaled and convert to integer.
		Eigen::Vector3d cent = centroids[i];
		cent = ( cent - world_min ).cwiseProduct( world_len );
		morton_encode_type ix = morton_encode_type( cent[0] );
		morton_encode_type iy = morton_encode_type( cent[1] );
		morton_encode_type iz = morton_encode_type( cent[2] );

		morton_codes[i] = std::make_pair( morton_encode( ix, iy, iz ), i );
	}

	// Find first non-zero most signficant bit
	int start_bit = sizeof(morton_type)*8-1;
	bool found=false;
	for( ; start_bit > 1 && !found; --start_bit ){

#pragma omp parallel for
		for( int i=0; i<morton_codes.size(); ++i ){
			if( helper::check_bit( morton_codes[i].first, start_bit ) ){
#pragma omp critical
				{ found=true; }
			}
		}

	} // end find starting bit

	// Now that we have the morton codes, we can recursively build the BVH in a top down manner
	lbvh_split( root, start_bit, prims, morton_codes, max_depth );

	end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end-start;
	runtime_s = float(elapsed_seconds.count());

	return n_nodes;

}


void BVHBuilder::lbvh_split( BVHNode *node,
	const int bit, const std::vector< std::shared_ptr<BaseObject> > &prims,
	const std::vector< std::pair< morton_type, int > > &morton_codes, const int max_depth ){

	// First, see what bit we're at. If it's the last bit of the morton code,
	// this is a child and we should add the objects to the scene.
	if( bit == 0 || max_depth <= 0 || morton_codes.size() == 1 ){
		node->m_objects.reserve( morton_codes.size() );
		for( int i=0; i<morton_codes.size(); ++i ){ node->m_objects.push_back( prims[ morton_codes[i].second ] ); }
	} // end add objects

	// Check the morton codes at the bit.
	// 0 = left child, 1 = right child.
	else{
		std::vector< std::pair< morton_type, int > > left_codes, right_codes;
		for( int i=0; i<morton_codes.size(); ++i ){

			if( helper::check_bit( morton_codes[i].first, bit ) ){
				right_codes.push_back( morton_codes[i] );
			} else {
				left_codes.push_back( morton_codes[i] );
			}

		} // end sort morton codes

		// Check to make sure things got sorted. Sometimes small meshes fail.
		if( left_codes.size()==0 ){ left_codes.push_back( right_codes.back() ); right_codes.pop_back(); }
		if( right_codes.size()==0 ){ right_codes.push_back( left_codes.back() ); left_codes.pop_back(); }

		avg_balance += float(left_codes.size())/float(right_codes.size());
		num_avg_balance++;
//		num_objects = left_codes.size()+right_codes.size();

		// Create the children
		node->left_child = new BVHNode();
		node->right_child = new BVHNode();
		lbvh_split( node->left_child, bit-1, prims, left_codes, max_depth-1 );
		lbvh_split( node->right_child, bit-1, prims, right_codes, max_depth-1 );
		n_nodes += 2;

	} // end create childrend

	// Now that the tree is constructed, create the aabb
	for( int i=0; i<node->m_objects.size(); ++i ){
		Eigen::Vector3d bmin, bmax;
		node->m_objects[i]->bounds( bmin, bmax );
		*(node->aabb) += bmin; *(node->aabb) += bmax;
	}
	if( node->left_child != NULL ){ *(node->aabb) += *(node->left_child->aabb); }
	if( node->right_child != NULL ){ *(node->aabb) += *(node->right_child->aabb); }

}



int BVHBuilder::make_tree_spatial( BVHNode *root, const std::vector< std::shared_ptr<BaseObject> > &objects, int max_depth ){

	std::chrono::time_point<std::chrono::system_clock> start, end;
	start = std::chrono::system_clock::now();
	if( root == NULL ){ root = new BVHNode(); }
	else{
		if( root->left_child != NULL ){ delete root->left_child; root->left_child = NULL; }
		if( root->right_child != NULL){ delete root->right_child; root->right_child = NULL; }
		root->m_objects.clear();
		root->aabb->valid = false;
	}

	// Get all the primitives in the domain and start construction
	std::vector< std::shared_ptr<BaseObject> > prims;
	prims.reserve( n_nodes ); // only helpful if we're recreating

	n_nodes = 1;
	avg_balance = 0.f;
	num_avg_balance = 0;

	for( int i=0; i<objects.size(); ++i ){ objects[i]->get_primitives( prims ); }

	if( prims.size()==0 ){ return 1; }

	std::vector< int > queue( prims.size() );
	std::iota( std::begin(queue), std::end(queue), 0 );
	spatial_split( root, prims, queue, 0, max_depth );

	end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end-start;
	runtime_s = float(elapsed_seconds.count());

	return n_nodes;
}


void BVHBuilder::spatial_split( BVHNode *node, const std::vector< std::shared_ptr<BaseObject> > &prims,
	const std::vector< int > &queue, const int split_axis, const int max_depth ) {
	using namespace trimesh;

	// Create the aabb
	std::vector< Eigen::Vector3d > obj_centers( queue.size() ); // store the centers for later lookup
	for( int i=0; i<queue.size(); ++i ){
		Eigen::Vector3d bmin, bmax; prims[ queue[i] ]->bounds( bmin, bmax );
		*(node->aabb) += bmin; *(node->aabb) += bmax;
		obj_centers[i] = Eigen::Vector3d( (bmin+bmax)*0.5 );
	}
	Eigen::Vector3d center = node->aabb->center();

	// If num faces == 1, we're done
	if( queue.size()==0 ){ return; }
	else if( queue.size()==1 || max_depth <= 0 ){
		node->m_objects.reserve( queue.size() );
		for( int i=0; i<queue.size(); ++i ){ node->m_objects.push_back( prims[ queue[i] ] ); }
		return;
	}

	// Split faces
	std::vector<int> left_queue, right_queue;
	for( int i=0; i<queue.size(); ++i ){
		double oc = obj_centers[i][split_axis];
		if( oc <= center[ split_axis ] ){ left_queue.push_back( queue[i] ); }
		else if( oc > center[ split_axis ] ){ right_queue.push_back( queue[i] ); }
	}

	// Check to make sure things got sorted. Sometimes small meshes fail.
	if( left_queue.size()==0 ){ left_queue.push_back( right_queue.back() ); right_queue.pop_back(); }
	if( right_queue.size()==0 ){ right_queue.push_back( left_queue.back() ); left_queue.pop_back(); }

	// Create the children
	node->left_child = new BVHNode();
	node->right_child = new BVHNode();
	spatial_split( node->left_child, prims, left_queue, ((split_axis+1)%3), max_depth-1 );
	spatial_split( node->right_child, prims, right_queue, ((split_axis+1)%3), max_depth-1 );
	n_nodes += 2;

} // end build spatial split tree


//
//	BVH Traversal
//


bool BVHTraversal::closest_hit( const BVHNode* node, const intersect::Ray *ray, intersect::Payload *payload, std::shared_ptr<BaseObject> *obj ){

	// See if we hit the box
	if( !intersect::ray_aabb( ray, node->aabb->min, node->aabb->max, payload ) ){ return false; }

	// See if there are children to intersect
	bool left_hit=false, right_hit=false;
	if( node->left_child != NULL ){ left_hit = BVHTraversal::closest_hit( node->left_child, ray, payload, obj ); }
	if( node->right_child != NULL ){ right_hit = BVHTraversal::closest_hit( node->right_child, ray, payload, obj ); }
	if( left_hit || right_hit ){ return true; }

	// Loop over objects stored on this bvh node
	bool obj_hit = false;
	for( int i=0; i<node->m_objects.size(); ++i ){
		if( node->m_objects[i]->ray_intersect( ray, payload ) ){ obj=&(node->m_objects[i]); obj_hit=true; }
	}
	return obj_hit;

} // end ray intersect


bool BVHTraversal::any_hit( const BVHNode* node, const intersect::Ray *ray, intersect::Payload *payload ){

	// See if we hit the box
	if( !intersect::ray_aabb( ray, node->aabb->min, node->aabb->max, payload ) ){ return false; }

	// See if there are children to intersect
	if( node->left_child != NULL ){ if( BVHTraversal::any_hit( node->left_child, ray, payload ) ){ return true; } }
	if( node->right_child != NULL ){ if( BVHTraversal::any_hit( node->right_child, ray, payload ) ){ return true; } }

	// Otherwise it's a leaf node, check objects
	for( int i=0; i<node->m_objects.size(); ++i ){
		if( node->m_objects[i]->ray_intersect( ray, payload ) ){ return true; }
	}

	return false;

} // end ray intersect


bool BVHTraversal::closest_object( const BVHNode *node, const trimesh::vec &point, trimesh::vec &projection, std::shared_ptr<BaseObject> *obj ){

	double dist = trimesh::len2( projection - point );
	if( intersect::point_aabb( point, node->aabb->min, node->aabb->max ) > dist ){ return false; }

	// See if there are children to intersect
	bool left_hit=false, right_hit=false;
	if( node->left_child != NULL ){ left_hit = BVHTraversal::closest_object( node->left_child, point, projection, obj ); }
	if( node->right_child != NULL ){ right_hit = BVHTraversal::closest_object( node->right_child, point, projection, obj ); }
	if( left_hit || right_hit ){ return true; }

	// Otherwise it's a leaf node, check objects
	bool obj_hit = false;
	for( int i=0; i<node->m_objects.size(); ++i ){
		trimesh::vec p = node->m_objects[i]->projection( point );
		trimesh::vec n = point - p;

		// See if this projection is closer
		double curr_dist = trimesh::len2( n );
		if( curr_dist < dist ){
			projection = p;
			obj_hit = true;
			obj=&(node->m_objects[i]);
			dist = curr_dist;
		}
	}
	return obj_hit;

} // end closest object


void BVHNode::get_edges( std::vector<Eigen::Vector3d> &edges, bool add_children ){

	using namespace trimesh;
	{
		Eigen::Vector3d min = aabb->min;
		Eigen::Vector3d max = aabb->max;

		// Bottom quad
		Eigen::Vector3d a = min;
		Eigen::Vector3d b( max[0], min[1], min[2] );
		Eigen::Vector3d c( max[0], min[1], max[2] );
		Eigen::Vector3d d( min[0], min[1], max[2] );
		// Top quad
		Eigen::Vector3d e( min[0], max[1], min[2] );
		Eigen::Vector3d f( max[0], max[1], min[2] );
		Eigen::Vector3d g = max;
		Eigen::Vector3d h( min[0], max[1], max[2] );

		// make edges
		// bottom
		edges.push_back( a ); edges.push_back( b );
		edges.push_back( a ); edges.push_back( d );
		edges.push_back( c ); edges.push_back( b );
		edges.push_back( c ); edges.push_back( d );
		// top
		edges.push_back( e ); edges.push_back( f );
		edges.push_back( e ); edges.push_back( h );
		edges.push_back( g ); edges.push_back( f );
		edges.push_back( g ); edges.push_back( h );
		// columns
		edges.push_back( d ); edges.push_back( h );
		edges.push_back( min ); edges.push_back( e );
		edges.push_back( b ); edges.push_back( f );
		edges.push_back( c ); edges.push_back( max );
	}

	if( left_child != NULL && add_children ){ left_child->get_edges( edges ); }
	if( right_child != NULL && add_children ){ right_child->get_edges( edges ); }
}
