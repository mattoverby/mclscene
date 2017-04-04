
#include "MCL/SceneManager.hpp"
#include "MCL/Application.hpp"
#include "MCL/MicroTimer.hpp"
#include "MCL/ShapeFactory.hpp"
#include "MCL/KDTree.hpp"

using namespace mcl;


/*
#include "TriMesh_algo.h"
#include "MCL/TriangleMesh.hpp"
static void trimesh_copy( std::shared_ptr<mcl::TriangleMesh> &to_mesh, trimesh::TriMesh *from_mesh ){
	for( int i=0; i<from_mesh->vertices.size(); ++i ){ to_mesh->vertices.push_back( mcl::Vec3f( from_mesh->vertices[i][0], from_mesh->vertices[i][1], from_mesh->vertices[i][2] ) ); }
	for( int i=0; i<from_mesh->faces.size(); ++i ){ to_mesh->faces.push_back( mcl::Vec3i( from_mesh->faces[i][0], from_mesh->faces[i][1], from_mesh->faces[i][2] ) ); }
	for( int i=0; i<from_mesh->texcoords.size(); ++i ){ to_mesh->texcoords.push_back( mcl::Vec2f( from_mesh->texcoords[i][0], from_mesh->texcoords[i][1] ) ); }
	to_mesh->update();
}
static void trimesh_copy( trimesh::TriMesh *to_mesh, BaseObject::AppData *from_mesh ){
	to_mesh->vertices.clear(); to_mesh->vertices.reserve( from_mesh->num_vertices );
	to_mesh->faces.clear(); to_mesh->faces.reserve( from_mesh->num_faces );
	to_mesh->texcoords.clear(); to_mesh->texcoords.reserve( from_mesh->num_texcoords );
	for( int i=0; i<from_mesh->num_vertices; ++i ){ to_mesh->vertices.push_back( trimesh::vec( from_mesh->vertices[i*3+0], from_mesh->vertices[i*3+1], from_mesh->vertices[i*3+2] ) ); }
	for( int i=0; i<from_mesh->num_faces; ++i ){ to_mesh->faces.push_back( trimesh::TriMesh::Face( from_mesh->faces[i*3+0], from_mesh->faces[i*3+1], from_mesh->faces[i*3+2] ) ); }
	for( int i=0; i<from_mesh->num_texcoords; ++i ){ to_mesh->texcoords.push_back( trimesh::vec2( from_mesh->texcoords[i*2+0], from_mesh->texcoords[i*2+1] ) ); }
}

*/


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

/*
	std::shared_ptr<TriangleMesh> mesh = std::dynamic_pointer_cast<TriangleMesh>( scene.objects[0] );
	std::shared_ptr<mcl::TriangleMesh> newmesh( new TriangleMesh() );
	trimesh::TriMesh tmesh;
	trimesh_copy( &tmesh, &mesh->app );
	trimesh::subdiv(&tmesh);
	trimesh_copy( newmesh, &tmesh );
	newmesh->update();
	newmesh->save( "blob.obj" );
	scene.objects[0] = newmesh;

	t.reset();
	scene.get_bvh();
	printf( "BVH Build time: %fs\n", t.elapsed_s() );
*/
	Application gui( &scene );
	gui.display();

	return 0;
}
