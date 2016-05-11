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
}

int n_nodes = 0;

void BVHNode::spatial_split( const std::vector< std::shared_ptr<BaseObject> > &objects,
	const std::vector< int > &queue, const int split_axis, const int max_depth ) {
	using namespace trimesh;

	m_split = split_axis;

	// Create the aabb
	std::vector< point > obj_centers( queue.size() ); // store the centers for later lookup
	for( int i=0; i<queue.size(); ++i ){
		vec bmin, bmax; objects[ queue[i] ]->get_aabb( bmin, bmax );
		*aabb += bmin; *aabb += bmax;
		obj_centers[i] = point( (bmin+bmax)*0.5f );
	}
	point center = aabb->center();

	// If num faces == 1, we're done
	if( queue.size()==0 ){ return; }
	else if( queue.size()==1 || max_depth <= 0 ){
		m_objects.reserve( queue.size() );
		for( int i=0; i<queue.size(); ++i ){ m_objects.push_back( objects[ queue[i] ] ); }
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
	num_objects = left_queue.size()+right_queue.size();
	left_child = std::shared_ptr<BVHNode>( new BVHNode() );
	right_child = std::shared_ptr<BVHNode>( new BVHNode() );
	left_child->spatial_split( objects, left_queue, ((split_axis+1)%3), max_depth-1 );
	right_child->spatial_split( objects, right_queue, ((split_axis+1)%3), max_depth-1 );
	n_nodes += 2;

} // end build spatial split tree


float avg_balance = 0.f;
int num_avg_balance = 0;



void BVHNode::lbvh_split( const int bit, const std::vector< std::shared_ptr<BaseObject> > &prims,
	const std::vector< std::pair< morton_type, int > > &morton_codes, const int max_depth ){

	// First, see what bit we're at. If it's the last bit of the morton code,
	// this is a child and we should add the objects to the scene.
	if( bit == 0 || max_depth <= 0 || morton_codes.size() == 1 ){
		m_objects.reserve( morton_codes.size() );
		for( int i=0; i<morton_codes.size(); ++i ){ m_objects.push_back( prims[ morton_codes[i].second ] ); }
	} // end add objects

	// Check the morton codes at the bit.
	// 0 = left child, 1 = right child.
	else{
		std::vector< std::pair< morton_type, int > > left_codes, right_codes;
		for( int i=0; i<morton_codes.size(); ++i ){

			if( helper::check_bit( morton_codes[i].first, bit ) ){
				right_codes.push_back( morton_codes[i] );

//		std::cout << bit << ":\t" << std::bitset<8*8>( morton_codes[i].first ) << std::endl;

			} else {
				left_codes.push_back( morton_codes[i] );
			}

		} // end sort morton codes

		// Check to make sure things got sorted. Sometimes small meshes fail.
		if( left_codes.size()==0 ){ left_codes.push_back( right_codes.back() ); right_codes.pop_back(); }
		if( right_codes.size()==0 ){ right_codes.push_back( left_codes.back() ); left_codes.pop_back(); }

		avg_balance += float(left_codes.size())/float(right_codes.size());
		num_avg_balance++;
		num_objects = left_codes.size()+right_codes.size();

		// Create the children
		assert( left_codes.size() > 0 && right_codes.size() > 0 );
		left_child = std::shared_ptr<BVHNode>( new BVHNode() );
		right_child = std::shared_ptr<BVHNode>( new BVHNode() );
		left_child->lbvh_split( bit-1, prims, left_codes, max_depth-1 );
		right_child->lbvh_split( bit-1, prims, right_codes, max_depth-1 );
		n_nodes += 2;

	} // end create childrend

	// Now that the tree is constructed, create the aabb
	for( int i=0; i<m_objects.size(); ++i ){
		trimesh::vec bmin, bmax; m_objects[i]->get_aabb( bmin, bmax );
		*aabb += bmin; *aabb += bmax;
	}
	if( left_child != NULL ){ *aabb += *(left_child->aabb); }
	if( right_child != NULL ){ *aabb += *(right_child->aabb); }
}


//
//	BVH Traversal
//


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


int BVHBuilder::make_tree_lbvh( std::shared_ptr<BVHNode> &root, const std::vector< std::shared_ptr<BaseObject> > &objects ){

	root.reset( new BVHNode );

	n_nodes = 1;

	using namespace trimesh;

	// Get all the primitives in the domain
	std::vector< std::shared_ptr<BaseObject> > prims;
	for( int i=0; i<objects.size(); ++i ){ objects[i]->get_primitives( prims ); }

	// Compute centroids
	std::vector< vec > centroids( prims.size() );
	AABB world_aabb;
	for( int i=0; i<prims.size(); ++i ){
		vec bmin, bmax; prims[i]->get_aabb( bmin, bmax );
		world_aabb += bmin; world_aabb += bmax;
		centroids[i]=( (bmin+bmax)*0.5f );
	}

	float max_scaled = 1024.f;
	vec world_min( world_aabb.min );
	vec world_max( world_aabb.max );
	vec world_len = max_scaled / (world_max-world_min);

	// Assign morton codes
	std::vector< std::pair< morton_type, int > > morton_codes( prims.size() );
	#pragma omp parallel for
	for( int i=0; i<prims.size(); ++i ){

		// Scale the centroid to a value between 0 and max_scaled and convert to integer.
		vec cent = centroids[i];
		cent = ( cent - world_min ) * world_len;
		morton_encode_type ix = morton_encode_type( cent[0] );
		morton_encode_type iy = morton_encode_type( cent[1] );
		morton_encode_type iz = morton_encode_type( cent[2] );

		morton_codes[i] = std::make_pair( morton_encode( ix, iy, iz ), i );
	}

	// Find first non-zero most signficant bit
	int start_bit = sizeof(morton_type)*8-1;
	bool found=false;
	for( start_bit; start_bit > 1 && !found; --start_bit ){

		#pragma omp parallel for
		for( int i=0; i<morton_codes.size(); ++i ){
			if( helper::check_bit( morton_codes[i].first, start_bit ) ){
				#pragma omp critical
				{ found=true; }
			}
		}

	} // end find starting bit

	// Now that we have the morton codes, we can recursively build the BVH in a top down manner
	root->lbvh_split( start_bit, prims, morton_codes, 10000 );

	std::cout << "\nLBVH Balance: " << avg_balance / float(num_avg_balance) << std::endl;
	std::cout << "Linear BVH made " << n_nodes << " nodes for " << prims.size() << " primitives." << std::endl;

	return n_nodes;

}


int BVHBuilder::make_tree_spatial( std::shared_ptr<BVHNode> &root, const std::vector< std::shared_ptr<BaseObject> > &objects ){

	n_nodes = 1;

	// Get all the primitives in the domain and start construction
	std::vector< std::shared_ptr<BaseObject> > prims;
	for( int i=0; i<objects.size(); ++i ){ objects[i]->get_primitives( prims ); }
	std::vector< int > queue( prims.size() );
	std::iota( std::begin(queue), std::end(queue), 0 );
	root->spatial_split( prims, queue, 0, 10000 );

	return n_nodes;

	std::cout << "Object Median BVH made " << n_nodes << " nodes for " << prims.size() << " primitives." << std::endl;
}

