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
//	NOTE:
//	    I was having trouble with template type deduction. I'll have to figure out the right
//	    way to do this later. For now, there are just a few duplicate functions
//	    to wrap the implementation for different types.
//
namespace projection {

	//
	//	Projection on Triangle
	//	triangle should be array of 3
	//
	template <typename T> static Vec3<T> Triangle( const Vec3<T> &point, const Vec3<T> &p1, const Vec3<T> &p2, const Vec3<T> &p3 );
	static Vec3f point_triangle( const Vec3f &point, const Vec3f &p1, const Vec3f &p2, const Vec3f &p3 ){ return Triangle<float>(point,p1,p2,p3); }
	static Vec3d point_triangle( const Vec3d &point, const Vec3d &p1, const Vec3d &p2, const Vec3d &p3 ){ return Triangle<double>(point,p1,p2,p3); }

	//
	//	Projection on Sphere
	//
	template <typename T> static Vec3<T> Sphere( const Vec3<T> &center, const T &rad, const Vec3<T> &point );
	static Vec3f Sphere( const Vec3f &center, const float &rad, const Vec3f &point ){ return Sphere<float>(center,rad,point); }
	static Vec3d Sphere( const Vec3d &center, const double &rad, const Vec3d &point ){ return Sphere<double>(center,rad,point); }

	//
	//	Helper functions
	//
	template <typename T> static T myclamp( const T &val, const T &min, const T &max ){
		return val < min ? min : (val > max ? max : val);
	}

}; // end namespace Projection

//
//	Implementation
//

template <typename T> static Vec3<T> projection::Triangle( const Vec3<T> &point, const Vec3<T> &p1, const Vec3<T> &p2, const Vec3<T> &p3 ){

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
			    s = myclamp( -d/a, zero, one );
			    t = zero;
			}
			else {
			    s = zero;
			    t = myclamp( -e/c, zero, one );
			}
		    }
		    else {
			s = zero;
			t = myclamp( -e/c, zero, one );
		    }
		}
		else if ( t < zero ) {
		    s = myclamp( -d/a, zero, one );
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
			s = myclamp( numer/denom, zero, one );
			t = one-s;
		    }
		    else {
			t = myclamp( -e/c, zero, one );
			s = zero;
		    }
		}
		else if ( t < zero ) {
		    if ( a+d > b+e ) {
			T numer = c+e-b-d;
			T denom = a-T(2)*b+c;
			s = myclamp( numer/denom, zero, one );
			t = one-s;
		    }
		    else {
			s = myclamp( -e/c, zero, one );
			t = zero;
		    }
		}
		else {
		    T numer = c+e-b-d;
		    T denom = a-T(2)*b+c;
		    s = myclamp( numer/denom, zero, one );
		    t = one - s;
		}
	}

	Vec3<T> result = p1 + edge0*s + edge1*t;
	return result;

} // end project triangle


template <typename T> static Vec3<T> projection::Sphere( const Vec3<T> &center, const T &rad, const Vec3<T> &point ){
	Vec3<T> dir = point-center;
	dir.normalize();
	return ( center + dir*rad );
} // end project sphere


} // end namespace mcl

#endif

