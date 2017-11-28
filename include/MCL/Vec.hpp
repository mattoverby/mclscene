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

#ifndef MCLSCENE_VEC3_H
#define MCLSCENE_VEC3_H 1

#include <Eigen/Geometry> // needed for cross product
#include "XForm.h" // trimesh transforms

//
//	You can replace the internal vector type
//	by changing these typedefs.
//
namespace mcl {

	// Common vecs that I don't feel like typing out all the time:
	template <typename T> using Vec4 = Eigen::Matrix<T,4,1>;
	template <typename T> using Vec3 = Eigen::Matrix<T,3,1>;
	template <typename T> using Vec2 = Eigen::Matrix<T,2,1>;
	typedef Vec4<float> Vec4f;
	typedef Vec3<float> Vec3f;
	typedef Vec2<float> Vec2f;
	typedef Vec4<double> Vec4d;
	typedef Vec3<double> Vec3d;
	typedef Vec2<double> Vec2d;
	typedef Vec4<int> Vec4i;
	typedef Vec3<int> Vec3i;
	typedef Vec2<int> Vec2i;

	template <typename T> static inline Vec3<T> normalized(const Vec3<T> &v){ Vec3<T> t=v; t.normalize(); return t; }

	// Just for formatting output
	template <typename T> static inline std::string to_str(const Vec3<T> &v){
		std::stringstream ss; ss << v[0] << ' ' << v[1] << ' ' << v[2];
		return ss.str();
	}

	// Compute barycentric coords for a point on a triangle
	template <typename T>
	static inline Vec3<T> barycoords(const Vec3<T> &p, const Vec3<T> &p0, const Vec3<T> &p1, const Vec3<T> &p2){
		Vec3<T> v0 = p1 - p0, v1 = p2 - p0, v2 = p - p0;
		T d00 = v0.dot(v0);
		T d01 = v0.dot(v1);
		T d11 = v1.dot(v1);
		T d20 = v2.dot(v0);
		T d21 = v2.dot(v1);
		T invDenom = 1.0 / (d00 * d11 - d01 * d01);
		Vec3<T> r;
		r[1] = (d11 * d20 - d01 * d21) * invDenom;
		r[2] = (d00 * d21 - d01 * d20) * invDenom;
		r[0] = 1.0 - r[1] - r[2];
		return r;
	}

	// Assumes T is float or double, and theta/phi in radians
	template <typename T> static inline Vec3<T> spherical_to_cartesian(T theta, T phi){
		T sin_t = std::sin(theta);
		T cos_t = std::cos(theta);
		T sin_p = std::sin(phi);
		T cos_p = std::cos(phi);
		return Vec3<T>( sin_t * sin_p, sin_t * cos_p, cos_t );
	}

	template <typename T> static inline Vec2<T> cartesian_to_spherical(const Vec3<T> &v){
		Vec2<T> r( std::acos(v[2]), std::atan2(v[1], v[0]) );
		if(r[1] < 0){ r[1] += 2*M_PI; }
		return r;
	}

	

} // end namespace mcl


//
//	trimesh and mcl::Vec xforms:
//
namespace trimesh {

	template <typename T, typename U>
	static inline mcl::Vec3<T> operator*(const trimesh::XForm<U> &m, const mcl::Vec3<T> &v){
		mcl::Vec3<T> r;
		r[0] = m[0]*v[0]+m[4]*v[1]+m[8]*v[2]+m[12];
		r[1] = m[1]*v[0]+m[5]*v[1]+m[9]*v[2]+m[13];
		r[2] = m[2]*v[0]+m[6]*v[1]+m[10]*v[2]+m[14];
		return r;
	}

	template <typename T, typename U>
	static inline mcl::Vec4<T> operator*(const trimesh::XForm<U> &m, const mcl::Vec4<T> &v){
		mcl::Vec4<T> r;
		r[0] = m[0]*v[0]+m[4]*v[1]+m[8]*v[2]+m[12]*v[3];
		r[1] = m[1]*v[0]+m[5]*v[1]+m[9]*v[2]+m[13]*v[3];
		r[2] = m[2]*v[0]+m[6]*v[1]+m[10]*v[2]+m[14]*v[3];
		r[3] = m[3]*v[0]+m[7]*v[1]+m[11]*v[2]+m[15]*v[3];
		return r;
	}

} // end namespace trimesh

#endif
