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
#include "MCL/Raycast.hpp"
#include "MCL/Object.hpp"

namespace mcl {

//
// KDTree node
//
template <typename T>
class KDNode {
public:
	KDNode() : left_child(nullptr), right_child(nullptr), vertices(nullptr), indices(nullptr), mode(-1) {}
	~KDNode() {
		if(left_child != nullptr){ delete left_child; }
		if(right_child != nullptr){ delete right_child; }
	}

	// Allocated in make_tree:
	KDNode<T>* left_child;
	KDNode<T>* right_child;

	unsigned short axis; // 0, 1, 2
	float median;
	AABB aabb;

	T *vertices; // pointer to data
	int *indices; // index buffer not used for vertex-only (mode=0)
	short mode; // 0=vertices, 1=triangles, 2=quads, 3=tets

	// Object indices stored by this node:
	std::vector<int> m_indices; // indices into vertices, faces, tets, etc...

};
typedef KDNode<double> KDTNode;

//	
//
//
namespace kdtree {

	//
	//	Construction currently uses axis_median_split which has garbage performance but fast build time.
	//

	// Creates a k-d tree from a pool of vertices and triangle faces.
	template <typename T> static inline void make_tree_faces( KDNode<T> *root, T *vertices, int *indices, int n_faces, int max_depth );

	// Creates a k-d tree from vertices (no faces) for scatter-gather.
	template <typename T> static inline void make_tree_vertices( KDNode<T> *root, T *vertices, int n_verts, int max_depth );

	// Creates a k-d tree from a pool of vertices and tetrahedra
	template <typename T> static inline void make_tree_tets( KDNode<T> *root, T *vertices, int *indices, int n_tets, int max_depth );

	// Split the middle of the box into smaller grids. Faster build, less quality tree
	template <typename T> static inline void axis_median_split( KDNode<T> *node, const std::vector<int> &queue, const int split_axis, const int max_depth );

	// Get the n_verts closest vertex indices to a point.
	template <typename T> static inline bool closest_points( const KDNode<T> *node, const Vec3<T> &point, const int n_verts, std::vector<int> &verts );

	// Find the closest point on a triangle, so long as the triangle is within the specifed range(s).
	// If the range vector is left empty, all faces are considered.
	// This allows use to "skip self" or skip other geometry we might want to ignore.
	template <typename T> static inline bool closest_face( const KDNode<T> *node, const Vec3<T> &point, Vec3<T> &projection, Vec3<T> &normal, Vec3i &face, Vec3<T> &bary, 
		const std::vector<Vec2i> &range = std::vector<Vec2i>() );

	// Ray-Scene traversal for closest object
	template <typename T> static inline bool closest_hit( const KDNode<T> *node, const raycast::Ray<T> *ray, raycast::Payload<T> *payload, Vec3i &face,
		const std::vector<Vec2i> &range = std::vector<Vec2i>() );

	// Point-in-tet test for a given vertex. If idx belongs to the tet, it is ignored.
	template <typename T> static inline bool point_in_tet( const KDNode<T> *node, int idx, const Vec3<T> &point );

	// Closest face that doesn't include a particular index (for self collisions)
	template <typename T> static inline bool closest_face_nonself( int idx, const KDNode<T> *node, const Vec3<T> &point, Vec3<T> &projection, Vec3<T> &normal, Vec3i &face, Vec3<T> &bary );

}; // end namespace kdtree

//
//	Implementation
//

template <typename T> static inline void kdtree::make_tree_faces( KDNode<T> *root, T *vertices, int *indices, int n_faces, int max_depth ) {
	if (root == nullptr){ root = new KDNode<T>(); }
	else {
		if (root->left_child != nullptr) {
			delete root->left_child;
			root->left_child = nullptr;
		}
		if (root->right_child != nullptr) {
			delete root->right_child;
			root->right_child = nullptr;
		}
		root->vertices = nullptr;
		root->m_indices.clear();
		root->aabb.valid = false;
	}
	root->mode = 1;
	if( n_faces == 0 ){ return; }

	for (int i = 0; i < n_faces; i++) {
		Vec3i f( indices[i*3+0], indices[i*3+1], indices[i*3+2] );
		for (int j = 0; j < 3; j++) {
			Vec3f x(vertices[f[j]*3], vertices[f[j]*3+1], vertices[f[j]*3+2]);
			root->aabb += x;
		}
	}

	std::vector< int > queue( n_faces );
	std::iota( std::begin(queue), std::end(queue), 0 );
	root->vertices = vertices;
	root->indices = indices;
	axis_median_split(root, queue, 0, max_depth);
}


// Creates a k-d tree from vertices (no faces) for scatter-gather.
template <typename T> static inline void kdtree::make_tree_vertices( KDNode<T> *root, T *vertices, int n_verts, int max_depth ){
	if (root == nullptr){ root = new KDNode<T>(); }
	else {
		if (root->left_child != nullptr) {
			delete root->left_child;
			root->left_child = nullptr;
		}
		if (root->right_child != nullptr) {
			delete root->right_child;
			root->right_child = nullptr;
		}
		root->vertices = nullptr;
		root->indices.clear();
		root->aabb.valid = false;
	}
	root->mode = 0;
	if( n_verts == 0 ){ return; }

	for (int i = 0; i < n_verts; i++) {
		Vec3f x(vertices[i*3], vertices[i*3+1], vertices[i*3+2]);
		root->aabb += x;
	}

	std::vector< int > queue( n_verts );
	std::iota( std::begin(queue), std::end(queue), 0 );
	root->vertices = vertices;
	axis_median_split(root, queue, 0, max_depth);
}


// Creates a k-d tree from tets for point-in-polygon tests
template <typename T> static inline void kdtree::make_tree_tets( KDNode<T> *root, T *vertices, int *indices, int n_tets, int max_depth ){
	if (root == nullptr){ root = new KDNode<T>(); }
	else {
		if (root->left_child != nullptr) {
			delete root->left_child;
			root->left_child = nullptr;
		}
		if (root->right_child != nullptr) {
			delete root->right_child;
			root->right_child = nullptr;
		}
		root->vertices = nullptr;
		root->m_indices.clear();
		root->aabb.valid = false;
	}
	root->mode = 3;
	if( n_tets == 0 ){ return; }

	for (int i = 0; i < n_tets; i++) {
		Vec4i f( indices[i*4+0], indices[i*4+1], indices[i*4+2], indices[i*4+3] );
		for( int j = 0; j < 4; ++j ){
			Vec3f x(vertices[f[j]*3], vertices[f[j]*3+1], vertices[f[j]*3+2]);
			root->aabb += x;
		}
	}

	std::vector< int > queue( n_tets );
	std::iota( std::begin(queue), std::end(queue), 0 );
	root->vertices = vertices;
	root->indices = indices;
	axis_median_split(root, queue, 0, max_depth);
}


template <typename T> static inline void kdtree::axis_median_split( KDNode<T> *node,
	const std::vector<int> &queue, const int split_axis, const int max_depth ){

	// AABB set by parent during build
	node->axis = split_axis;
	node->median = node->aabb.center()[ split_axis ];
	const int min_items = 8; // Don't need to go too deep...

	// See if we're a leaf
	if( queue.size()==0 ){ return; }
	else if( queue.size()<min_items || max_depth <= 0 ){
		node->m_indices.reserve( queue.size() );
		for( int i=0; i<queue.size(); ++i ){ node->m_indices.push_back(queue[i]); }
		return;
	}

	// Otherwise, split geometry into left and right queues
	std::vector<int> left_queue, right_queue;

	// If faces = nullptr, then we're building a KD tree on vertices
	if( node->mode == 0 ){
		for( int i=0; i<queue.size(); ++i ){
			int v_idx = queue[i];
			double vc = node->vertices[v_idx*3 + split_axis];
			if( vc < node->median ){ left_queue.push_back( queue[i] ); }
			else if( vc >= node->median ){ right_queue.push_back( queue[i] ); }
		}
	}

	// Build a kd tree on faces
	else if( node->mode == 1 ){
		// Split faces: Note that a triangle may be added to both children!
		for( int i=0; i<queue.size(); ++i ){
			int f_idx = queue[i]*3;
			const Vec3i f( node->indices[f_idx], node->indices[f_idx+1], node->indices[f_idx+2] );
			AABB faceAABB; // probably not necessary to use an AABB here, but whatevs
			for (int j = 0; j < 3; j++){ faceAABB += Vec3f( node->vertices[f[j]*3], node->vertices[f[j]*3+1], node->vertices[f[j]*3+2] ); }
			if( faceAABB.min[split_axis] < node->median ){ left_queue.push_back( queue[i] ); }
			if( faceAABB.max[split_axis] >= node->median ){ right_queue.push_back( queue[i] ); }
		}
	}

	// Build a kd tree on tets
	else if( node->mode == 3 ){
		// Split faces: Note that a triangle may be added to both children!
		for( int i=0; i<queue.size(); ++i ){
			int t_idx = queue[i]*4;
			const Vec4i f( node->indices[t_idx], node->indices[t_idx+1], node->indices[t_idx+2], node->indices[t_idx+3] );
			AABB tetAABB; // probably not necessary to use an AABB here, but whatevs
			for( int j = 0; j<4; ++j ){ tetAABB += Vec3f( node->vertices[f[j]*3], node->vertices[f[j]*3+1], node->vertices[f[j]*3+2] ); }
			if( tetAABB.min[split_axis] < node->median ){ left_queue.push_back( queue[i] ); }
			if( tetAABB.max[split_axis] >= node->median ){ right_queue.push_back( queue[i] ); }
		}
	}

	// Create the children
	if( left_queue.size() > 0 ){
		node->left_child = new KDNode<T>();
		node->left_child->aabb = node->aabb;
		node->left_child->aabb.max[split_axis] = node->median;
		node->left_child->vertices = node->vertices;
		node->left_child->indices = node->indices;
		node->left_child->mode = node->mode;
		axis_median_split<T>( node->left_child, left_queue, ((split_axis+1)%3), max_depth-1 );
	}
	if( right_queue.size() > 0 ){
		node->right_child = new KDNode<T>();
		node->right_child->aabb = node->aabb;
		node->right_child->aabb.min[split_axis] = node->median;
		node->right_child->vertices = node->vertices;
		node->right_child->indices = node->indices;
		node->right_child->mode = node->mode;
		axis_median_split<T>( node->right_child, right_queue, ((split_axis+1)%3), max_depth-1 );
	}

} // end axis median


template <typename T> static inline bool kdtree::closest_points( const KDNode<T> *node, const Vec3<T> &point, const int n_verts, std::vector<int> &verts ){

	if( n_verts <= 0 ){ std::cerr << "\n**Error: verts vector must be resized before calling kdtree::closest_points" << std::endl; return false; }
	if( n_verts > 1 ){ std::cerr << "\n**Error: I haven't finished multiple verts for kdtree::closest_points" << std::endl; return false; }

	// TODO adapt to faces/tets/etc...
	if( node->mode != 0 ){ return false; }

	// Parse the tree
	bool left_hit = false; bool right_hit = false;
	T pt = point[ node->axis ]; // point
	T med = node->median; // node median

	// TODO sort the verts vector by closest, assuming last is furthest from the point
	Vec3<T> close(9999,9999,9999);
	if( verts.size()>0 ){ 
		int idx = verts.back();
		close = Vec3<T>( node->vertices[idx*3+0], node->vertices[idx*3+1], node->vertices[idx*3+2] );
	}

	// If the point is very close to the median of the node, there is a
	// chance something closer might be on the other branch.

	// Check left?
	if( pt < med && node->left_child != nullptr ){
		left_hit = kdtree::closest_points( node->left_child, point, n_verts, verts );

		T dist = (close-point).norm();
		bool check_both = dist > (pt-med)*(pt-med) && node->right_child != nullptr;
		if( check_both ){ right_hit = kdtree::closest_points( node->right_child, point, n_verts, verts ); }
	}

	// Check right?
	else if( pt >= med && node->right_child != nullptr ){
		right_hit = kdtree::closest_points( node->right_child, point, n_verts, verts );

		T dist = (close-point).norm();
		bool check_both = dist > (pt-med)*(pt-med) && node->left_child != nullptr;
		if( check_both ){ left_hit = kdtree::closest_points( node->left_child, point, n_verts, verts ); }
	}

	// If we traversed the tree, return
	if( left_hit || right_hit ){ return true; }

	// If we're a leaf, find closest point
	// TODO adapt to multiple points
	bool obj_found = false;
	double dist = (point-close).norm(); // current closest obj
	const int ni = node->m_indices.size();
	for( int i=0; i<ni; ++i ){
		int idx = node->indices[i];

		// Extract the face from the buffers
		const Vec3<T> p( node->vertices[idx*3], node->vertices[idx*3+1], node->vertices[idx*3+2] );
		Vec3<T> n = point - p;
		double curr_dist = n.norm();

		// See if this point is closer
		if( curr_dist < dist ){
			obj_found = true;
			verts[0] = idx; // TODO change here
		}
	}

	return obj_found;

} // end closest points


// Point-Scene traversal for closest object to a given point.
// Projection is the point on the object surface, obj is the pointer to the object.
template <typename T> static inline bool kdtree::closest_face( const KDNode<T> *node, const Vec3<T> &point,
	Vec3<T> &projection, Vec3<T> &normal, Vec3i &face, Vec3<T> &bary, const std::vector<Vec2i> &range ){

	// Parse the tree
	bool left_hit = false; bool right_hit = false;
	T pt = point[ node->axis ]; // point
	T med = node->median; // node median

	// If the point is very close to the median of the node, there is a
	// chance something closer might be on the other branch.

	// Check left?
	if( pt < med && node->left_child != nullptr ){
		left_hit = kdtree::closest_face( node->left_child, point, projection, normal, face, bary, range );

		T dist = (projection-point).norm();
		bool check_both = dist > (pt-med)*(pt-med) && node->right_child != nullptr;
		if( check_both ){ right_hit = kdtree::closest_face( node->right_child, point, projection, normal, face, bary, range ); }
	}

	// Check right?
	else if( pt >= med && node->right_child != nullptr ){
		right_hit = kdtree::closest_face( node->right_child, point, projection, normal, face, bary, range );

		T dist = (projection-point).norm();
		bool check_both = dist > (pt-med)*(pt-med) && node->left_child != nullptr;
		if( check_both ){ left_hit = kdtree::closest_face( node->left_child, point, projection, normal, face, bary, range ); }
	}

	// If we traversed the tree, return
	if( left_hit || right_hit ){ return true; }

	// If we're a leaf, find closest projection
	bool obj_found = false;
	double dist = (point-projection).norm(); // current closest obj
	const int nf = node->m_indices.size();
	for( int i=0; i<nf; ++i ){
		int fidx = node->m_indices[i];

		// See if we should skip this face
		bool face_in_range = false;
		for( int j=0; j<range.size(); ++j ){ if( fidx >= range[j][0] && fidx < range[j][1] ){ face_in_range=true; } }
		if( !face_in_range && range.size()>0 ){ continue; }

		// Extract the face from the buffers
		const Vec3i f( node->indices[fidx*3+0], node->indices[fidx*3+1], node->indices[fidx*3+2] );
		const Vec3<T> p0( node->vertices[f[0]*3], node->vertices[f[0]*3+1], node->vertices[f[0]*3+2] );
		const Vec3<T> p1( node->vertices[f[1]*3], node->vertices[f[1]*3+1], node->vertices[f[1]*3+2] );
		const Vec3<T> p2( node->vertices[f[2]*3], node->vertices[f[2]*3+1], node->vertices[f[2]*3+2] );
		const Vec3<T> p = projection::point_triangle<T>( point, p0, p1, p2 );
		Vec3<T> n = point - p;
		double curr_dist = n.norm();

		// See if this projection is closer
		if( curr_dist < dist ){
			bary = mcl::barycoords<T>( p, p0, p1, p2 );
			projection = p;
			obj_found = true;
			dist = curr_dist;
			face = f;
			Vec3<T> e0 = p1 - p0;
			Vec3<T> e1 = p2 - p0;
			normal = e0.cross(e1);
			normal.normalize();
		}
	}

	return obj_found;

} // end closest object


template <typename T> static inline bool kdtree::closest_face_nonself( int idx, const KDNode<T> *node, const Vec3<T> &point, Vec3<T> &projection, Vec3<T> &normal, Vec3i &face, Vec3<T> &bary ){

	// Parse the tree
	bool left_hit = false; bool right_hit = false;
	T pt = point[ node->axis ]; // point
	T med = node->median; // node median

	// If the point is very close to the median of the node, there is a
	// chance something closer might be on the other branch.
	if( pt < med && node->left_child != nullptr ){
		left_hit = kdtree::closest_face_nonself( idx, node->left_child, point, projection, normal, face, bary );
		T dist = (projection-point).norm();
		bool check_both = dist > (pt-med)*(pt-med) && node->right_child != nullptr;
		if( check_both ){ right_hit = kdtree::closest_face_nonself( idx, node->right_child, point, projection, normal, face, bary ); }
	}
	else if( pt >= med && node->right_child != nullptr ){
		right_hit = kdtree::closest_face_nonself( idx, node->right_child, point, projection, normal, face, bary );
		T dist = (projection-point).norm();
		bool check_both = dist > (pt-med)*(pt-med) && node->left_child != nullptr;
		if( check_both ){ left_hit = kdtree::closest_face_nonself( idx, node->left_child, point, projection, normal, face, bary ); }
	}
	if( left_hit || right_hit ){ return true; }

	// If we're a leaf, find closest projection
	bool obj_found = false;
	double dist = (point-projection).norm(); // current closest obj
	const int nf = node->m_indices.size();
	for( int i=0; i<nf; ++i ){
		int fidx = node->m_indices[i];
		const Vec3i f( node->indices[fidx*3+0], node->indices[fidx*3+1], node->indices[fidx*3+2] );

		// See if we should skip this face
		if( f[0]==idx || f[1]==idx || f[2]==idx ){ continue; }

		const Vec3<T> p0( node->vertices[f[0]*3], node->vertices[f[0]*3+1], node->vertices[f[0]*3+2] );
		const Vec3<T> p1( node->vertices[f[1]*3], node->vertices[f[1]*3+1], node->vertices[f[1]*3+2] );
		const Vec3<T> p2( node->vertices[f[2]*3], node->vertices[f[2]*3+1], node->vertices[f[2]*3+2] );
		const Vec3<T> p = projection::point_triangle<T>( point, p0, p1, p2 );
		Vec3<T> n = point - p;
		double curr_dist = n.norm();

		// See if this projection is closer
		if( curr_dist < dist ){
			bary = mcl::barycoords<T>( p, p0, p1, p2 );
			projection = p;
			obj_found = true;
			dist = curr_dist;
			face = f;
			Vec3<T> e0 = p1 - p0;
			Vec3<T> e1 = p2 - p0;
			normal = e0.cross(e1);
			normal.normalize();
		}
	}

	return obj_found;
}



// Ray-Scene traversal for closest object
template <typename T> static inline bool kdtree::closest_hit( const KDNode<T> *node, const raycast::Ray<T> *ray, raycast::Payload<T> *payload, Vec3i &face,
	const std::vector<Vec2i> &range ){

	// See if we hit the box (Eigen's cast<T>() doesn't work here?)
	Vec3<T> bmin( node->aabb.min[0], node->aabb.min[1], node->aabb.min[2] );
	Vec3<T> bmax( node->aabb.max[0], node->aabb.max[1], node->aabb.max[2] );
	if( !raycast::ray_aabb<T>( ray, bmin, bmax, payload ) ){ return false; }

	// See if there are children to intersect
	bool left_hit=false, right_hit=false;
	if( node->left_child != nullptr ){ left_hit = kdtree::closest_hit( node->left_child, ray, payload, face, range ); }
	if( node->right_child != nullptr ){ right_hit = kdtree::closest_hit( node->right_child, ray, payload, face, range ); }
	if( left_hit || right_hit ){ return true; }

	// For a leaf node
	bool obj_hit = false;
	for( int i=0; i<node->m_indices.size(); ++i ){
		int fidx = node->m_indices[i];

		// See if we should skip this face
		bool face_in_range = false;
		for( int j=0; j<range.size(); ++j ){ if( fidx >= range[j][0] && fidx < range[j][1] ){ face_in_range=true; } }
		if( !face_in_range && range.size()>0 ){ continue; }

		// Extract the face from the buffers
		const Vec3i f( node->indices[fidx*3+0], node->indices[fidx*3+1], node->indices[fidx*3+2] );
		const Vec3<T> p0( node->vertices[f[0]*3+0], node->vertices[f[0]*3+1], node->vertices[f[0]*3+2] );
		const Vec3<T> p1( node->vertices[f[1]*3+0], node->vertices[f[1]*3+1], node->vertices[f[1]*3+2] );
		const Vec3<T> p2( node->vertices[f[2]*3+0], node->vertices[f[2]*3+1], node->vertices[f[2]*3+2] );

		// raycast::ray_triangle sets everything except face index
		bool curr_hit = raycast::ray_triangle( ray, p0, p1, p2, payload );
		if( curr_hit ){
			obj_hit = true;
			face = f;
		}
	}

	return obj_hit;

} // end closest hit

// Point in tet test
template <typename T> static inline bool kdtree::point_in_tet( const KDNode<T> *node, int idx, const Vec3<T> &point ){

	if( node->mode != 3 ){ return false; }

	// See if we're inside the box
	Vec3<T> bmin( node->aabb.min[0], node->aabb.min[1], node->aabb.min[2] );
	Vec3<T> bmax( node->aabb.max[0], node->aabb.max[1], node->aabb.max[2] );
	if( !projection::point_in_aabb<T>( point, bmin, bmax ) ){ return false; }

	// See if there are children to intersect
	bool left_hit=false, right_hit=false;
	if( node->left_child != nullptr ){ left_hit = kdtree::point_in_tet( node->left_child, idx, point ); }
	if( node->right_child != nullptr ){ right_hit = kdtree::point_in_tet( node->right_child, idx, point ); }
	if( left_hit || right_hit ){ return true; }

	// For a leaf node
	for( int i=0; i<node->m_indices.size(); ++i ){
		int fidx = node->m_indices[i];

		// See if we should skip this face
		const Vec4i t( node->indices[fidx*4+0], node->indices[fidx*4+1], node->indices[fidx*4+2], node->indices[fidx*4+3] );

		// Extract the face from the buffers
		bool skip = false;
		for( int j=0; j<4; ++j ){ if(idx==t[j]){ skip==true; } }
		if( skip ){ continue; }

		const Vec3<T> p0( node->vertices[t[0]*3+0], node->vertices[t[0]*3+1], node->vertices[t[0]*3+2] );
		const Vec3<T> p1( node->vertices[t[1]*3+0], node->vertices[t[1]*3+1], node->vertices[t[1]*3+2] );
		const Vec3<T> p2( node->vertices[t[2]*3+0], node->vertices[t[2]*3+1], node->vertices[t[2]*3+2] );
		const Vec3<T> p3( node->vertices[t[3]*3+0], node->vertices[t[3]*3+1], node->vertices[t[3]*3+2] );

		if( projection::point_in_tet( point, p0, p1, p2, p3 ) ){ return true; }
	}

	return false;

} // end point in tet

} // end namespace mcl

#endif

