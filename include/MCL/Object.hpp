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

#ifndef MCLSCENE_OBJECT_H
#define MCLSCENE_OBJECT_H 1

#include <memory>
#include "MCL/Vec.hpp"

namespace mcl {


enum class Prim {
	// Point: stride 1
	// Edge: stride 2
	// Triangle: stride 3
	// Tet: stride 4
	Tri, Tet, Edge, Point
};


//
//	Base, pure virtual
//
class BaseObject {
public:
	BaseObject() : material(-1), flags(0) {}
	virtual ~BaseObject(){}

	// Return the bounding box of the object
	virtual void get_bounds( Vec3f &bmin, Vec3f &bmax ) = 0;

	// Get per-vertex data. Some pointers may be left unset if the object
	// does not have values for them. Returns true on success.
	virtual bool get_vertices(
		float* &vertices, int &num_vertices,
		float* &normals, int &num_normals,
		float* &texcoords, int &num_texcoords ) = 0;

	// Compute normals (need to be called on vertex update for physics sims)
	virtual void need_normals( bool recompute=false ){}

	// Get primitive data, returns true on success
	// Prim type is the type of primitive being requested, as objects
	// may be meshes of multiple primitives/simplexes
	virtual bool get_primitives( const Prim &type, int* &indices, int &num_prims ) = 0;

	// Apply a transformation matrix to the object. Most objects will
	// just apply the transform directly. Others (e.g. instances) will just store the xform.
	virtual void apply_xform( const trimesh::xform &xf ){}

	// Returns a string containing xml code for saving to a scenefile.
	virtual std::string get_xml( int mode=0 ){ return ""; }

	// The material index into SceneManager::materials
	int material;

	// Several flags can be tied to an object, to check them:
	//	bool has_flag = obj->flags & BaseObject::SOME_FLAG
	// You can add your own flag by using any bits past LASTFLAG
	int flags;
	enum {
		INVISIBLE = 1 << 1, // Do not render the mesh
		DYNAMIC = 1 << 2, // Use GL_DYNAMIC_DRAW
		SUBDIVIDE = 1 << 3, // Subdivide faces for smoother rendering
		FLAT = 1 << 4, // Use flat shading
		WIREFRAME = 1 << 5, // Use wireframe rendering
		LASTFLAG = 1 << 6,
	};
};


} // end namespace mcl

#endif
