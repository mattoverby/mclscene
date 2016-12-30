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

#ifndef MCLSCENE_TRIANGLEMESH_H
#define MCLSCENE_TRIANGLEMESH_H 1

#include "Object.hpp"
#include "AABB.hpp"
#include "TriMesh_algo.h"

namespace mcl {


//
//	Triangle (reference)
//	Contains pointers to vertices and normals of a TriangleMesh
//
class TriangleRef : public BaseObject {
public:
	TriangleRef( trimesh::vec *p0_, trimesh::vec *p1_, trimesh::vec *p2_,
		trimesh::vec *n0_, trimesh::vec *n1_, trimesh::vec *n2_ ) :
		p0(p0_), p1(p1_), p2(p2_), n0(n0_), n1(n1_), n2(n2_) {}

	trimesh::vec *p0, *p1, *p2, *n0, *n1, *n2;

	void bounds( trimesh::vec &bmin, trimesh::vec &bmax ){
		AABB aabb; aabb += *p0; aabb += *p1; aabb += *p2;
		bmin = aabb.min; bmax = aabb.max;
	}

	bool ray_intersect( const intersect::Ray *ray, intersect::Payload *payload ) const {
		bool hit = intersect::ray_triangle( ray, *p0, *p1, *p2, *n0, *n1, *n2, payload );
		if( hit ){ payload->material = this->app.material; }
		return hit;
	}

	trimesh::vec projection( const trimesh::vec &point ) const {
		return intersect::point_triangle( point, *p0, *p1, *p2 );
	}
};


//
//	Just a convenient wrapper to plug into the system
//
class TriangleMesh : public BaseObject {
private: std::unique_ptr<trimesh::TriMesh> tris; // tris is actually the data container
public:

	TriangleMesh() : tris(new trimesh::TriMesh),
		vertices(tris->vertices), normals(tris->normals), faces(tris->faces),
		aabb(new AABB) { app.mesh = tris.get(); } // end constructor

	// Returns true on success
	bool load( std::string filename );

	std::string get_xml( int mode=0 );

	// Mesh data
	std::vector<trimesh::point> &vertices;
	std::vector<trimesh::vec> &normals;
	std::vector<trimesh::TriMesh::Face> &faces;

	void apply_xform( const trimesh::xform &xf );

	void bounds( trimesh::vec &bmin, trimesh::vec &bmax );

	void update(){ aabb->valid=false; }

	void get_primitives( std::vector< std::shared_ptr<BaseObject> > &prims ){
		if( tri_refs.size() != faces.size() ){ make_tri_refs(); }
		prims.insert( prims.end(), tri_refs.begin(), tri_refs.end() );
	}

private:
	std::shared_ptr<AABB> aabb;

	// Triangle refs are used for BVH hook-in.
	void make_tri_refs();
	std::vector< std::shared_ptr<BaseObject> > tri_refs;
};


//
//	New mesh type
//
class Mesh : public BaseObject {
public:
	void bounds( trimesh::vec &bmin, trimesh::vec &bmax );
};


} // end namespace mcl

#endif
