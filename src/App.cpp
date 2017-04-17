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

using namespace mcl;


//
//	App
//
App::App( mcl::SceneManager *scene_, Simulator *sim_ ) : scene(scene_), sim(sim_),
	update_mesh_buffers(true), in_focus(true), close_window(false), save_frame_num(0),
	left_mouse_drag(false), right_mouse_drag(false),
	window_size( 1280, 960 ) {

	scene->get_bsphere(&scene_center,&scene_radius,true);
	std::cout << "Scene Radius: " << scene_radius << std::endl;
	std::cout << "Scene Center: " << scene_center.transpose() << std::endl;

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
}


App::App( mcl::SceneManager *scene_ ) : App(scene_,nullptr) {}

int App::display(){

	// Create the main window and OpenGL context
	sf::ContextSettings glSettings;
//	glSettings.antialiasingLevel = 4; using FXAA instead
	glSettings.majorVersion = 3;
	glSettings.minorVersion = 3;
	sf::RenderWindow window(sf::VideoMode(window_size.x, window_size.y), "Application", sf::Style::Default, glSettings);
	window.setFramerateLimit(100);

	// Init glew
	#ifdef MCL_USE_GLEW
	glewExperimental = GL_TRUE;
	glewInit();
	#endif

	// Init OpenGL
	glViewport(0, 0, window_size.x, window_size.y);
	glEnable(GL_DEPTH_TEST);
	if( settings.gamma_correction ){ glEnable(GL_FRAMEBUFFER_SRGB); } // gamma correction
	glClearColor(settings.clear_color[0],settings.clear_color[1],settings.clear_color[2],1.f);

	// Init other things
	current_cam->resize( window_size.x, window_size.y );
	if( !renderer.init( scene, window_size.x, window_size.y ) ){ return EXIT_FAILURE; } // creates shaders
	mouse_pos = sf::Mouse::getPosition();

	// Start the game loop
	sf::Clock clock;
	screen_dt = 0.f;
	float elapsed_dt = 0.f;
	while( window.isOpen() && !close_window ){

		// Update clock
		screen_dt = clock.restart().asSeconds();
		elapsed_dt += screen_dt;
		if( elapsed_dt > 1.f ){
//			std::cout << "FPS: " << ceil(1.f/screen_dt) << std::endl;
			elapsed_dt = 0.f;
		}

		// Process event:
		sf::Event event;
		while( window.pollEvent(event) ){
			process_event(event,window);
			if( event_callback ){ event_callback(&window,&event); }
		} // end poll event

		// Mouse events:
		process_mouse(window);

		// Simulation engine:
		if( settings.run_simulation ){
			if( settings.save_frames ){ save_screenshot(window); }
			run_simulator_step();
		}

		// Render:
		renderer.draw_objects( update_mesh_buffers );
		update_mesh_buffers = false;
		if( render_callback ){ render_callback(&window,current_cam,screen_dt); }

		// Display frame:
		window.display();

	} // end game loop

	return EXIT_SUCCESS;

} // end display


inline void App::process_mouse( sf::RenderWindow &window ) {

	sf::Vector2i curr_pos = sf::Mouse::getPosition(window);
	bool in_window = curr_pos.x > 0 && curr_pos.x < window_size.x && curr_pos.y > 0 && curr_pos.y < window_size.y;

	// If we're not in the window, return
	if( !in_window || !settings.enable_rotate ){
		left_mouse_drag = false;
		right_mouse_drag = false;
		mouse_pos = curr_pos;
		return;
	}

	// Otherwise check left and right mouse buttons
	if( sf::Mouse::isButtonPressed(sf::Mouse::Left) && in_focus ){
		left_mouse_drag = true;
		current_cam->rotate( float(curr_pos.x-mouse_pos.x)/100.f, float(curr_pos.y-mouse_pos.y)/100.f );
	}
	else if( sf::Mouse::isButtonPressed(sf::Mouse::Right) && in_focus ){
		right_mouse_drag = true;
		current_cam->pan( float(curr_pos.x-mouse_pos.x)/100.f, float(curr_pos.y-mouse_pos.y)/100.f );
	}
	else{
		left_mouse_drag = false;
		right_mouse_drag = false;
	}

	mouse_pos = curr_pos;

} // end process mouse


inline void App::process_event( sf::Event &event, sf::RenderWindow &window ){

	switch( event.type ){

		default: break;

		//
		//	Window Focus
		//
		case sf::Event::LostFocus: { in_focus = false; } break;
		case sf::Event::GainedFocus: { in_focus = true; } break;
	
		//
		//	Window closed
		//
		case sf::Event::Closed: { window.close(); } break;

		//
		//	Window resized
		//
		case sf::Event::Resized: {
			window_size.x = event.size.width;
			window_size.y = event.size.height;
			glViewport(0, 0, window_size.x, window_size.y );
			current_cam->resize( window_size.x, window_size.y );
			renderer.update_window_size( window_size.x, window_size.y );
		} break;

		//
		//	Key Pressed
		//
		case sf::Event::KeyPressed: {
			if( event.key.code == sf::Keyboard::Escape ){ window.close(); }
			else if( event.key.code == sf::Keyboard::P ){ run_simulator_step(); }
			else if( event.key.code == sf::Keyboard::F1 ){ save_screenshot(window); }
			else if( event.key.code == sf::Keyboard::Space ){ settings.run_simulation = !settings.run_simulation; }
			else if( event.key.code == sf::Keyboard::T ){
				settings.save_frames = !settings.save_frames;
				std::cout << "save screenshots: " << (int)settings.save_frames << std::endl;
			}
			else if( event.key.code == sf::Keyboard::S ){
				std::stringstream xml_file; xml_file << MCLSCENE_BUILD_DIR << "/currscene.xml";
				scene->save( xml_file.str() );
				std::cout << "exporting scene: " << xml_file.str() << std::endl;
			}
		} break;

		//
		//	Mouse Wheel
		//
		case sf::Event::MouseWheelMoved: {
			current_cam->zoom( float(event.mouseWheel.delta)*scene_radius );
		} break;

	} // end switch event type

} // end process event


inline void App::run_simulator_step(){

	if( sim == nullptr ){ return; }

	sim->step( scene, screen_dt );
	sim->update( scene );

	// Update geometry on device
	update_mesh_buffers = true;
}

inline void App::save_screenshot( sf::RenderWindow &window ){

	std::stringstream filename;
	filename << MCLSCENE_BUILD_DIR << "/" << std::setfill('0') << std::setw(5) << save_frame_num << ".png";
	sf::Image screen = window.capture();
	screen.saveToFile( filename.str().c_str() );
	save_frame_num++;
}

