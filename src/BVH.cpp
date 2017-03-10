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

#include "MCL/BVH.hpp"
#include <chrono>
#include <bitset>
#include "MCL/Projection.hpp"

using namespace mcl;

namespace helper {
	// use: bool is_one = helper::check_bit( myInt, bit_position );
	static inline bool check_bit( BVHBuilder::morton_type variable, int bit ){
		std::bitset<sizeof(BVHBuilder::morton_type)*8> bs(variable);
		return ( bs[bit]==1 );
	}
}

static inline BVHBuilder::morton_type morton_encode(const BVHBuilder::morton_encode_type x, const BVHBuilder::morton_encode_type y, const BVHBuilder::morton_encode_type z){
	int n_iters = sizeof(BVHBuilder::morton_type)*8;
	// Step through the bits and assign them. The x2 for i is required to round-robinish interleaving
	// and differs from typical morton encoding. Without them, I got thin slices along the x and z axes.
	BVHBuilder::morton_type result = 0;
	for( BVHBuilder::morton_type i = 0; i<n_iters; ++i ){
		result |= (x & (BVHBuilder::morton_type(1) << i)) << i*2
			| (y & (BVHBuilder::morton_type(1) << i)) << (i*2 + 1)
			| (z & (BVHBuilder::morton_type(1) << i)) << (i*2 + 2);
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
	if( root == nullptr ){ root = new BVHNode(); }

	// Get all the primitives in the domain
	std::vector< std::shared_ptr<BaseObject> > prims;
	prims.reserve( n_nodes ); // only helpful if we're recreating

	n_nodes = 1;
	avg_balance = 0.f;
	num_avg_balance = 0;

	for( int i=0; i<objects.size(); ++i ){ objects[i]->get_primitives( prims ); }
	if( prims.size()==0 ){ return 1; }

	// Compute centroids
	std::vector< Vec3f > centroids( prims.size() );
	AABB world_aabb;
	for( int i=0; i<prims.size(); ++i ){
		Vec3f bmin, bmax; prims[i]->get_bounds( bmin, bmax );
		world_aabb += bmin; world_aabb += bmax;
		centroids[i]=( (bmin+bmax)*0.5f );
	}

	double max_scaled = 1024.f;
	Vec3f world_min( world_aabb.min );
	Vec3f world_max( world_aabb.max );
	Vec3f world_len = (world_max-world_min) * (1.0/max_scaled);

	// Assign morton codes
	std::vector< std::pair< morton_type, int > > morton_codes( prims.size() );
#pragma omp parallel for
	for( int i=0; i<prims.size(); ++i ){

		// Scale the centroid to a value between 0 and max_scaled and convert to integer.
		Vec3f cent = centroids[i];
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
		Vec3f bmin, bmax;
		node->m_objects[i]->get_bounds( bmin, bmax );
		*(node->aabb) += bmin; *(node->aabb) += bmax;
	}
	if( node->left_child != nullptr ){ *(node->aabb) += *(node->left_child->aabb); }
	if( node->right_child != nullptr ){ *(node->aabb) += *(node->right_child->aabb); }

}



int BVHBuilder::make_tree_spatial( BVHNode *root, const std::vector< std::shared_ptr<BaseObject> > &objects, int max_depth ){

	std::chrono::time_point<std::chrono::system_clock> start, end;
	start = std::chrono::system_clock::now();
	if( root == nullptr ){ root = new BVHNode(); }
	else{
		if( root->left_child != nullptr ){ delete root->left_child; root->left_child = nullptr; }
		if( root->right_child != nullptr){ delete root->right_child; root->right_child = nullptr; }
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

	// Create the aabb
	std::vector< Vec3f > obj_centers( queue.size() ); // store the centers for later lookup
	for( int i=0; i<queue.size(); ++i ){
		Vec3f bmin, bmax; prims[ queue[i] ]->get_bounds( bmin, bmax );
		*(node->aabb) += bmin; *(node->aabb) += bmax;
		obj_centers[i] = Vec3f( (bmin+bmax)*0.5 );
	}
	Vec3f center = node->aabb->center();

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


int BVHBuilder::make_tree_spatial_dbl( BVHNode *root, Eigen::VectorXd *vertices, Eigen::VectorXi *faces, int max_depth ){

	if( root == nullptr ){ root = new BVHNode(); }
	else{
		if( root->left_child != nullptr ){ delete root->left_child; root->left_child = nullptr; }
		if( root->right_child != nullptr){ delete root->right_child; root->right_child = nullptr; }
		root->vertices = nullptr;
		root->face_indices.clear();
		root->aabb->valid = false;
	}

	n_nodes = 1;
	if( faces->size()==0 ){ return 1; }

	std::vector< int > queue( faces->size()/3 );
	std::iota( std::begin(queue), std::end(queue), 0 );
	spatial_split( root, vertices, faces, queue, 0, max_depth );

	return n_nodes;

} // end make spatial tree with double


void BVHBuilder::spatial_split( BVHNode *node, Eigen::VectorXd *vertices, Eigen::VectorXi *faces,
	const std::vector<int> &queue, const int split_axis, const int max_depth ) {

	// Create the aabb
	int n_queue = queue.size();
	std::vector< Vec3d > obj_centers( n_queue, Vec3d(0,0,0) ); // store the centers for later lookup
	for( int i=0; i<n_queue; ++i ){
		int idx = queue[i];
		const Vec3i &f = faces->segment<3>(idx*3);
		Vec3d p0 = vertices->segment<3>(f[0]*3);
		Vec3d p1 = vertices->segment<3>(f[1]*3);
		Vec3d p2 = vertices->segment<3>(f[2]*3);
		obj_centers[i] = ( p0 + p1 + p2 )*0.5;
		*node->aabb += p0.cast<float>();
		*node->aabb += p1.cast<float>();
		*node->aabb += p2.cast<float>();
	}
	Vec3d center = node->aabb->center().cast<double>();

	// See if we're done creating nodes
	if( queue.size()==0 ){ return; }
	else if( queue.size()==1 || max_depth <= 0 ){
		node->vertices = vertices;
		node->faces = faces;
		node->face_indices.reserve( queue.size() );
		for( int i=0; i<queue.size(); ++i ){
			node->face_indices.push_back(queue[i]);
		}
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
	spatial_split( node->left_child, vertices, faces, left_queue, ((split_axis+1)%3), max_depth-1 );
	spatial_split( node->right_child, vertices, faces, right_queue, ((split_axis+1)%3), max_depth-1 );
	n_nodes += 2;

} // end build spatial split tree


void BVHBuilder::update( BVHNode *node ){

	node->aabb->valid = false;

	if( node->left_child != nullptr ){
		update( node->left_child );
		*(node->aabb) += *(node->left_child->aabb);
	}
	if( node->right_child != nullptr ){
		update( node->right_child );
		*(node->aabb) += *(node->left_child->aabb);
	}

	for( int i=0; i<node->m_objects.size(); ++i ){
		Vec3f min, max;
		node->m_objects[i]->get_bounds(min,max);
		*(node->aabb) += min;
		*(node->aabb) += max;
	}

	int n_faces = node->face_indices.size();
	for( int i=0; i<n_faces; ++i ){
		int idx = node->face_indices[i];
		const Vec3i &f = node->faces->segment<3>(idx*3);
		*(node->aabb) += node->vertices->segment<3>(f[0]*3).cast<float>();
		*(node->aabb) += node->vertices->segment<3>(f[1]*3).cast<float>();
		*(node->aabb) += node->vertices->segment<3>(f[2]*3).cast<float>();
	}

} // end update bvh

//
//	BVH Traversal
//


bool BVHTraversal::closest_hit( const BVHNode* node, const raycast::Ray *ray, raycast::Payload *payload, std::shared_ptr<BaseObject> *obj ){

	// See if we hit the box
	if( !raycast::ray_aabb<float>( ray, node->aabb->min, node->aabb->max, payload ) ){ return false; }

	// See if there are children to intersect
	bool left_hit=false, right_hit=false;
	if( node->left_child != nullptr ){ left_hit = BVHTraversal::closest_hit( node->left_child, ray, payload, obj ); }
	if( node->right_child != nullptr ){ right_hit = BVHTraversal::closest_hit( node->right_child, ray, payload, obj ); }
	if( left_hit || right_hit ){ return true; }

	// Loop over objects stored on this bvh node
	bool obj_hit = false;
	for( int i=0; i<node->m_objects.size(); ++i ){
		if( node->m_objects[i]->ray_intersect( ray, payload ) ){ obj=&(node->m_objects[i]); obj_hit=true; }
	}
	return obj_hit;

} // end ray intersect


bool BVHTraversal::any_hit( const BVHNode* node, const raycast::Ray *ray, raycast::Payload *payload ){

	// See if we hit the box
	if( !raycast::ray_aabb<float>( ray, node->aabb->min, node->aabb->max, payload ) ){ return false; }

	// See if there are children to intersect
	if( node->left_child != nullptr ){ if( BVHTraversal::any_hit( node->left_child, ray, payload ) ){ return true; } }
	if( node->right_child != nullptr ){ if( BVHTraversal::any_hit( node->right_child, ray, payload ) ){ return true; } }

	// Otherwise it's a leaf node, check objects
	for( int i=0; i<node->m_objects.size(); ++i ){
		if( node->m_objects[i]->ray_intersect( ray, payload ) ){ return true; }
	}

	return false;

} // end ray intersect


bool BVHTraversal::closest_hit_dbl( const BVHNode *node, const raycast::rtRay<double> *ray,
	raycast::rtPayload<double> *payload, Vec2i skip_stride, Vec3i *face_hit, Vec3i *closest_face ){

	if( !raycast::ray_aabb<double>( ray, node->aabb->min.cast<double>(), node->aabb->max.cast<double>(), payload ) ){ return false; }

	// See if there are children to intersect
	bool left_hit=false, right_hit=false;
	if( node->left_child != nullptr ){ left_hit = BVHTraversal::closest_hit_dbl( node->left_child, ray, payload, skip_stride, face_hit, closest_face ); }
	if( node->right_child != nullptr ){ right_hit = BVHTraversal::closest_hit_dbl( node->right_child, ray, payload, skip_stride, face_hit, closest_face ); }
	if( left_hit || right_hit ){ return true; }

	// Loop over objects stored on this bvh node
	bool obj_hit = false;
	const int n_faces = node->face_indices.size();
	for( int i=0; i<n_faces; ++i ){
		int idx = node->face_indices[i];
		if( idx >= skip_stride[0] && idx < skip_stride[1] ){ continue; }
		const Vec3i &f = node->faces->segment<3>(idx*3);
		const Vec3d &p0 = node->vertices->segment<3>(f[0]*3);
		const Vec3d &p1 = node->vertices->segment<3>(f[1]*3);
		const Vec3d &p2 = node->vertices->segment<3>(f[2]*3);
		if( raycast::ray_triangle<double>( ray, p0, p1, p2, payload ) ){
			obj_hit = true;
			if( face_hit != nullptr ){ *face_hit = f; }
		}
	}

	return obj_hit;

} // return closest hit double


void BVHNode::get_edges( std::vector<Vec3f> &edges, bool add_children ){

	{
		Vec3f min = aabb->min;
		Vec3f max = aabb->max;

		// Bottom quad
		Vec3f a = min;
		Vec3f b( max[0], min[1], min[2] );
		Vec3f c( max[0], min[1], max[2] );
		Vec3f d( min[0], min[1], max[2] );
		// Top quad
		Vec3f e( min[0], max[1], min[2] );
		Vec3f f( max[0], max[1], min[2] );
		Vec3f g = max;
		Vec3f h( min[0], max[1], max[2] );

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

	if( left_child != nullptr && add_children ){ left_child->get_edges( edges ); }
	if( right_child != nullptr && add_children ){ right_child->get_edges( edges ); }
}
