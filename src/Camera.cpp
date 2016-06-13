// Copyright 2016 Matthew Overby.
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

#include "MCL/Camera.hpp"

using namespace mcl;


OrthonormalBasis::OrthonormalBasis( trimesh::vec direction, trimesh::vec up ){

	using namespace trimesh;

	//
	//  Create UVW
	//

	W = direction * -1.f;
	normalize( W );

	if( W[0] == 0.f && W[2] == 0.f ){
		up[0]+=0.0001;
		up[1]-=0.0001;
		up[2]+=0.0001;
		normalize( up );
	}

	U = up.cross( W );
	normalize( U );

	V = W.cross( U );
	normalize( V );

} // end ortho base



PerspectiveCamera::PerspectiveCamera( trimesh::vec pos, trimesh::vec dir, float focal_len ) : 
	position(pos), focal_length(focal_len), basis(dir) {

} // end constructor



void PerspectiveCamera::compute_rays( unsigned int img_width, unsigned int img_height, std::vector<mcl::intersect::Ray> &rays, unsigned int rpp ){

	using namespace trimesh;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(0.0,1.0);
/*
	for( int i=0; i<rpp; ++i ){

		// Get a stochastic sample position
		float xterm = (img_width + (i + dis(gen)) / float(rpp) );
		float yterm = (img_height + (i + dis(gen)) / float(rpp) );

		float u = l + (r - l)*(xterm)/img_width;
		float v = b + (t - b)*(yterm)/img_height;

		vec wVec = basis.W * focal_length * -1.f;
		vec uVec = basis.U * u;
		vec vVec = basis.V * v;

		vec rayDir = wVec + uVec + vVec;
		normalize( rayDir );

		intersect::Ray ray = Ray( position, rayDir );
		rays.push_back( ray );
	}
*/
} // end compute rays


