#include "MCL/SceneManager.hpp"
#include "MCL/Application.hpp"
#include "MCL/MicroTimer.hpp"
#include <Eigen/Core>
using namespace mcl;

//
//	Mesh type for simulators with vertices that point elsewhere
//
class RefMesh : public BaseObject {
private:
	Eigen::VectorXd vdata;
public:
	RefMesh( double *ref, int start, int end ) : vertices(ref), start_idx(start), end_idx(end) {}
	void bounds( trimesh::vec &bmin, trimesh::vec &bmax ){}

	int start_idx, end_idx;
	double *vertices;
};

Eigen::VectorXd verts; 
std::shared_ptr<TriangleMesh> drawmesh;
std::shared_ptr<RefMesh> refmesh;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void render_callback( GLFWwindow* window, Camera *cam, float screen_dt );
void copy_to_drawmesh();

int main(int argc, char *argv[]){

	SceneManager scene;

	//
	// This is the vertex data that would be housed by a simulation
	// class, or something.
	//
	int orig_size = 3*20;
	verts.resize( orig_size );

	// Make a mesh, copy it to the refmesh (for initialization)
	{
		// Create the original mesh
		drawmesh = scene.make_object<TriangleMesh>("sphere");
		verts.conservativeResize( orig_size+drawmesh->vertices.size()*3 );
		for( int i=0; i<drawmesh->vertices.size(); ++i ){
			int idx = orig_size + i*3;
			verts[idx+0] = drawmesh->vertices[i][0];
			verts[idx+1] = drawmesh->vertices[i][1];
			verts[idx+2] = drawmesh->vertices[i][2];
		}

		// Make the reference mesh
		refmesh = std::shared_ptr<RefMesh>( new RefMesh( &verts[3*20], 0, verts.size()-3*20 ) );
	}

	// Start up the interface
	{
		Application gui( &scene );

		// Add callback
		using namespace std::placeholders;
		std::function<void ( GLFWwindow* window, Camera *cam, float screen_dt )> render_cb(render_callback);
		gui.add_callback( render_cb );
		Input::key_callbacks.push_back( std::bind(&key_callback,_1,_2,_3,_4,_5) );

		// Start up the application
		gui.display();
	}

	return 0;
}

void copy_to_drawmesh(){

	//
	// Here, we're just pretending that the refmesh is the real BaseObject
	// being drawn, since it will eventually replace TriangleMesh.
	//
	for( int i=0; i<drawmesh->vertices.size(); ++i ){
		int idx = refmesh->start_idx+3*i;
		drawmesh->vertices[i][0] = refmesh->vertices[idx+0];
		drawmesh->vertices[i][1] = refmesh->vertices[idx+1];
		drawmesh->vertices[i][2] = refmesh->vertices[idx+2];
	}
}

void render_callback( GLFWwindow* window, Camera *cam, float screen_dt ){}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	if (action != GLFW_PRESS){ return; }
	switch(key){
		case GLFW_KEY_RIGHT: {
			for( int i=3*20+1; i<verts.size(); i+=3 ){
				verts[i] += 1.f; 
			}
			for( int i=0; i<3*20; ++i ){
				verts[i]-=1.f;
			}
			copy_to_drawmesh();
		} break;
	}
}
