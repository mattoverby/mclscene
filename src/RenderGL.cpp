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

static inline std::string fullpath( std::string file ){
	std::stringstream ss; ss << MCLSCENE_SRC_DIR << "/" << file;
	return ss.str();
}

static inline float lerp(float a, float b, float f){ return a + f * (b - a); }


// RenderQuad() Renders a 1x1 quad in NDC, best used for framebuffer color targets
void RenderGL::RenderQuad(){
	if (quadVAO == 0){
		GLfloat quadVertices[] = {
		// Positions        // Texture Coords
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// Setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}



bool RenderGL::init( mcl::SceneManager *scene_, int win_width, int win_height ) {

	scene = scene_;
	quadVAO = 0;
	quadVAO = 0;
	quadVBO = 0;
	gBuffer = 0;
	gPosition = 0;
	gNormal = 0;
	gDiffuse = 0;
	gSpec = 0;
	rboDepth = 0;
	ssaoFBO = 0;
	ssaoBlurFBO = 0;
	lightingFBO = 0;
	lightingBuffer = 0;
	ssaoColorBuffer = 0;
	ssaoColorBufferBlur = 0;
	noiseTexture = 0;

	shaderGeometryPass.init_from_files(fullpath("src/shaders/ssao_geometry.vs"), fullpath("src/shaders/ssao_geometry.frag"));
	shaderLightingPass.init_from_files(fullpath("src/shaders/passthrough.vs"), fullpath("src/shaders/ssao_lighting.frag"));
	shaderSSAO.init_from_files(fullpath("src/shaders/passthrough.vs"), fullpath("src/shaders/ssao.frag"));
	shaderSSAOBlur.init_from_files(fullpath("src/shaders/passthrough.vs"), fullpath("src/shaders/ssao_blur.frag"));
	shaderFXAA.init_from_files(fullpath("src/shaders/passthrough.vs"), fullpath("src/shaders/fxaa.frag"));

	// Set samplers
	shaderLightingPass.enable();
	glUniform1i(shaderLightingPass.uniform("gPosition"), 0);
	glUniform1i(shaderLightingPass.uniform("gNormal"), 1);
	glUniform1i(shaderLightingPass.uniform("gDiffuse"), 2);
	glUniform1i(shaderLightingPass.uniform("gSpec"), 3); 
	glUniform1i(shaderLightingPass.uniform("ssao"), 4); 
	shaderSSAO.enable();
	glUniform1i(shaderSSAO.uniform("gPosition"), 0);
	glUniform1i(shaderSSAO.uniform("gNormal"), 1);
	glUniform1i(shaderSSAO.uniform("texNoise"), 2);

	update_window_size( win_width, win_height );

	// Sample kernel
	std::uniform_real_distribution<float> randomFloats(0.f,1.f);
	std::default_random_engine generator;
	for(int i = 0; i < 32; ++i){
		Vec3f sample(randomFloats(generator)*2.f-1.f, randomFloats(generator)*2.f-1.f, randomFloats(generator));
		sample.normalize();
		sample *= randomFloats(generator);
		float scale = float(i) / 64.0;

		// Scale samples s.t. they're more aligned to center of kernel
		scale = lerp(0.1f, 1.0f, scale*scale);
		sample *= scale;
		ssaoKernel.push_back(sample);
	}

	// Noise texture
	std::vector<Vec3f> ssaoNoise;
	for(int i=0; i<16; i++){
		Vec3f noise(randomFloats(generator)*2.f-1.f, randomFloats(generator)*2.f-1.f, 0.f); // rotate around z-axis (in tangent space)
		ssaoNoise.push_back(noise);
	}
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Load model textures
	load_textures();

	return true;

} // end init shaders


void RenderGL::update_window_size( int win_width, int win_height ){

	window_size = Vec2i( win_width, win_height );

	// Set up G-Buffer
	//
	// 3 textures:
	// 1. Positions (RGB)
	// 2. Color (RGB) 
	// 3. Normals (RGB) 
	if( !gBuffer ){ glGenFramebuffers(1, &gBuffer); }
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

	// - Position buffer
	if( !gPosition ){ glGenTextures(1, &gPosition); }
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, win_width, win_height, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

	// - Normal color buffer
	if( !gNormal ){ glGenTextures(1, &gNormal); }
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, win_width, win_height, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

	// - Diffuse color buffer
	if( !gDiffuse ){ glGenTextures(1, &gDiffuse); }
	glBindTexture(GL_TEXTURE_2D, gDiffuse);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, win_width, win_height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gDiffuse, 0);

	// - Specular color buffer
	if( !gSpec ){ glGenTextures(1, &gSpec); }
	glBindTexture(GL_TEXTURE_2D, gSpec);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, win_width, win_height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gSpec, 0);

	// - Tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	const GLuint attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);

	// - Create and attach depth buffer (renderbuffer)
	if( !rboDepth ){ glGenRenderbuffers(1, &rboDepth); }
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, win_width, win_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

	// - Finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
		std::cout << "GBuffer Framebuffer not complete!" << std::endl;
	}

	// Also create framebuffer to hold SSAO processing stage 
	if( !ssaoFBO ){ glGenFramebuffers(1, &ssaoFBO); }
	if( !ssaoBlurFBO ){ glGenFramebuffers(1, &ssaoBlurFBO); }
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

	// - SSAO color buffer
	if( !ssaoColorBuffer ){ glGenTextures(1, &ssaoColorBuffer); }
	glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, win_width, win_height, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
	std::cout << "SSAO Framebuffer not complete!" << std::endl; }

	// - and blur stage
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
	if( !ssaoColorBufferBlur ){ glGenTextures(1, &ssaoColorBufferBlur); }
	glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, win_width, win_height, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
	std::cout << "SSAO Blur Framebuffer not complete!" << std::endl; }

	// Render stage
	if( !lightingFBO ){ glGenFramebuffers(1, &lightingFBO); }
	glBindFramebuffer(GL_FRAMEBUFFER, lightingFBO);
	if( !lightingBuffer ){ glGenTextures(1, &lightingBuffer); }
	glBindTexture(GL_TEXTURE_2D, lightingBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, win_width, win_height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightingBuffer, 0);

	// Done
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


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

	//
	//	Update VBOs
	//
	for( int i=0; i<scene->objects.size(); ++i ){

		// Only update the VBOs if we have to
		BaseObject::AppData *mesh = &scene->objects[i]->app;
		if( mesh->faces_ibo > 0 && mesh->tris_vao > 0 && !update_vbo ){ continue; }

		if( mesh->subdivide_mesh ){

			// Do subdivision with trimesh
			trimesh::TriMesh tempmesh;
			trimesh_copy( &tempmesh, mesh );
			trimesh::subdiv( &tempmesh ); // creates faces
			tempmesh.need_normals(true);

			// We will use the app data stored with that object.
			mesh->vertices = &tempmesh.vertices[0][0];
			mesh->normals = &tempmesh.normals[0][0];
			mesh->faces = &tempmesh.faces[0][0];
			mesh->texcoords = &tempmesh.texcoords[0][0];
			mesh->num_vertices = tempmesh.vertices.size();
			mesh->num_normals = tempmesh.normals.size();
			mesh->num_faces = tempmesh.faces.size();
			mesh->num_texcoords = tempmesh.texcoords.size();
			load_mesh_buffers( mesh );

		} else if( mesh->flat_shading ){

			// Loop through faces and store redundant vertices
			// and normals for each face to do flat shading.
			TriangleMesh tempmesh;
			tempmesh.vertices.reserve( mesh->num_vertices );
			tempmesh.faces.reserve( mesh->num_faces );
			for( int i=0; i<mesh->num_faces; ++i ){
				Vec3i f( mesh->faces[i*3], mesh->faces[i*3+1], mesh->faces[i*3+2] );
				int v_idx = tempmesh.vertices.size();
				tempmesh.vertices.push_back( Vec3f( mesh->vertices[f[0]*3], mesh->vertices[f[0]*3+1], mesh->vertices[f[0]*3+2] ) );
				tempmesh.vertices.push_back( Vec3f( mesh->vertices[f[1]*3], mesh->vertices[f[1]*3+1], mesh->vertices[f[1]*3+2] ) );
				tempmesh.vertices.push_back( Vec3f( mesh->vertices[f[2]*3], mesh->vertices[f[2]*3+1], mesh->vertices[f[2]*3+2] ) );
				tempmesh.faces.push_back( Vec3i(v_idx,v_idx+1,v_idx+2) );
			}
			tempmesh.need_normals(true);
		
			// We will use the app data stored with that object.
			mesh->vertices = &tempmesh.vertices[0][0];
			mesh->normals = &tempmesh.normals[0][0];
			mesh->faces = &tempmesh.faces[0][0];
			mesh->texcoords = 0;
			mesh->num_vertices = tempmesh.vertices.size();
			mesh->num_normals = tempmesh.normals.size();
			mesh->num_faces = tempmesh.faces.size();
			mesh->num_texcoords = 0;
			load_mesh_buffers( mesh );

		} else { load_mesh_buffers( mesh ); }

	} // end update vbos

	//
	//	Geometry Pass
	//
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	geometry_pass( camera );
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//
	//	Lighting Pass
	//
	lighting_pass( camera );

} // end draw objects


void RenderGL::geometry_pass( Camera *camera ){

	shaderGeometryPass.enable();

	// Camera transforms
	trimesh::fxform model;
	glUniformMatrix4fv(shaderGeometryPass.uniform("projection"), 1, GL_FALSE, camera->app.projection);
	glUniformMatrix4fv(shaderGeometryPass.uniform("view"), 1, GL_FALSE, camera->app.view);
	glUniformMatrix4fv(shaderGeometryPass.uniform("model"), 1, GL_FALSE, model);

	//
	//	Geometry pass is for each object
	//
	for( int i=0; i<scene->objects.size(); ++i ){

		// Get the mesh
		BaseObject::AppData *mesh = &scene->objects[i]->app;
		if( mesh->num_vertices <= 0 || mesh->num_faces <= 0 ){ continue; }

		// Get the material
		Material *mat = 0;
		int mat_id = scene->objects[i]->get_material();
		if( mat_id < scene->materials.size() && mat_id >= 0 ){ mat = scene->materials[mat_id].get(); }
		else { mat = &defaultMat; }
		if( mat->app.mode == 2 ){ continue; } // invisible

		// Textures
		GLuint texture_id = 0;
		if( textures.count(mat->app.texture)>0 ){ texture_id = textures[ mat->app.texture ]; }

		// Set material diffuse color
		glUniform4f( shaderGeometryPass.uniform("diff_color"), mat->app.diff[0], mat->app.diff[1], mat->app.diff[2], mat->app.amb.norm() );
		glUniform4f( shaderGeometryPass.uniform("spec_color"), mat->app.spec[0], mat->app.spec[1], mat->app.spec[2], mat->app.shini );

		{ // Draw mesh triangles
			glBindVertexArray(mesh->tris_vao);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->faces_ibo);
			glDrawElements(GL_TRIANGLES, mesh->num_faces*3, GL_UNSIGNED_INT, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}

	}

	glUseProgram(0);

} // end geometry pass



void RenderGL::lighting_pass( Camera *cam ){

	// Camera transforms
	trimesh::fxform model;
	trimesh::fxform &view = cam->app.view;
	trimesh::fxform &projection = cam->app.projection;

        // 2. Create SSAO texture
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
	glClear(GL_COLOR_BUFFER_BIT);
	shaderSSAO.enable();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);

	// Send kernel + rotation 
	for (GLuint i = 0; i < (size_t)ssaoKernel.size(); ++i){
		glUniform3fv(shaderSSAO.uniform(("samples[" + std::to_string(i) + "]").c_str()), 1, &ssaoKernel[i][0]);
	}
	glUniformMatrix4fv(shaderSSAO.uniform("projection"), 1, GL_FALSE, projection);
	RenderQuad();

        // 3. Blur SSAO texture to remove noise
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
            glClear(GL_COLOR_BUFFER_BIT);
            shaderSSAOBlur.enable();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
	RenderQuad();

        // 4. Lighting Pass: traditional deferred Blinn-Phong lighting now with added screen-space ambient occlusion
        glBindFramebuffer(GL_FRAMEBUFFER, lightingFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shaderLightingPass.enable();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gDiffuse);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, gSpec);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);

	// Setup lighting
	glUniform1i( shaderLightingPass.uniform("num_lights"), scene->lights.size() );
	for( int l=0; l<scene->lights.size(); ++l ){
		Light::AppData *light = &scene->lights[l]->app;
		std::stringstream array_ss; array_ss << "lights[" << l << "].";
		std::string array_str = array_ss.str();
		glUniform3f( shaderLightingPass.uniform(array_str+"position"), light->position[0], light->position[1], light->position[2] );
		glUniform3f( shaderLightingPass.uniform(array_str+"direction"), light->direction[0], light->direction[1], light->direction[2] );
		glUniform3f( shaderLightingPass.uniform(array_str+"intensity"), light->intensity[0], light->intensity[1], light->intensity[2] );
		glUniform3f( shaderLightingPass.uniform(array_str+"falloff"), light->falloff[0], light->falloff[1], light->falloff[2] );
		glUniform1f( shaderLightingPass.uniform(array_str+"halfangle"), 0.5*(light->angle*M_PI/180.f) );
		glUniform1i( shaderLightingPass.uniform(array_str+"type"), light->type );
	} // end loop lights

	// Set up camera
	Vec3f eyepos = cam->get_eye();
	glUniform3f( shaderLightingPass.uniform("eye"), eyepos[0], eyepos[1], eyepos[2] );

	RenderQuad(); // render

	// Post-FX
	// 5. Anti-Aliasing (FXAA)
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // Render to screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	shaderFXAA.enable();
	glUniform2f( shaderFXAA.uniform("screen_size"), window_size[0], window_size[1] );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, lightingBuffer);

	// Render to screen space
	RenderQuad();
}


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

		// location=1 is the normal
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->normals_vbo);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, 0);

		// location=2 is the tex coord
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->texcoords_vbo);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, mesh->texcoord_stride(), 0);

		// Done setting data for the vao
		glBindVertexArray(0);
	}

	return true;

}










//RenderGL::~RenderGL(){
	// Apparently in modern GL, textures are released when OpenGL context is
	// destroyed. Unless I missunderstood something...

	// Release texture memory
//	std::unordered_map< std::string, int >::iterator it = textures.begin();
//	for( ; it != textures.end(); ++it ){
//		GLuint texid = it->second;
//		glDeleteTextures(1, &texid);
//	}

	// Release ssao buffers
//	glDeleteTextures(1, &gPosition);
//	glDeleteTextures(1, &gNormal);
//	glDeleteTextures(1, &gDiffuse);
//	glDeleteTextures(1, &gSpec);
//	glDeleteTextures(1, &noiseTexture);
//	glDeleteTextures(1, &ssaoColorBuffer);
//	glDeleteTextures(1, &ssaoColorBufferBlur);
//}



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

/*
// Old draw mesh function, saved for reference
void RenderGL::draw_mesh( BaseObject::AppData *mesh, Material *mat, Camera *camera, bool update_vbo ){

//	draw_mesh_new( mesh, mat, camera, update_vbo ); return;

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
	glDrawElements(GL_TRIANGLES, mesh->num_faces*3, GL_UNSIGNED_INT, 0);

	// Unbind
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	curr_shader->disable();	
}
*/

