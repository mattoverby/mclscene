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

#ifndef MCLSCENE_GUI_H
#define MCLSCENE_GUI_H 1

#include <GL/glew.h>
#include "TextureManager.hpp"
#include "SceneManager.hpp"
#include <png.h>
#include "SFML/Window.hpp"
#include "GLCamera.h"
#include <functional>
#include "RenderUtils.hpp"

namespace mcl {

class Gui {
public:
	Gui( SceneManager *scene_ );
	virtual ~Gui() {}

	// Opens the GUI window and begins the display.
	// Returns when window closes.
	virtual void display();

	void add_render_callback( std::function<void ()> cb ){ render_callbacks.push_back( cb ); }
	void add_event_callback( std::function<void (sf::Event &event)> cb ){ event_callbacks.push_back( cb ); }

	void set_init_xform( trimesh::xform init ){
		init_xf = init;
		global_xf = trimesh::xform::trans(0, 0, -10.0f * scene_bvh->aabb->radius()) *
			    trimesh::xform::trans(-scene_bvh->aabb->center()) * init_xf;
	}

protected:
	virtual bool update( const float screen_dt );
	virtual bool draw( const float screen_dt );
	virtual void clear_screen();
	virtual void setup_lighting( const std::vector<std::shared_ptr<BaseLight> > &lights );
	virtual void draw_tstrips( const trimesh::TriMesh *themesh );
	virtual void draw_trimesh( std::shared_ptr<BaseMaterial> material, const trimesh::TriMesh *themesh );
	virtual void check_mouse( const sf::Event &event, const float screen_dt );

	void draw_background();
	void load_textures();
	TextureManager m_textures;


	void save_screenshot();
	void save_meshes();
	bool draw_floor;
	bool draw_red_back; // make the back side of triangles red
	bool smooth_shade;
	int sphereDisplayList;

	std::vector< std::function<void ()> > render_callbacks;
	std::vector< std::function<void (sf::Event &event)> > event_callbacks;

	std::vector< std::shared_ptr<BaseMaterial> > trimesh_materials;

	trimesh::xform global_xf, init_xf;
	std::shared_ptr<BVHNode> scene_bvh;
	sf::Clock clock;
	SceneManager *scene;
	trimesh::GLCamera cam;
	std::shared_ptr<sf::Window> window;

	bool draw_points;




	// Shadow stuff
	void make_shadow_mat( float light_pos[4], float dest_mat[4][4] );
	void draw_shadow( const std::vector<std::shared_ptr<BaseLight> > &lights,
		const std::vector<std::shared_ptr<trimesh::TriMesh> > &meshes );

};

} // end namespace mcl

#endif
