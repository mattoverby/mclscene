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

#ifndef MCLSCENE_AABB_H
#define MCLSCENE_AABB_H 1

#include "MCL/Vec3.hpp"

namespace mcl {

class AABB { // axis aligned bounding box
public:
	AABB() : valid(false) {}
//	AABB( trimesh::vec min_, trimesh::vec max_ ) : min(min_), max(max_), valid(true) {}
//	trimesh::vec min, max;
	AABB( Vec3d min_, Vec3d max_ ) : min(min_), max(max_), valid(true) {}
	Vec3d min, max;
	bool valid;

//	trimesh::vec center(){ return (min+max)*0.5f; }
	Vec3d center(){ return (min+max)*0.5; }

//	float radius(){ return trimesh::len(max-min)*0.5f; }
	double radius(){ return (max-min).norm()*0.5f; }

	AABB& operator+=(const AABB& aabb){
		if( valid ){ minp( min, aabb.min ); maxp( max, aabb.max ); }
		else{ min = aabb.min; max = aabb.max; valid = true; }
		return *this;
	}

	AABB& operator+=(const Vec3d& p){
		if( valid ){ minp( min, p ); maxp( max, p ); }
		else{ min = p; max = p; valid = true; }
		return *this;
	}

	AABB& operator+=(const trimesh::vec& tmp){
		Vec3d p(tmp[0],tmp[1],tmp[2]);
		if( valid ){ minp( min, p ); maxp( max, p ); }
		else{ min = p; max = p; valid = true; }
		return *this;
	}

	static inline void minp( Vec3d &m, const Vec3d &b ){
		for( int i=0; i<3; ++i ){ m[i] = m[i] < b[i] ? m[i] : b[i]; }
	}

	static inline void maxp( Vec3d &m, const Vec3d &b ){
		for( int i=0; i<3; ++i ){ m[i] = m[i] > b[i] ? m[i] : b[i]; }
	}

};


} // end namespace mcl



#endif
