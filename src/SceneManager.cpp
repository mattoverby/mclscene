// Copyright 2014 Matthew Overby.
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

trimesh::box3 SceneManager::get_bbox( bool recompute ){
	if( bbox.valid && !recompute ){ return bbox; }
	build_boundary();
	return bbox;
}


trimesh::TriMesh::BSphere SceneManager::get_bsphere( bool recompute ){
	if( bsphere.valid && !recompute ){ return bsphere; }
	build_boundary();
	return bsphere;
}


void SceneManager::build_boundary(){

	bbox.clear();
	bsphere.valid = false;
	Miniball<3,float> mb;

	for( int i=0; i<cameras.size(); ++i ){
		bbox += cameras[i].pos;
		mb.check_in(cameras[i].pos);
	}

	for( int i=0; i<lights.size(); ++i ){
		if( lights[i].type=="point" ){ bbox += lights[i].pos; mb.check_in(lights[i].pos); }
	}

	for( int i=0; i<objects.size(); ++i ){
		if( objects[i].built_TriMesh != NULL ){
			for( int j=0; j<objects[i].built_TriMesh.get()->vertices.size(); ++j ){
				bbox += objects[i].built_TriMesh.get()->vertices[j];
			}
			mb.check_in( objects[i].built_TriMesh.get()->vertices.begin(), objects[i].built_TriMesh.get()->vertices.end() );
		}
		if( objects[i].built_TetMesh != NULL ){
			for( int j=0; j<objects[i].built_TetMesh.get()->vertices.size(); ++j ){
				bbox += objects[i].built_TetMesh.get()->vertices[j];
			}
			mb.check_in( objects[i].built_TetMesh.get()->vertices.begin(), objects[i].built_TetMesh.get()->vertices.end() );
		}
	}

	mb.build();
	bsphere.center = mb.center();
	bsphere.r = sqrt(mb.squared_radius());
	bsphere.valid = true;
}


bool SceneManager::load( std::string xmlfile ){

	std::string xmldir = parse::fileDir( xmlfile );

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(xmlfile.c_str());
	if( !result ){
		std::cerr << "\n**SceneManager Error: Unable to load " << xmlfile << std::endl;
		return false;
	}

	// Get the node that stores scene info
	pugi::xml_node head_node = doc.first_child();
	while( parse::to_lower(head_node.name()) != "mclscene" && head_node ){ head_node = head_node.next_sibling(); }

	// Now parse scene information
	for( pugi::xml_node::iterator main_node = head_node.begin(); main_node != head_node.end(); ++main_node ){

		pugi::xml_node curr_node = *main_node;
		std::string name = curr_node.attribute("name").as_string();
		if( name.size() == 0 ){
			std::cerr << "\n**SceneManager Error: Everything needs a name!" << std::endl;
			return false;
		}

		//
		//	Parse Camera
		//
		if( parse::to_lower(curr_node.name()) == "camera" ){

			CameraMeta cam;
			cam.name = name;

			// Set defaults
			cam.p.str_vals[ "type" ].push_back( "perspective" );
			cam.p.vec3_vals[ "position" ].push_back( vec3(0,0,0) );
			cam.p.vec3_vals[ "direction" ].push_back( vec3(0,0,-1) );
			cam.p.vec3_vals[ "up" ].push_back( vec3(0,1,0) );

			// Load parameters
			if( !cam.p.load_params( curr_node ) ){ return false; }

			// Check the parameters and store the data
			if( !cam.check_params() ){ printf("\n**SceneManager Error: Camera check_params\n"); return false; }
			cameras.push_back( cam );
//			camera_map[name] = &cameras.back();

		} // end parse camera

		//
		//	Parse Light
		//
		if( parse::to_lower(curr_node.name()) == "light" ){

			LightMeta light;
			light.name = name;

			// Set defaults
			light.p.str_vals[ "type" ].push_back( "point" );
			light.p.vec3_vals[ "position" ].push_back( vec(0,0,0) );
			light.p.vec3_vals[ "intensity" ].push_back( vec(1,1,1) );

			// Load parameters
			if( !light.p.load_params( curr_node ) ){ return false; }

			// Check the parameters and store the data
			if( !light.check_params() ){ printf("\n**SceneManager Error: Light check_params\n"); return false; }
			lights.push_back( light );
//			light_map[name] = &lights.back();

		} // end parse light

		//
		//	Parse Material
		//
		if( parse::to_lower(curr_node.name()) == "material" ){

			MaterialMeta mat;
			mat.name = name;

			// Set defaults
			mat.p.str_vals[ "type" ].push_back( "diffuse" );

			// Load parameters
			if( !mat.p.load_params( curr_node ) ){ return false; }

			// Check the parameters and store the data
			if( !mat.check_params() ){ printf("\n**SceneManager Error: Material check_params\n"); return false; }
			materials.push_back( mat );
			material_map[name] = materials.size()-1;

		} // end parse material

		//
		//	Parse Object
		//
		if( parse::to_lower(curr_node.name()) == "object" ){

			ObjectMeta object;
			object.name = name;

			// Set defaults
			object.p.str_vals[ "type" ].push_back( "none" );

			// Load parameters
			if( !object.p.load_params( curr_node ) ){ return false; }

			// If there is a file, append the full path
			if( object.p.str_vals.count("file")>0 ){
				for( int fi=0; fi<object.p.str_vals["file"].size(); ++fi ){
					std::string file = object.p.str_vals["file"][fi];
					std::string full_path = xmldir + file;
					object.p.str_vals["file"][fi] = full_path;
				}
			}

			// Check the parameters and store the data
			if( !object.check_params() ){ printf("\n**SceneManager Error: Object check_params\n"); return false; }

			objects.push_back( object );
//			object_map[name] = &objects.back();

		} // end parse object

	} // end loop scene info

	return true;

} // end load xml file


