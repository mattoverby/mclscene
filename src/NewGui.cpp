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

#include "MCL/NewGui.hpp"

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
// NewGui
//

NewGui::NewGui( mcl::SceneManager *scene_, mcl::Simulator *sim_ ) : scene(scene_), sim(sim_) {
	Input &input = Input::getInstance(); // initialize the singleton
	cursorX = 0.f;
	cursorY = 0.f;
	alpha = 0.f;
	beta = 0.f;
	zoom = 2.f;
	run_simulation = false;
	screen_dt = 0.f;

	// Create a vector of triangle mesh pointer for the simulator
	for( int i=0; i<scene->objects.size(); ++i ){
		trimesh::TriMesh *themesh = scene->objects[i]->get_TriMesh().get();
		if( themesh==NULL ){ continue; }
		mesh_pointers.push_back( themesh );
	}
}


NewGui::NewGui( mcl::SceneManager *scene_ ) : NewGui(scene_,0) {}


int NewGui::display(){

	// Initialize the simulator
	if( sim ){
		if( !sim->initialize(mesh_pointers) ){
			std::cerr << "\n**NewGui::display Error: Problem initializing the simulator" << std::endl;
			return EXIT_FAILURE;
		}
	} // end init sim

	GLFWwindow* window;
	glfwSetErrorCallback(&Input::error_callback);

	// Initialize the window
	if (!glfwInit()){ return false; }
	glfwWindowHint(GLFW_SAMPLES, 4); // anti aliasing
	glEnable(GL_MULTISAMPLE);
	window = glfwCreateWindow(1024, 768, "Viewer", NULL, NULL);
	if( !window ){ glfwTerminate(); return false; }

	// Set default callbacks
	if( !init_callbacks( window ) ){ return (EXIT_FAILURE); }

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	glewInit();
	if( !init_shaders() ){ return EXIT_FAILURE; }

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	framebuffer_size_callback(window, width, height);

	// Initialize OpenGL
	glShadeModel(GL_SMOOTH); // Use Gouraud (smooth) shading
	glEnable(GL_DEPTH_TEST); // Switch on the z-buffer
	glClearColor(1,1,1,1); // Background color is white

	// Game loop
	float t_old = glfwGetTime();
	while( !glfwWindowShouldClose(window) ){

		float t = glfwGetTime();
		screen_dt = t - t_old;
		t_old = t;

		// Simulation engine:
		if( sim && run_simulation ){
			bool s1 = sim->step( screen_dt );
			bool s2 = sim->update( mesh_pointers );
			if( !s1 ){ std::cerr << "\n**NewGui::display Error: Problem in simulation step" << std::endl; }
			if( !s2 ){ std::cerr << "\n**NewGui::display Error: Problem in mesh update" << std::endl; }

			// Recalculate normals for trimeshes and tetmeshes
			for( int i=0; i<mesh_pointers.size(); ++i ){ mesh_pointers[i]->need_normals(true); }
		}

		// Render:
		clear_screen(window);
		draw_meshes(window);
		for( int i=0; i<render_callbacks.size(); ++i ){ render_callbacks[i]( window, screen_dt ); }
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	return (EXIT_SUCCESS);

} // end display


bool NewGui::init_callbacks( GLFWwindow* window ){

	// Add callbacks to the input class
	using namespace std::placeholders;    // adds visibility of _1, _2, _3,...
	Input::key_callbacks.push_back( std::bind( &NewGui::key_callback, this, _1, _2, _3, _4, _5 ) );
	Input::mouse_button_callbacks.push_back( std::bind( &NewGui::mouse_button_callback, this, _1, _2, _3, _4 ) );
	Input::cursor_position_callbacks.push_back( std::bind( &NewGui::cursor_position_callback, this, _1, _2, _3 ) );
	Input::scroll_callbacks.push_back( std::bind( &NewGui::scroll_callback, this, _1, _2, _3 ) );
	Input::framebuffer_size_callbacks.push_back( std::bind( &NewGui::framebuffer_size_callback, this, _1, _2, _3 ) );

	// Bind them to the window
	glfwSetKeyCallback(window, &Input::key_callback);
	glfwSetMouseButtonCallback(window, &Input::mouse_button_callback);
	glfwSetCursorPosCallback(window, &Input::cursor_position_callback);
	glfwSetScrollCallback(window, &Input::scroll_callback);
	glfwSetFramebufferSizeCallback(window, &Input::framebuffer_size_callback);

	return true;
}


bool NewGui::init_shaders(){

	shader = new ShaderProgram();

	shader->initFromFiles("simpleshader.vert", "simpleshader.frag");


//	for( int i=0; i<scene->objects.size(); ++i ){
//	}

	return true;
}


void NewGui::draw_meshes(GLFWwindow* window){

	for( int i=0; i<scene->objects.size(); ++i ){
		trimesh::TriMesh *themesh = scene->objects[i]->get_TriMesh().get();
		if( themesh==NULL ){ continue; }

		// Vertices
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer( 3, GL_FLOAT, sizeof(themesh->vertices[0]), &themesh->vertices[0][0] );

		// Normals
		if (!themesh->normals.empty() ) {
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer( GL_FLOAT, sizeof(themesh->normals[0]), &themesh->normals[0][0] );
		} else { glDisableClientState(GL_NORMAL_ARRAY); }

		
		if( scene->objects[i]->get_type()=="pointcloud" ){
			glColor3f(1,0,0);
			glPointSize(3.f);
			glDrawArrays(GL_POINTS, 0, themesh->vertices.size());
		} // end draw vertices as points
		else{
			shader->enable();

			// Triangle strips
			const int *t = &themesh->tstrips[0];
			const int *end = t + themesh->tstrips.size();
			while (likely(t < end)) {
				int striplen = *t++;
				glDrawElements(GL_TRIANGLE_STRIP, striplen, GL_UNSIGNED_INT, t);
				t += striplen;
			}

			shader->disable();
		} // end draw as triangle mesh

	} // end loop scene objects

}


void NewGui::clear_screen(GLFWwindow* window){

	// Clear the color and depth buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// We don't want to modify the projection matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Move back
	glTranslatef(0.0, 0.0, -zoom);
	// Rotate the view
	glRotatef(beta, 1.0, 0.0, 0.0);
	glRotatef(alpha, 0.0, 0.0, 1.0);

} // end clear screne



void NewGui::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{

    if (button != GLFW_MOUSE_BUTTON_LEFT){ return; }

    if (action == GLFW_PRESS){
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwGetCursorPos(window, &cursorX, &cursorY);
    }
    else{ glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); }

}



void NewGui::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS){ return; }

    switch (key)
    {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
	case GLFW_KEY_SPACE:
		run_simulation = !run_simulation;
		break;
        case GLFW_KEY_P:
		if( sim ){ sim->step( screen_dt ); }
            break;
        default:
            break;
    }
}


void NewGui::cursor_position_callback(GLFWwindow* window, double x, double y){

	if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED){
		alpha += (GLfloat) (x - cursorX) / 10.f;
		beta += (GLfloat) (y - cursorY) / 10.f;
		cursorX = x;
		cursorY = y;
	}

}


void NewGui::scroll_callback(GLFWwindow* window, double x, double y){
	zoom -= float(y) / 4.f;
	if( zoom < 0.f ){ zoom=0.f; }
}


void NewGui::framebuffer_size_callback(GLFWwindow* window, int width, int height){

	float ratio = 1.f;
	mat4x4 projection;
	if( height > 0 ){ ratio = (float) width / (float) height; }

	// Setup viewport
	glViewport(0, 0, width, height);

	// Change to the projection matrix and set our viewing volume
	glMatrixMode(GL_PROJECTION);
	mat4x4_perspective(projection, 60.f * (float) M_PI / 180.f, ratio, 1.f, 1024.f );
	glLoadMatrixf( (const GLfloat*) projection );

}

