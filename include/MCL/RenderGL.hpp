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

#ifndef MCLSCENE_RENDERGL_H
#define MCLSCENE_RENDERGL_H 1

#include "MCL/RenderMesh.hpp"
#include "MCL/Shader.hpp"
#include "MCL/SceneManager.hpp"
#include <random>

namespace mcl {

class RenderGL  {
public:
	std::vector<RenderMesh> render_meshes; // SceneManager::object -> render meshes

	// Initialize shaders. Must be called after
	// OpenGL context has been created.
	bool init( mcl::SceneManager *scene_, int win_width, int win_height );

	// Draws all objects in the SceneManager (that have AppData::mesh)
	// If VBOs have not been generated for the AppData mesh, they will be generated.
	// If the VBOs need to be updated (e.g. a deforming mesh) set update_vbo to true.
	// Texture coordinates and face ibo are NOT updated.
	void draw_objects( bool update_vbo=false );

	// Updates the screen space buffers
	void update_window_size( int win_width, int win_height );

	~RenderGL();

private:
	// For SSAO, do a geometry pass to set up g-buffers
	void geometry_pass( Camera *camera );

	// For SSAO, do the lighting pass using g-buffers
	void lighting_pass( Camera *camera );

	std::shared_ptr<mcl::Material> defaultMat;

	Shader shaderGeometryPass;
	Shader shaderGeometryPassTextured;
	Shader shaderLightingPass;
	Shader shaderSSAO;
	Shader shaderSSAOBlur;
	Shader shaderFXAA;

	Vec2i window_size;

	std::vector<Vec3f> ssaoKernel;
	void RenderQuad();

	GLuint quadVAO, quadVBO; // for deferred shading
	GLuint depthMapFBO; // for shadows
	GLuint depthMap;
	GLuint gBuffer; // G-Buffer
	GLuint gPosition, gNormal, gDiffuse, gSpec; // render buffs
	GLuint rboDepth; // depth buffer
	GLuint ssaoFBO, ssaoBlurFBO; // ambient occlusion
	GLuint lightingFBO, lightingBuffer; // post lighting stage
	GLuint ssaoColorBuffer, ssaoColorBufferBlur;
	GLuint noiseTexture; // occlusion noise

	const GLuint SHADOW_WIDTH = 1024;
	const GLuint SHADOW_HEIGHT = 1024;
	float ssao_radius;

	mcl::SceneManager *scene;

}; // end class RenderGL


} // end namespace mcl

#endif
