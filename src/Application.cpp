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

#include "SOIL2.h"
#include "MCL/Application.hpp"
#include <png.h>

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
// Application
//

Application::Application( mcl::SceneManager *scene_, Simulator *sim_ ) : scene(scene_), sim(sim_), update_mesh_buffers(true), close_window(false) {
	Input &input = Input::getInstance(); // initialize the singleton

	scene->get_bsphere(&scene_center,&scene_radius,true);
	std::cout << "Scene Radius: " << scene_radius << std::endl;

	// Make camera if one was not loaded
	if( scene->cameras.size()==0 ){
		Vec3f eye = scene_center; eye[2]+=(scene_radius*4.f);
		std::shared_ptr<Trackball> cam( new Trackball(eye, scene_center) );
		cam->clipping = Vec2f( scene_radius*0.1f, scene_radius*20.f );
		scene->cameras.push_back( cam );
//		scene->camera_params.push_back( std::vector<Param>() );
		cam->update_view();
	}
	current_cam = scene->cameras[0].get();

	// Add lights if not described in scene
	if( scene->lights.size()==0 ){ scene->make_3pt_lighting( current_cam->get_eye(), scene_center ); }

	// Runtime variables
	cursorX = 0.f;
	cursorY = 0.f;
	left_mouse_drag = false;
	right_mouse_drag = false;
	update_view = false;

	// Add callbacks to the input class
	using namespace std::placeholders;    // adds visibility of _1, _2, _3,...
	Input::clear(); // clear existing callbacks
	Input::key_callbacks.push_back( std::bind( &Application::key_callback, this, _1, _2, _3, _4, _5 ) );
	Input::mouse_button_callbacks.push_back( std::bind( &Application::mouse_button_callback, this, _1, _2, _3, _4 ) );
	Input::cursor_position_callbacks.push_back( std::bind( &Application::cursor_position_callback, this, _1, _2, _3 ) );
	Input::scroll_callbacks.push_back( std::bind( &Application::scroll_callback, this, _1, _2, _3 ) );
	Input::framebuffer_size_callbacks.push_back( std::bind( &Application::framebuffer_size_callback, this, _1, _2, _3 ) );


	if( current_cam->has_key_cb() ){
		Input::key_callbacks.push_back( std::bind( &Camera::key_callback, current_cam, _1, _2, _3, _4, _5 ) );
	}
	if( current_cam->has_mouse_button_cb() ){
		Input::mouse_button_callbacks.push_back( std::bind( &Camera::mouse_button_callback, current_cam, _1, _2, _3, _4 ) );
	}
	if( current_cam->has_cursor_position_cb() ){
		Input::cursor_position_callbacks.push_back( std::bind( &Camera::cursor_position_callback, current_cam, _1, _2, _3 ) );
	}
	if( current_cam->has_scroll_cb() ){
		Input::scroll_callbacks.push_back( std::bind( &Camera::scroll_callback, current_cam, _1, _2, _3 ) );
	}
	if( current_cam->has_framebuffer_size_cb() ){
		Input::framebuffer_size_callbacks.push_back( std::bind( &Camera::framebuffer_size_callback, current_cam, _1, _2, _3 ) );
	}
}


Application::Application( mcl::SceneManager *scene_ ) : Application(scene_,0) {}

int Application::display(){

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
	const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	int max_width = 1024;
	int max_height = 768;
//	int max_width = mode->width;
//	int max_height = mode->height;
//	if( max_width >= 1920 ){ max_width=1920; max_height=1080; } // just use 1080 if they have it
//	else{ max_width=1366; max_height=768; }

	// Create the glfw window
	window = glfwCreateWindow(max_width, max_height, "Viewer", NULL, NULL);
	if( !window ){ glfwTerminate(); return EXIT_FAILURE; }

	// Bind callbacks to the window
	glfwSetKeyCallback(window, &Input::key_callback);
	glfwSetMouseButtonCallback(window, &Input::mouse_button_callback);
	glfwSetCursorPosCallback(window, &Input::cursor_position_callback);
	glfwSetScrollCallback(window, &Input::scroll_callback);
	glfwSetFramebufferSizeCallback(window, &Input::framebuffer_size_callback);

	// Make current
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	glewExperimental = GL_TRUE;
	glewInit();

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	framebuffer_size_callback(window, width, height); // sets the projection matrix

	if( !renderer.init( scene, width, height ) ){ glfwTerminate(); return EXIT_FAILURE; } // creates shaders

	// Initialize OpenGL
	glEnable(GL_DEPTH_TEST);
	if( settings.gamma_correction ){ glEnable(GL_FRAMEBUFFER_SRGB); } // gamma correction
	glClearColor(settings.clear_color[0],settings.clear_color[1],settings.clear_color[2],1.f);

	// Game loop
	float t_old = glfwGetTime();
	screen_dt = 0.f;
	while( !glfwWindowShouldClose(window) && !close_window ){

		//
		//	Update
		//

		float t = glfwGetTime();
		screen_dt = t - t_old;
		t_old = t;

		// Simulation engine:
		if( sim && settings.run_simulation ){
			if( settings.save_frames ){ save_screenshot(window); }
			run_simulator_step();
		}

		{ // Update camera
			if( update_view ){ current_cam->update_view(); update_view = false; }
		}

		//
		//	Render
		//

		{ // Render scene stuff
			renderer.draw_objects( update_mesh_buffers );
			for( int i=0; i<render_callbacks.size(); ++i ){ render_callbacks[i]( window, current_cam, screen_dt ); }
			update_mesh_buffers = false;
		}

		{ // Finalize:
			glfwSwapBuffers(window);
			glfwPollEvents();
		}

	} // end game loop

	glfwDestroyWindow(window);
	glfwTerminate();

	return EXIT_SUCCESS;

} // end display



void Application::mouse_button_callback(GLFWwindow* window, int button, int action, int mods){

	if( action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT ){
		glfwGetCursorPos(window, &cursorX, &cursorY);
		left_mouse_drag = true;
		right_mouse_drag = false;
	}
	else if(  action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_RIGHT ){
		glfwGetCursorPos(window, &cursorX, &cursorY);
		right_mouse_drag = true;
		left_mouse_drag = false;
	}
	else{
		left_mouse_drag = false;
		right_mouse_drag = false;
	}

}



void Application::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){

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


void Application::cursor_position_callback(GLFWwindow* window, double x, double y){

	if( left_mouse_drag ){
		current_cam->rotate( (x-cursorX)/100.f, (y-cursorY)/100.f );
		update_view = true;
	}
	else if( right_mouse_drag ){
		current_cam->pan( float(x-cursorX)/100.f, float(y-cursorY)/100.f );
		update_view = true;
	}
	cursorX = x;
	cursorY = y;
}


void Application::scroll_callback(GLFWwindow* window, double x, double y){
	current_cam->zoom( float(y)*scene_radius );
	update_view = true;
}


void Application::framebuffer_size_callback(GLFWwindow* window, int width, int height){

	float scene_d = std::fmaxf( scene_radius*2.f, 0.2f );
	float aspect_ratio = 1.f;
	if( height > 0 ){ aspect_ratio = std::fmaxf( (float)width / (float)height, 1e-6f ); }
	glViewport(0, 0, width, height);
	current_cam->update_proj( aspect_ratio );
	renderer.update_window_size( width, height );
}




// Swap pixel locations
static inline void swapchar( unsigned char &p1, unsigned char &p2 ){
	unsigned char temp = p1;
	p1 = p2;
	p2 = temp;
}

// Flip storage order of image rows
static inline void flip_image (int w, int h, unsigned char *pixels) {

    for (int j = 0; j < h/2; j++)
	for (int i = 0; i < w; i++)
	    for (int c = 0; c < 3; c++)
	        swapchar(pixels[(i+w*j)*3+c], pixels[(i+w*(h-1-j))*3+c]);

}

// Write an image buffer to a PNG file
static inline void save_png (const char *filename, int width, int height,
	       unsigned char *pixels, bool has_alpha) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
	printf("Couldn't open file %s for writing.\n", filename);
	return;
    }
    // initialize the PNG structures
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
	                                          NULL, NULL);
    if (!png_ptr) {
	printf("Couldn't create a PNG write structure.\n");
	fclose(file);
	return;
    }
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
	printf("Couldn't create a PNG info structure.\n");
	png_destroy_write_struct(&png_ptr, NULL);
	fclose(file);
	return;
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
	printf("Had a problem writing %s.\n", filename);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(file);
	return;
    }
    png_init_io(png_ptr, file);
    png_set_IHDR(png_ptr, info_ptr, width, height, 8,
	         has_alpha ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB,
	         PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
	         PNG_FILTER_TYPE_DEFAULT);
    // set the pixel data
    int channels = has_alpha ? 4 : 3;
    png_bytep* row_pointers = (png_bytep*) new unsigned char*[height];
    for (int y = 0; y < height; y++)
	row_pointers[y] = (png_bytep) &pixels[y*width*channels];
    png_set_rows(png_ptr, info_ptr, row_pointers);
    // write the file
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    // clean up
    delete[] row_pointers;
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(file);
}

int frame_num = 0;
void Application::save_screenshot(GLFWwindow* window){
	std::string MY_DATE_FORMAT = "h%H_m%M_s%S";
	const int MY_DATE_SIZE = 20;
	static char name[MY_DATE_SIZE];
	time_t now = time(0);
	strftime(name, sizeof(name), MY_DATE_FORMAT.c_str(), localtime(&now));
	std::stringstream filename;
//	filename << MCLSCENE_BUILD_DIR << "/screenshot_" << name << ".png";
	filename << MCLSCENE_BUILD_DIR << "/"; filename << std::setfill('0') << std::setw(5) << frame_num << ".png";
	int w=256, h=256;
	glfwGetFramebufferSize(window, &w, &h);
	unsigned char *pixels = new unsigned char[w*h*3];
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0,0, w,h, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	flip_image(w,h, pixels);
	save_png(filename.str().c_str(), w,h, pixels,false);
	delete[] pixels;
	frame_num++;
}

void Application::run_simulator_step(){

	if( sim == 0 ){ return; }

	sim->step( scene, screen_dt );
	sim->update( scene );

	// Probably don't need to do this
//	Vec3f unused;
//	scene->get_bsphere(&unused,&scene_radius,true);

	// Update geometry on device
	update_mesh_buffers = true;
}

