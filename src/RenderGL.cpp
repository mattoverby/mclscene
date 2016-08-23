// Copyright 2016 Matthew Overby.
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

using namespace mcl;

bool RenderGL::init( mcl::SceneManager *scene_, AppCamera *cam_ ){

	scene = scene_;
	camera = cam_;

	std::stringstream bp_ss;
	bp_ss << MCLSCENE_SRC_DIR << "/src/blinnphong.";

	blinnphong = std::unique_ptr<Shader>( new Shader() );
	blinnphong->init_from_files( bp_ss.str()+"vert", bp_ss.str()+"frag");

	// Get lighting properties
	for( int l=0; l<scene->lights.size(); ++l ){
		// Max number of lights is 8
		if( scene->lights[l]->get_type()=="ogl" && lights.size() < 8 ){
			lights.push_back( std::dynamic_pointer_cast<OGLLight>(scene->lights[l]) );
		}
	}

	return true;

} // end init shaders


void RenderGL::draw_objects(){

	for( int i=0; i<scene->objects.size(); ++i ){
		std::string mat = scene->objects[i]->get_material();
		if( mat.size() > 0 ){
			draw( scene->objects[i], scene->materials_map[mat] );
		} else {
			draw( scene->objects[i] );
		}
	}

} // end draw objects


void RenderGL::draw( std::shared_ptr<BaseObject> obj, std::shared_ptr<BaseMaterial> mat ){

	trimesh::TriMesh *themesh = obj->get_TriMesh().get();
	if( themesh==NULL ){ return; }

	// Vertices
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer( 3, GL_FLOAT, sizeof(themesh->vertices[0]), &themesh->vertices[0][0] );

	// Normals
	if (!themesh->normals.empty() ) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer( GL_FLOAT, sizeof(themesh->normals[0]), &themesh->normals[0][0] );
	} else { glDisableClientState(GL_NORMAL_ARRAY); }

	// Draw a point cloud
	if( obj->get_type()=="pointcloud" ){

		// TODO

	} // end draw vertices as points

	// Draw the mesh
	else{
		// Get material properties
		trimesh::vec ambient(0,0,0);
		trimesh::vec diffuse(1,0,0);
		trimesh::vec specular(.8,.8,.8);
		float shininess = 32.f;
		if( mat!=NULL ){ // Need to have a better way than dynamic cast
			if( mat->get_type()=="ogl" ){
				std::shared_ptr<OGLMaterial> glMat = std::dynamic_pointer_cast<OGLMaterial>(mat);
				ambient = glMat->ambient;
				diffuse = glMat->diffuse;
				specular = glMat->specular;
				shininess = glMat->shininess;
			}
		}	

		blinnphong->enable();

		// Set the matrices
		glUniformMatrix4fv( blinnphong->uniform("model"), 1, GL_FALSE, camera->model );
		glUniformMatrix4fv( blinnphong->uniform("view"), 1, GL_FALSE, camera->view );
		glUniformMatrix4fv( blinnphong->uniform("projection"), 1, GL_FALSE, camera->projection );
		trimesh::XForm<float> eyepos = trimesh::inv( (camera->projection) * (camera->view) * (camera->model) );
		glUniform3f( blinnphong->uniform("CamPos"), eyepos(0,3), eyepos(1,3), eyepos(2,3) );

		// Set lighting properties
		glUniform1i( blinnphong->uniform("num_point_lights"), lights.size() );
		for( int l=0; l<lights.size(); ++l ){

			OGLLight *light = lights[l].get();
			std::stringstream array_ss; array_ss << "pointLights[" << l << "].";
			std::string array_str = array_ss.str();

			glUniform3f( blinnphong->uniform(array_str+"position"), light->m_pos[0], light->m_pos[1], light->m_pos[2] );
			glUniform3f( blinnphong->uniform(array_str+"intensity"), light->m_diffuse[0], light->m_diffuse[1], light->m_diffuse[2] );
		}

		// Set material properties
		glUniform3f( blinnphong->uniform("material.ambient"), ambient[0], ambient[1], ambient[2] );
		glUniform3f( blinnphong->uniform("material.diffuse"), diffuse[0], diffuse[1], diffuse[2] );
		glUniform3f( blinnphong->uniform("material.specular"), specular[0], specular[1], specular[2] );
		glUniform1f( blinnphong->uniform("material.shininess"), shininess );

		// Triangle strips
		const int *t = &themesh->tstrips[0];
		const int *end = t + themesh->tstrips.size();
		while (likely(t < end)) {
			int striplen = *t++;
			glDrawElements(GL_TRIANGLE_STRIP, striplen, GL_UNSIGNED_INT, t);
			t += striplen;
		}

		blinnphong->disable();

	} // end draw as triangle mesh

} // end draw


void RenderGL::draw_lights(){

	for( int i=0; i<lights.size(); ++i ){
		//TODO
	}

} // end draw lights


