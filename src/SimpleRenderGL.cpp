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

#include "MCL/SimpleRenderGL.hpp"
#include "SOIL2.h"
#include "MCL/MicroTimer.hpp"
#include "TriMesh_algo.h"
#include "MCL/TriangleMesh.hpp"

using namespace mcl;

SimpleRenderGL::RenderMesh::RenderMesh() : RenderMesh(NULL) {}

SimpleRenderGL::RenderMesh::RenderMesh( std::shared_ptr<BaseObject> obj ) :
	vertices(0), normals(0), texcoords(0), faces(0), edges(0),
	num_vertices(0), num_normals(0), num_texcoords(0), num_faces(0), num_edges(0),
	verts_vbo(0), normals_vbo(0), texcoords_vbo(0), faces_ibo(0), wire_ibo(0), tris_vao(0),
	object(obj) {
	update();
}

void SimpleRenderGL::RenderMesh::update(){

	if( object == NULL ){ return; }

	bool success = object->get_vertices(
		vertices, num_vertices,
		normals, num_normals,
		texcoords, num_texcoords );
	if( !success ){ num_vertices = 0; }

	success = object->get_primitives( Prim::Tri, faces, num_faces );

	if( !success ){ num_faces = 0; }
	object->get_primitives( Prim::Edge, edges, num_edges );

}


bool SimpleRenderGL::init( mcl::SceneManager *scene_, int win_width, int win_height ) {

	scene = scene_;

	// Create shaders
	std::stringstream bp_ss;
	bp_ss << MCLSCENE_SRC_DIR << "/src/shaders/blinnphong.";
	blinnphong.init_from_files( bp_ss.str()+"vert", bp_ss.str()+"frag");

/*
	// Load textures
	std::unordered_map< std::string, std::shared_ptr<BaseMaterial> >::iterator mIter = scene->materials_map.begin();
	for( ; mIter != scene->materials_map.end(); ++mIter ){
		if( mIter->second->texture_file.size() ){

			int channels, tex_width, tex_height;
			GLuint texture_id = SOIL_load_OGL_texture( mIter->second->texture_file.c_str(), &tex_width, &tex_height, &channels, SOIL_LOAD_AUTO, 0, 0 );
			if( texture_id == 0 ){ std::cerr << "\n**Texture::load Error: Failed to load file " << mIter->second->texture_file << std::endl; continue; }

			// Add some filters to this texture
			glBindTexture(GL_TEXTURE_2D, texture_id);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);

			// Store it for later use
			textures[ mIter->second->texture_file ] = texture_id;
		}
	}
*/
	return true;

} // end init shaders


void SimpleRenderGL::draw_objects( bool update_vbo ){

	Camera *camera = scene->cameras[0].get();

	//
	//	Update VBOs
	//
	for( int i=0; i<scene->objects.size(); ++i ){

		if( i >= render_meshes.size() ){
			render_meshes.push_back( RenderMesh( scene->objects[i] ) );
		}

		// Only update the VBOs if we have to
		RenderMesh *mesh = &render_meshes[i];
		if( mesh->faces_ibo > 0 && mesh->tris_vao > 0 && !update_vbo ){ continue; }

		// Skip invisibles
		if( mesh->object->flags & BaseObject::INVISIBLE ){ continue; }

		load_mesh_buffers( mesh );

	} // end update vbos

	blinnphong.enable();

	//
	//	Camera
	//
	trimesh::fxform model;
	glUniformMatrix4fv(blinnphong.uniform("projection"), 1, GL_FALSE, camera->get_projection());
	glUniformMatrix4fv(blinnphong.uniform("view"), 1, GL_FALSE, camera->get_view());
	glUniformMatrix4fv(blinnphong.uniform("model"), 1, GL_FALSE, model);

	//
	//	Lights
	//
	// Everything a point light for now
//	glUniform1i( blinnphong.uniform("num_point_lights"), scene->lights.size() );
//	for( int l=0; l<scene->lights.size(); ++l ){
//		Light *light = scene->lights[l].get();
//		std::stringstream array_ss; array_ss << "pointLights[" << l << "].";
//		std::string array_str = array_ss.str();
//		glUniform3f( blinnphong.uniform(array_str+"position"), light->app.position[0], light->app.position[1], light->app.position[2] );
//		glUniform3f( blinnphong.uniform(array_str+"intensity"), light->app.intensity[0], light->app.intensity[1], light->app.intensity[2] );
//		glUniform3f( blinnphong.uniform(array_str+"falloff"), light->app.falloff[0], light->app.falloff[1], light->app.falloff[2] );
//	}


	//
	//	Meshes
	//
	for( int i=0; i<render_meshes.size(); ++i ){

		RenderMesh *mesh = &render_meshes[i];
		if( mesh->num_vertices <= 0 || mesh->num_faces <= 0 ){ continue; }
		if( mesh->object->flags & BaseObject::INVISIBLE ){ continue; }

		// Get the material
		Material *mat = 0;
		if( mesh->object->material == Material::NOTSET ){ mat = &defaultMat; }
		else{ mat = scene->materials[mesh->object->material].get(); } // could segfault!

		// Draw the mesh
		draw_mesh( mesh, mat );
	}

	glUseProgram(0);

} // end draw objects



void SimpleRenderGL::draw_mesh( SimpleRenderGL::RenderMesh *mesh, Material *mat ){

	glBindVertexArray(mesh->tris_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->faces_ibo);
	glDrawElements(GL_TRIANGLES, mesh->num_faces*3, GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

} // end draw



bool SimpleRenderGL::load_mesh_buffers( RenderMesh *mesh ){

	// Check
	if( mesh->num_vertices<=0 || mesh->num_normals<=0 || mesh->num_faces<=0 ){ return false; }

	GLenum draw_mode = GL_STATIC_DRAW;
	if( mesh->object->flags & BaseObject::DYNAMIC ){ draw_mode = GL_DYNAMIC_DRAW; }

	size_t stride = sizeof(float)*3;

	if( !mesh->verts_vbo ){ // Create the buffer for vertices
		glGenBuffers(1, &mesh->verts_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->verts_vbo);
		glBufferData(GL_ARRAY_BUFFER, mesh->num_vertices*stride, mesh->vertices, draw_mode);
	} else { // Otherwise update
		glBindBuffer(GL_ARRAY_BUFFER, mesh->verts_vbo);
		glBufferSubData( GL_ARRAY_BUFFER, 0, mesh->num_vertices*stride, mesh->vertices );
	}

	if( !mesh->normals_vbo ){ // Create the buffer for normals
		glGenBuffers(1, &mesh->normals_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->normals_vbo);
		glBufferData(GL_ARRAY_BUFFER, mesh->num_normals*stride, mesh->normals, draw_mode);
	} else { // Otherwise update
		glBindBuffer(GL_ARRAY_BUFFER, mesh->normals_vbo);
		glBufferSubData( GL_ARRAY_BUFFER, 0, mesh->num_normals*stride, mesh->normals );
	}

	 // Create the buffer for tex coords, these won't change
//	if( !mesh->texcoords_vbo ){
//		glGenBuffers(1, &mesh->texcoords_vbo);
//		glBindBuffer(GL_ARRAY_BUFFER, mesh->texcoords_vbo);
//		glBufferData(GL_ARRAY_BUFFER, mesh->num_texcoords*sizeof(float)*2, mesh->texcoords, GL_STATIC_DRAW);
//	}

	// Create the buffer for indices, these won't change
	if( !mesh->faces_ibo ){
		glGenBuffers(1, &mesh->faces_ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->faces_ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->num_faces*sizeof(int)*3, mesh->faces, GL_STATIC_DRAW);
	}

	if( !mesh->wire_ibo && ( mesh->object->flags & BaseObject::WIREFRAME ) ){
		glGenBuffers(1, &mesh->wire_ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->wire_ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->num_edges*sizeof(int)*2, mesh->edges, GL_STATIC_DRAW);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Create the VAO
	if( !mesh->tris_vao ){

		glGenVertexArrays(1, &mesh->tris_vao);
		glBindVertexArray(mesh->tris_vao);

		// location=0 is the vertex
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->verts_vbo);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);

		// location=1 is the normal
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->normals_vbo);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, 0);

		// location=2 is the tex coord
//		glEnableVertexAttribArray(2);
//		glBindBuffer(GL_ARRAY_BUFFER, mesh->texcoords_vbo);
//		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float)*2, 0);

		// Done setting data for the vao
		glBindVertexArray(0);
	}

	return true;

}




SimpleRenderGL::~SimpleRenderGL(){
	// Apparently in modern GL, textures are released when OpenGL context is
	// destroyed. Unless I missunderstood something...
}


