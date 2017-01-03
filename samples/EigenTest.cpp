#include "MCL/SceneManager.hpp"
#include "MCL/Application.hpp"
#include "MCL/MicroTimer.hpp"
#include <Eigen/Core>
using namespace mcl;


Eigen::VectorXd verts; 

int main(int argc, char *argv[]){

	SceneManager scene;

	//
	// This is the vertex data that would be housed by a simulation
	// class, or something.
	//
	int orig_size = 3*20;
	verts.resize( orig_size );


	return 0;
}
