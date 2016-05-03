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

#ifndef MCLSCENE_OBJECT_H
#define MCLSCENE_OBJECT_H 1

#include "TriMeshBuilder.h"
#include "Param.hpp"
#include "AABB.hpp"

///
///	Simple object types
///
namespace mcl {


//
//	Base, pure virtual
//
class BaseObject {
public:
	virtual ~BaseObject(){}
	virtual const std::shared_ptr<trimesh::TriMesh> get_TriMesh(){ return NULL; }
	virtual void apply_xform( const trimesh::xform &xf ){}
	virtual std::string get_material() const { return ""; }

	virtual std::string get_type() const = 0;
	// virtual std::string save() = 0; TODO
	virtual void get_aabb( trimesh::vec &bmin, trimesh::vec &bmax ) = 0;
	virtual void get_edges( std::vector<trimesh::vec> &edges ){} // return edges of BVH for debugging visuals
};


//
//	Sphere
//
class Sphere : public BaseObject {
public:
	Sphere() : center(0,0,0), radius(1.0) {}

	Sphere( trimesh::vec cent, double rad ) : center(cent), radius(rad) {}

	std::string get_type() const { return "sphere"; }

	void get_aabb( trimesh::vec &bmin, trimesh::vec &bmax ){
		bmin = trimesh::vec( center[0]-radius, center[1]-radius, center[2]-radius );
		bmax = trimesh::vec( center[0]+radius, center[1]+radius, center[2]+radius );
	}

	trimesh::vec center;
	double radius;
};


//
//	Box, represented by a trimesh
//
class Box : public BaseObject {
public:
	Box() : boxmin(0,0,0), boxmax(1,1,1) {}

	Box( trimesh::vec bmin, trimesh::vec bmax ) : boxmin(bmin), boxmax(bmax) {}

	std::string get_type() const { return "box"; }

	trimesh::vec boxmin, boxmax;

	void get_aabb( trimesh::vec &bmin, trimesh::vec &bmax ){
		bmin = boxmin; bmax = boxmax;
	}
};


//
//	Plane, 2 or more triangles
//	TODO have a position
//
class Plane : public BaseObject {
public:
	Plane() : width(20), length(20) {}

	Plane( int w, int l ) : width(w), length(l) {}

	std::string get_type() const { return "plane"; }

	void get_aabb( trimesh::vec &bmin, trimesh::vec &bmax ){
		bmin = trimesh::vec(-1,-1,0); bmax = trimesh::vec(1,1,0);
	}

	int width, length;
};


} // end namespace mcl

#endif
