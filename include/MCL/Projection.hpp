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


#ifndef MCLSCENE_PROJECTION_H
#define MCLSCENE_PROJECTION_H 1

#include <math.h>
#include "MCL/Vec3.hpp"

namespace mcl {

//	
//	Several static functions for projecting a point onto a geometric surface.
//	Will return a point on the surface that is nearest to the one given.
//
namespace Projection {

	//
	//	Projection on Triangle
	//	triangle should be array of 3
	//
	static Vec3d Triangle( Vec3d *triangle, const Vec3d &point );

	//
	//	Projection on Sphere
	//
	static Vec3d Sphere( const Vec3d &center, const double &rad, const Vec3d &point );

}; // end namespace Projection

//
//	Implementation
//

static Vec3d Projection::Triangle( Vec3d *tri, const Vec3d &point ){

	using namespace trimesh;
	Vec3d edge0 = tri[1] - tri[0];
	Vec3d edge1 = tri[2] - tri[0];
	Vec3d v0 = tri[0] - point;

	double a = edge0.dot( edge0 );
	double b = edge0.dot( edge1 );
	double c = edge1.dot( edge1 );
	double d = edge0.dot( v0 );
	double e = edge1.dot( v0 );
	double det = a*c - b*b;
	double s = b*e - c*d;
	double t = b*d - a*e;

	if ( s + t < det ) {
		if ( s < 0.0 ) {
		    if ( t < 0.0 ) {
			if ( d < 0.0 ) {
			    s = clamp( -d/a, 0.0, 1.0 );
			    t = 0.0;
			}
			else {
			    s = 0.0;
			    t = clamp( -e/c, 0.0, 1.0 );
			}
		    }
		    else {
			s = 0.0;
			t = clamp( -e/c, 0.0, 1.0 );
		    }
		}
		else if ( t < 0.0 ) {
		    s = clamp( -d/a, 0.0, 1.0 );
		    t = 0.0;
		}
		else {
		    double invDet = 1.0 / det;
		    s *= invDet;
		    t *= invDet;
		}
	}
	else {
		if ( s < 0.0 ) {
		    double tmp0 = b+d;
		    double tmp1 = c+e;
		    if ( tmp1 > tmp0 ) {
			double numer = tmp1 - tmp0;
			double denom = a-2.0*b+c;
			s = clamp( numer/denom, 0.0, 1.0 );
			t = 1.0-s;
		    }
		    else {
			t = clamp( -e/c, 0.0, 1.0 );
			s = 0.0;
		    }
		}
		else if ( t < 0.0 ) {
		    if ( a+d > b+e ) {
			double numer = c+e-b-d;
			double denom = a-2.0*b+c;
			s = clamp( numer/denom, 0.0, 1.0 );
			t = 1.0-s;
		    }
		    else {
			s = clamp( -e/c, 0.0, 1.0 );
			t = 0.0;
		    }
		}
		else {
		    double numer = c+e-b-d;
		    double denom = a-2.0*b+c;
		    s = clamp( numer/denom, 0.0, 1.0 );
		    t = 1.0 - s;
		}
	}

	return Vec3d( tri[0] + s*edge0 + t*edge1 );

} // end project triangle

static Vec3d Projection::Sphere( const Vec3d &center, const double &rad, const Vec3d &point ){
	Vec3d dir = point-center;
	dir.normalize();
	return Vec3d( center + rad*dir );
} // end project sphere

} // end namespace mcl

#endif

