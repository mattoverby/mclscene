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

#ifndef MCLSCENE_APPLICATION_H
#define MCLSCENE_APPLICATION_H 1

#include "MCL/Simulator.hpp"
#include "MCL/SceneManager.hpp"
#include "RenderGL.hpp"



namespace mcl {

//
//	Application Class
//

class Application {
public:
	struct Settings {
		bool save_frames; // saves every render frame to a timestamped png in your build directory
		bool run_simulation; // run the simulator every frame
		bool gamma_correction;
		Vec3f clear_color;
		Settings() : save_frames(false), run_simulation(false),
			gamma_correction(false),
			clear_color(1,1,1) {}
	} settings;

	// Initializes the the Input singleton so callbacks can be added
	Application( mcl::SceneManager *scene_ );

	// Creates a gui with a combined simulation engine.
	Application( mcl::SceneManager *scene_, Simulator *sim_ );

	// Starts the game loop and returns success status
	int display();

	// Stop the game loop
	void close(){ close_window=true; }

	// Add a callback to the gui to be called every frame
	void add_callback( std::function<void ( GLFWwindow* window, Camera *cam, float screen_dt )> &cb ){ render_callbacks.push_back( cb ); }

	// You can use the renderer to draw custom objects
	RenderGL renderer;

	// Camera
	// Currently you cannot change the camera after you create an Application.
	// current_cam will be set to the zero-th cam in SceneManager (or assigned a default).
	bool update_view;
	Camera *current_cam;

protected:
	SceneManager *scene;
	Simulator *sim;

	// Runtime stuff:
	float screen_dt;
	double cursorX, cursorY;

	float scene_radius; // recomputed on simulation step
	Vec3f scene_center; // set once in constructor
	bool left_mouse_drag, right_mouse_drag;
	bool close_window;

	bool update_mesh_buffers; // set to true on run_simulator_step
	void run_simulator_step();

	// Utility functions:
	void save_screenshot(GLFWwindow* window);

	// Callbacks:
	std::vector< std::function<void ( GLFWwindow* window, Camera *cam, float screen_dt )> > render_callbacks;
	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	void cursor_position_callback(GLFWwindow* window, double x, double y);
	void scroll_callback(GLFWwindow* window, double x, double y);
	void framebuffer_size_callback(GLFWwindow* window, int width, int height);

}; // end class Application


//
//	A bit hacky and hurts performance, but allows us to use class functions as input callbacks.
//	This will have to do for now.
//	E.g.
//		using namespace std::placeholders; // adds visibility of _1, _2, _3,...
//		Input::key_callbacks.push_back( std::bind(&MyClass::key_callback,myClassPtr,_1,_2,_3,_4,_5) );
//
class Input {
public:
	static Input& getInstance(){ // Singleton is accessed via getInstance()
		static Input instance; // lazy singleton, instantiated on first use
		return instance;
	}

	static void framebuffer_size_callback(GLFWwindow* window, int width, int height){
		for( int i=0; i<framebuffer_size_callbacks.size(); ++i ){ framebuffer_size_callbacks[i](window,width,height); }
	}

	static void scroll_callback(GLFWwindow* window, double x, double y){
		for( int i=0; i<scroll_callbacks.size(); ++i ){ scroll_callbacks[i](window,x,y); }
	}

	static void cursor_position_callback(GLFWwindow* window, double x, double y){
		for( int i=0; i<cursor_position_callbacks.size(); ++i ){ cursor_position_callbacks[i](window,x,y); }
	}

	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
		for( int i=0; i<mouse_button_callbacks.size(); ++i ){ mouse_button_callbacks[i](window,button,action,mods); }
	}

	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
		for( int i=0; i<key_callbacks.size(); ++i ){ key_callbacks[i](window,key,scancode,action,mods); }
	}

	static void error_callback(int error, const char* description){ fprintf(stderr, "Error: %s\n", description); }

	static std::vector< std::function<void ( GLFWwindow* window, int key, int scancode, int action, int mods )> > key_callbacks;
	static std::vector< std::function<void ( GLFWwindow* window, int button, int action, int mods )> > mouse_button_callbacks;
	static std::vector< std::function<void ( GLFWwindow* window, double x, double y )> > cursor_position_callbacks;
	static std::vector< std::function<void ( GLFWwindow* window, double x, double y )> > scroll_callbacks;
	static std::vector< std::function<void ( GLFWwindow* window, int width, int height )> > framebuffer_size_callbacks;

	static void clear(){
		key_callbacks.clear();
		mouse_button_callbacks.clear();
		cursor_position_callbacks.clear();
		scroll_callbacks.clear();
		framebuffer_size_callbacks.clear();
	}

private:
	Input(void){}
	Input(Input const&); // prevent copies
	void operator=(Input const&); // prevent assignments
};

} // end namespace mcl

#endif
