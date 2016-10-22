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

#ifndef MCLSCENE_OBJECT_H
#define MCLSCENE_OBJECT_H 1

#include <memory>
#include "XForm.h"
#include "TriMesh.h"
#include "RayIntersect.hpp"

namespace mcl {

//
//	Base, pure virtual
//
class BaseObject : public std::enable_shared_from_this<BaseObject> {
public:
	virtual ~BaseObject(){}

	// Return the bounding box of the object
	virtual void bounds( trimesh::vec &bmin, trimesh::vec &bmax ) = 0;

	// When an object's physical parameters are changed, it may need to
	// update its bounding box, internal parameters, etc...
	virtual void update(){}

	// Apply a transformation matrix to the object. Most objects will
	// just apply the transform directly. Others (instances) will
	// store the xfrom in AppData.
	virtual void apply_xform( const trimesh::xform &xf ){}

	// Used by BVHTraversal.
	virtual bool ray_intersect( const intersect::Ray *ray, intersect::Payload *payload ) const { return false; }

	// Returns a string containing xml code for saving to a scenefile.
	virtual std::string get_xml( int mode=0 ){ return ""; }

	// If an object is made up of other (smaller) objects, they are needed for BVH construction.
	// This function expects you to append the prims vector, not overwrite the whole thing.
	virtual void get_primitives( std::vector< std::shared_ptr<BaseObject> > &prims ){ prims.push_back( shared_from_this() ); }

	// The following data is used by mcl::Application.
	// Derived object is responsible for allocating/deallocating AppData::mesh.
	struct AppData {
		AppData() : mesh(NULL), material(-1) {}
		trimesh::TriMesh *mesh; // If null, the object is not rendered
		int material; // material index into SceneManager::materials
		trimesh::xform xf; // xform used for instancing, inits to identity
	} app ;
};


} // end namespace mcl

#endif
