// Copyright 2016 Matthew Overby.
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

#include "MCL/SceneManager.hpp"

using namespace mcl;
using namespace trimesh;


trimesh::TriMesh::BSphere SceneManager::get_bsphere( bool recompute ){
	if( bsphere.valid && !recompute ){ return bsphere; }
	build_boundary();
	return bsphere;
}


void SceneManager::build_boundary(){

	bsphere.valid = false;
	Miniball<3,float> mb;

//	for( int i=0; i<lights.size(); ++i ){
//		if( lights[i].type=="point" ){ mb.check_in(lights[i].pos); }
//	}

	for( int i=0; i<objects.size(); ++i ){
		vec min, max;
		objects[i]->get_aabb( min, max );
		mb.check_in( min ); mb.check_in( max );
	}

	mb.build();
	bsphere.center = mb.center();
	bsphere.r = sqrt(mb.squared_radius());
	if( std::isnan( bsphere.r ) ){ bsphere.r=0.0; }
	bsphere.valid = true;

}


void SceneManager::build_meshes(){

	if( meshes.size() == objects.size() ){ return; }
	meshes.clear();
	meshes.reserve( objects.size() );

	for( int i=0; i<objects.size(); ++i ){
		std::shared_ptr<trimesh::TriMesh> mesh = objects[i]->get_TriMesh();
		if( mesh != NULL ){ meshes.push_back( mesh ); }
	}
}


bool SceneManager::load( std::string xmlfile, bool auto_build ){

	std::string xmldir = parse::fileDir( xmlfile );

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(xmlfile.c_str());
	if( !result ){
		std::cerr << "\n**SceneManager Error: Unable to load " << xmlfile << std::endl;
		return false;
	}

	objects_built = false;

	// Get the node that stores scene info
	pugi::xml_node head_node = doc.first_child();
	while( parse::to_lower(head_node.name()) != "mclscene" && head_node ){ head_node = head_node.next_sibling(); }

	// Now parse scene information
	for( pugi::xml_node::iterator main_node = head_node.begin(); main_node != head_node.end(); ++main_node ){

		pugi::xml_node curr_node = *main_node;
		std::string name = curr_node.attribute("name").as_string();
		std::string type = curr_node.attribute("type").as_string();
		std::string tag = curr_node.name();
		if( name.size() == 0 || type.size() == 0 ){
			std::cerr << "\n**SceneManager Error: Component \"" << curr_node.name() << "\" need a name and type." << std::endl;
			return false;
		}

		// Load the parameters
		std::vector<Param> params;
		{
			load_params( params, curr_node );
			// If any parameters are "file" give it the full path name
			for( int i=0; i<params.size(); ++i ){
				if( parse::to_lower(params[i].tag) == "file" ){
					params[i].value = xmldir + params[i].as_string();
				}
			}
		} // end load parameters

		// Create the component
		components.push_back( Component( tag, name, type ) );
		components.back().params = params;

	} // end loop scene info

	if( auto_build ){ return build_components(); }
	return true;

} // end load xml file


bool SceneManager::build_components(){

	// Only build scene components once per load(...) call
	if( objects_built ){ return false; }

	// Add default builders
	if( obj_builders.size()==0 ){ add_callback( BuildObjCallback(default_build_object) ); }
	if( mat_builders.size()==0 ){ add_callback( BuildMatCallback(default_build_material) ); }

	// Loop components and invoke callbacks
	for( int j=0; j<components.size(); ++j ){

		std::string tag = parse::to_lower(components[j].tag);
		std::string name = parse::to_lower(components[j].name);

		//	Build Camera
		if( tag == "camera" ){

			// Call the builders
			for( int i=0; i<cam_builders.size(); ++i ){
				std::shared_ptr<BaseCamera> cam = cam_builders[i]( components[j] );
				if( cam != NULL ){
					cameras.push_back( cam );
					cameras_map[name] = cam;
				}
			}

		} // end build Camera

		//	Build Light
		else if( tag == "light" ){

			// Call the builders
			for( int i=0; i<light_builders.size(); ++i ){
				std::shared_ptr<BaseLight> light = light_builders[i]( components[j] );
				if( light != NULL ){
					lights.push_back( light );
					lights_map[name] = light;
				}
			}

		} // end build Light

		//	Build Material
		else if( tag == "material" ){

			// Call the builders
			for( int i=0; i<mat_builders.size(); ++i ){
				std::shared_ptr<BaseMaterial> mat = mat_builders[i]( components[j] );
				if( mat != NULL ){
					materials.push_back( mat );
					materials_map[name] = mat;
				}
			}

		} // end build material

		//	Build Object
		else if( tag == "object" ){

			// Call the builders
			for( int i=0; i<obj_builders.size(); ++i ){
				std::shared_ptr<BaseObject> obj = obj_builders[i]( components[j] );
				if( obj != NULL ){
					objects.push_back( obj );
					objects_map[name] = obj;
				}
			}

		} // end build object

		// Build unknown
		else{
			for( int i=0; i<builders.size(); ++i ){ builders[i]( components[j] ); }

		} // end build unknown

	} // end loop components

	objects_built = true;
	return true;

} // end build components



void SceneManager::build_bvh( int split_mode ){

	if( root_bvh==NULL ){ root_bvh = std::shared_ptr<BVHNode>( new BVHNode() ); }
	else{ root_bvh.reset( new BVHNode() ); }
//	std::chrono::time_point<std::chrono::system_clock> start, end;

	if( split_mode == 0 ){
//		std::cout << "spatial bvh begin: " << std::flush;
//		start = std::chrono::system_clock::now();
		int num_nodes = root_bvh->make_tree_spatial( objects );
//		end = std::chrono::system_clock::now();
//		std::chrono::duration<double> elapsed_seconds = end-start;
//		std::cout << elapsed_seconds.count() << "s\n";
	}

	else if( split_mode == 1 ){
//		std::cout << "linear bvh begin: " << std::flush;
//		start = std::chrono::system_clock::now();
		int num_nodes = root_bvh->make_tree_lbvh( objects );
//		end = std::chrono::system_clock::now();
//		std::chrono::duration<double> elapsed_seconds = end-start;
//		std::cout << elapsed_seconds.count() << "s\n";
	}

} // end build bvh


std::shared_ptr<BVHNode> SceneManager::get_bvh( bool recompute, std::string type ){
	int split_mode=0;
	if( parse::to_lower(type)=="spatial" ){ split_mode=0; }
	else if( parse::to_lower(type)=="linear" ){ split_mode=1; }

	if( recompute || root_bvh==NULL ){ build_bvh( split_mode ); }
	return root_bvh;
}


mcl::Component &SceneManager::get( std::string name ){
	for( int i=0; i<components.size(); ++i ){
		if( components[i].name == name ){ return components[i]; }
	}
	// not found, add it
	components.push_back( mcl::Component( "", name, "" ) );
	return components.back();
}


bool SceneManager::exists( std::string name ) const {
	for( int i=0; i<components.size(); ++i ){
		if( components[i].name == name ){ return true; }
	}
	return false;
}

