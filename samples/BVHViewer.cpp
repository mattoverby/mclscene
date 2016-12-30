
#include "MCL/SceneManager.hpp"
#include "MCL/Application.hpp"
#include "MCL/MicroTimer.hpp"

using namespace mcl;

SceneManager scene;
void render_callback(GLFWwindow* window, Camera *cam, float screen_dt);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
std::vector< bool > traversal; // 0 = left, 1 = right
bool view_all = false;
bool view_root = false;
std::string bvh_mode = "linear";
std::vector<trimesh::point> edges;

int main(int argc, char *argv[]){

	if( argc < 2 ){ printf("Usage: %s <config file>\n", argv[0]); return 0; }

	if( !scene.load( std::string(argv[1]) ) ){ return 0; }
	else{ printf( "Successfully loaded xml file.\n"); }

	// Build the initial bvh
	scene.get_bvh(true,bvh_mode);

	Application app( &scene );

	// Add a render callback to draw the BVH
	std::function<void ( GLFWwindow* window, Camera *cam, float screen_dt )> draw_cb(render_callback);
	app.add_callback( draw_cb );

	// Add a key callback to change the viewer settings
	std::function<void ( GLFWwindow* window, int key, int scancode, int action, int mods )> key_cb(key_callback);
	Input::key_callbacks.push_back( key_cb );

	app.display();

	// Compute runtime for deallocation
	mcl::MicroTimer t;
	std::shared_ptr<BVHNode> root = scene.get_bvh();
	delete root->left_child;
	delete root->right_child;
	root->left_child = NULL;
	root->right_child = NULL;
	printf("BVH Deletion took %fms\n",t.elapsed_ms());

	return 0;
}



void render_callback(GLFWwindow* window, Camera *cam, float screen_dt){

	BVHNode *bvh = scene.get_bvh().get();


	int num_boxes = 4;
	bvh->get_edges( edges );
	for( int i=edges.size(); i>(num_boxes*24); i-- ){
		edges.pop_back();
	}
	// Remove root box
	for( int i=0; i<24; ++i ){
		edges.front() = std::move(edges.back());
		edges.pop_back();
	}

/*
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
	
		bvh->get_edges( edges );
	}
	else if( edges.size()<=24 ) {
		traversal.clear();
		bvh->get_edges( edges );
	}
*/
	trimesh::XForm<float> xf = cam->app.projection * cam->app.view;

	std::vector<trimesh::vec> colors;
	colors.push_back( trimesh::vec(1,0,0) );
	colors.push_back( trimesh::vec(0,1,0) );
	colors.push_back( trimesh::vec(0,0,1) );
	colors.push_back( trimesh::vec(1,1,0) );
	colors.push_back( trimesh::vec(1,0,1) );
	colors.push_back( trimesh::vec(0,1,1) );


	glLineWidth(10.f);
	glDisable(GL_LIGHTING);
	glBegin(GL_LINES);
	int color_idx = 0;
	int depth = 0;
	for( int j=0; j<edges.size(); j+=2 ){
		trimesh::vec c = colors[color_idx];
		glColor3f(c[0], c[1], c[2]);
		glLineWidth(10.f / float(10.f-depth));

//		glMultMatrixf(xform);
		trimesh::vec e1 = xf*edges[j];
		trimesh::vec e2 = xf*edges[j+1];
		glVertex3f(e1[0],e1[1],e1[2]);
		glVertex3f(e2[0],e2[1],e2[2]);
		if( j%24==0 ){
			color_idx++;
			if(color_idx>=colors.size()){ color_idx=0; }
			depth++;
		}
	}
	glEnd();
	glEnable(GL_LIGHTING);

}


void key_callback( GLFWwindow* window, int key, int scancode, int action, int mods ){

	if (action != GLFW_PRESS){ return; }

	switch(key){

	case GLFW_KEY_M:
		if( bvh_mode=="spatial" ){ bvh_mode="linear"; }
		else{ bvh_mode = "spatial"; }
		edges.clear();
		scene.get_bvh(true,bvh_mode);
		std::cout << "BVH Mode: " << bvh_mode << std::endl;
		break;
	case GLFW_KEY_DOWN:
		view_all = !view_all;
		break;
	case GLFW_KEY_R:
		view_root = !view_root;
		break;
	case GLFW_KEY_LEFT:{
		traversal.push_back(false);
	} break;
	case GLFW_KEY_RIGHT:{
		traversal.push_back(true);
	} break;
	case GLFW_KEY_UP:{
		traversal.pop_back();
	} break;
	} // end switch
}


