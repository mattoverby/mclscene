
#include "MCL/SceneManager.hpp"
#include "MCL/Application.hpp"

using namespace mcl;

int main(int argc, char *argv[]){

	if( argc < 2 ){ printf("Usage: %s <config file>\n", argv[0]); return 0; }

	SceneManager scene;
	if( !scene.load( std::string(argv[1]) ) ){ return 0; }
	else{ printf( "Successfully loaded xml file.\n"); }

	Application gui( &scene );
	gui.display();

	return 0;
}
