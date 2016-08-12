
#include "MCL/SceneManager.hpp"
//#include "MCL/Gui.hpp"
#include "MCL/NewGui.hpp"

using namespace mcl;
//void event_callback(sf::Event &event);
//void refine_mesh();
mcl::SceneManager *g_scene;

int main(int argc, char *argv[]){

	if( argc < 2 ){ printf("Usage: %s <config file>\n", argv[0]); return 0; }

	SceneManager scene;
	g_scene = &scene;
	if( !scene.load( std::string(argv[1]) ) ){ return 0; }
	else{ printf( "Successfully loaded xml file.\n"); }

	NewGui gui;
	gui.run( &scene );

//	Gui gui( &scene );

//	std::function<void (sf::Event &event)> event_cb(event_callback);
//	gui.add_event_callback( event_cb );
//	gui.display();

	return 0;
}


/*
void event_callback(sf::Event &event){

	if( event.type == sf::Event::KeyPressed ){
		if( event.key.code == sf::Keyboard::R ){ refine_mesh(); }
	}
}


void refine_mesh(){

	std::cout << "Refining mesh... " << std::flush;

	using namespace trimesh;
	for( int i=0; i<g_scene->objects.size(); ++i ){
		std::string t = parse::to_lower( g_scene->objects[i]->get_type() );
		if( t == "trimesh" || t == "tetmesh" ){

			std::shared_ptr<TriMesh> mesh = g_scene->objects[i]->get_TriMesh();
			subdiv( mesh.get() );
//			edgeflip( mesh.get() );
			lmsmooth( mesh.get(), 10 );
			mesh->need_normals();
			mesh->need_tstrips();

		}
	} // end loop objects

	std::cout << "done." << std::endl;
}
*/
