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
#include <Eigen/Core>

namespace mcl {

//
//	Base, pure virtual
//
class BaseObject : public std::enable_shared_from_this<BaseObject> {
public:
	virtual ~BaseObject(){}

	// Return the bounding box of the object
	virtual void bounds( Vec3d &bmin, Vec3d &bmax ) = 0;

	// When an object's physical parameters are changed, it may need to
	// update its bounding box, internal parameters, etc...
	virtual void update(){}

	// Apply a transformation matrix to the object. Most objects will
	// just apply the transform directly. Others (instances) will
	// store the xfrom in AppData.
	virtual void apply_xform( const trimesh::xform &xf ){}

	// Used by BVHTraversal.
	virtual bool ray_intersect( const intersect::Ray *ray, intersect::Payload *payload ) const { return false; }

	// Projection on to surface
	virtual Vec3d projection( const Vec3d &point ) const { return point; }

	// Returns a string containing xml code for saving to a scenefile.
	virtual std::string get_xml( int mode=0 ){ return ""; }

	// If an object is made up of other (smaller) objects, they are needed for BVH construction.
	// This function expects you to append the prims vector, not overwrite the whole thing.
	virtual void get_primitives( std::vector< std::shared_ptr<BaseObject> > &prims ){ prims.push_back( shared_from_this() ); }

	// The following data is used by mcl::Application.
	// Derived object is responsible for allocating/deallocating AppData::mesh.
	struct AppData {
		AppData() : mesh(NULL), material(-1), faces(0), is_dynamic(true),
		num_vertices(0), num_normals(0), num_colors(0), num_faces(0),
		verts_vbo(0), colors_vbo(0), normals_vbo(0), faces_ibo(0), tris_vao(0) {}

		~AppData(){ if( faces ){ delete faces; } }

		trimesh::TriMesh *mesh; // If null, the object is not rendered
		int material; // material index into SceneManager::materials
		trimesh::fxform xf; // xform used for instancing, inits to identity

		size_t stride(){ return 3*sizeof(float); }

		// Temporary function for debugging:
		void update( trimesh::TriMesh *themesh ){

			if(themesh==NULL){ return; }
			themesh->need_normals();
			themesh->need_faces();
			if( themesh->vertices.size() ){ vertices = &themesh->vertices[0][0]; }
			if( themesh->normals.size() ){ normals = &themesh->normals[0][0]; }
			if( themesh->colors.size() ){ colors = &themesh->colors[0][0]; }
			if( num_faces<themesh->faces.size() ){ faces = new int[themesh->faces.size()*3]; }
			for( int i=0; i<themesh->faces.size(); ++i ){
				faces[i*3+0] = themesh->faces[i][0];
				faces[i*3+1] = themesh->faces[i][1];
				faces[i*3+2] = themesh->faces[i][2];
			}

			num_vertices = themesh->vertices.size();
			num_normals = themesh->normals.size();
			num_colors = themesh->colors.size();
			num_faces = themesh->faces.size();

		}

		// New stuff:
		// Assumes 3D vertices/normals/colors/faces
		bool is_dynamic;
		float* vertices;
		int num_vertices;
		float* normals;
		int num_normals;
		float* colors;
		int num_colors;
		int* faces;
		int num_faces;

		unsigned int verts_vbo, colors_vbo, normals_vbo, faces_ibo, tris_vao;

	} app ;
};

} // end namespace mcl

#endif
