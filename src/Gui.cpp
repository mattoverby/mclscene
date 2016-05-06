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

#include "MCL/Gui.hpp"

using namespace mcl;


Gui::Gui( SceneManager *scene_ ) : scene(scene_) {

	// Set up the window and create the opengl context.
	window = std::shared_ptr<sf::Window>( new sf::Window(sf::VideoMode(1024, 768), "Viewer",
		sf::Style::Default, sf::ContextSettings(32)) );
	window.get()->setVerticalSyncEnabled(true);

	std::cout << "Gui Warning: Ignoring lights and camera settings" << std::endl;

	// If there aren't any materials, use a default one
	std::shared_ptr<DiffuseMaterial> flat_gray( new DiffuseMaterial() );
	flat_gray->diffuse = trimesh::vec( 0.5, 0.5, 0.5 );
	flat_gray->edge_color = trimesh::vec( 0.9, 0.9, 0.9 );
	std::shared_ptr<BaseMaterial> mat( flat_gray );
	scene->materials.push_back( mat ); // store it to the scene for later

	// Get tet and tri meshes
	scene->build_meshes();
	for( int i=0; i<scene->objects.size(); ++i ){
		std::string mat_str = scene->objects[i]->get_material();
		if( mat_str.size()==0 ){ trimesh_materials.push_back( mat ); }
		else{ trimesh_materials.push_back( scene->materials_map[mat_str] ); }
	} // end draw scene objects

	// build the bvh
	scene->get_bvh();

	bsphere = scene->get_bsphere();
	global_xf = trimesh::xform::trans(0, 0, -10.0f * bsphere.r) *
		    trimesh::xform::trans(-bsphere.center);
	cam.setupGL( global_xf * bsphere.center, bsphere.r );

	std::cout << "Gui bsphere radius: " << bsphere.r << std::endl;

}


void Gui::display(){

	// The game loop
	while( true ){

		float screen_dt = clock.getElapsedTime().asSeconds();
		clock.restart();

		if( !update(screen_dt) ){ break; }
		if( !draw(screen_dt) ){ break; }

	} // end game loop

} // end display


// Returns false on window close
bool Gui::update( const float screen_dt ){

	sf::Event event;
	while( window->pollEvent(event) ){

		for( int i=0; i<event_callbacks.size(); ++i ){ event_callbacks[i](event); }

		if( event.type == sf::Event::Closed ){ return false; }
		else if( event.type == sf::Event::Resized ){
			// adjust the viewport when the window is resized
			glViewport(0, 0, event.size.width, event.size.height);
			cam.setupGL( bsphere.center, bsphere.r );
		}
		else if( event.type == sf::Event::KeyPressed ){
			if( event.key.code == sf::Keyboard::Escape ){ return false; }
			if( event.key.code == sf::Keyboard::S ){ save_screenshot(); }
		}

	} // end event loop

	// Camera controls
	check_mouse( event, screen_dt );

	// Finished updating
	return true;

} // end update


// Returns true on success
bool Gui::draw( const float screen_dt ){

	clear_screen();
	cam.setupGL( global_xf * bsphere.center, bsphere.r );
	glPushMatrix();
	glMultMatrixd(global_xf);
	setup_lighting( scene->materials[0], scene->lights );

	// Draw the meshes
	for( int i=0; i<scene->meshes.size(); ++i ){
		setup_lighting( trimesh_materials[i], scene->lights );
		draw_trimesh( trimesh_materials[i], scene->meshes[i].get() );
	}

	for( int i=0; i<render_callbacks.size(); ++i ){ render_callbacks[i](); }

	glPopMatrix();

	window->display();

	return true;		

} // end render function


void Gui::check_mouse( const sf::Event &event, const float screen_dt ){

	using namespace trimesh;

	// If mouse isn't inside the window just return
	sf::Vector2i mouse_pos = sf::Mouse::getPosition(*window);
	sf::Vector2u win_size = window->getSize();
	if( mouse_pos.x < 0 || mouse_pos.x > win_size.x ){ return; }
	if( mouse_pos.y < 0 || mouse_pos.y > win_size.y ){ return; }

	bool shift_down = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift);

	Mouse::button b = Mouse::NONE;
	if (event.type == sf::Event::MouseWheelMoved){
		if( event.mouseWheel.delta > 0 ){ b = Mouse::WHEELDOWN; }
		else if( event.mouseWheel.delta < 0 ){ b = Mouse::WHEELUP; }
	}
	else if( sf::Mouse::isButtonPressed( sf::Mouse::Left ) ){ b = Mouse::ROTATE; }
	else if( sf::Mouse::isButtonPressed( sf::Mouse::Right ) && shift_down ){ b = Mouse::LIGHT; }

	cam.mouse( mouse_pos.x, mouse_pos.y, b, global_xf*bsphere.center, bsphere.r, global_xf );
	bsphere = scene->get_bsphere(true);

}


// Clear the screen
void Gui::clear_screen(){

	glDisable(GL_DITHER);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_NORMALIZE);
	glDisable(GL_LIGHTING);
	glDisable(GL_NORMALIZE);
	glDisable(GL_COLOR_MATERIAL);
	glClearColor(1.f, 1.f, 1.f, 0);
	glClearDepth(1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


// Set up lights and materials, by Szymon Rusinkiewicz
//void Gui::setup_lighting( MaterialComponent *material, const std::vector<LightComponent> &lights ){
void Gui::setup_lighting( const std::shared_ptr<BaseMaterial> mat, const std::vector<std::shared_ptr<BaseLight> > &lights ){

	// This isn't toooooo unsafe...
	std::shared_ptr<DiffuseMaterial> diffuse = NULL;
	std::shared_ptr<SpecularMaterial> specular = NULL;
	if( parse::to_lower(mat->get_type())=="diffuse" ){ diffuse = std::static_pointer_cast<DiffuseMaterial>(mat); }
	else if( parse::to_lower(mat->get_type())=="specular" ){ specular = std::static_pointer_cast<SpecularMaterial>(mat); }

	// Diffuse color
	if( diffuse != NULL ){ glColor3f( diffuse->diffuse[0], diffuse->diffuse[1], diffuse->diffuse[2] ); }
	else if( specular != NULL ){ glColor3f( specular->diffuse[0], specular->diffuse[1], specular->diffuse[2] ); }
//	if( trimesh::len2(material->diffuse)>0 ){
//		glColor3f(material->diffuse[0],material->diffuse[1],material->diffuse[2]);
//	}
	else{ glColor3f(0.5f,0.5f,0.5f); }

	// Specular color
	GLfloat mat_specular[4] = { 0.f, 0.f, 0.f, 0.f };
//	if( trimesh::len2(material->specular)>0 ){
	if( specular != NULL ){
		trimesh::vec c = specular->specular;
		double w = (c[0]+c[1]+c[2])/3.f;
		mat_specular[0]=c[0]; mat_specular[1]=c[1]; mat_specular[2]=c[2]; mat_specular[3]=w;
	}

	// shininess
	GLfloat mat_shininess[] = { 64 };
	if( specular != NULL ){ mat_shininess[0]=specular->shininess; }

	GLfloat global_ambient[] = { 0.02f, 0.02f, 0.05f, 0.05f };
	GLfloat light0_ambient[] = { 0, 0, 0, 0 };
	GLfloat light0_diffuse[] = { 0.85f, 0.85f, 0.8f, 0.85f };

	GLfloat light1_diffuse[] = { -0.01f, -0.01f, -0.03f, -0.03f };
	GLfloat light0_specular[] = { 0.85f, 0.85f, 0.85f, 0.85f };

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
//	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_NORMALIZE);

}


// Draw triangle strips.  They are stored as length followed by values. By Szymon Rusinkiewicz
void Gui::draw_tstrips( const trimesh::TriMesh *themesh ){

	static bool use_glArrayElement = false;
	static bool tested_renderer = false;
	if (!tested_renderer) {
		use_glArrayElement = !!strstr(
			(const char *) glGetString(GL_RENDERER), "Intel");
		tested_renderer = true;
	}

	const int *t = &themesh->tstrips[0];
	const int *end = t + themesh->tstrips.size();
	if (use_glArrayElement) {
		while (likely(t < end)) {
			glBegin(GL_TRIANGLE_STRIP);
			int striplen = *t++;
			for (int i = 0; i < striplen; i++)
				glArrayElement(*t++);
			glEnd();
		}
	} else {
		while (likely(t < end)) {
			int striplen = *t++;
			glDrawElements(GL_TRIANGLE_STRIP, striplen, GL_UNSIGNED_INT, t);
			t += striplen;
		}
	}

}


// Draw the mesh, by Szymon Rusinkiewicz
void Gui::draw_trimesh( std::shared_ptr<BaseMaterial> material, const trimesh::TriMesh *themesh ){
	bool draw_falsecolor = false;
	bool draw_index = false;
	bool draw_2side = false;
	int point_size = 1, line_width = 1;

	trimesh::vec edge_color(0,0,0);
	if( parse::to_lower(material->get_type())=="diffuse" ){ edge_color=std::static_pointer_cast<DiffuseMaterial>(material)->edge_color; }
	else if( parse::to_lower(material->get_type())=="specular" ){ edge_color=std::static_pointer_cast<SpecularMaterial>(material)->edge_color; }
	bool draw_edges = trimesh::len2( edge_color ) > 0.0001f;

	glPushMatrix();
//	glMultMatrixd(xforms[i]);

	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);

	if (draw_2side) {
		glDisable(GL_CULL_FACE);
	} else {
		glCullFace(GL_BACK);
		glEnable(GL_CULL_FACE);
	}

	// Vertices
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT,
			sizeof(themesh->vertices[0]),
			&themesh->vertices[0][0]);

	// Normals
	if (!themesh->normals.empty() && !draw_index) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT,
				sizeof(themesh->normals[0]),
				&themesh->normals[0][0]);
	} else {
		glDisableClientState(GL_NORMAL_ARRAY);
	}

	// Colors
	if (!themesh->colors.empty() && !draw_falsecolor && !draw_index) {
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(3, GL_FLOAT,
			       sizeof(themesh->colors[0]),
			       &themesh->colors[0][0]);
	} else {
		glDisableClientState(GL_COLOR_ARRAY);
	}

	// Main drawing pass
	if (draw_points || themesh->tstrips.empty()) {
		// No triangles - draw as points
		glPointSize(float(point_size));
		glDrawArrays(GL_POINTS, 0, themesh->vertices.size());
		glPopMatrix();
		return;
	}

	if (draw_edges) {
		glPolygonOffset(10.0f, 10.0f);
		glEnable(GL_POLYGON_OFFSET_FILL);
	}

	draw_tstrips(themesh);
	glDisable(GL_POLYGON_OFFSET_FILL);

	// Edge drawing pass
	if (draw_edges) {
		trimesh::vec edge_c = edge_color;
		glPolygonMode(GL_FRONT, GL_LINE);
		glLineWidth(float(line_width));
		glDisableClientState(GL_COLOR_ARRAY);
		glDisable(GL_COLOR_MATERIAL);
		GLfloat global_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
		GLfloat light0_diffuse[] = { 0.8f, 0.8f, 0.8f, 0.0f };
		GLfloat light1_diffuse[] = { -0.2f, -0.2f, -0.2f, 0.0f };
		GLfloat light0_specular[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
		glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
		GLfloat mat_diffuse[4] = { edge_c[0], edge_c[1], edge_c[2], 1.0f };
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mat_diffuse);
		glColor3f(0, 0, 1); // Used iff unlit
		draw_tstrips(themesh);
		glPolygonMode(GL_FRONT, GL_FILL);
	}

	glPopMatrix();

}

void Gui::save_screenshot(){

	std::string MY_DATE_FORMAT = "h%H_m%M_s%S";
	const int MY_DATE_SIZE = 20;
	static char name[MY_DATE_SIZE];
	time_t now = time(0);
	strftime(name, sizeof(name), MY_DATE_FORMAT.c_str(), localtime(&now));

	std::stringstream filename;
	filename << MCLSCENE_BUILD_DIR << "/screenshot_" << name << ".png";
	sf::Vector2u windowsize = window->getSize();
	int w = int(windowsize.x), h = int(windowsize.y);
	unsigned char *pixels = new unsigned char[w*h*3];
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0,0, w,h, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	mcl::Draw::flip_image(w,h, pixels);
	mcl::Draw::save_png(filename.str().c_str(), w,h, pixels);
	delete[] pixels;

}


