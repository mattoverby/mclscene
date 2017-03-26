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

namespace mcl {

//
// KDTree node
//
template <typename T>
class KDNode {
public:
	KDNode() : left_child(nullptr), right_child(nullptr), vertices(nullptr), faces(nullptr) {}
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
	int *faces; // pointer to data (if nullptr, indices points to vertices)
	std::vector<int> indices; // indices into faces OR vertices
};
typedef KDNode<double> KDTNode;


//	
//
//
namespace kdtree {

	// Creates a k-d tree from a pool of vertices and triangle faces.
	template <typename T> static inline void make_tree( KDNode<T> *root, T *vertices, int *faces, int n_faces, int max_depth );

	// Creates a k-d tree from vertices (no faces) for scatter-gather.
	template <typename T> static inline void make_tree( KDNode<T> *root, T *vertices, int n_verts, int max_depth );

	// Object-median split
//	template <typename T> static inline void object_median_split( KDNode<T> *node, const std::vector<int> &queue, const int split_axis, const int max_depth );

	// Split the middle of the box into smaller grids. Faster build, less quality tree
	template <typename T> static inline void axis_median_split( KDNode<T> *node, const std::vector<int> &queue, const int split_axis, const int max_depth );

	// Get the n_verts closest vertex indices to a point.
	template <typename T> static inline bool closest_points( const KDNode<T> *node, const Vec3<T> &point, const int n_verts, std::vector<int> &verts );

	// Find the closest point on a triangle, so long as the triangle is within the specifed range(s).
	// If the range vector is left empty, all faces are considered.
	// This allows use to "skip self" or skip other geometry we might want to ignore.
	template <typename T> static inline bool closest_object( const KDNode<T> *node, const Vec3<T> &point, Vec3<T> &projection, Vec3<T> &normal, Vec3i &face, Vec3<T> &bary, 
		const std::vector<Vec2i> &range = std::vector<Vec2i>() );

	// Ray-Scene traversal for closest object
	template <typename T> static inline bool closest_hit( const KDNode<T> *node, const raycast::rtRay<T> *ray, raycast::rtPayload<T> *payload, Vec3i &face,
		const std::vector<Vec2i> &range = std::vector<Vec2i>() );


	template <typename T> static inline bool collision( const KDNode<T> *node, const Vec3<T> &point, Vec3<T> &projection, Vec3<T> &normal, Vec3i &face, Vec3<T> &bary,
		const std::vector<Vec2i> &range = std::vector<Vec2i>() );

	static inline bool closest_object( const KDTNode *node, const Vec2i stride, const bool toSkip, double& closest, const Vec3d &point, Vec3d &projection, Vec3d &norm, int* tri_idx);
	static inline bool ray_intersection( const KDTNode *node, const mcl::raycast::rtRay<double> ray, const Vec2i skip_stride, double& t_max, Vec3d &projection, Vec3d &norm, int* tri_idx); 

	static inline bool intersect_with_box(const AABB& aabb, const mcl::raycast::rtRay<double> ray, const double t_max, Vec3d& projection);
	static inline bool close_to_box(const AABB& aabb, const Vec3d &point, const double closest);

}; // end namespace kdtree

//
//	Implementation
//

template <typename T> static inline void kdtree::make_tree( KDNode<T> *root, T *vertices, int *faces, int n_faces, int max_depth ) {
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
	for (int i = 0; i < n_faces; i++) {
		Vec3i f( faces[i*3+0], faces[i*3+1], faces[i*3+2] );
		for (int j = 0; j < 3; j++) {
			Vec3f x(vertices[f[j]*3], vertices[f[j]*3+1], vertices[f[j]*3+2]);
			root->aabb += x;
		}
	}
	std::vector< int > queue( n_faces );
	std::iota( std::begin(queue), std::end(queue), 0 );
	root->vertices = vertices;
	root->faces = faces;
	axis_median_split(root, queue, 0, max_depth);
}


// Creates a k-d tree from vertices (no faces) for scatter-gather.
template <typename T> static inline void kdtree::make_tree( KDNode<T> *root, T *vertices, int n_verts, int max_depth ){
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

	for (int i = 0; i < n_verts; i++) {
		Vec3f x(vertices[i*3], vertices[i*3+1], vertices[i*3+2]);
		root->aabb += x;
	}

	std::vector< int > queue( n_verts );
	std::iota( std::begin(queue), std::end(queue), 0 );
	root->vertices = vertices;
	axis_median_split(root, queue, 0, max_depth);
}

/*
// object median split
static inline void kdtree::object_median_split( KDTNode *node, const std::vector< std::shared_ptr<BaseObject> > &prims,
	const std::vector< int > &queue, const int split_axis, const int max_depth ){

	// AABB aabb; // Create an AABB to compute center
	std::vector< float > obj_min( queue.size() );
	std::vector< float > obj_max( queue.size() );
	for( int i=0; i<queue.size(); ++i ){
		Vec3f bmin, bmax;
		prims[ queue[i] ]->get_bounds( bmin, bmax );
		obj_min[i] = bmin[ split_axis ];
		obj_max[i] = bmax[ split_axis ];
		node->aabb += bmin; node->aabb += bmax;
	}

	// Set KDTree node details
	node->axis = split_axis;
	node->median = node->aabb.center()[ split_axis ];

	// See if we're a leaf
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
		if( obj_min[i] < node->median ){ left_queue.push_back( queue[i] ); }
		if( obj_max[i] >= node->median ){ right_queue.push_back( queue[i] ); }
	}

	// Create the children
	node->left_child = new KDTNode();
	node->right_child = new KDTNode();
	object_median_split( node->left_child, prims, left_queue, ((split_axis+1)%3), max_depth-1 );
	object_median_split( node->right_child, prims, right_queue, ((split_axis+1)%3), max_depth-1 );
}
*/


template <typename T> static inline void kdtree::axis_median_split( KDNode<T> *node,
	const std::vector<int> &queue, const int split_axis, const int max_depth ){

	// AABB set by parent during build
	node->axis = split_axis;
	node->median = node->aabb.center()[ split_axis ];
	const int min_items = 8; // Don't need to go too deep...

	// See if we're a leaf
	if( queue.size()==0 ){ return; }
	else if( queue.size()<min_items || max_depth <= 0 ){
		node->indices.reserve( queue.size() );
		for( int i=0; i<queue.size(); ++i ){ node->indices.push_back(queue[i]); }
		return;
	}

	// Otherwise, split geometry into left and right queues
	std::vector<int> left_queue, right_queue;

	// If faces = nullptr, then we're building a KD tree on vertices
	if( node->faces == nullptr ){
		for( int i=0; i<queue.size(); ++i ){
			int v_idx = queue[i];
			double vc = node->vertices[v_idx*3 + split_axis];
			if( vc < node->median ){ left_queue.push_back( queue[i] ); }
			else if( vc >= node->median ){ right_queue.push_back( queue[i] ); }
		}
	}

	// Otherwise, build a kd tree on faces
	else {
		// Split faces: Note that a triangle may be added to both children!
		for( int i=0; i<queue.size(); ++i ){
			int f_idx = queue[i]*3;
			const Vec3i f( node->faces[f_idx], node->faces[f_idx+1], node->faces[f_idx+2] );
			AABB faceAABB; // probably not necessary to use an AABB here, but whatevs
			for (int j = 0; j < 3; j++){ faceAABB += Vec3f( node->vertices[f[j]*3], node->vertices[f[j]*3+1], node->vertices[f[j]*3+2] ); }
			if( faceAABB.min[split_axis] < node->median ){ left_queue.push_back( queue[i] ); }
			if( faceAABB.max[split_axis] >= node->median ){ right_queue.push_back( queue[i] ); }
		}
	}

	// Create the children
	if( left_queue.size() > 0 ){
		node->left_child = new KDNode<T>();
		node->left_child->aabb = node->aabb;
		node->left_child->aabb.max[split_axis] = node->median;
		node->left_child->vertices = node->vertices;
		node->left_child->faces = node->faces;
		axis_median_split<T>( node->left_child, left_queue, ((split_axis+1)%3), max_depth-1 );
	}
	if( right_queue.size() > 0 ){
		node->right_child = new KDNode<T>();
		node->right_child->aabb = node->aabb;
		node->right_child->aabb.min[split_axis] = node->median;
		node->right_child->vertices = node->vertices;
		node->right_child->faces = node->faces;
		axis_median_split<T>( node->right_child, right_queue, ((split_axis+1)%3), max_depth-1 );
	}

} // end axis median


template <typename T> static inline bool kdtree::closest_points( const KDNode<T> *node, const Vec3<T> &point, const int n_verts, std::vector<int> &verts ){

	if( n_verts <= 0 ){ std::cerr << "\n**Error: verts vector must be resized before calling kdtree::closest_points" << std::endl; return false; }
	if( n_verts > 1 ){ std::cerr << "\n**Error: I haven't finished multiple verts for kdtree::closest_points" << std::endl; return false; }

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
	const int ni = node->indices.size();
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
template <typename T> static inline bool kdtree::closest_object( const KDNode<T> *node, const Vec3<T> &point,
	Vec3<T> &projection, Vec3<T> &normal, Vec3i &face, Vec3<T> &bary, const std::vector<Vec2i> &range ){

	// Parse the tree
	bool left_hit = false; bool right_hit = false;
	T pt = point[ node->axis ]; // point
	T med = node->median; // node median

	// If the point is very close to the median of the node, there is a
	// chance something closer might be on the other branch.

	// Check left?
	if( pt < med && node->left_child != nullptr ){
		left_hit = kdtree::closest_object( node->left_child, point, projection, normal, face, bary, range );

		T dist = (projection-point).norm();
		bool check_both = dist > (pt-med)*(pt-med) && node->right_child != nullptr;
		if( check_both ){ right_hit = kdtree::closest_object( node->right_child, point, projection, normal, face, bary, range ); }
	}

	// Check right?
	else if( pt >= med && node->right_child != nullptr ){
		right_hit = kdtree::closest_object( node->right_child, point, projection, normal, face, bary, range );

		T dist = (projection-point).norm();
		bool check_both = dist > (pt-med)*(pt-med) && node->left_child != nullptr;
		if( check_both ){ left_hit = kdtree::closest_object( node->left_child, point, projection, normal, face, bary, range ); }
	}

	// If we traversed the tree, return
	if( left_hit || right_hit ){ return true; }

	// If we're a leaf, find closest projection
	bool obj_found = false;
	double dist = (point-projection).norm(); // current closest obj
	const int nf = node->indices.size();
	for( int i=0; i<nf; ++i ){
		int fidx = node->indices[i];

		// See if we should skip this face
		bool face_in_range = false;
		for( int j=0; j<range.size(); ++j ){ if( fidx >= range[j][0] && fidx < range[j][1] ){ face_in_range=true; } }
		if( !face_in_range && range.size()>0 ){ continue; }

		// Extract the face from the buffers
		const Vec3i f( node->faces[fidx*3+0], node->faces[fidx*3+1], node->faces[fidx*3+2] );
		const Vec3<T> p0( node->vertices[f[0]*3], node->vertices[f[0]*3+1], node->vertices[f[0]*3+2] );
		const Vec3<T> p1( node->vertices[f[1]*3], node->vertices[f[1]*3+1], node->vertices[f[1]*3+2] );
		const Vec3<T> p2( node->vertices[f[2]*3], node->vertices[f[2]*3+1], node->vertices[f[2]*3+2] );
		const Vec3<T> p = projection::Triangle<T>( point, p0, p1, p2 );
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


// Ray-Scene traversal for closest object
template <typename T> static inline bool kdtree::closest_hit( const KDNode<T> *node, const raycast::rtRay<T> *ray, raycast::rtPayload<T> *payload, Vec3i &face,
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
	for( int i=0; i<node->indices.size(); ++i ){
		int fidx = node->indices[i];

		// See if we should skip this face
		bool face_in_range = false;
		for( int j=0; j<range.size(); ++j ){ if( fidx >= range[j][0] && fidx < range[j][1] ){ face_in_range=true; } }
		if( !face_in_range && range.size()>0 ){ continue; }

		// Extract the face from the buffers
		const Vec3i f( node->faces[fidx*3+0], node->faces[fidx*3+1], node->faces[fidx*3+2] );
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


static void round_zero( double &val ){
	if( std::abs(val)<std::numeric_limits<float>::min() ){ val=0.0; }
}

// Point-Scene traversal for closest object to a given point.
// Projection is the point on the object surface, obj is the pointer to the object.
template <typename T> static inline bool kdtree::collision( const KDNode<T> *node, const Vec3<T> &point,
	Vec3<T> &projection, Vec3<T> &normal, Vec3i &face, Vec3<T> &bary, const std::vector<Vec2i> &range ){

	// Parse the tree
	bool left_hit = false; bool right_hit = false;
	T pt = point[ node->axis ]; // point
	T med = node->median; // node median

	// If the point is very close to the median of the node, there is a
	// chance something closer might be on the other branch.

	// Check left?
	if( pt < med && node->left_child != nullptr ){
		left_hit = kdtree::collision( node->left_child, point, projection, normal, face, bary, range );

		T dist = (projection-point).norm();
		bool check_both = dist > (pt-med)*(pt-med) && node->right_child != nullptr;
		if( check_both ){ right_hit = kdtree::collision( node->right_child, point, projection, normal, face, bary, range ); }
	}

	// Check right?
	else if( pt >= med && node->right_child != nullptr ){
		right_hit = kdtree::collision( node->right_child, point, projection, normal, face, bary, range );

		T dist = (projection-point).norm();
		bool check_both = dist > (pt-med)*(pt-med) && node->left_child != nullptr;
		if( check_both ){ left_hit = kdtree::collision( node->left_child, point, projection, normal, face, bary, range ); }
	}

	// If we traversed the tree, return
	if( left_hit || right_hit ){ return true; }

	// If we're a leaf, find closest projection
	bool obj_found = false;
	double dist = (point-projection).norm(); // current closest obj
	const int nf = node->indices.size();
	for( int i=0; i<nf; ++i ){
		int fidx = node->indices[i];

		// See if we should skip this face
		bool face_in_range = false;
		for( int j=0; j<range.size(); ++j ){ if( fidx >= range[j][0] && fidx < range[j][1] ){ face_in_range=true; } }
		if( !face_in_range && range.size()>0 ){ continue; }

		// Extract the face from the buffers
		const Vec3i f( node->faces[fidx*3+0], node->faces[fidx*3+1], node->faces[fidx*3+2] );
		const Vec3<T> p0( node->vertices[f[0]*3], node->vertices[f[0]*3+1], node->vertices[f[0]*3+2] );
		const Vec3<T> p1( node->vertices[f[1]*3], node->vertices[f[1]*3+1], node->vertices[f[1]*3+2] );
		const Vec3<T> p2( node->vertices[f[2]*3], node->vertices[f[2]*3+1], node->vertices[f[2]*3+2] );
		const Vec3<T> p = projection::Triangle<T>( point, p0, p1, p2 );
		const Vec3<T> n = p - point;
		const Vec3<T> n2 = p - projection;

		// Compute triangle norm
		const Vec3<T> e0 = p1 - p0;
		const Vec3<T> e1 = p2 - p0;
		Vec3<T> norm = e0.cross(e1);
		norm.normalize();

		double eps = 1e-4;

		// See if we're on the wrong side of the face
		if( n.dot(norm) < -eps ){ continue; }

		// See if we're in the triangle area
		Vec3<T> curr_bary = mcl::barycoords( p, p0, p1, p2 );
		bool in_area = (curr_bary[0]>-eps && curr_bary[1]>-eps && curr_bary[2]>-eps && curr_bary[0]+curr_bary[1]+curr_bary[2]<1.0+eps);
		if( !in_area ){ continue; }

		// See if its closer
		double curr_dist = n.norm();
		if( curr_dist > dist ){ continue; }

		// Closest face, set the payload
		bary = curr_bary;
		projection = p;
		obj_found = true;
		dist = curr_dist;
		face = f;
		normal = norm;
//		normal.normalize();

	}

	return obj_found;

} // end closest object



static inline bool kdtree::ray_intersection( const KDTNode *node, const mcl::raycast::rtRay<double> ray, const Vec2i skip_stride, double& t_max, Vec3d &projection, Vec3d &norm, int* tri_idx) {
/*
	bool left_hit = false, right_hit = false;
	Vec3d box_proj;
	if (node->left_child != nullptr && intersect_with_box(node->left_child->aabb, ray, t_max, box_proj)) {
		left_hit = ray_intersection(node->left_child, ray, skip_stride, t_max, projection, norm, tri_idx);
	} 
	if (node->right_child != nullptr && intersect_with_box(node->right_child->aabb, ray, t_max, box_proj)) {
		right_hit = ray_intersection(node->right_child, ray, skip_stride, t_max, projection, norm, tri_idx);
	}
	if (left_hit || right_hit) {
		return true;
	}

	Vec3d x(ray.origin[0], ray.origin[1], ray.origin[2]);
	Vec3d dir(ray.direction[0], ray.direction[1], ray.direction[2]);


	// For a leaf node
	bool obj_hit = false;
	for( int i=0; i<node->indices.size(); ++i ){
		int idx = node->indices[i];
		if (idx >= skip_stride[0] && idx < skip_stride[1]) {
			continue;
		}
		const Vec3i &f = node->faces->segment<3>(idx*3);
		Vec3d vs[3];
		for (int j = 0; j < 3; j++) {
			Vec3d v_d = node->vertices->segment<3>(f[j]*3);
			vs[j] << v_d[0], v_d[1], v_d[2];
		}
		Vec3d e0 = vs[1] - vs[0];
		Vec3d e1 = vs[0] - vs[2];
		Vec3d curr_n = e1.cross( e0 );

		double d = curr_n.dot(vs[0]);
		double t = (d - curr_n.dot(x))/(curr_n.dot(dir));
		Vec3d temp_proj = x + t*dir;
		double area = e0.cross(e1).norm();
		Vec3d l0 = temp_proj - vs[0];
		Vec3d l1 = temp_proj - vs[1];
		Vec3d l2 = temp_proj - vs[2];
		double alpha = l1.cross(l2).norm()/area;
		double beta = l0.cross(l2).norm()/area;
		double gamma = l0.cross(l1).norm()/area;
		bool bary_test = std::abs(alpha + beta + gamma - 1) < 1e-6;

		// Vec3f e2 = ( 1.0 / curr_n.dot( dir ) ) * ( vs[0] - x );
		// Vec3f inter  = dir.cross( e2 );
		// double beta  = inter.dot( e1 );
		// double gamma = inter.dot( e0 );
		// double alpha = 1.0 - beta - gamma;
		// // alpha = std::abs(alpha) < 1e-6 ? 0 : alpha;
		// beta = abs(beta) < std::numeric_limits<double>::min() ? 0 : beta;
		// gamma = abs(gamma) < std::numeric_limits<double>::min() ? 0 : gamma;
		// double t = curr_n.dot( e2 );
		// bool bary_test = alpha>0.0 && beta>0.0 && gamma>0.0 && (alpha+beta+gamma)<=1.0;

		bool t_test = ( t>0 && t<t_max );
		if (bary_test && t_test) {
			obj_hit = true;
			t_max = t;
			projection = x + t*dir;
			curr_n.normalize();
			norm = curr_n;
			tri_idx[0] = f[0]*3;
			tri_idx[1] = f[1]*3;
			tri_idx[2] = f[2]*3;
		}
	}

	return obj_hit;
*/
}

static inline bool kdtree::close_to_box(const AABB& aabb, const Vec3d &point, const double closest) {
/*
	Vec3f diff1 = point.cast<float>() - aabb.max;
	Vec3f diff2 = aabb.min - point.cast<float>();
	if (diff1[0] <= 0 && diff1[1] <= 0 && diff1[2] <= 0
		&& diff2[0] <= 0 && diff2[1] <= 0 && diff2[2] <= 0) {	// inside the box
		return true;
	}
	double distance;
	Vec3i s1, s2;
	for (int i = 0; i < 3; i++) {
		s1[i] = diff1[i] > 0 ? 1 : 0;
		s2[i] = diff2[i] > 0 ? 1 : 0;
	}
	Vec3i ones(1, 1, 1);
	Vec3d max(aabb.max[0]*s1[0], aabb.max[1]*s1[1], aabb.max[2]*s1[2]);
	Vec3d min(aabb.min[0]*s2[0], aabb.min[1]*s2[1], aabb.min[2]*s2[2]);
	if (ones.dot(s1) + ones.dot(s2) == 3) { // close to corner
		if ((max + min - point).norm() < closest) {
			return true;
		}
	} else if (ones.dot(s1) + ones.dot(s2) == 2) {
		Vec3d d = max + min - point;
		for (int i = 0; i < 3; i++) {
			if ((s1 + s2)[i] == 0) {
				d[i] = 0;
			}
		}
		if (d.norm() < closest) {
			return true;
		}
	} else if (ones.dot(s1) + ones.dot(s2) == 1) {
		for (int i = 0; i < 3; i++) {
			if ((s1 + s2)[i] == 1) {
				if (std::abs(max[i] + min[i] - point[i]) > closest) {
					return true;
				}
			}
		}
	}
	return false;
*/
}

static inline bool kdtree::intersect_with_box(const AABB& aabb, const mcl::raycast::rtRay<double> ray, const double t_max, Vec3d& projection) {
/*
	Vec3d x(ray.origin[0], ray.origin[1], ray.origin[2]);
	Vec3d dir(ray.direction[0], ray.direction[1], ray.direction[2]);
	Vec3d point = x + t_max*dir;

	Vec3f diff1 = x.cast<float>() - aabb.max;
	Vec3f diff2 = aabb.min - x.cast<float>();
	if (diff1[0] <= 0 && diff1[1] <= 0 && diff1[2] <= 0
		&& diff2[0] <= 0 && diff2[1] <= 0 && diff2[2] <= 0) {	// inside the box
		return true;
	}
	for (int a = 0; a < 3; a++) {
		if (diff1[a] > 0) {
			Vec3d n(0, 0, 0);
			n[a] = -1;
			double cos_theta = dir.dot(n);
			double t = diff1[a]/cos_theta;
			if (t < t_max && t > 0) {
				projection = x + t*dir;
				int a1 = (a + 1)%3, a2 = (a + 2)%3;
				double eps = 1e-6;
				if (std::abs(projection[a1] - aabb.max[a1]) <= eps && eps >= std::abs(aabb.min[a1] - projection[a1]) && std::abs(projection[a2] - aabb.max[a2]) <= eps && eps >= std::abs(aabb.min[a2] - projection[a2])) {
					return true;
				}
			}
		}
		if (diff2[a] > 0) {
			Vec3d n(0, 0, 0);
			n[a] = 1;
			double cos_theta = dir.dot(n);
			double t = diff2[a]/cos_theta;
			if (t < t_max && t > 0) {
				projection = x + t*dir;
				int a1 = (a + 1)%3, a2 = (a + 2)%3;
				double eps = 1e-6;
				if (std::abs(projection[a1] - aabb.max[a1]) <= eps && eps >= std::abs(aabb.min[a1] - projection[a1]) && std::abs(projection[a2] - aabb.max[a2]) <= eps && eps >= std::abs(aabb.min[a2] - projection[a2])) {
					return true;
				}
			}
		}
	}
	return false;
*/
}



static inline bool kdtree::closest_object( const KDTNode *node, const Vec2i stride, const bool toSkip, double& closest, const Vec3d &point, Vec3d &projection, Vec3d &norm, int* tri_idx) {
/*
	bool left_hit = false, right_hit = false;
	float pt = point[node->axis];
	float med = node->median;

	if (pt < med && node->left_child != nullptr && close_to_box(node->left_child->aabb, point, closest)) {
		left_hit = closest_object(node->left_child, stride, toSkip, closest, point, projection, norm, tri_idx);
		// float dist = (projection - point).squaredNorm();
		// bool check_both = dist > (pt - med)*(pt - med) && node->right_child != nullptr;
		// if (check_both) {
		// 	right_hit = closest_object(node->right_child, skip_stride, closest, point, projection, norm, tri_idx);
		// }
	} 
	if (pt >= med && node->right_child != nullptr && close_to_box(node->right_child->aabb, point, closest)) {
		right_hit = closest_object(node->right_child, stride, toSkip, closest, point, projection, norm, tri_idx);
		// float dist = (projection - point).squaredNorm();
		// bool check_both = dist > (pt - med)*(pt - med) && node->left_child != nullptr;
		// if (check_both) {
		// 	left_hit = closest_object(node->left_child, skip_stride, closest, point, projection, norm, tri_idx);
		// }
	}

	if (left_hit || right_hit) {
		return true;
	}

	bool obj_hit = false;
	// double dist = (point - projection).squaredNorm();

	for (int i = 0; i < node->indices.size(); i++) {
		int idx = node->indices[i];
		if (toSkip) {
			if (idx >= stride[0] && idx < stride[1]) {
				continue;
			}
		} else {
			if (idx < stride[0] || idx >= stride[1]) {
				continue;
			}
		}
		const Vec3i &f = node->faces->segment<3>(node->indices[i]*3);
		Vec3d vs[3];
		for (int j = 0; j < 3; j++) {
			Vec3d v_d = node->vertices->segment<3>(f[j]*3);
			vs[j] << v_d[0], v_d[1], v_d[2];
		}
		Vec3d p = mcl::projection::point_triangle(point, vs[0], vs[1], vs[2]);
		double curr_dist = (p - point).squaredNorm();
		Vec3d e0 = vs[1] - vs[0];
		Vec3d e1 = vs[0] - vs[2];
		Vec3d curr_n = e1.cross(e0);
		if (curr_dist < closest) {
			projection = p;
			obj_hit = true;
			closest = curr_dist;
			curr_n.normalize();
			norm = curr_n;
			tri_idx[0] = f[0]*3;
			tri_idx[1] = f[1]*3;
			tri_idx[2] = f[2]*3;
		}
	}

	return obj_hit;
*/
}

} // end namespace mcl

#endif

