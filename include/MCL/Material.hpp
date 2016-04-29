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

#ifndef MCLSCENE_MATERIAL_H
#define MCLSCENE_MATERIAL_H 1

#include <memory>
#include <cassert>
#include "MCL/Param.hpp"

///
///	Simple object types
///
namespace mcl {


//
//	Base, pure virtual
//
class BaseMaterial {
public:
	virtual ~BaseMaterial(){}

	virtual std::string get_type() const = 0;
};


//
//	Diffuse
//
class DiffuseMaterial : public BaseMaterial {
public:
	trimesh::vec diffuse; // diffuse color
	trimesh::vec edge_color;

	std::string get_type() const { return "diffuse"; }
};


//
//	Specular
//
class SpecularMaterial : public BaseMaterial {
public:
	trimesh::vec diffuse; // diffuse color
	trimesh::vec specular; // specular color
	double shininess; // i.e. phong exponent

	trimesh::vec edge_color;

	std::string get_type() const { return "specular"; }
};


} // end namespace mcl

#endif
