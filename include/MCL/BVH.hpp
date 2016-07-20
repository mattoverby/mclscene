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
#include <chrono>
#include <bitset>


namespace mcl {

typedef long morton_type;
typedef unsigned long long morton_encode_type;

namespace helper {
	static inline trimesh::point face_center( const trimesh::TriMesh::Face &f, const std::vector<trimesh::point> &vertices ){
		return (vertices[ f[0] ]+vertices[ f[1] ]+vertices[ f[2] ])/3.f;
	}

	// use: bool is_one = helper::check_bit( myInt, bit_position );
	static inline bool check_bit( morton_type variable, int bit ){
		assert( bit >=0 && bit < sizeof(morton_type)*8 );
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



class BVHNode {
public:
	BVHNode() : aabb( new AABB ), m_split(0) { left_child=NULL; right_child=NULL; }

	std::shared_ptr<BVHNode> left_child;
	std::shared_ptr<BVHNode> right_child;
	std::shared_ptr<AABB> aabb;
	std::vector< std::shared_ptr<BaseObject> > m_objects; // empty unless a leaf node

	int m_split; // split axis, used for Object Median BVH build.

	bool is_leaf() const { return m_objects.size()>0; }

	void get_edges( std::vector<trimesh::vec> &edges ); // for visual debugging
	void bounds( trimesh::vec &bmin, trimesh::vec &bmax ){ bmin=aabb->min; bmax=aabb->max; }

	//
	//	Split functions
	//

	// Object Median split, round robin axis
	void spatial_split( const std::vector< std::shared_ptr<BaseObject> > &objects,
		const std::vector< int > &queue, const int split_axis, const int max_depth );

	// Use the parallel sorting construction (Lauterbach et al. 2009)
	void lbvh_split( const int bit, const std::vector< std::shared_ptr<BaseObject> > &prims,
		const std::vector< std::pair< morton_type, int > > &morton_codes, const int max_depth );

};


class BVHBuilder {
public:
	static int make_tree_lbvh( std::shared_ptr<BVHNode> &root, const std::vector< std::shared_ptr<BaseObject> > &objects, int max_depth=10 ); // returns num nodes in tree
	static int make_tree_spatial( std::shared_ptr<BVHNode> &root, const std::vector< std::shared_ptr<BaseObject> > &objects, int max_depth=10 ); // returns num nodes in tree
};


class BVHTraversal {
public:
	static bool closest_hit( const std::shared_ptr<BVHNode> node, const intersect::Ray &ray, intersect::Payload &payload );
	static bool any_hit( const std::shared_ptr<BVHNode> node, const intersect::Ray &ray, intersect::Payload &payload );
};


} // end namespace mcl



#endif
