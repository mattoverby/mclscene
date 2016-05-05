
#include "MCL/SceneManager.hpp"
#include "MCL/Gui.hpp"

using namespace mcl;

SceneManager scene;
void render_callback();
void event_callback(sf::Event &event);
std::vector< bool > traversal; // 0 = left, 1 = right
bool view_all = false;

int main(int argc, char *argv[]){

	if( argc < 2 ){ printf("Usage: %s <config file>\n", argv[0]); return 0; }

	if( !scene.load( std::string(argv[1]) ) ){ return 0; }
	else{ printf( "Successfully loaded xml file.\n"); }

	Gui gui( &scene );

	boost::function<void ()> draw_cb(render_callback);
	gui.add_render_callback( draw_cb );

	boost::function<void (sf::Event &event)> event_cb(event_callback);
	gui.add_event_callback( event_cb );

	gui.display();

	return 0;
}



void render_callback(){

	static std::vector<trimesh::point> edges;
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
	} else {
		bvh->get_edges( edges );
	}

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
	}
}
