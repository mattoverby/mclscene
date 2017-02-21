
#include "MCL/SceneManager.hpp"
#include "MCL/Application.hpp"
#include "MCL/MicroTimer.hpp"
#include "MCL/ShapeFactory.hpp"

using namespace mcl;

int main(int argc, char *argv[]){

	if( argc < 2 ){ printf("Usage: %s <config file>\n", argv[0]); return 0; }

	MicroTimer t;
	SceneManager scene;

//	std::shared_ptr<TriangleMesh> mesh = factory::make_cyl( 12, 8 , 1, &scene );
//	mesh->clear();
//	mesh->load( "noodle.obj" );
//	mesh->update();
//	trimesh::xform xf = trimesh::xform::scale( 1, 1, 20 );
//	mesh->apply_xform( xf );
//	mesh->save( "noodle.obj" );
	
	if( !scene.load( std::string(argv[1]) ) ){ return 0; }
	else{ printf( "Successfully loaded xml file: %fms\n", t.elapsed_ms() ); }

	t.reset();
	scene.get_bvh();
	printf( "BVH Build time: %fs\n", t.elapsed_s() );

	Application gui( &scene );
	gui.display();

	return 0;
}
