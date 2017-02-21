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
// By Matt Overby (http://www.mattoverby.net) and
// some collision code from ArcSim (http://graphics.berkeley.edu/resources/ARCSim)


#ifndef MCLSCENE_PROJECTION_H
#define MCLSCENE_PROJECTION_H 1

#include <math.h>
#include "MCL/Vec.hpp"

namespace mcl {

//	
//	Several static functions for projecting a point onto a geometric surface.
//	Will return a point on the surface that is nearest to the one given.
//
//	NOTE:
//	    I was having trouble with template type deduction. I'll have to figure out the right
//	    way to do this later. For now, there are just a few duplicate functions
//	    to wrap the implementation for different types.
//
//
//	Example usage:
//
//		Vec3f pt_on_tri = projection::point_triangle( some_point, vertex1, vertex2, vertex3 );
//		Vec3d pt_on_sphere = projection::point_sphere( some_point, center, radius );
//		float dist_to_aabb = projection::point_aabb( some_point, aabb_min, aabb_max );
//
namespace projection {

	//
	//	Projection on Triangle
	//
	template <typename T> static inline Vec3<T> Triangle( const Vec3<T> &point, const Vec3<T> &p1, const Vec3<T> &p2, const Vec3<T> &p3 );
	static inline Vec3f point_triangle( const Vec3f &point, const Vec3f &p1, const Vec3f &p2, const Vec3f &p3 ){ return Triangle<float>(point,p1,p2,p3); }
	static inline Vec3d point_triangle( const Vec3d &point, const Vec3d &p1, const Vec3d &p2, const Vec3d &p3 ){ return Triangle<double>(point,p1,p2,p3); }

	//
	//	Projection on Sphere
	//
	template <typename T> static inline Vec3<T> Sphere( const Vec3<T> &point, const Vec3<T> &center, const T &rad );
	static inline Vec3f point_sphere( const Vec3f &point, const Vec3f &center, const float &rad ){ return Sphere<float>(point,center,rad); }
	static inline Vec3d point_sphere( const Vec3d &point, const Vec3d &center, const double &rad ){ return Sphere<double>(point,center,rad); }

	//
	// Returns signed distance between vertex and face
	// Also gets the normal and barycentric coordinates of the collision, which are packaged into an array of 4 doubles (w)
	// By default w[0], the barycentric coordinate of the vert is 1
	// w[1], w[2], w[3] are actually the negative of the barycentric coords of the triangle
	// That is:  alpha = -w[1], beta = -w[2], gamma = -w[3]
	//
	template <typename T> static inline T signed_vf_distance( const Vec3<T> &x, const Vec3<T> &y0, const Vec3<T> &y1, const Vec3<T> &y2, Vec3<T> *n, T *w );

	//
	//	Instead of projection, point_aabb returns squared, unsigned distance from a point to the AABB
	//
	template <typename T> static inline T AABB_dist( const Vec3<T> &point, const Vec3<T> &min, const Vec3<T> &max );
	static inline float point_aabb_dist( const Vec3f &point, const Vec3f &min, const Vec3f &max ){ return AABB_dist<float>(point,min,max); }
	static inline double point_aabb_dist( const Vec3d &point, const Vec3d &min, const Vec3d &max ){ return AABB_dist<double>(point,min,max); }

	//
	//	A Node is a dual-vertex consisting of its last position (x0) and current position (x).
	//	Impact stores results of the collision (see Triangle_dist above).
	//

	struct Node {
		Vec3d x0, x;
		Node( Vec3d x0_, Vec3d x_ ) : x0(x0_), x(x_) {}
		Node(){}
	};

	struct Impact {
		double t;
		double w[4];
		Vec3d n;
	};

	//
	//	Collision Detection test from Bridson '02 (https://dl.acm.org/citation.cfm?id=566623).
	//
	static inline bool vf_continuous_collision_test( const Node &vert, const Node &vert0, const Node &vert1, const Node &vert2, Impact &impact );

	//
	//	Helper functions
	//
	template <typename T> static inline T myclamp( const T &val ){ return val < 0 ? 0 : (val > 1 ? 1 : val); }
	template <typename T> static inline T stp( const Vec3<T> &u, const Vec3<T> &v, const Vec3<T> &w ){ return u.dot(v.cross(w)); } // scalar triple product
	template <typename T> static inline T sgn( const T &val ){ return val < 0 ? T(-1) : T(1); }
	template <typename T> static inline void swapv( T &p1, T &p2 ){ T temp = p1; p1 = p2; p2 = temp; }
	static inline Vec3d pos_at( const Node &node, double t ){ return node.x0 + t*(node.x - node.x0); }
	template <typename T> static inline int solve_quadratic(T a, T b, T c, T x[2]);
	template <typename T> static inline T newtons_method( T a, T b, T c, T d, T x0, int init_dir );
	template <typename T> static inline int solve_cubic( T a, T b, T c, T d, T x[3]);



}; // end namespace Projection

//
//	Implementation
//

template <typename T> static inline Vec3<T> projection::Triangle( const Vec3<T> &point, const Vec3<T> &p1, const Vec3<T> &p2, const Vec3<T> &p3 ){

	Vec3<T> edge0 = p2 - p1;
	Vec3<T> edge1 = p3 - p1;
	Vec3<T> v0 = p1 - point;

	T a = edge0.dot( edge0 );
	T b = edge0.dot( edge1 );
	T c = edge1.dot( edge1 );
	T d = edge0.dot( v0 );
	T e = edge1.dot( v0 );
	T det = a*c - b*b;
	T s = b*e - c*d;
	T t = b*d - a*e;

	const T zero(0);
	const T one(1);

	if ( s + t < det ) {
		if ( s < zero ) {
		    if ( t < zero ) {
			if ( d < zero ) {
			    s = myclamp( -d/a );
			    t = zero;
			}
			else {
			    s = zero;
			    t = myclamp( -e/c );
			}
		    }
		    else {
			s = zero;
			t = myclamp( -e/c );
		    }
		}
		else if ( t < zero ) {
		    s = myclamp( -d/a );
		    t = zero;
		}
		else {
		    T invDet = one / det;
		    s *= invDet;
		    t *= invDet;
		}
	}
	else {
		if ( s < zero ) {
		    T tmp0 = b+d;
		    T tmp1 = c+e;
		    if ( tmp1 > tmp0 ) {
			T numer = tmp1 - tmp0;
			T denom = a-T(2)*b+c;
			s = myclamp( numer/denom );
			t = one-s;
		    }
		    else {
			t = myclamp( -e/c );
			s = zero;
		    }
		}
		else if ( t < zero ) {
		    if ( a+d > b+e ) {
			T numer = c+e-b-d;
			T denom = a-T(2)*b+c;
			s = myclamp( numer/denom );
			t = one-s;
		    }
		    else {
			s = myclamp( -e/c );
			t = zero;
		    }
		}
		else {
		    T numer = c+e-b-d;
		    T denom = a-T(2)*b+c;
		    s = myclamp( numer/denom );
		    t = one - s;
		}
	}

	return ( p1 + edge0*s + edge1*t );

} // end project triangle


template <typename T> static inline Vec3<T> projection::Sphere( const Vec3<T> &point, const Vec3<T> &center, const T &rad ){
	Vec3<T> dir = point-center;
	dir.normalize();
	return ( center + dir*rad );
} // end project sphere


template <typename T> static inline T projection::AABB_dist( const Vec3<T> &point, const Vec3<T> &min, const Vec3<T> &max ){
	T sqDist(0);
	for( int i=0; i<3; ++i ){
		if( point[i] < min[i] ){ sqDist += (min[i]-point[i])*(min[i]-point[i]); }
		if( point[i] > max[i] ){ sqDist += (point[i]-max[i])*(point[i]-max[i]); }
	}
	return sqDist;
} // end point aabb


template <typename T> static inline T projection::signed_vf_distance( const Vec3<T> &x, const Vec3<T> &y0, const Vec3<T> &y1, const Vec3<T> &y2, Vec3<T> *n, T *w ){
	Vec3<T> _n; if(!n){ n = &_n; }
	T _w[4]; if(!w){ w = _w; }
	Vec3<T> e1 = y1-y0; e1.normalize();
	Vec3<T> e2 = y2-y0; e1.normalize();
	*n = e1.cross(e2);
	T big_value(99999999.0); // arbitrary but whatevs
	if( double(n->squaredNorm()) < 1e-6 ){ return big_value; }

	n->normalize();
	T h = (x-y0).dot(*n);
	T b0 = stp<T>(y1-x, y2-x, *n),
	b1 = stp<T>(y2-x, y0-x, *n),
	b2 = stp<T>(y0-x, y1-x, *n);
	w[0] = 1;
	w[1] = -b0/(b0 + b1 + b2);
	w[2] = -b1/(b0 + b1 + b2);
	w[3] = -b2/(b0 + b1 + b2);
	return h;
}


template <typename T> static inline int projection::solve_quadratic(T a, T b, T c, T x[2]){
	// http://en.wikipedia.org/wiki/Quadratic_formula#Floating_point_implementation
	T d = b*b - 4*a*c;
	if (d < 0) {
		x[0] = -b/(2*a);
		return 0;
	}
	T q = -(b + sgn(b)*sqrt(d))/2;
	int i = 0;
	const double eps = 1e-12; // might need to play with this
	if((double)abs(a) > eps*double(abs(q))){ x[i++] = q/a; }
	if((double)abs(q) > eps*double(abs(c))){ x[i++] = c/q; }
	if(i==2 && x[0] > x[1]){ swapv(x[0], x[1]); }
	return i;
}


template <typename T> static inline T projection::newtons_method( T a, T b, T c, T d, T x0, int init_dir ){
	if (init_dir != 0) {
		// quadratic approximation around x0, assuming y' = 0
		T y0 = d + x0*(c + x0*(b + x0*a)),
		ddy0 = 2*b + x0*(6*a);
		x0 += init_dir*sqrt(abs(2*y0/ddy0));
	}
	for (int iter = 0; iter < 100; iter++) {
		T y = d + x0*(c + x0*(b + x0*a));
		T dy = c + x0*(2*b + x0*3*a);
		if(dy == 0){ return x0; }
		T x1 = x0 - y/dy;
		if(abs(x0 - x1) < 1e-6){ return x0; }
		x0 = x1;
	}
	return x0;
}



template <typename T> static inline int projection::solve_cubic( T a, T b, T c, T d, T x[3]) {
// solves a x^3 + b x^2 + c x + d == 0
	T xc[2];
	int ncrit = solve_quadratic(3*a, 2*b, c, xc);
	if(ncrit == 0){
		x[0] = newtons_method(a, b, c, d, xc[0], 0);
		return 1;
	}
	else if(ncrit == 1){ return solve_quadratic(b, c, d, x); } // cubic is actually quadratic

	T yc[2] = {d + xc[0]*(c + xc[0]*(b + xc[0]*a)),
	d + xc[1]*(c + xc[1]*(b + xc[1]*a))};
	int i = 0;
	if (yc[0]*a >= 0){ x[i++] = newtons_method(a, b, c, d, xc[0], -1); }
	if (yc[0]*yc[1] <= 0) {
		int closer = abs(yc[0])<abs(yc[1]) ? 0 : 1;
		x[i++] = newtons_method(a, b, c, d, xc[closer], closer==0?1:-1);
	}
	if (yc[1]*a <= 0){ x[i++] = newtons_method(a, b, c, d, xc[1], 1); }
	return i;	
}



static inline bool projection::vf_continuous_collision_test( const Node &vert, const Node &vert0, const Node &vert1, const Node &vert2, Impact &impact ){
			
	const Vec3d &x0 = vert.x0;
	const Vec3d v0 = vert.x - x0;
	Vec3d x1 = vert0.x0 - x0;
	Vec3d x2 = vert1.x0 - x0;
	Vec3d x3 = vert2.x0 - x0;
	Vec3d v1 = (vert0.x - vert0.x0) - v0;
	Vec3d v2 = (vert1.x - vert1.x0) - v0;
	Vec3d v3 = (vert2.x - vert2.x0) - v0;
	double a0 = stp(x1, x2, x3);
	double a1 = stp(v1, x2, x3) + stp(x1, v2, x3) + stp(x1, x2, v3);
	double a2 = stp(x1, v2, v3) + stp(v1, x2, v3) + stp(v1, v2, x3);
	double a3 = stp(v1, v2, v3);    
	double t[4];

	// Solving the cubic equation
	int nsol = solve_cubic<double>(a3,a2,a1,a0,t);
	t[nsol] = 1; // also check at end of timestep

	// Looping through each solution from cubic solve
	for (int i = 0; i < nsol; i++) {
		if (t[i] < 0 || t[i] > 1){ continue; }
		impact.t = t[i];
		Vec3d x0 = pos_at(vert,t[i]);
		Vec3d x1 = pos_at(vert0,t[i]);
		Vec3d x2 = pos_at(vert1,t[i]);
		Vec3d x3 = pos_at(vert2,t[i]);
		Vec3d &n = impact.n;
		double *w = impact.w;
		double d = signed_vf_distance<double>(x0,x1,x2,x3,&n,w);
		bool inside = (std::min(std::min(-w[1], -w[2]), -w[3]) >= -1e-6);
		if( n.dot(w[1]*v1 + w[2]*v2 + w[3]*v3) > 0){ n = -n; }
		if( abs(d) < 1e-6 && inside ){ return true; }
	}

	return false;
    
} // end continous collision test


} // end namespace mcl

#endif

