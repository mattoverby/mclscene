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

#ifndef MCLSCENE_SIMPLERENDERGL_H
#define MCLSCENE_SIMPLERENDERGL_H 1

#ifdef MCL_USE_GLEW
#include <GL/glew.h>
#endif

#include <SFML/OpenGL.hpp>
#include <SFML/Graphics.hpp>
#include "MCL/Shader.hpp"
#include "MCL/SceneManager.hpp"
#include <random>

namespace mcl {

class SimpleRenderGL  {
public:
	// A wrapper for base-class objects to store render info
	// for faster lookup.
	class RenderMesh {
	public:
		float *vertices, *normals, *texcoords;
		int *faces, *edges;
		int num_vertices, num_normals, num_texcoords, num_faces, num_edges;
		unsigned int verts_vbo, normals_vbo, texcoords_vbo, faces_ibo, wire_ibo, tris_vao;
		std::shared_ptr<BaseObject> object; // Index into SceneManager::objects
		void update();
		RenderMesh();
		RenderMesh( std::shared_ptr<BaseObject> obj );
	};

	std::vector<RenderMesh> render_meshes; // SceneManager::object -> render meshes

	// Initialize shaders. Must be called after
	// OpenGL context has been created.
	bool init( mcl::SceneManager *scene_, int win_width, int win_height );

	// Draws all objects in the SceneManager (that have AppData::mesh)
	// If VBOs have not been generated for the AppData mesh, they will be generated.
	// If the VBOs need to be updated (e.g. a deforming mesh) set update_vbo to true.
	// Texture coordinates and face ibo are NOT updated.
	void draw_objects( bool update_vbo=false );

	~SimpleRenderGL();

private:
	void draw_mesh( SimpleRenderGL::RenderMesh *mesh, Material *mat );

	// Texture coordinates and face ibo are NOT updated.
	// If the IBOs have already been generated, they are instead overwritten.
	// Returns true on success
	bool load_mesh_buffers( RenderMesh *mesh );

	Material defaultMat;
	std::unordered_map< std::string, int > textures; // file->texture_id

	Shader blinnphong;
	Vec2i window_size;

	mcl::SceneManager *scene;

}; // end class SimpleRenderGL


} // end namespace mcl

#endif
