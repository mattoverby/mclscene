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

	sf::ContextSettings settings;
	settings.depthBits = 24;
	settings.stencilBits = 8;
	settings.antialiasingLevel = 4;
	settings.majorVersion = 3;
	settings.minorVersion = 0;

	// Set up the window and create the opengl context.
	window = std::shared_ptr<sf::Window>( new sf::Window(sf::VideoMode(1024, 768), "Viewer",
		sf::Style::Default, settings ) );
	window.get()->setVerticalSyncEnabled(true);

	load_textures();

	// If there are no lights, add a default one
	if( scene->lights.size() == 0 ){
		std::shared_ptr<BaseLight> light( std::shared_ptr<OGLLight>( new OGLLight() ) );
		scene->lights.push_back( light );
		scene->lights_map[ "default_light" ] = light;
	}

	// If there aren't any materials, use a default one
	std::shared_ptr<BaseMaterial> mat( std::shared_ptr<OGLMaterial>( new OGLMaterial() ) );

	// Get tet and tri meshes
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
			if( event.key.code == sf::Keyboard::M ){ save_meshes(); }
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
	draw_background();
	cam.setupGL( global_xf * bsphere.center, bsphere.r+10.f );
	glPushMatrix();
	glMultMatrixd(global_xf);
	setup_lighting( scene->lights );

	draw_shadow( scene->lights, scene->meshes );

	// Draw the meshes
	for( int i=0; i<scene->meshes.size(); ++i ){
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


void Gui::setup_lighting( const std::vector<std::shared_ptr<BaseLight> > &lights ){

	glEnable(GL_LIGHTING);

	for( int i=0; i<lights.size(); ++i ){

		if( lights[i]->get_type() != "ogl" ){ continue; }
		std::shared_ptr<OGLLight> light = std::static_pointer_cast<OGLLight>(lights[i]);

		int l_id = GL_LIGHT0;
		switch( i%8 ){
			case 0: l_id = GL_LIGHT0; break;
			case 1: l_id = GL_LIGHT1; break;
			case 2: l_id = GL_LIGHT2; break;
			case 3: l_id = GL_LIGHT3; break;
			case 4: l_id = GL_LIGHT4; break;
			case 5: l_id = GL_LIGHT5; break;
			case 6: l_id = GL_LIGHT6; break;
			case 7: l_id = GL_LIGHT7; break;
		} // end get light id

		GLfloat light_ambient[] = { light->m_ambient[0], light->m_ambient[1], light->m_ambient[2], 1.f };
		GLfloat light_diffuse[] = { light->m_diffuse[0], light->m_diffuse[1], light->m_diffuse[2], 1.f };
		GLfloat light_specular[] = { light->m_specular[0], light->m_specular[1], light->m_specular[2], 1.f };

		// Enable the light
		glLightfv(l_id, GL_AMBIENT, light_ambient);
		glLightfv(l_id, GL_DIFFUSE, light_diffuse);
		glLightfv(l_id, GL_SPECULAR, light_specular);
		glEnable(l_id);

		GLfloat light_pos[] = { light->m_pos[0], light->m_pos[1], light->m_pos[2], float( light->m_type ) };
		glLightfv(l_id, GL_POSITION, light_pos);

	} // end loop lights

	// Enable other drawing settings
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_NORMALIZE);

} // end setup lighting


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

	if( parse::to_lower(material->get_type())!="ogl" ){ std::cout << "Unknown material type." << std::endl; return; }
	std::shared_ptr<OGLMaterial> mat = std::static_pointer_cast<OGLMaterial>(material);

	bool draw_edges = ( mat->edge_color[0]>=0.f && mat->edge_color[1]>=0.f && mat->edge_color[2]>=0.f );

	glPushMatrix();

	if( !draw_floor ){
		if (draw_2side) {
			glDisable(GL_CULL_FACE);
		} else {
			glCullFace(GL_BACK);
			glEnable(GL_CULL_FACE);
		}
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

	// Texture coordinates
	if( !themesh->texcoords.empty() ){
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(themesh->texcoords[0]), &themesh->texcoords[0][0]);
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	// Colors
	if (!themesh->colors.empty() && !draw_falsecolor && !draw_index) {
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(3, GL_FLOAT,
			       sizeof(themesh->colors[0]),
			       &themesh->colors[0][0]);
	} else {

		glDisableClientState(GL_COLOR_ARRAY);

		// Color settings
		GLfloat mat_diffuse[4] = { mat->diffuse[0], mat->diffuse[1], mat->diffuse[2], 1.f };
		GLfloat mat_specular[4] = { mat->specular[0], mat->specular[1], mat->specular[2], 1.f };
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mat_diffuse );
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
		glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, mat->shininess);

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


	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if( material->has_texture() ){ m_textures.bind( material->m_texture.m_name ); }
	draw_tstrips(themesh);
	m_textures.unbind();

	glDisable(GL_POLYGON_OFFSET_FILL);

	// Edge drawing pass
	if (draw_edges) {
		glPolygonMode(GL_FRONT, GL_LINE);
		glLineWidth(float(line_width));
		glDisableClientState(GL_COLOR_ARRAY);
		glDisable(GL_COLOR_MATERIAL);
		GLfloat mat_diffuse[4] = { mat->edge_color[0], mat->edge_color[1], mat->edge_color[2], 1.0f };
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


void Gui::save_meshes(){

	std::unordered_map< std::string, std::shared_ptr<BaseObject> >::iterator obj_it = scene->objects_map.begin();
	for( obj_it; obj_it != scene->objects_map.end(); ++obj_it ){
		std::shared_ptr<trimesh::TriMesh> mesh = obj_it->second->get_TriMesh();
		if( mesh == NULL ){ continue; }

		std::stringstream filename; filename << obj_it->first << ".obj";
		std::cout << "Saving mesh: " << filename.str() << std::endl;
		mesh->write( filename.str().c_str() );

	} // end loop objects
}


void Gui::load_textures(){

	// Material textures
	for( int i=0; i<scene->materials.size(); ++i ){
		if( scene->materials[i]->has_texture() ){
			TextureResource tex = scene->materials[i]->m_texture;
			if( !m_textures.load( tex.m_name, tex.m_file ) ){ exit(0); }
		}
	}

	// Load the backdrop
	draw_floor = false;
	for( int i=0; i<scene->components.size(); ++i ){
		if( parse::to_lower( scene->components[i].tag ) == "background" ){
			if( scene->components[i].exists("file") ){ m_textures.load( "bg", scene->components[i]["file"].as_string() ); }
			if( scene->components[i]["floor"].as_bool() == true ){ 	draw_floor = true; }
		}
	}
}



void Gui::draw_background(){

	// Make sure a background texture was loaded
	sf::Texture *tex = m_textures.get("bg");
	if( tex == NULL ){ return; }

	// Set up the orthographic projection
	sf::Vector2u windowsize = window->getSize();
	int w1 = windowsize.x;
	int h1 = windowsize.y;
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(-w1/2, w1/2, -h1/2, h1/2);
	glMatrixMode(GL_MODELVIEW);

	// texture width/height
	// Scale the image to fit the background
	sf::Vector2u tex_size = tex->getSize();
	int iw = windowsize.x;
	int ih = windowsize.y;

	// Draw the texture on a quad
	m_textures.bind("bg");
	glPushMatrix();
	glTranslatef( -iw/2, -ih/2, 0 );
	glColor3f(1,1,1);
	glBegin(GL_QUADS);
	glTexCoord2i(1,1); glVertex2i(0, 0);
	glTexCoord2i(0,1); glVertex2i(iw, 0);
	glTexCoord2i(0,0); glVertex2i(iw, ih);
	glTexCoord2i(1,0); glVertex2i(0, ih);
	glEnd();
	glPopMatrix();

	// Done
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	m_textures.unbind();
}






void Gui::make_shadow_mat( float light_pos[4], float destMat[4][4] ){

	// Floor always at y=0
	float normal[4] = { 0.f, 1.f, 0.f, 0.1f };
	float points[3][3] = { { -3.f, 0.1f, -3.f }, { -3.f, 0.1f, 3.f }, { 3.f, 0.1f, 3.f } };
	float dot = normal[0]*light_pos[0] + normal[1]*light_pos[1] + normal[2]*light_pos[2] + normal[3]*light_pos[3] + normal[4]*light_pos[4];

	// Now do the projection
	destMat[0][0] = dot - light_pos[0] * normal[0];
	destMat[1][0] = 0.0f - light_pos[0] * normal[1];
	destMat[2][0] = 0.0f - light_pos[0] * normal[2];
	destMat[3][0] = 0.0f - light_pos[0] * normal[3];
	destMat[0][1] = 0.0f - light_pos[1] * normal[0];
	destMat[1][1] = dot - light_pos[1] * normal[1];
	destMat[2][1] = 0.0f - light_pos[1] * normal[2];
	destMat[3][1] = 0.0f - light_pos[1] * normal[3];
	destMat[0][2] = 0.0f - light_pos[2] * normal[0];
	destMat[1][2] = 0.0f - light_pos[2] * normal[1];
	destMat[2][2] = dot - light_pos[2] * normal[2];
	destMat[3][2] = 0.0f - light_pos[2] * normal[3];
	destMat[0][3] = 0.0f - light_pos[3] * normal[0];
	destMat[1][3] = 0.0f - light_pos[3] * normal[1];
	destMat[2][3] = 0.0f - light_pos[3] * normal[2];
	destMat[3][3] = dot - light_pos[3] * normal[3];
}


void Gui::draw_shadow( const std::vector<std::shared_ptr<BaseLight> > &lights,
		const std::vector<std::shared_ptr<trimesh::TriMesh> > &meshes ){

	// Only draw ground if there is no background
	if( draw_floor != true ){ return; }

	// Draw the ground
	GLfloat mat_diffuse[4] = { 0.7f, 0.7f, 0.7f, 1.f };
	GLfloat mat_specular[4] = { 0.f, 0.f, 0.f, 1.f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mat_diffuse );
//	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
//	glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 0);

// Checkboard
	int floor_r = int(bsphere.r*50.f);

	glPushMatrix();
	glDisable(GL_LIGHTING);
//	glTranslatef(-bsper,0.f,-floor_r.f);
	glBegin(GL_QUADS);
	glNormal3f(0.f,1.f,0.f);
	for( int x=-floor_r; x<floor_r; ++x ){
		for( int z=-floor_r; z<floor_r; ++z ){

			if( (x+z) % 2 == 0 ){ glColor3f(0.85f,0.85f,0.85f); }
			else{ glColor3f(0.95f,0.95f,0.95f); }

			glVertex3f( x, 0.f, z);
			glVertex3f( (x+1), 0.f, z);
			glVertex3f( (x+1), 0.f, (z+1));
			glVertex3f( x, 0.f, (z+1)); 
		}
	}
	glEnd();
//	glEnable(GL_LIGHTING);
	glPopMatrix();

//	float floor_r = bsphere.r*10.f;
//	glBegin(GL_QUADS);
//		glNormal3f(0.f,1.f,0.f);
//		glVertex3f(floor_r, 0.0f, -floor_r);
//		glVertex3f(-floor_r, 0.0f, -floor_r);
//		glVertex3f(-floor_r, 0.0f, floor_r);
//		glVertex3f(floor_r, 0.0f, floor_r);
//	glEnd();

	// Disable lighting and save projection state to draw the shadow
	glPushMatrix();
	glDisable(GL_DEPTH_TEST);
//	glDisable(GL_LIGHTING);
//	glEnable(GL_COLOR);
//	glEnable(GL_BLEND);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//	glDisable(GL_CULL_FACE);

	// Loop over the lights and draw shadows on the ground plane
	for( int i=0; i<lights.size(); ++i ){

		if( lights[i]->get_type() != "ogl" ){ continue; }
		std::shared_ptr<OGLLight> light = std::static_pointer_cast<OGLLight>(lights[i]);

		float light_pos[4] = { light->m_pos[0], light->m_pos[1], light->m_pos[2], float( light->m_type ) };
		float shadow_matrix[4][4];
		make_shadow_mat( light_pos, shadow_matrix );

		glMultMatrixf( (GLfloat*)shadow_matrix );

		// Draw meshes to flattened space
		for( int j=0; j<meshes.size(); ++j ){

			trimesh::TriMesh *themesh = meshes[j].get();

			glColor4f(0.5f,0.5f,0.5f,0.5f);
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT,
					sizeof(themesh->vertices[0]),
					&themesh->vertices[0][0]);
			draw_tstrips( themesh );
		}

	} // end loop lights

//	glDisable(GL_COLOR);
	glEnable(GL_LIGHTING);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glPopMatrix();

} // end draw shadows


