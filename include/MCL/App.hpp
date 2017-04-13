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

#ifndef MCLSCENE_APP_H
#define MCLSCENE_APP_H 1

#include "MCL/Simulator.hpp"
#include "MCL/SceneManager.hpp"
#include "RenderGL.hpp"
//#include <functional>

namespace mcl {

//
//	Application Class
//

class App {
public:
	struct Settings {
		bool save_frames; // saves every render frame to a timestamped png in your build directory
		bool run_simulation; // run the simulator every frame
		bool gamma_correction;
		bool enable_rotate; // enable camera rotate
		Vec3f clear_color;
		Settings() : save_frames(false), run_simulation(false),
			gamma_correction(false), enable_rotate(true), clear_color(1,1,1) {}
	} settings;

	// Initializes the the Input singleton so callbacks can be added
	App( mcl::SceneManager *scene_ );

	// Creates a gui with a combined simulation engine.
	App( mcl::SceneManager *scene_, Simulator *sim_ );

	// Starts the game loop and returns success status
	int display();

	// Stop the game loop
	void close(){ close_window=true; }

	// You can use the renderer to draw custom objects
	RenderGL renderer;

	// Camera
	// Currently you cannot change the camera after you create an Application.
	// current_cam will be set to the zero-th cam in SceneManager (or assigned a default).
	Camera *current_cam;

	// You can set a render callback that is called every frame
	// or an event callback that is called for each event
	std::function<void(sf::RenderWindow*, Camera*, float dt)> render_callback;
	std::function<void(sf::RenderWindow*, sf::Event*)> event_callback;

protected:
	inline void process_event( sf::Event &event, sf::RenderWindow &window );
	inline void process_mouse( sf::RenderWindow &window );
	inline void save_screenshot( sf::RenderWindow &window );
	inline void run_simulator_step();

	SceneManager *scene;
	Simulator *sim;

	// Runtime stuff:
	sf::Vector2i mouse_pos;
	sf::Vector2i window_size;
	bool in_focus, close_window;
	bool left_mouse_drag, right_mouse_drag;
	int save_frame_num;
	float screen_dt;
	bool update_mesh_buffers; // set to true on run_simulator_step

	float scene_radius; // recomputed on simulation step
	Vec3f scene_center; // set once in constructor

}; // end class App

} // end namespace mcl

#endif
