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

#ifndef MCLSCENE_VEC3_H
#define MCLSCENE_VEC3_H 1

#include <Eigen/Geometry> // needed for cross product
#include "Vec.h"
#include "XForm.h"

namespace mcl {

//	typedef Eigen::Vector3d Vec3f;
//	typedef Eigen::Vector2d Vec2d;
	typedef Eigen::Vector4f Vec4f;
	typedef Eigen::Vector3f Vec3f;
	typedef Eigen::Vector2f Vec2f;
	typedef Eigen::Vector3i Vec3i;
	typedef Eigen::Vector4i Vec4i;

	// Temporary quick fix:
	static inline Vec3f to_Vec3f(trimesh::vec p){ return Vec3f(p[0],p[1],p[2]); } 

	static inline std::string to_str(const Vec3f &v){
		std::stringstream ss; ss << v[0] << ' ' << v[1] << ' ' << v[2];
		return ss.str();
	}

} // end namespace mcl


namespace trimesh {

	static inline Eigen::Vector4f operator*(const trimesh::xform &m, const Eigen::Vector4f &v){
		Eigen::Vector4f r;
		r[0] = m[0]*v[0]+m[4]*v[1]+m[8]*v[2]+m[12]*v[3];
		r[1] = m[1]*v[0]+m[5]*v[1]+m[9]*v[2]+m[13]*v[3];
		r[2] = m[2]*v[0]+m[6]*v[1]+m[10]*v[2]+m[14]*v[3];
		r[3] = m[3]*v[0]+m[7]*v[1]+m[11]*v[2]+m[15]*v[3];
		return r;
	}

	static inline Eigen::Vector3f operator*(const trimesh::xform &m, const Eigen::Vector3f &v){
		Eigen::Vector3f r;
		r[0] = m[0]*v[0]+m[4]*v[1]+m[8]*v[2]+m[12];
		r[1] = m[1]*v[0]+m[5]*v[1]+m[9]*v[2]+m[13];
		r[2] = m[2]*v[0]+m[6]*v[1]+m[10]*v[2]+m[14];
		return r;
	}

	static inline Eigen::Vector3f operator*(const trimesh::fxform &m, const Eigen::Vector3f &v){
		Eigen::Vector3f r;
		r[0] = m[0]*v[0]+m[4]*v[1]+m[8]*v[2]+m[12];
		r[1] = m[1]*v[0]+m[5]*v[1]+m[9]*v[2]+m[13];
		r[2] = m[2]*v[0]+m[6]*v[1]+m[10]*v[2]+m[14];
		return r;
	}

} // end namespace trimesh

#endif
