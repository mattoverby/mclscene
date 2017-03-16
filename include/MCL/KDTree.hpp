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
class KDTNode {
public:
	KDTNode(){ left_child=nullptr; right_child=nullptr; }
	~KDTNode() {
		if(left_child != nullptr){ delete left_child; }
		if(right_child != nullptr){ delete right_child; }
	}

	// Allocated in make_tree:
	KDTNode* left_child;
	KDTNode* right_child;
	mutable std::vector< std::shared_ptr<BaseObject> > m_objects; // empty unless a leaf node
	bool is_leaf() const { return m_objects.size()>0; }


	unsigned short axis; // 0, 1, 2
	float median;
	AABB aabb; 

	Eigen::VectorXd *vertices; // pointer to data
	Eigen::VectorXi *faces; // pointer to data
	std::vector<int> face_indices; // indices into faces
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

	static inline void make_tree( KDTNode *root, Eigen::VectorXd *vertices, Eigen::VectorXi *faces, int max_depth=10 );

	static inline void median_split( KDTNode *node, Eigen::VectorXd *vertices, Eigen::VectorXi *faces, const std::vector<int> &queue, const int split_axis, const int max_depth );

	// Point-Scene traversal for closest object to a given point.
	// Projection is the point on the object surface, obj is the pointer to the object.
	// Returns true if a closest object was found (closer than distance between point and projection).
	static inline bool closest_object( const KDTNode *node, const Vec3f &point, Vec3f &projection, Vec3f &norm, std::shared_ptr<BaseObject> *obj );



	static inline bool closest_object( const KDTNode *node, const Vec2i stride, const bool toSkip, double& closest, const Vec3d &point, Vec3d &projection, Vec3d &norm, int* tri_idx);
	static inline bool ray_intersection( const KDTNode *node, const mcl::raycast::rtRay<double> ray, const Vec2i skip_stride, double& t_max, Vec3d &projection, Vec3d &norm, int* tri_idx); 
	static inline bool intersect_with_box(const AABB& aabb, const mcl::raycast::rtRay<double> ray, const double t_max, Vec3d& projection);
	static inline bool close_to_box(const AABB& aabb, const Vec3d &point, const double closest);

	static inline bool isValid(const KDTNode* root);
}; // end namespace kdtree

//
//	Implementation
//

static inline void kdtree::make_tree( KDTNode *root, const std::vector< std::shared_ptr<BaseObject> > &objects, int max_depth ){
	
	if( root == nullptr ){ root = new KDTNode(); }
	else{
		if( root->left_child != nullptr ){ delete root->left_child; root->left_child = nullptr; }
		if( root->right_child != nullptr ){ delete root->right_child; root->right_child = nullptr; }
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


// Point-Scene traversal for closest object to a given point.
// Projection is the point on the object surface, obj is the pointer to the object.
static inline bool kdtree::closest_object( const KDTNode *node, const Vec3f &point, Vec3f &projection, Vec3f &norm, std::shared_ptr<BaseObject> *obj ){

	// Parse the tree
	bool left_hit = false; bool right_hit = false;
	float pt = point[ node->axis ]; // point
	float med = node->median; // node median

	// If the point is very close to the median of the node, there is a
	// chance something closer might be on the other branch.

	// Check left?
	if( pt < med && node->left_child != nullptr ){
		left_hit = kdtree::closest_object( node->left_child, point, projection, norm, obj );

		float dist = (projection-point).squaredNorm();
		bool check_both = dist > (pt-med)*(pt-med) && node->right_child != nullptr;
		if( check_both ){ right_hit = kdtree::closest_object( node->right_child, point, projection, norm, obj ); }
	}

	// Check right?
	else if( pt >= med && node->right_child != nullptr ){
		right_hit = kdtree::closest_object( node->right_child, point, projection, norm, obj );

		float dist = (projection-point).squaredNorm();
		bool check_both = dist > (pt-med)*(pt-med) && node->left_child != nullptr;
		if( check_both ){ left_hit = kdtree::closest_object( node->left_child, point, projection, norm, obj ); }
	}

	// If we traversed the tree, return
	if( left_hit || right_hit ){ return true; }

	// If we're a leaf, find closest projection
	bool obj_hit = false;
	double dist = (point-projection).squaredNorm(); // current closest obj
	for( int i=0; i<node->m_objects.size(); ++i ){

		Vec3f pn(0,0,0);
		Vec3f p = node->m_objects[i]->projection(point,pn);
		Vec3f n = point - p;

		// See if this projection is closer
		double curr_dist = n.squaredNorm();
		if( curr_dist < dist ){
			projection = p;
			obj_hit = true;
			obj=&(node->m_objects[i]);
			dist = curr_dist;
			if( pn.norm()>0 ){ norm = pn; }
			else{ norm = n; }
		}
	}

	return obj_hit;
}

static inline void kdtree::make_tree( KDTNode *root, Eigen::VectorXd *vertices, Eigen::VectorXi *faces, int max_depth ) {
	if (root == nullptr){ root = new KDTNode(); }
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
		root->face_indices.clear();
		root->aabb.valid = false;
	}
	for (int i = 0; i < faces->size()/3; i++) {
		Vec3i f = faces->segment<3>(i*3);
		for (int j = 0; j < 3; j++) {
			Vec3d x_d = vertices->segment<3>(f[j]*3);
			Vec3f x(x_d(0), x_d(1), x_d(2));
			root->aabb += x;
		}
	}
	std::vector< int > queue( faces->size()/3 );
	std::iota( std::begin(queue), std::end(queue), 0 );
	median_split(root, vertices, faces, queue, 0, max_depth);
}

static inline void kdtree::median_split( KDTNode *node, Eigen::VectorXd *vertices, Eigen::VectorXi *faces, const std::vector<int> &queue, const int split_axis, const int max_depth ) {
	std::vector< float > obj_min( queue.size() );
	std::vector< float > obj_max( queue.size() );

	AABB& aabb = node->aabb;
	node->axis = split_axis;
	node->median = aabb.center()[ split_axis ];

	if (queue.size() == 0) {
		return;
	}
	if (queue.size() == 1 || max_depth <= 0) {
		node->vertices = vertices;
		node->faces = faces;
		node->face_indices.reserve( queue.size() );
		for( int i=0; i<queue.size(); ++i ){
			node->face_indices.push_back(queue[i]);
		}
		return;
	}

	std::vector<int> left_queue, right_queue;
	for( int i=0; i<queue.size(); ++i ){
		const Vec3i &f = faces->segment<3>(queue[i]*3);
		AABB faceAABB;
		for (int j = 0; j < 3; j++) {
			Vec3d v_d = vertices->segment<3>(f[j]*3);
			Vec3f v(v_d[0], v_d[1], v_d[2]);
			faceAABB += v;
		}
		if (faceAABB.min[split_axis] < node->median) {
			left_queue.push_back( queue[i] );
		}
		if (faceAABB.max[split_axis] >= node->median) {
			right_queue.push_back( queue[i] );
		}
	}

	// Create the children
	node->left_child = new KDTNode();
	node->left_child->aabb = aabb;
	node->left_child->aabb.max[split_axis] = node->median;
	node->right_child = new KDTNode();
	node->right_child->aabb = aabb;
	node->right_child->aabb.min[split_axis] = node->median;
	median_split( node->left_child, vertices, faces, left_queue, ((split_axis+1)%3), max_depth-1 );
	median_split( node->right_child, vertices, faces, right_queue, ((split_axis+1)%3), max_depth-1 );
}

static inline bool kdtree::closest_object( const KDTNode *node, const Vec2i stride, const bool toSkip, double& closest, const Vec3d &point, Vec3d &projection, Vec3d &norm, int* tri_idx) {
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

	for (int i = 0; i < node->face_indices.size(); i++) {
		int idx = node->face_indices[i];
		if (toSkip) {
			if (idx >= stride[0] && idx < stride[1]) {
				continue;
			}
		} else {
			if (idx < stride[0] || idx >= stride[1]) {
				continue;
			}
		}
		const Vec3i &f = node->faces->segment<3>(node->face_indices[i]*3);
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
}

static inline bool kdtree::ray_intersection( const KDTNode *node, const mcl::raycast::rtRay<double> ray, const Vec2i skip_stride, double& t_max, Vec3d &projection, Vec3d &norm, int* tri_idx) {
	
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
	for( int i=0; i<node->face_indices.size(); ++i ){
		int idx = node->face_indices[i];
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

		// double d = curr_n.dot(vs[0]);
		// double t = (d - curr_n.dot(x))/(curr_n.dot(dir));
		// Vec3f temp_proj = x + t*dir;
		// double area = e0.cross(e1).norm();
		// Vec3f l0 = temp_proj - vs[0];
		// Vec3f l1 = temp_proj - vs[1];
		// Vec3f l2 = temp_proj - vs[2];
		// double alpha = l1.cross(l2).norm()/area;
		// double beta = l0.cross(l2).norm()/area;
		// double gamma = l0.cross(l1).norm()/area;
		// bool bary_test = abs(alpha + beta + gamma - 1) < 1e-6;

		Vec3d e2 = ( 1.0 / curr_n.dot( dir ) ) * ( vs[0] - x );
		Vec3d inter  = dir.cross( e2 );
		double beta  = inter.dot( e1 );
		double gamma = inter.dot( e0 );
		double alpha = 1.0 - beta - gamma;
		// alpha = abs(alpha) < 1e-6 ? 0 : alpha;
		beta = abs(beta) < std::numeric_limits<double>::min() ? 0 : beta;
		gamma = abs(gamma) < std::numeric_limits<double>::min() ? 0 : gamma;
		double t = curr_n.dot( e2 );
		bool bary_test = alpha>0.0 && beta>0.0 && gamma>0.0 && (alpha+beta+gamma)<=1.0;

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
}

static inline bool kdtree::close_to_box(const AABB& aabb, const Vec3d &point, const double closest) {
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
				if (abs(max[i] + min[i] - point[i]) > closest) {
					return true;
				}
			}
		}
	}
	return false;
}

static inline bool kdtree::intersect_with_box(const AABB& aabb, const mcl::raycast::rtRay<double> ray, const double t_max, Vec3d& projection) {
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
				if (abs(projection[a1] - aabb.max[a1]) <= eps && eps >= abs(aabb.min[a1] - projection[a1]) && abs(projection[a2] - aabb.max[a2]) <= eps && eps >= abs(aabb.min[a2] - projection[a2])) {
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
				if (abs(projection[a1] - aabb.max[a1]) <= eps && eps >= abs(aabb.min[a1] - projection[a1]) && abs(projection[a2] - aabb.max[a2]) <= eps && eps >= abs(aabb.min[a2] - projection[a2])) {
					return true;
				}
			}
		}
	}
	return false;
}

static inline bool kdtree::isValid(const KDTNode* root) {
	if( root == nullptr ){
		return false;
	}
	else if (root->left_child == nullptr && root->right_child == nullptr) {
		return false;
	}
	return true;
}

} // end namespace mcl

#endif

