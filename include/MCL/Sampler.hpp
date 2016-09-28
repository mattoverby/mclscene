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


#ifndef MCLSCENE_SAMPLER_H
#define MCLSCENE_SAMPLER_H 1

#include <math.h>

namespace mcl {
	
// Randoms (u1, u2, etc...): 0 to 1
namespace Sampler {

	//
	//	Uniform Cone
	//
	static void uniform_cone( float u1, float u2, float max_theta, float *vec_3f ){
		float cos_theta = (1.f - u1) + u1 * cos(max_theta);
		float sin_theta = sqrtf(1.f - cos_theta*cos_theta);
		float phi = u2 * 2.f * M_PI;
		vec_3f[0]=cosf(phi)*sin_theta;
		vec_3f[1]=sinf(phi)*sin_theta;
		vec_3f[2]=cos_theta;
	}


	//
	//	Cosine Hemisphere
	//
	static void cosine_hemisphere( float u1, float u2, float *vec_3f ){
		float r = sqrt( u1 );
		float theta = 2.f * M_PI * u2;
		vec_3f[0] = r * cosf(theta);
		vec_3f[1] = r * sinf(theta);
		vec_3f[2] = sqrt( fmaxf(0.f, 1.f-u1) );
	}

}; // end namespace Sampler

} // end namespace mcl

#endif

