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

#include "MCL/RenderGL.hpp"
#include "SOIL2.h"
#include "MCL/MicroTimer.hpp"

using namespace mcl;

RenderGL::~RenderGL(){
	// Release texture memory
	std::unordered_map< std::string, int >::iterator it = textures.begin();
	for( ; it != textures.end(); ++it ){
		GLuint texid = it->second;
		glDeleteTextures(1, &texid);
	}
}

bool RenderGL::init( mcl::SceneManager *scene_ ) {

	scene = scene_;

	std::stringstream bp_ss;
	bp_ss << MCLSCENE_SRC_DIR << "/src/shader";

	// Create shaders
	blinnphong = std::unique_ptr<Shader>( new Shader() );
	blinnphong->init_from_files( bp_ss.str()+".vert", bp_ss.str()+".frag");

	blinnphong_textured = std::unique_ptr<Shader>( new Shader() );
	blinnphong_textured->init_from_files( bp_ss.str()+"_textured.vert", bp_ss.str()+"_textured.frag");

	load_textures();

	return true;

} // end init shaders


void RenderGL::load_textures(){

	// Load the materials and textures
	for( int i=0; i<scene->materials.size(); ++i ){

		// Get the app information of the material
		std::shared_ptr< Material > mat = scene->materials[i];

		// Load the texture if it hasn't been loaded already.
		if( mat->app.texture.size() && textures.count(mat->app.texture)==0 ){

			int channels, tex_width, tex_height;
			GLuint texture_id = SOIL_load_OGL_texture( mat->app.texture.c_str(), &tex_width, &tex_height, &channels, SOIL_LOAD_AUTO, 0, 0 );
			if( texture_id == 0 ){ std::cerr << "\n**Texture::load Error: Failed to load file " << mat->app.texture << std::endl; continue; }

			// Add some filters to this texture
			glBindTexture(GL_TEXTURE_2D, texture_id);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);

			// Store it for later use
			textures[ mat->app.texture ] = texture_id;
		}
	}	

} // end reload materials


void RenderGL::draw_objects( bool update_vbo ){

	Camera *camera = scene->cameras[0].get();

	for( int i=0; i<scene->objects.size(); ++i ){

		std::shared_ptr<BaseObject> obj = scene->objects[i];

		// Get the material
		Material *mat = NULL;
		int mat_id = obj->get_material();
		if( mat_id < scene->materials.size() && mat_id >= 0 ){ mat = scene->materials[mat_id].get(); }
		else { mat = &defaultMat; }

		draw_mesh( &obj->app, mat, camera, update_vbo );
	}

} // end draw objects


void RenderGL::draw_objects_subdivided( bool update_vbo ){

	Camera *camera = scene->cameras[0].get();

	// Making a copy of the mesh with TriMesh, then
	// do subdivision on that (since I don't want to implement it myself).
	// This is pretty dirty and not efficient, but oh well.
	//
	for( int i=0; i<scene->objects.size(); ++i ){

		std::shared_ptr<BaseObject> obj = scene->objects[i];

		// Get the material
		Material *mat = NULL;
		int mat_id = obj->get_material();
		if( mat_id < scene->materials.size() && mat_id >= 0 ){ mat = scene->materials[mat_id].get(); }
		else { mat = &defaultMat; }

		// Only subdivide if it has enough vertices
		if( obj->app.num_vertices < 100 ){
			draw_mesh( &obj->app, mat, camera );
			continue;
		}

		// We will use the app data stored with that object.
		BaseObject::AppData *appdata = &obj->app;
		trimesh::TriMesh tempmesh;

		// I can think of a million ways to break this
		if( update_vbo ){

			// Do subdivision with trimesh
			trimesh_copy( &tempmesh, &obj->app );
			trimesh::subdiv( &tempmesh ); // creates faces
			tempmesh.need_normals(true);

			appdata->vertices = &tempmesh.vertices[0][0];
			appdata->normals = &tempmesh.normals[0][0];
			appdata->faces = &tempmesh.faces[0][0];
			appdata->texcoords = &tempmesh.texcoords[0][0];
			appdata->colors = &tempmesh.colors[0][0];
			appdata->num_vertices = tempmesh.vertices.size();
			appdata->num_normals = tempmesh.normals.size();
			appdata->num_faces = tempmesh.faces.size();
			appdata->num_texcoords = tempmesh.texcoords.size();
			appdata->num_colors = tempmesh.colors.size();
		}

		draw_mesh( appdata, mat, camera, update_vbo );
	}

} // end draw objects


void RenderGL::setup_lights( Shader *curr_shader ){

	glUniform1i( curr_shader->uniform("num_lights"), scene->lights.size() );

	// Set lighting properties
	for( int l=0; l<scene->lights.size(); ++l ){
		Light::AppData *light = &scene->lights[l]->app;
		std::stringstream array_ss; array_ss << "lights[" << l << "].";
		std::string array_str = array_ss.str();
		glUniform3f( curr_shader->uniform(array_str+"position"), light->position[0], light->position[1], light->position[2] );
		glUniform3f( curr_shader->uniform(array_str+"direction"), light->direction[0], light->direction[1], light->direction[2] );
		glUniform3f( curr_shader->uniform(array_str+"intensity"), light->intensity[0], light->intensity[1], light->intensity[2] );
		glUniform3f( curr_shader->uniform(array_str+"falloff"), light->falloff[0], light->falloff[1], light->falloff[2] );
		glUniform1f( curr_shader->uniform(array_str+"halfangle"), 0.5*(light->angle*M_PI/180.f) );
		glUniform1i( curr_shader->uniform(array_str+"type"), light->type );
	} // end loop lights

} // end setup lights


void RenderGL::draw_mesh( BaseObject::AppData *mesh, Material *mat, Camera *camera, bool update_vbo ){

	// Check if mesh has the "invisible" material and we can just ignore it.
	if( mat->app.mode == 2 ){ return; }

	// Check for valid mesh data
	if( mesh->num_vertices <= 0 || mesh->num_faces <= 0 ){ return; }

	// Otherwise check if we need to do an update
	if( mesh->faces_ibo <= 0 || mesh->tris_vao <=0 || update_vbo ){
		if( !load_mesh_buffers( mesh ) ){ return; }
	}

	// Get material properties
	Vec3f ambient = mat->app.amb;
	Vec3f diffuse = mat->app.diff;
	Vec3f specular = mat->app.spec;
	float shininess = mat->app.shini;
	GLuint texture_id = 0;
	if( textures.count(mat->app.texture)>0 ){ texture_id = textures[ mat->app.texture ]; }

	// Start shader
	Shader *curr_shader = blinnphong.get();
	if( texture_id > 0 ){ curr_shader = blinnphong_textured.get(); }
	curr_shader->enable();

	// Bind shader
	glBindTexture( GL_TEXTURE_2D, texture_id );

	// Pass lighting to shader
	setup_lights( curr_shader );

	// Set the camera matrices
	trimesh::fxform model;
	glUniformMatrix4fv( curr_shader->uniform("model"), 1, GL_FALSE, model );
	glUniformMatrix4fv( curr_shader->uniform("view"), 1, GL_FALSE, camera->app.view );
	glUniformMatrix4fv( curr_shader->uniform("projection"), 1, GL_FALSE, camera->app.projection );
	Vec3f eyepos = camera->get_eye();
	glUniform3f( curr_shader->uniform("eye"), eyepos[0], eyepos[1], eyepos[2] );
//	trimesh::XForm<float> eyepos = trimesh::inv( (camera->app.projection) * (camera->app.view) * (model) );
//	glUniform3f( curr_shader->uniform("eye"), eyepos(0,3), eyepos(1,3), eyepos(2,3) );

	// Set material properties
	glUniform3f( curr_shader->uniform("material.ambient"), ambient[0], ambient[1], ambient[2] );
	glUniform3f( curr_shader->uniform("material.diffuse"), diffuse[0], diffuse[1], diffuse[2] );
	glUniform3f( curr_shader->uniform("material.specular"), specular[0], specular[1], specular[2] );
	glUniform1f( curr_shader->uniform("material.shininess"), shininess );

	// Bind buffers
	glBindVertexArray(mesh->tris_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->faces_ibo);

	//
	//	Draw a solid mesh
	//
	if( mat->app.mode==0 ){
		glDrawElements(GL_TRIANGLES, mesh->num_faces*3, GL_UNSIGNED_INT, 0);
	} // end draw as triangle mesh

	// Unbind
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	curr_shader->disable();	

}


// Color blending, saved for reference:
/*
	// From: https://stackoverflow.com/questions/1700211/to-dynamically-increment-from-blue-to-red-using-c
	static inline float blend(float a, float b, float alpha){ return (1.f - alpha) * a + alpha * b; }

	// gradient should be 0-1. blended needs to be a 3-element array
	// From https://stackoverflow.com/questions/1700211/to-dynamically-increment-from-blue-to-red-using-c
	static inline void colorBlend( float *blended, float a[3], float b[3], float gradient ){
		if( gradient > 1.f ){ gradient = 1.f; }
		if( gradient < 0.f ){ gradient = 0.f; }
		blended[0] = blend( a[0], b[0], gradient );
		blended[1] = blend( a[1], b[1], gradient );
		blended[2] = blend( a[2], b[2], gradient );
	}
*/


bool RenderGL::load_mesh_buffers( BaseObject::AppData *mesh ){

	// Check
	if( mesh->num_vertices<=0 || mesh->num_normals<=0 || mesh->num_faces<=0 ){ return false; }

	GLenum draw_mode = GL_STATIC_DRAW;
	if( mesh->dynamic ){ draw_mode = GL_DYNAMIC_DRAW; }

	size_t stride = mesh->vertex_stride();

	if( !mesh->verts_vbo ){ // Create the buffer for vertices
		glGenBuffers(1, &mesh->verts_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->verts_vbo);
		glBufferData(GL_ARRAY_BUFFER, mesh->num_vertices*stride, mesh->vertices, draw_mode);
	} else { // Otherwise update
		glBindBuffer(GL_ARRAY_BUFFER, mesh->verts_vbo);
		glBufferSubData( GL_ARRAY_BUFFER, 0, mesh->num_vertices*stride, mesh->vertices );
	}

	if( !mesh->colors_vbo ){ // Create the buffer for colors
		glGenBuffers(1, &mesh->colors_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->colors_vbo);
		glBufferData(GL_ARRAY_BUFFER, mesh->num_colors*stride, mesh->colors, draw_mode);
	} else { // Otherwise update
		glBindBuffer(GL_ARRAY_BUFFER, mesh->colors_vbo);
		glBufferSubData( GL_ARRAY_BUFFER, 0, mesh->num_colors*stride, mesh->colors );
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
	if( !mesh->texcoords_vbo ){
		glGenBuffers(1, &mesh->texcoords_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->texcoords_vbo);
		glBufferData(GL_ARRAY_BUFFER, mesh->num_texcoords*mesh->texcoord_stride(), mesh->texcoords, GL_STATIC_DRAW);
	}

	// Create the buffer for indices, these won't change
	if( !mesh->faces_ibo ){
		glGenBuffers(1, &mesh->faces_ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->faces_ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->num_faces*mesh->face_stride(), mesh->faces, GL_STATIC_DRAW);
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

		// location=1 is the color
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->colors_vbo);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, 0);

		// location=2 is the normal
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->normals_vbo);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, 0);

		// location=3 is the tex coord
		glEnableVertexAttribArray(3);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->texcoords_vbo);
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, mesh->texcoord_stride(), 0);

		// Done setting data for the vao
		glBindVertexArray(0);
	}

	return true;

}
