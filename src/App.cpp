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

#include "MCL/App.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace mcl;

//
//	Initialize static members
//
std::vector< std::function<void ( GLFWwindow* window, int key, int scancode, int action, int mods )> > mcl::Input::key_callbacks;
std::vector< std::function<void ( GLFWwindow* window, int button, int action, int mods )> > mcl::Input::mouse_button_callbacks;
std::vector< std::function<void ( GLFWwindow* window, double x, double y )> > mcl::Input::cursor_position_callbacks;
std::vector< std::function<void ( GLFWwindow* window, double x, double y )> > mcl::Input::scroll_callbacks;
std::vector< std::function<void ( GLFWwindow* window, int width, int height )> > mcl::Input::framebuffer_size_callbacks;

//
//	App
//
App::App( mcl::SceneManager *scene_, Simulator *sim_ ) : scene(scene_), sim(sim_),
	update_mesh_buffers(true), in_focus(true), close_window(false), save_frame_num(0) {
//	Input &input = Input::getInstance(); // initialize Input
	Input::getInstance();

	scene->get_bsphere(&scene_center,&scene_radius,true);
	std::cout << "Scene Radius: " << scene_radius << std::endl;
	std::cout << "Scene Center: " << scene_center[0] << ' ' << scene_center[1] << ' ' << scene_center[2] << std::endl;

	// Init runtime vars
	left_mouse_drag = false;
	right_mouse_drag = false;

	// Make camera if one was not loaded
	if( scene->cameras.size()==0 ){
		Vec3f eye = scene_center; eye[2]+=(scene_radius*4.f);
		std::shared_ptr<Trackball> cam( new Trackball(eye, scene_center) );
		cam->clipping = Vec2f( scene_radius*0.1f, scene_radius*20.f );
		scene->cameras.push_back( cam );
	}
	current_cam = scene->cameras[0].get();

	// Add lights if not described in scene
	if( scene->lights.size()==0 ){ scene->make_3pt_lighting( current_cam->get_eye(), scene_center ); }

	// Add callbacks to the input class
	using namespace std::placeholders;    // adds visibility of _1, _2, _3,...
	Input::clear(); // clear existing callbacks
	Input::key_callbacks.push_back( std::bind( &App::key_callback, this, _1, _2, _3, _4, _5 ) );
	Input::mouse_button_callbacks.push_back( std::bind( &App::mouse_button_callback, this, _1, _2, _3, _4 ) );
	Input::cursor_position_callbacks.push_back( std::bind( &App::cursor_position_callback, this, _1, _2, _3 ) );
	Input::scroll_callbacks.push_back( std::bind( &App::scroll_callback, this, _1, _2, _3 ) );
	Input::framebuffer_size_callbacks.push_back( std::bind( &App::framebuffer_size_callback, this, _1, _2, _3 ) );
}


App::App( mcl::SceneManager *scene_ ) : App(scene_,nullptr) {}

int App::display(){

GLFWwindow* window;
	glfwSetErrorCallback(&Input::error_callback);

	// Initialize the window
	if (!glfwInit()){ return EXIT_FAILURE; }
	glfwWindowHint(GLFW_SRGB_CAPABLE, settings.gamma_correction); // gamma correction

	// Ask for OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	// Get the monitor max window size
//	const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	int max_width = 1280;
	int max_height = 960;
//	int max_width = mode->width;
//	int max_height = mode->height;
//	if( max_width >= 1920 ){ max_width=1920; max_height=1080; } // just use 1080 if they have it
//	else{ max_width=1366; max_height=768; }

	// Create the glfw window
	window = glfwCreateWindow(max_width, max_height, "Viewer", NULL, NULL);
	if( !window ){ glfwTerminate(); return EXIT_FAILURE; }
//	if( !settings.vsync ){ glfwSwapInterval(0); }

	// Bind callbacks to the window
	glfwSetKeyCallback(window, &Input::key_callback);
	glfwSetMouseButtonCallback(window, &Input::mouse_button_callback);
	glfwSetCursorPosCallback(window, &Input::cursor_position_callback);
	glfwSetScrollCallback(window, &Input::scroll_callback);
	glfwSetFramebufferSizeCallback(window, &Input::framebuffer_size_callback);

	// Make current
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	// Init glew
	#ifdef MCL_USE_GLEW
	glewExperimental = GL_TRUE;
	glewInit();
	#endif

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	framebuffer_size_callback(window, width, height); // sets the projection matrix
	bool has_render_cb( render_callback );

	if( !renderer.init( scene, width, height ) ){ glfwTerminate(); return EXIT_FAILURE; } // creates shaders

	// Initialize OpenGL
	glEnable(GL_DEPTH_TEST);
	if( settings.gamma_correction ){ glEnable(GL_FRAMEBUFFER_SRGB); } // gamma correction
	glClearColor(settings.clear_color[0],settings.clear_color[1],settings.clear_color[2],1.f);

	// Game loop
	float t_old = glfwGetTime();
	screen_dt = 0.f;
	float elapsed_dt = 0.f;
	while( !glfwWindowShouldClose(window) && !close_window ){

		glfwSwapBuffers(window);

		//
		//	Update
		//
		float t = glfwGetTime();
		screen_dt = t - t_old;
		t_old = t;
		elapsed_dt += screen_dt;
		if( elapsed_dt > 1.f ){
//			std::cout << "FPS: " << ceil(1.f/screen_dt) << std::endl;
			elapsed_dt = 0.f;
		}

		// Handle events
		if( settings.run_simulation ){ glfwPollEvents(); }
		else{ glfwWaitEvents(); }

		// Simulation engine:
		if( sim && settings.run_simulation ){
			if( settings.save_frames ){ save_screenshot(window); }
			run_simulator_step();
		}

		//
		//	Render
		//
		renderer.draw_objects( update_mesh_buffers );
		update_mesh_buffers = false;
		if( has_render_cb ){ render_callback(window,current_cam,screen_dt); }

	} // end game loop

	glfwDestroyWindow(window);
	glfwTerminate();

	return EXIT_SUCCESS;

} // end display


inline void App::run_simulator_step(){

	if( sim == nullptr ){ return; }

	sim->step( scene, screen_dt );
	sim->update( scene );

	// Update geometry on device
	update_mesh_buffers = true;
}




void App::mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
	(void)mods;

	if( action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT ){
		glfwGetCursorPos(window, &mouse_pos[0], &mouse_pos[1]);
		left_mouse_drag = true;
		right_mouse_drag = false;
	}
	else if(  action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_RIGHT ){
		glfwGetCursorPos(window, &mouse_pos[0], &mouse_pos[1]);
		right_mouse_drag = true;
		left_mouse_drag = false;
	}
	else{
		left_mouse_drag = false;
		right_mouse_drag = false;
	}

}



void App::key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/){

	if (action != GLFW_PRESS){ return; }

	switch(key){
	case GLFW_KEY_ESCAPE:
		glfwSetWindowShouldClose(window, true);
		break;
	case GLFW_KEY_SPACE:
		settings.run_simulation = !settings.run_simulation;
		break;
	case GLFW_KEY_P:
		if( settings.save_frames ){ save_screenshot(window); }
		run_simulator_step();
		break;
	case GLFW_KEY_T:
		settings.save_frames=!settings.save_frames;
		std::cout << "save screenshots: " << (int)settings.save_frames << std::endl;
		break;
	case GLFW_KEY_S:{
		std::stringstream xml_file; xml_file << MCLSCENE_BUILD_DIR << "/currscene.xml";
		scene->save( xml_file.str() );
		std::cout << "exporting scene: " << xml_file.str() << std::endl;
		} break;
	default:
		break;
	}
}


void App::cursor_position_callback(GLFWwindow* /*window*/, double x, double y){

	if( left_mouse_drag ){
		current_cam->rotate( (x-mouse_pos[0])/100.f, (y-mouse_pos[1])/100.f );
	}
	else if( right_mouse_drag ){
		current_cam->pan( float(x-mouse_pos[0])/100.f, float(y-mouse_pos[1])/100.f );
	}
	mouse_pos[0] = x;
	mouse_pos[1] = y;
}


void App::scroll_callback(GLFWwindow* /*window*/, double /*x*/, double y){
	current_cam->zoom( float(y)*scene_radius );
}


void App::framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height){
	glViewport(0, 0, width, height);
	current_cam->resize( width, height );
	renderer.update_window_size( width, height );
}


inline void App::save_screenshot( GLFWwindow *window ){

	int w=256, h=256;
	glfwGetFramebufferSize(window, &w, &h);
	int row_stride = w*sizeof(unsigned char)*3;

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	std::vector<unsigned char> rgbdata(3*w*h);
	std::vector<unsigned char> temp_rgbdata(3*w*h);
	glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE, &temp_rgbdata[0]);

	// loop and swap rows (invert vertical)
	for (int i=0; i < h; i++){
		std::memcpy( &rgbdata[i*w*3], &temp_rgbdata[(h-i-1)*w*3], row_stride );
	}
	temp_rgbdata.clear();

	std::stringstream filename;
	filename << MCLSCENE_BUILD_DIR << "/"; filename << std::setfill('0') << std::setw(5) << save_frame_num << ".png";

	// Write to file
	// stb is not as fast/quality as libpng, but with only one include it's easier to include
	int comp = 3; // RGB, for alpha, comp=4
	stbi_write_png(filename.str().c_str(), w, h, comp, &rgbdata[0], row_stride);

	save_frame_num++;
}

