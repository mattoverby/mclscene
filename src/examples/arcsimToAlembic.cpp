// Copyright (c) 2017 University of Minnesota
// 
// MCLSCENE Uses the BSD 2-Clause License (http://www.opensource.org/licenses/BSD-2-Clause)
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other materials
//    provided with the distribution.
// THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR  A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE UNIVERSITY OF MINNESOTA, DULUTH OR CONTRIBUTORS BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// By Matt Overby (http://www.mattoverby.net)

#include "MCL/MeshIO.hpp"
#include "MCL/AlembicIO.hpp"
#include "MCL/ArgParser.hpp"
#include "../pugixml/pugixml.hpp"

using namespace mcl;

void help();
bool file_exists( const std::string &filename );
int num_meshes( const std::string &dir );
int num_obs( const std::string &dir );
std::string pad_leading_zeros( int num_digits, int num );
std::string get_name( const std::string &dir );
bool test_conf( const std::string &dir );

int main(int argc, char *argv[]){

	// Winding order matters for whatever is reading in the abc file.
	bool ccw_output = false;

	ArgParser parser(argc,argv);
	if(	parser.exists("-h") || parser.exists("-help") ||
		parser.exists("--h") || parser.exists("--help") ||
		!parser.exists("-i") || !parser.exists("-dt") ){
		help();
		return EXIT_SUCCESS;
	}

	std::string dir = parser.get<std::string>("-i");
	if( dir.back()!='/' ){ dir += '/'; }
	unsigned int skipframe = 1;
	parser.get<unsigned int>( "-skip", &skipframe );
	double dt = parser.get<double>("-dt");
	dt += 1.0 / double(skipframe);

	// Attempt to open conf.json to make sure the diretory is all dandy.
	if( !test_conf(dir) ){ return EXIT_FAILURE; }

	// Create the ABC file
	std::string abc_name = get_name( dir );
	abc_name = abc_name + ".abc";
	parser.get<std::string>("-o", &abc_name);
	AlembicExporter exporter( int(1.0/dt), abc_name );

	// Get the number of meshes and obs
	int n_meshes = num_meshes( dir );
	if( n_meshes <= 0 ){ return EXIT_FAILURE; }
	int n_obs = num_obs( dir );
	std::cout << "ArcSim export has " << n_meshes << " meshes and " << n_obs << " obstacles." << std::endl;

	// Load the obstacle meshes ahead of time. They are only transformed each frame.
	std::vector<TriangleMesh> obstacles;
	std::vector< Eigen::AlignedBox<float,3> > obs_aabb;
	std::vector<int> obs_handles;
	for( int i=0; i<n_obs; ++i ){
		obstacles.emplace_back( TriangleMesh() );
		std::string objfile = dir + "obs_" + pad_leading_zeros(2,i) + ".obj";
		if( !meshio::load_obj( &obstacles.back(), objfile, false, false, false ) ){
			std::cerr << "\n**arcsimeToAlembic Error: Failed to load " << objfile << "\n" << std::endl;
		}
		std::string mesh_name = "obs"+std::to_string(i);
		obs_handles.emplace_back( exporter.add_object(mesh_name) );
		obs_aabb.emplace_back( obstacles.back().bounds() );
	}

	// Create the objects so that the alembic exporter knows about them
	std::vector<int> deform_handles;
	for( int i=0; i<n_meshes; ++i ){
		std::string mesh_name = "mesh"+std::to_string(i);
		deform_handles.emplace_back( exporter.add_object(mesh_name) );
	}

	// Loop frames of the simulation and add them to the exporter
	std::cout << "Saving ArcSim frames to " << abc_name << std::endl;
	std::cout << "(This will take a while...)" << std::endl;
	for( int frame_num=0; frame_num<999999; frame_num += skipframe ){
		std::cout << '\r';
		std::cout << "frame " << frame_num << std::flush;
		std::string framestr = pad_leading_zeros(6,frame_num);

		// Load all deforming meshes
		for( int m=0; m<n_meshes; ++m ){
			std::string meshstr = pad_leading_zeros(2,m);
			std::string objfile = dir + framestr + '_' + meshstr + ".obj";

			// If the file doesn't exists, we have reached the end of the sim
			if( !file_exists(objfile) ){ 
				frame_num = 999999; // break outer loop too
				break;
			}

			// Load the deforming mesh
			TriangleMesh mesh;
			if( !meshio::load_obj( &mesh, objfile, false, false, false ) ){
				std::cerr << "\n**arcsimeToAlembic Error: Failed to load " << objfile << "\n" << std::endl;
				return EXIT_FAILURE;
			}

			// Add it to the exporter
			exporter.add_frame( deform_handles[m], &mesh.vertices[0][0], mesh.vertices.size(),
				&mesh.faces[0][0], mesh.faces.size(), ccw_output );

		} // end loop meshes

		// Load all obstacle transforms
		for( int o=0; o<n_obs && frame_num<999999; ++o ){
			std::string meshstr = pad_leading_zeros(2,o);
			std::string xformfile = dir + framestr + "obs" + meshstr + ".txt";
			pugi::xml_document doc;
			pugi::xml_parse_result result = doc.load_file(xformfile.c_str());
			if( !result ){
				std::cerr << "\n**arcsimeToAlembic Error: Unable to load " << xformfile << std::endl;
				return EXIT_FAILURE;
			}

			TriangleMesh mesh = obstacles[o];
			Eigen::AlignedBox<float,3> aabb = obs_aabb[o];
			Vec3f obs_center = aabb.center();

			pugi::xml_node::iterator node_iter = doc.first_child();
			for( ; node_iter != doc.end(); node_iter++ ){
				pugi::xml_node curr_node = *node_iter;
				std::string name = curr_node.name();
				XForm<float> xf; xf.setIdentity();
				if( name == "rotate" ){
					float angle = curr_node.attribute("angle").as_float() * (180.f/M_PI);
					float x = curr_node.attribute("x").as_float();
					float y = curr_node.attribute("y").as_float();
					float z = curr_node.attribute("z").as_float();

					// Translate obs to origin before rotation
					XForm<float> xf0 = xform::make_trans<float>(-obs_center);
					XForm<float> xf1 = xform::make_rot<float>( angle, Vec3f(x,y,z) );
					XForm<float> xf2 = xform::make_trans<float>(obs_center);
					xf = xf2 * xf1 * xf0;
				}
				else if( name == "scale" ){
					float s = curr_node.attribute("value").as_float();
					xf = xform::make_scale<float>(s,s,s);
				}
				else if( name == "translate" ){
					float x = curr_node.attribute("x").as_float();
					float y = curr_node.attribute("y").as_float();
					float z = curr_node.attribute("z").as_float();
					xf = xform::make_trans<float>(x,y,z);
				}
				mesh.apply_xform(xf);
			} // end load xform

			// Add it to the exporter
			exporter.add_frame( obs_handles[o], &mesh.vertices[0][0], mesh.vertices.size(),
				&mesh.faces[0][0], mesh.faces.size(), ccw_output );

		} // end loop obstacles

	} // end loop frames

	std::cout << "\nAll done!" << std::endl;

	return EXIT_SUCCESS;
}

void help(){

	std::cout << "\n====================\nUsage:" <<
		"\n\t -i <input directory>" <<
		"\n\t -dt <time step>" <<
		"\n\t -o <output file, optional>" <<
		"\n\t -skip <frames to skip, optional>" <<
	"\n" << std::endl;
		

}

bool file_exists( const std::string &filename ){
	std::ifstream infile( filename.c_str() );
	if( !infile ){ return false; }
	return true;
}

int num_meshes( const std::string &dir ){
	int n_meshes = 0;
	for( int i=0; i<99; ++i ){
		std::stringstream obj;
		obj << dir << "000000_";
		if( i < 10 ){ obj << "0"; }
		obj << i << ".obj";
		if( file_exists( obj.str() ) ){
			n_meshes++;
		} else { break; }
	}
	if( n_meshes == 0 ){
		std::cerr << "\n**arcsimeToAlembic Error: Problem counting meshes\n" << std::endl;
	}
	return n_meshes;
}

int num_obs( const std::string &dir ){
	int n_obs = 0;
	for( int i=0; i<99; ++i ){
		std::stringstream obj;
		obj << dir << "obs_";
		if( i < 10 ){ obj << "0"; }
		obj << i << ".obj";
		if( file_exists( obj.str() ) ){
			n_obs++;
		} else { break; }
	}
	return n_obs;
}

std::string pad_leading_zeros( int num_digits, int num ){
	return std::string(num_digits-std::to_string(num).length(),'0') + std::to_string(num);
}

std::string get_name( const std::string &dir ){
	std::string filename = dir;
	filename.pop_back();
	std::size_t endir = filename.find_last_of("/\\");
	return filename.substr(endir+1);
}

bool test_conf( const std::string &dir ){
	std::stringstream conf_file;
	conf_file << dir << "conf.json";
	if( !file_exists( conf_file.str())  ){
		std::cerr << "\n**arcsimeToAlembic Error: No conf.json found, not an arcsim export\n" << std::endl;
		return false;
	}
	return true;
}
