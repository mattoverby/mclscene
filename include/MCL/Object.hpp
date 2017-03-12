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
#include "Raycast.hpp"

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
	virtual bool ray_intersect( const raycast::Ray *ray, raycast::Payload *payload ) const { return false; }

	// Projection on to surface
	virtual Vec3f projection( const Vec3f &point ) const { return point; }
	virtual Vec3f projection( const Vec3f &point, Vec3f &norm ) const { return point; }

	// Returns a string containing xml code for saving to a scenefile.
	virtual std::string get_xml( int mode=0 ){ return ""; }

	// If an object is made up of other (smaller) objects, they are needed for BVH construction.
	// This function expects you to append the prims vector, not overwrite the whole thing.
	virtual void get_primitives( std::vector< std::shared_ptr<BaseObject> > &prims ){ prims.push_back( shared_from_this() ); }

	// The following data is used by mcl::Application.
	// Derived object is responsible for managing these pointers.
	struct AppData {

		AppData() : material(MATERIAL_NOTSET), dynamic(false), subdivide_mesh(0), flat_shading(false), wireframe(false),
			vertices(0), normals(0), texcoords(0), faces(0),
			num_vertices(0), num_normals(0), num_texcoords(0), num_faces(0), num_edges(0),
			verts_vbo(0), normals_vbo(0), texcoords_vbo(0), faces_ibo(0), wire_ibo(0), tris_vao(0) {}

		bool dynamic; // Set to true if mesh vertices often change

		// TODO: make these component of material instead of object
		unsigned int subdivide_mesh; // Number of mesh subdivisions before rendering for better visuals (SLOW)
		bool flat_shading; // Double up on verts/norms before rendering (ALSO SLOW)
		bool wireframe; // Draw edges of the mesh. Can be combined with invisible material. TODO

		// Index into SceneManager::materials
		int material;

		// Vertex stride used for vertices, normals
		size_t vertex_stride(){ return sizeof(float)*3; }
		size_t texcoord_stride(){ return sizeof(float)*2; }
		size_t face_stride(){ return sizeof(int)*3; }

		float* vertices; // XYZ
		float* normals; // XYZ, normalized
		float* texcoords; // uv tex coords
		int* faces;
		int* edges;
		int num_vertices, num_normals, num_texcoords, num_faces, num_edges;
		unsigned int verts_vbo, normals_vbo, texcoords_vbo, barycoords_vbo, faces_ibo, wire_ibo, tris_vao;

	} app ;
};


} // end namespace mcl

#endif
