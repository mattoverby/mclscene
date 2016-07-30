
#include "MCL/SceneManager.hpp"
#include "MCL/Gui.hpp"

using namespace mcl;

SceneManager scene;
void render_callback();
void event_callback(sf::Event &event);
std::vector< bool > traversal; // 0 = left, 1 = right
void scale_mesh( std::string dir );
void refine_mesh( std::string amt );
bool view_all = false;
std::string bvh_mode = "linear";
std::vector<trimesh::point> edges;

int main(int argc, char *argv[]){

	if( argc < 2 ){ printf("Usage: %s <config file>\n", argv[0]); return 0; }

	if( !scene.load( std::string(argv[1]) ) ){ return 0; }
	else{ printf( "Successfully loaded xml file.\n"); }

	// Build the initial bvh
	scene.get_bvh(true,bvh_mode);

	Gui gui( &scene );

	std::function<void ()> draw_cb(render_callback);
	gui.add_render_callback( draw_cb );

	std::function<void (sf::Event &event)> event_cb(event_callback);
	gui.add_event_callback( event_cb );

	gui.display();

	return 0;
}



void render_callback(){

	std::shared_ptr<BVHNode> bvh = scene.get_bvh();
	if( !view_all ){
		edges.clear();
		for( int i=0; i<traversal.size(); ++i ){
			bool right = traversal[i];
			if( right ){
				if( bvh->right_child != NULL ){ bvh = bvh->right_child; }
				else{ traversal.pop_back(); }
			} else {
				if( bvh->left_child != NULL ){ bvh = bvh->left_child; }
				else{ traversal.pop_back(); }
			}
		}
	
		bvh->aabb->get_edges( edges );
	} else if( edges.size()<=24 ) {
		bvh->get_edges( edges );
	}

//		glLineWidth(10.f);
	glDisable(GL_LIGHTING);
	glColor3f(1.0, 0.0, 0.0);
	glBegin(GL_LINES);
	for( int j=0; j<edges.size(); j+=2 ){
		glVertex3f(edges[j][0],edges[j][1],edges[j][2]);
		glVertex3f(edges[j+1][0],edges[j+1][1],edges[j+1][2]);
	}
	glEnd();
	glEnable(GL_LIGHTING);

}


void event_callback(sf::Event &event){

	if( event.type == sf::Event::KeyPressed ){

		if( event.key.code == sf::Keyboard::Up && traversal.size() ){ traversal.pop_back(); }
		else if( event.key.code == sf::Keyboard::Left ){ traversal.push_back( false ); }
		else if( event.key.code == sf::Keyboard::Right ){ traversal.push_back( true ); }
		else if( event.key.code == sf::Keyboard::Down ){ view_all = !view_all; }
		else if( event.key.code == sf::Keyboard::R ){ refine_mesh("-"); }
		else if( event.key.code == sf::Keyboard::M ){ 
			if( bvh_mode=="spatial" ){ bvh_mode="linear"; }
			else{ bvh_mode = "spatial"; }
			edges.clear();
			scene.get_bvh(true,bvh_mode);
		}
		else if( event.key.code == sf::Keyboard::Numpad6 ){ scale_mesh("+x"); }
		else if( event.key.code == sf::Keyboard::Numpad4 ){ scale_mesh("-x"); }
		else if( event.key.code == sf::Keyboard::Numpad8 ){ scale_mesh("+y"); }
		else if( event.key.code == sf::Keyboard::Numpad2 ){ scale_mesh("-y"); }
	}
}


void scale_mesh( std::string dir ){

	edges.clear();
	float scale_x=1.f;
	float scale_y=1.f;
	if( dir=="+x" ){ scale_x = 1.1f; }
	else if( dir=="-x" ){ scale_x = 0.9f; }
	else if( dir=="+y" ){ scale_y = 1.1f; }
	else if( dir=="-y" ){ scale_y = 0.9f; }

	trimesh::xform scale = trimesh::xform::scale(scale_x,scale_y,1.f);
	if( scene.objects.size() < 1 ){ return; }

	// Scale the object and remake the BVH
//	trimesh::vec center = trimesh::mesh_center_of_mass( scene.objects[0]->get_TriMesh().get() );
//	trimesh::xform translate_in = trimesh::xform::scale(-center[0],-center[1],-center[2]);
//	trimesh::xform translate_out = trimesh::xform::scale(center[0],center[1],center[2]);
//	trimesh::xform x_form = translate_out * scale * translate_in;

	scene.objects[0]->apply_xform( scale );

	// Recomputed bounding volumes
	scene.get_bvh(true,bvh_mode);

}


void refine_mesh( std::string amt ){

	edges.clear();
	if( scene.objects.size() != 1 ){ return; }

	std::shared_ptr<BVHNode> bvh = scene.get_bvh();
	trimesh::TriMesh *mesh = scene.objects[0]->get_TriMesh().get();
	trimesh::box b( bvh->aabb->min ); b+=bvh->aabb->max;
	b.min[1] += ((b.max[1]-b.min[1])*0.05f);

	std::cout << "Clipping mesh. Vertices before: " << scene.objects[0]->get_TriMesh()->vertices.size() << std::flush;
	trimesh::clip( mesh, b );
	trimesh::remove_unused_vertices( mesh );
//	mesh->normals.clear();
//	mesh->faces.clear();
//	mesh->tstrips.clear();
//	mesh->need_faces();
//	mesh->need_normals();
//	mesh->need_tstrips();
	std::cout << ", vertices after: " << scene.objects[0]->get_TriMesh()->vertices.size() << std::endl;

	// Recompute bounding volumes
	scene.get_bvh(true,bvh_mode);

}



