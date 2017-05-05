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
#include "TriMesh_algo.h"
#include "MCL/TriangleMesh.hpp"

using namespace mcl;

bool SimpleRenderGL::init( mcl::SceneManager *scene_, int win_width, int win_height ) {

	scene = scene_;
	defaultMat = std::shared_ptr<Material>( new Material() );

	// Create a basic white texture to use in the absence of textures
	mcl::Vec3f white(1,1,1);
	defaultTex = std::unique_ptr<Texture>( new Texture() );
	defaultTex->create_from_memory(1,1,&white[0]);

	// Create shaders
	std::stringstream bp_ss;
	bp_ss << MCLSCENE_SRC_DIR << "/src/shaders/blinnphong.";
	blinnphong.init_from_files( bp_ss.str()+"vert", bp_ss.str()+"frag");

	return true;

} // end init shaders


void SimpleRenderGL::draw_objects( bool update_vbo ){

	Camera *camera = scene->cameras[0].get();

	//
	//	Update VBOs
	//
	int n_obj = scene->objects.size();
	for( int i=0; i<n_obj; ++i ){

		// Initialize the render meshes. Doing this here allows new meshes
		// to be added while the scene is rendering.
		if( i >= render_meshes.size() ){
			int mat_idx = scene->objects[i]->material;
			std::shared_ptr<mcl::Material> mat(NULL);
			if( mat_idx == Material::NOTSET ){ mat = defaultMat; }
			else if( mat_idx >= 0 && mat_idx < scene->materials.size() ){ mat = scene->materials[mat_idx]; }
			render_meshes.push_back( std::shared_ptr<RenderMesh>( new RenderMesh( scene->objects[i], mat ) ) );
		}

		// Only update the VBOs if we have to
		RenderMesh *mesh = render_meshes[i].get();
		if( mesh->faces_ibo > 0 && mesh->tris_vao > 0 && !update_vbo ){ continue; }
		if( mesh->is_invisible() ){ continue; }

		mesh->load_buffers();

	} // end update vbos

	blinnphong.enable();

	//
	//	Lights
	//
	int nl = scene->lights.size();
	glUniform1i( blinnphong.uniform("num_lights"), nl );
	for( size_t l=0; l<nl; ++l ){
		Light::AppData *light = &scene->lights[l]->app;
		std::stringstream array_ss; array_ss << "lights[" << l << "].";
		std::string array_str = array_ss.str();
		glUniform3f( blinnphong.uniform(array_str+"position"), light->position[0], light->position[1], light->position[2] );
		glUniform3f( blinnphong.uniform(array_str+"direction"), light->direction[0], light->direction[1], light->direction[2] );
		glUniform3f( blinnphong.uniform(array_str+"intensity"), light->intensity[0], light->intensity[1], light->intensity[2] );
		glUniform3f( blinnphong.uniform(array_str+"falloff"), light->falloff[0], light->falloff[1], light->falloff[2] );
		glUniform1f( blinnphong.uniform(array_str+"halfangle"), 0.5*(light->angle*M_PI/180.f) );
		glUniform1i( blinnphong.uniform(array_str+"type"), light->type );
	} // end loop lights
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//
	//	Camera
	//
	trimesh::fxform model;
	glUniformMatrix4fv(blinnphong.uniform("projection"), 1, GL_FALSE, camera->get_projection());
	glUniformMatrix4fv(blinnphong.uniform("view"), 1, GL_FALSE, camera->get_view());
	glUniformMatrix4fv(blinnphong.uniform("model"), 1, GL_FALSE, model);
	Vec3f eyepos = camera->get_eye();
	glUniform3f( blinnphong.uniform("eye"), eyepos[0], eyepos[1], eyepos[2] );

	//
	//	Action
	//
	for( int i=0; i<render_meshes.size(); ++i ){
		RenderMesh *mesh = render_meshes[i].get();
		draw_mesh( mesh );
	}

	glUseProgram(0);

} // end draw objects



void SimpleRenderGL::draw_mesh( RenderMesh *mesh ){

	if( mesh->num_vertices <= 0 || mesh->num_faces <= 0 ){ return; }
	if( mesh->is_invisible() ){ return; }
	glActiveTexture(GL_TEXTURE0);
	if( mesh->tex_id > 0 ){ glBindTexture( GL_TEXTURE_2D, mesh->tex_id); }
	else { glBindTexture( GL_TEXTURE_2D, defaultTex->handle() ); }

	// Set material
	mcl::Material *mat = mesh->material.get();
	glUniform3f( blinnphong.uniform("material.diffuse"), mat->app.diff[0], mat->app.diff[1], mat->app.diff[2] );
	glUniform4f( blinnphong.uniform("material.specular"), mat->app.spec[0], mat->app.spec[1], mat->app.spec[2], mat->app.shini );
	glUniform3f( blinnphong.uniform("material.ambient"), mat->app.amb[0], mat->app.amb[1], mat->app.amb[2] );

	// Draw the mesh
	glBindVertexArray(mesh->tris_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->faces_ibo);
	glDrawElements(GL_TRIANGLES, mesh->num_faces*3, GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glBindTexture( GL_TEXTURE_2D, 0 );

} // end draw



