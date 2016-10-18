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
	active_camera_idx = 0;

	std::stringstream bp_ss;
	bp_ss << MCLSCENE_SRC_DIR << "/src/blinnphong.";

	// Create shaders
	blinnphong = std::unique_ptr<Shader>( new Shader() );
	blinnphong->init_from_files( bp_ss.str()+"vert", bp_ss.str()+"frag");

	// Create cameras
	reload_cameras();
	camera = &cameras[active_camera_idx];

	// Get lighting properties
	reload_lights();

	// Load the materials and textures
	reload_materials();

	return true;

} // end init shaders


void RenderGL::reload_materials(){

	// Load the materials and textures
	for( int i=0; i<scene->materials.size(); ++i ){

		// Get the app information of the material
		std::shared_ptr< BaseMaterial > mat = scene->materials[i];
		if( i < materials.size() ){ mat->get_app( materials[i] ); }
		else{
			// A new material has appeared
			materials.push_back( AppMaterial() );
			mat->get_app( materials[i] );
		}
		AppMaterial *appmat = &materials[i];

		// Load the texture if it hasn't been loaded already.
		if( appmat->texture.size() && textures.count(appmat->texture)>0 ){

			int channels, tex_width, tex_height;
			GLuint texture_id = SOIL_load_OGL_texture( appmat->texture.c_str(), &tex_width, &tex_height, &channels, SOIL_LOAD_AUTO, 0, 0 );
			if( texture_id == 0 ){ std::cerr << "\n**Texture::load Error: Failed to load file " << appmat->texture << std::endl; continue; }

			// Add some filters to this texture
			glBindTexture(GL_TEXTURE_2D, texture_id);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);

			// Store it for later use
			textures[ appmat->texture ] = texture_id;
		}
	}	

} // end reload materials


void RenderGL::reload_lights(){

	for( int l=0; l<scene->lights.size() && l < 8; ++l ){
		if( l < lights.size() ){ scene->lights[l]->get_app( lights[l] ); }
		else{
			lights.push_back( AppLight() );
			scene->lights[l]->get_app( lights[l] );
		}
	}

} // end reload lights


void RenderGL::reload_cameras(){

	for( int i=0; i<scene->cameras.size(); ++i ){
		if( i < cameras.size() ){ scene->cameras[i]->get_app( cameras[i] ); }
		else{
			cameras.push_back( AppCamera() );
			scene->cameras[i]->get_app( cameras[i] );
		}
	}

	if( cameras.size()==0 ){
		cameras.push_back( AppCamera() );
	}

} // end reload lights


void RenderGL::draw_objects(){

	AppMaterial defaultMat;

	for( int i=0; i<scene->objects.size(); ++i ){

		int mat = scene->objects[i]->get_material();

		trimesh::TriMesh *themesh = scene->objects[i]->get_TriMesh().get();
		if( themesh==NULL ){ continue; }

		AppMaterial *appmat = NULL;
		if( mat < materials.size() && mat >= 0 ){ appmat = &materials[mat]; }
		else { appmat = &defaultMat; }

		draw_mesh( themesh, appmat );
	}

} // end draw objects


void RenderGL::draw_objects_subdivided(){

	AppMaterial defaultMat;

	for( int i=0; i<scene->objects.size(); ++i ){

		int mat = scene->objects[i]->get_material();

		trimesh::TriMesh *themesh = scene->objects[i]->get_TriMesh().get();
		if( themesh==NULL ){ continue; }

		// Subdivide the mesh and draw that
		trimesh::TriMesh mesh2( *themesh );

		// Only subdivide if necessary
		if( themesh->vertices.size() > 100 ){
			trimesh::subdiv( &mesh2 );
		}

		mesh2.need_normals(true);
		mesh2.need_tstrips();

		AppMaterial *appmat = NULL;
		if( mat < materials.size() && mat >= 0 ){ appmat = &materials[mat]; }
		else { appmat = &defaultMat; }

		draw_mesh( &mesh2, appmat );
	}

} // end draw objects


void RenderGL::draw_mesh( trimesh::TriMesh *themesh, AppMaterial* mat ){

	if( themesh==NULL || mat==NULL ){ return; }
	if( mat->mode == 2 ){ return; } // invisible

	// Vertices
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer( 3, GL_FLOAT, sizeof(themesh->vertices[0]), &themesh->vertices[0][0] );

	// Normals
	if( themesh->normals.size() == 0 ){ themesh->need_normals(true); }
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer( GL_FLOAT, sizeof(themesh->normals[0]), &themesh->normals[0][0] );

	// Texture coordinates
	GLuint texture_id = 0;
	if( !themesh->texcoords.empty() ){
		if( textures.count(mat->texture)>0 ){
			texture_id = textures[ mat->texture ];
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, sizeof(themesh->texcoords[0]), &themesh->texcoords[0][0]);
		} else { glDisableClientState(GL_TEXTURE_COORD_ARRAY); }
	} else { glDisableClientState(GL_TEXTURE_COORD_ARRAY); }

	// Color array -> todo
//	if( !themesh->colors.empty() ){
//		glEnableClientState(GL_COLOR_ARRAY);
//		glColorPointer(3, GL_FLOAT, sizeof(themesh->colors[0]), &themesh->colors[0][0]);
//	} else { glDisableClientState(GL_COLOR_ARRAY); }

	// For setting gl_PointSize
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

	// Get material properties
	trimesh::vec ambient = mat->amb;
	trimesh::vec diffuse = mat->diff;
	trimesh::vec specular = mat->spec;
	float shininess = mat->shini;


	//
	//	Set up lighting and materials
	//
	glBindTexture(GL_TEXTURE_2D, texture_id);
	blinnphong->enable();

	// Texture stuff
	glUniform1i( blinnphong->uniform("hastex"), int(texture_id) );

	// Set the matrices
	glUniformMatrix4fv( blinnphong->uniform("model"), 1, GL_FALSE, camera->model );
	glUniformMatrix4fv( blinnphong->uniform("view"), 1, GL_FALSE, camera->view );
	glUniformMatrix4fv( blinnphong->uniform("projection"), 1, GL_FALSE, camera->projection );
	trimesh::XForm<float> eyepos = trimesh::inv( (camera->projection) * (camera->view) * (camera->model) );
	glUniform3f( blinnphong->uniform("CamPos"), eyepos(0,3), eyepos(1,3), eyepos(2,3) );

	// Set lighting properties
	glUniform1i( blinnphong->uniform("num_point_lights"), lights.size() );
	for( int l=0; l<lights.size(); ++l ){

		AppLight *light = &lights[l];
		std::stringstream array_ss; array_ss << "pointLights[" << l << "].";
		std::string array_str = array_ss.str();

		glUniform3f( blinnphong->uniform(array_str+"position"), light->position[0], light->position[1], light->position[2] );
		glUniform3f( blinnphong->uniform(array_str+"intensity"), light->intensity[0], light->intensity[1], light->intensity[2] );
		glUniform3f( blinnphong->uniform(array_str+"falloff"), light->falloff[0], light->falloff[1], light->falloff[2] );
	}

	// Set material properties
	glUniform3f( blinnphong->uniform("material.ambient"), ambient[0], ambient[1], ambient[2] );
	glUniform3f( blinnphong->uniform("material.diffuse"), diffuse[0], diffuse[1], diffuse[2] );
	glUniform3f( blinnphong->uniform("material.specular"), specular[0], specular[1], specular[2] );
	glUniform1f( blinnphong->uniform("material.shininess"), shininess );

	//
	//	Draw a solid mesh
	//	TODO array buffers for tstrips
	//
	if( mat->mode==0 ){

		if( themesh->tstrips.size()==0 ){ themesh->need_tstrips(); }

		// This doesn't work:
//		glDrawElements(GL_TRIANGLE_STRIP, (themesh->tstrips.size()/3), GL_UNSIGNED_INT, &themesh->tstrips[0]);

		// But this does:
		int *t = &themesh->tstrips[0];
		const int *end = t + themesh->tstrips.size();
		while( t < end ){
			int striplen = *t++;
			glDrawElements(GL_TRIANGLE_STRIP, striplen, GL_UNSIGNED_INT, t);
			t += striplen;
		}

	} // end draw as triangle mesh

	//
	//	Draw a point cloud with gl points for now
	//
	else if( mat->mode==1 ) { glDrawArrays(GL_POINTS, 0, themesh->vertices.size()); }

	blinnphong->disable();
	glBindTexture(GL_TEXTURE_2D, 0);

} // end draw


void RenderGL::draw_lights(){

} // end draw lights


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

