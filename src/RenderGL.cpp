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

#include "MCL/RenderGL.hpp"
#include "MCL/MicroTimer.hpp"
#include "MCL/TriangleMesh.hpp"
#include "MCL/Texture.hpp"

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

	defaultMat = std::shared_ptr<Material>( new Material() );

	scene = scene_;
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
	depthMapFBO = 0;
	lightingBuffer = 0;
	ssaoColorBuffer = 0;
	ssaoColorBufferBlur = 0;
	noiseTexture = 0;
	depthMap = 0;

	shaderGeometryPassTextured.init_from_files(fullpath("src/shaders/ssao_geometry.vs"), fullpath("src/shaders/ssao_geometry_textured.frag"));
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
	int num_ssao_samples = 64;
	for(int i = 0; i < num_ssao_samples; ++i){
		Vec3f sample(randomFloats(generator)*2.f-1.f, randomFloats(generator)*2.f-1.f, randomFloats(generator));
		sample.normalize();
		sample *= randomFloats(generator);
		float scale = float(i) / float(num_ssao_samples);

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
/*
	// Shadows
	if( !depthMapFBO ){ glGenFramebuffers(1, &depthMapFBO); }
	if( !depthMap ){ glGenTextures(1, &depthMap); }
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glNamedFramebufferDrawBuffer(depthMapFBO, GL_NONE);
	glNamedFramebufferDrawBuffer(depthMapFBO, GL_NONE);
*/
	// - SSAO color buffer
	if( !ssaoFBO ){ glGenFramebuffers(1, &ssaoFBO); }
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
	if( !ssaoColorBuffer ){ glGenTextures(1, &ssaoColorBuffer); }
	glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, win_width, win_height, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
	std::cout << "SSAO Framebuffer not complete!" << std::endl; }

	// - and blur stage
	if( !ssaoBlurFBO ){ glGenFramebuffers(1, &ssaoBlurFBO); }
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
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
	std::cout << "Lighting Framebuffer not complete!" << std::endl; }

	// Done
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void RenderGL::draw_objects( bool update_vbo ){

	Camera *camera = scene->cameras[0].get();

	//
	//	Update VBOs
	//
	size_t n_objs = scene->objects.size();
	for( size_t i=0; i<n_objs; ++i ){

		// Initialize the render meshes. Doing this here allows new meshes
		// to be added while the scene is rendering.
		if( i >= render_meshes.size() ){
			int mat_idx = scene->objects[i]->material;
			std::shared_ptr<mcl::Material> mat(NULL);
			if( mat_idx == Material::NOTSET ){ mat = defaultMat; }
			else if( mat_idx >= 0 && mat_idx < scene->materials.size() ){ mat = scene->materials[mat_idx]; }
			render_meshes.push_back( RenderMesh( scene->objects[i], mat ) );
		}

		// Only update the VBOs if we have to
		RenderMesh *mesh = &render_meshes[i];
		if( mesh->faces_ibo > 0 && mesh->tris_vao > 0 && !update_vbo ){ continue; }
		if( mesh->is_invisible() ){ continue; }

		mesh->load_buffers();

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
	glUniformMatrix4fv(shaderGeometryPass.uniform("projection"), 1, GL_FALSE, camera->get_projection());
	glUniformMatrix4fv(shaderGeometryPass.uniform("view"), 1, GL_FALSE, camera->get_view());
	glUniformMatrix4fv(shaderGeometryPass.uniform("model"), 1, GL_FALSE, model);

	//
	//	Geometry pass is for each object
	//
	size_t n_objs = scene->objects.size();
	for( size_t i=0; i<n_objs; ++i ){

		// Get the mesh
		RenderMesh *mesh = &render_meshes[i];
		if( mesh->num_vertices <= 0 || mesh->num_faces <= 0 ){ continue; }
		if( mesh->object->flags & BaseObject::INVISIBLE ){ continue; }
		Material *mat = mesh->material.get();

		if( mesh->tex_id>0 ){
			shaderGeometryPassTextured.enable();
		        glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, mesh->tex_id);

			glUniformMatrix4fv(shaderGeometryPassTextured.uniform("projection"), 1, GL_FALSE, camera->get_projection());
			glUniformMatrix4fv(shaderGeometryPassTextured.uniform("view"), 1, GL_FALSE, camera->get_view());
			glUniformMatrix4fv(shaderGeometryPassTextured.uniform("model"), 1, GL_FALSE, model);
			glUniform4f( shaderGeometryPassTextured.uniform("spec_color"), mat->app.spec[0], mat->app.spec[1], mat->app.spec[2], mat->app.shini );
			glUniform1i( shaderGeometryPassTextured.uniform("red_back"), int(mat->flags & Material::RED_BACKFACE) );
		}
		else{
			// Set material diffuse color
			glUniform4f( shaderGeometryPass.uniform("diff_color"), mat->app.diff[0], mat->app.diff[1], mat->app.diff[2], mat->app.amb.norm() );
			glUniform4f( shaderGeometryPass.uniform("spec_color"), mat->app.spec[0], mat->app.spec[1], mat->app.spec[2], mat->app.shini );
			glUniform1i( shaderGeometryPass.uniform("red_back"), int(mat->flags & Material::RED_BACKFACE) );
		}

		{ // Draw mesh triangles
			glBindVertexArray(mesh->tris_vao);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->faces_ibo);
			glDrawElements(GL_TRIANGLES, mesh->num_faces*3, GL_UNSIGNED_INT, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}

		if( mesh->object->flags & BaseObject::WIREFRAME ){
			glUniform4f( shaderGeometryPass.uniform("diff_color"), 0.f, 0.f, 0.f, mat->app.amb.norm() );
			glUniform4f( shaderGeometryPass.uniform("spec_color"), 0.f, 0.f, 0.f, 1.f );
			glBindVertexArray(mesh->tris_vao);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->wire_ibo);
			glDrawElements(GL_LINES, mesh->num_edges*2, GL_UNSIGNED_INT, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}

		if( mesh->tex_id>0 ){
			glBindTexture( GL_TEXTURE_2D, 0 );
			shaderGeometryPass.enable();
			glUniformMatrix4fv(shaderGeometryPass.uniform("projection"), 1, GL_FALSE, camera->get_projection());
			glUniformMatrix4fv(shaderGeometryPass.uniform("view"), 1, GL_FALSE, camera->get_view());
			glUniformMatrix4fv(shaderGeometryPass.uniform("model"), 1, GL_FALSE, model);
		}
	}

	glUseProgram(0);

} // end geometry pass



void RenderGL::lighting_pass( Camera *cam ){

	// Get scene radius
	float scene_radius = 0.f;
	Vec3f scene_center;
	scene->get_bsphere(&scene_center,&scene_radius,false);
	ssao_radius = scene_radius*0.3f;

	// Camera transforms
	trimesh::fxform model;
//	trimesh::fxform &view = cam->get_view();
	trimesh::fxform &projection = cam->get_projection();

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
	int ks = ssaoKernel.size();
	for (size_t i = 0; i < ks; ++i){
		glUniform3fv(shaderSSAO.uniform(("samples[" + std::to_string(i) + "]").c_str()), 1, &ssaoKernel[i][0]);
	}
	glUniform1f(shaderSSAO.uniform("radius"), ssao_radius);
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
	int nl = scene->lights.size();
	glUniform1i( shaderLightingPass.uniform("num_lights"), nl );
	for( size_t l=0; l<nl; ++l ){
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



RenderGL::~RenderGL(){
	// Apparently in modern GL, textures are released when OpenGL context is
	// destroyed. Unless I missunderstood something...
//	glDeleteTextures(1, &gPosition);
//	glDeleteTextures(1, &gNormal);
//	glDeleteTextures(1, &gDiffuse);
//	glDeleteTextures(1, &gSpec);
//	glDeleteTextures(1, &noiseTexture);
//	glDeleteTextures(1, &ssaoColorBuffer);
//	glDeleteTextures(1, &ssaoColorBufferBlur);
}

