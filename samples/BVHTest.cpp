
#include "MCL/SceneManager.hpp"
#include "MCL/Gui.hpp"

using namespace mcl;

void clip_mesh( mcl::SceneManager &scene );

int main(int argc, char *argv[]){

	std::vector<std::string> types;
	types.push_back( "spatial" );
	types.push_back( "linear" );

	for( int i=0; i<types.size(); ++i ){

		SceneManager scene;
		std::stringstream ss; ss << MCLSCENE_SRC_DIR << "/conf/Lucy.xml";
		if( !scene.load( ss.str() ) ){ return 0; }
		assert( scene.objects.size()==1 );

		std::stringstream fnss; fnss << "bvh_" << types[i] << ".txt";
		std::ofstream filestream;
		filestream.open( fnss.str().c_str() );
		filestream << "%% num_tris\tnum_bvh_nodes\tbuild_time_s";

		for( int j=0; j<20; ++j ){

			std::stringstream oss; oss << "\n";

			// Run a BVH build and store results
			int n_tris = scene.objects[0]->get_TriMesh()->faces.size();
			oss << n_tris << "\t";

			// Time the rebuild
			std::chrono::time_point<std::chrono::system_clock> start, end;
			start = std::chrono::system_clock::now();
			std::shared_ptr<BVHNode> bvh = scene.get_bvh(true,types[i]);
			end = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds = end-start;

			oss << bvh->num_children << "\t" << elapsed_seconds.count();
			std::string line = oss.str();
			std::cout << types[i] << ", " << j << ":\t" << line << std::endl;

			// Skip first iteration: overly fast because it skips reallocations
			// with deletes in my trimesh geometry class.
			if( j>0 ){ filestream << line.c_str(); }

			clip_mesh(scene);

		} // end loop clips

		filestream.close();

	} // end loop types

	return 0;
}

void clip_mesh( mcl::SceneManager &scene ){

	if( scene.objects.size() != 1 ){ return; }

	std::shared_ptr<BVHNode> bvh = scene.get_bvh(false);
	trimesh::TriMesh *mesh = scene.objects[0]->get_TriMesh().get();
	trimesh::box b( bvh->aabb->min ); b+=bvh->aabb->max;
	b.min[1] += ((b.max[1]-b.min[1])*0.05f); // increase the bmin and clip
	trimesh::clip( mesh, b );
	trimesh::remove_unused_vertices( mesh );
}






