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


#ifndef MCLSCENE_SAMPLE_H
#define MCLSCENE_SAMPLE_H 1

#include <math.h>
#include "MCL/Vec.hpp"

namespace mcl {
	
// Randoms (u1, u2, etc...): 0 to 1
namespace sample {

	//
	//	Uniform Cone
	//
	template<typename T> static inline Vec3<T> uniform_cone( T u1, T u2, T max_theta ){
		T cos_theta = (1 - u1) + u1 * cos(max_theta);
		T sin_theta = std::sqrt(1 - cos_theta*cos_theta);
		T phi = u2 * 2 * M_PI;
		return Vec3<T>( cosf(phi)*sin_theta, sinf(phi)*sin_theta, cos_theta );
	}


	//
	//	Cosine Hemisphere
	//
	template<typename T> static inline Vec3<T> cosine_hemisphere( T u1, T u2 ){
		T r = std::sqrt( u1 );
		T theta = 2 * M_PI * u2;
		return Vec3<T>( r * cosf(theta), r * sinf(theta), std::sqrt( fmaxf(0.f, 1.f-u1) ) );
	}

}; // end namespace Sampler

} // end namespace mcl

#endif

