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

#include "TriMeshBuilder.h"
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
class BaseObject {
public:
	virtual ~BaseObject(){}
	virtual std::shared_ptr<trimesh::TriMesh> as_TriMesh() = 0;
	virtual void init( const std::vector< Param > &params ) = 0;
	virtual void apply_xform( const trimesh::xform &xf ) = 0;
};


//
//	Sphere
//
class Sphere : public BaseObject {
public:
	Sphere() : tris(NULL), center(0,0,0), radius(1.0), tessellation(32) {}

	Sphere( trimesh::vec cent, double rad, int tess=32 ) : tris(NULL), center(cent), radius(rad), tessellation(tess) {}

	std::shared_ptr<trimesh::TriMesh> as_TriMesh(){
		if( tris == NULL ){ build_trimesh(); }
		return tris;
	}

	void init( const std::vector< Param > &params ){
		for( int i=0; i<params.size(); ++i ){
			if( parse::to_lower(params[i].name)=="radius" ){ radius=params[i].as_double(); }
			else if( parse::to_lower(params[i].name)=="center" ){ center=params[i].as_vec3(); }
			else if( parse::to_lower(params[i].name)=="tess" ){ tessellation=params[i].as_int(); }
		}
		build_trimesh();
	}

	// Unlike other objects, sphere only changes trimesh, not radius/center.
	// What I shoooould do is store the xform and only apply it at render time...
	void apply_xform( const trimesh::xform &xf ){
		if( tris == NULL ){ build_trimesh(); }
		trimesh::apply_xform( tris.get(), xf );
	}

	trimesh::vec center;
	double radius;

private:
	std::shared_ptr<trimesh::TriMesh> tris;
	int tessellation;

	void build_trimesh(){
		if( tris == NULL ){ tris = std::shared_ptr<trimesh::TriMesh>( new trimesh::TriMesh() ); }
		else{ tris.reset( new trimesh::TriMesh() ); }
		trimesh::make_sphere_polar( tris.get(), tessellation, tessellation );
		tris.get()->need_normals();
		tris.get()->need_tstrips();
	}
};


//
//	Box, represented by a trimesh
//
class Box : public BaseObject {
public:
	Box() : boxmin(0,0,0), boxmax(1,1,1), tris(NULL), tessellation(1) {}

	Box( trimesh::vec bmin, trimesh::vec bmax, int tess=1 ) : boxmin(bmin), boxmax(bmax), tris(NULL), tessellation(tess) {}

	std::shared_ptr<trimesh::TriMesh> as_TriMesh(){
		if( tris == NULL ){ build_trimesh(); }
		return tris;
	}

	void init( const std::vector< Param > &params ){
		for( int i=0; i<params.size(); ++i ){
			if( parse::to_lower(params[i].name)=="boxmin" ){ boxmin=params[i].as_vec3(); }
			else if( parse::to_lower(params[i].name)=="boxmin" ){ boxmin=params[i].as_vec3(); }
			else if( parse::to_lower(params[i].name)=="tess" ){ tessellation=params[i].as_int(); }
		}
		build_trimesh();
	}

	void apply_xform( const trimesh::xform &xf ){
		if( tris == NULL ){ build_trimesh(); }
		trimesh::apply_xform( tris.get(), xf );

		// Reset boxmin and boxmax
		tris.get()->bbox.valid = false;
		tris.get()->need_bbox();
		boxmin = tris.get()->bbox.min;
		boxmax = tris.get()->bbox.max;
	}

	trimesh::vec boxmin, boxmax;

private:
	std::shared_ptr<trimesh::TriMesh> tris;
	int tessellation;

	void build_trimesh(){
		if( tris == NULL ){ tris = std::shared_ptr<trimesh::TriMesh>( new trimesh::TriMesh() ); }
		else{ tris.reset( new trimesh::TriMesh() ); }

		tris = std::shared_ptr<trimesh::TriMesh>( new trimesh::TriMesh() );

		// First create a boring cube
		trimesh::make_cube( tris.get(), tessellation ); // tess=1 -> 12 tris
		tris.get()->need_bbox();

		// Now translate it so boxmins are the same
		trimesh::vec offset = tris.get()->bbox.min - boxmin;
		trimesh::xform t_xf = trimesh::xform::trans(offset[0],offset[1],offset[2]);
		trimesh::apply_xform(tris.get(), t_xf);
		tris.get()->bbox.valid = false;
		tris.get()->need_bbox();

		// Now scale so that boxmaxes are the same
		trimesh::vec size = tris.get()->bbox.max - boxmax;
		trimesh::xform s_xf = trimesh::xform::scale(size[0],size[1],size[2]);
		trimesh::apply_xform(tris.get(), s_xf);
		tris.get()->bbox.valid = false;
		tris.get()->need_bbox();

		tris.get()->need_normals();
		tris.get()->need_tstrips();
	}
};


//
//	Just a convenient wrapper to plug into the system
//
class TriangleMesh : public BaseObject {
public:
	TriangleMesh() : tris(NULL), filename("") {}

	std::shared_ptr<trimesh::TriMesh> as_TriMesh(){
		if( tris == NULL ){ build_trimesh(); }
		return tris;
	}

	void init( const std::vector< Param > &params ){
		filename = "";
		for( int i=0; i<params.size(); ++i ){
			if( parse::to_lower(params[i].name)=="file" ){ filename=params[i].as_string(); }
		}
		if( !filename.size() ){ printf("\nTriangleMesh Error: No file specified"); assert(false); }

	}

	void apply_xform( const trimesh::xform &xf ){
		if( tris == NULL ){ build_trimesh(); }
		trimesh::apply_xform( tris.get(), xf );
	}

private:
	std::string filename;
	std::shared_ptr<trimesh::TriMesh> tris;

	void build_trimesh(){
		if( tris == NULL ){ tris = std::shared_ptr<trimesh::TriMesh>( trimesh::TriMesh::read( filename.c_str() ) ); }
		else{ tris.reset( trimesh::TriMesh::read( filename.c_str() ) ); }

		// Try to load the trimesh
		if( !tris.get() ){ printf("\nTriMesh Error: Could not load %s", filename.c_str() ); assert(false); }
		tris.get()->set_verbose(0);

		// Now clean the mesh
		trimesh::remove_unused_vertices( tris.get() );

		// Create triangle strip for rendering
		tris.get()->need_normals();
		tris.get()->need_tstrips();
	}
};


} // end namespace mcl

#endif
