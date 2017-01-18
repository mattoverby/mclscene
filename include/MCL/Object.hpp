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
#include "RayIntersect.hpp"

namespace mcl {

class RenderMesh; // forward declare

//
//	Base, pure virtual
//
class BaseObject : public std::enable_shared_from_this<BaseObject> {
public:
	virtual ~BaseObject(){}

	// Return the bounding box of the object
	virtual void get_bounds( Vec3f &bmin, Vec3f &bmax ) = 0;

	// Called externally when the object has been modified, sometimes
	// needed to update bounding, recalculate normals, etc...
	virtual void update(){}

	// Apply a transformation matrix to the object. Most objects will
	// just apply the transform directly. Others (instances) will just store the xform.
	virtual void apply_xform( const trimesh::xform &xf ){}

	// Used by BVHTraversal.
	virtual bool ray_intersect( const intersect::Ray *ray, intersect::Payload *payload ) const { return false; }

	// Projection on to surface
	virtual Vec3f projection( const Vec3f &point ) const { return point; }

	// Returns a string containing xml code for saving to a scenefile.
	virtual std::string get_xml( int mode=0 ){ return ""; }

	// Returns the material index in SceneManager::materials
	virtual int get_material() const { return app.material; }

	// If an object is made up of other (smaller) objects, they are needed for BVH construction.
	// This function expects you to append the prims vector, not overwrite the whole thing.
	virtual void get_primitives( std::vector< std::shared_ptr<BaseObject> > &prims ){ prims.push_back( shared_from_this() ); }

	// The following data is used by mcl::Application.
	// Derived object is responsible for managing these pointers.
	struct AppData {
		AppData() : material(-1), dynamic(false), subdivide_mesh(false),
		vertices(0), normals(0), colors(0), texcoords(0), faces(0),
		num_vertices(0), num_normals(0), num_colors(0), num_texcoords(0), num_faces(0),
		verts_vbo(0), colors_vbo(0), normals_vbo(0), texcoords_vbo(0), faces_ibo(0), tris_vao(0) {}

		// If the vertices are often changing (e.g. a deforming mesh) set this to true.
		bool dynamic;
		bool subdivide_mesh; // subdivide the mesh before rendering for better visuals

		// Index into SceneManager::materials
		int material;

		// Vertex stride used for vertices, normals, and colors
		virtual size_t vertex_stride(){ return sizeof(float)*3; }
		virtual size_t texcoord_stride(){ return sizeof(float)*2; }
		virtual size_t face_stride(){ return sizeof(int)*3; }

		float* vertices; // XYZ
		float* normals; // XYZ, normalized
		float* colors; // RGB, 0-1
		float* texcoords; // uv tex coords
		int* faces;
		int num_vertices, num_normals, num_colors, num_texcoords, num_faces;
		unsigned int verts_vbo, colors_vbo, normals_vbo, texcoords_vbo, faces_ibo, tris_vao;
	} app ;
};


} // end namespace mcl

#endif
