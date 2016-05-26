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

#ifndef MCLSCENE_LIGHT_H
#define MCLSCENE_LIGHT_H 1

#include <memory>
#include <cassert>
#include "Vec.h"

namespace mcl {


//
//	Base, pure virtual
//
class BaseLight {
public:
	virtual ~BaseLight(){}

	virtual std::string get_type() const = 0;

	// A light is sampled from random numbers 0 <= u < 1
	// If u=0.5 it returns position (or center).
	virtual trimesh::vec sample( double u1, double u2 ) = 0;

	trimesh::vec m_intensity; // i.e. color
};


//
//	Point Light
//
//	If radius = 0, the sampler returns its position
//
class PointLight : public BaseLight {
public:
	PointLight( trimesh::vec intensity, trimesh::vec pos, double rad ) : m_pos(pos), m_rad(rad) { this->m_intensity=intensity; }

	std::string get_type() const { return "point"; }

	// Uniform sphere sample
	trimesh::vec sample( double u1, double u2 ){
		return m_pos;
//		if( m_rad<=0.0 ){ return m_pos; }
//		float z = 1.f - 2.f * u1;
//		float r = sqrtf(fmaxf(0.f, 1.f - z*z))*m_rad;
//		float phi = 2.f * M_PI * u2;
//		float x = r * cosf(phi);
//		float y = r * sinf(phi);
//		return m_pos+trimesh::vec(x, y, z);
	}

	double m_rad;
	trimesh::vec m_pos;
};



//
//	Ambient Light
//
class AmbientLight : public BaseLight {
public:
	AmbientLight( trimesh::vec intensity ) { this->m_intensity=intensity; }

	std::string get_type() const { return "ambient"; }

	// Uniform sphere sample
	trimesh::vec sample( double u1, double u2 ){ return trimesh::vec(0,1,0); }
};


} // end namespace mcl

#endif
