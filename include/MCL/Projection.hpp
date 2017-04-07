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


#ifndef MCLSCENE_PROJECTION_H
#define MCLSCENE_PROJECTION_H 1

#include <math.h>
#include "MCL/Vec.hpp"

namespace mcl {

//	
//	Several static functions for projecting a point onto a geometric surface.
//	Will return a point on the surface that is nearest to the one given.
//
//	Example usage:
//		Vec3f pt_on_tri = projection::point_triangle<float>( some_point, vertex1, vertex2, vertex3 );
//		Vec3d pt_on_sphere = projection::point_sphere<double>( some_point, center, radius );
//		float dist_to_aabb = projection::point_aabb_dist<float>( some_point, aabb_min, aabb_max );
//
namespace projection {

	//
	//	Projection on Triangle
	//
	template <typename T> static inline Vec3<T> point_triangle( const Vec3<T> &point, const Vec3<T> &p1, const Vec3<T> &p2, const Vec3<T> &p3 );

	//
	//	Projection on Sphere
	//
	template <typename T> static inline Vec3<T> point_sphere( const Vec3<T> &point, const Vec3<T> &center, const T &rad );

	//
	//	Instead of projection, point_aabb returns squared unsigned distance from a point to the AABB
	//
	template <typename T> static inline T point_aabb_dist( const Vec3<T> &point, const Vec3<T> &min, const Vec3<T> &max );

	//
	//	Point in whatever
	//
	template <typename T> static inline bool point_in_aabb( const Vec3<T> &point, const Vec3<T> &min, const Vec3<T> &max );
	template <typename T> static inline bool point_in_tet( const Vec3<T> &point, const Vec3<T> &p0, const Vec3<T> &p1, const Vec3<T> &p2, const Vec3<T> &p3 );

	//
	//	Helper functions
	//
	template <typename T> static inline T myclamp( const T &val ){ return val < 0 ? 0 : (val > 1 ? 1 : val); }
	template <typename T> static inline T stp( const Vec3<T> &u, const Vec3<T> &v, const Vec3<T> &w ){ return u.dot(v.cross(w)); } // scalar triple product

}; // end namespace Projection

//
//	Implementation
//

template <typename T> static inline Vec3<T> projection::point_triangle( const Vec3<T> &point, const Vec3<T> &p1, const Vec3<T> &p2, const Vec3<T> &p3 ){

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


template <typename T> static inline Vec3<T> projection::point_sphere( const Vec3<T> &point, const Vec3<T> &center, const T &rad ){
	Vec3<T> dir = point-center;
	dir.normalize();
	return ( center + dir*rad );
} // end project sphere


template <typename T> static inline T projection::point_aabb_dist( const Vec3<T> &point, const Vec3<T> &min, const Vec3<T> &max ){
	T sqDist(0);
	for( int i=0; i<3; ++i ){
		if( point[i] < min[i] ){ sqDist += (min[i]-point[i])*(min[i]-point[i]); }
		if( point[i] > max[i] ){ sqDist += (point[i]-max[i])*(point[i]-max[i]); }
	}
	return sqDist;
} // end point aabb

template <typename T> static inline bool projection::point_in_aabb( const Vec3<T> &point, const Vec3<T> &min, const Vec3<T> &max ){
	for( int i=0; i<3; ++i ){
		if( point[i] < min[i] || point[i] > max[i] ){ return false; }
	}
	return true;
}

template <typename T> static inline bool check_norm( const Vec3<T> &point,
	const Vec3<T> &p0, const Vec3<T> &p1, const Vec3<T> &p2, const Vec3<T> &p3 ){
	const Vec3<T> n = (p1 - p0).cross(p2 - p0);
	const T dp3 = n.dot(p3 - p0);
	const T dp = n.dot(point - p0);
	return (dp3*dp>0);
}

template <typename T> static inline bool projection::point_in_tet( const Vec3<T> &point,
	const Vec3<T> &p0, const Vec3<T> &p1, const Vec3<T> &p2, const Vec3<T> &p3 ){
	return check_norm<T>(point, p0, p1, p2, p3) && check_norm<T>(point, p1, p2, p3, p0) &&
		check_norm<T>(point, p2, p3, p0, p1) && check_norm<T>(point, p3, p0, p1, p2);
}

} // end namespace mcl

#endif

