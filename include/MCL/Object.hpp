// Copyright 2014 Matthew Overby.
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

#ifndef MCLSCENE_OBJECT_H
#define MCLSCENE_OBJECT_H 1

#include "TriMesh.h"
#include "TriMesh_algo.h"
#include "TriMeshBuilder.h"
#include "XForm.h"

///
///	Simple object types
///
namespace mcl {


//
//	Base
//
class BaseObject {
public:
	virtual ~BaseObject(){}
	std::string material;
	trimesh::xform x_form;
};


//
//	Sphere
//
class Sphere : public BaseObject {
public:
	Sphere( trimesh::vec pos, double rad ) : center(pos), radius(rad) {}
	trimesh::vec center;
	double radius;
};


//
//	Box, represented by a trimesh
//
class Box : public BaseObject {
public:
	Box( trimesh::vec boxmin, trimesh::vec boxmax, int tess=1 ){ build_trimesh(boxmin,boxmax,tess); }
	trimesh::TriMesh tris;
private:
	void build_trimesh( trimesh::vec boxmin, trimesh::vec boxmax, int tess ){
		using namespace trimesh;

		// First create a boring cube
		make_cube( &tris, tess ); // tess=1 -> 12 tris
		tris.need_bbox();

		// Now translate it so boxmins are the same
		vec offset = tris.bbox.min - boxmin;
		xform t_xf = xform::trans(offset[0],offset[1],offset[2]);
		apply_xform(&tris, t_xf);
		tris.bbox.valid = false;
		tris.need_bbox();

		// Now scale so that boxmaxes are the same
		vec size = tris.bbox.max - boxmax;
		xform s_xf = xform::scale(size[0],size[1],size[2]);
		apply_xform(&tris, s_xf);
		tris.bbox.valid = false;
		tris.need_bbox();
	}
};


} // end namespace mcl

#endif
