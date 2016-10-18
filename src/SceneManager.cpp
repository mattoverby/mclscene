// Copyright (c) 2016 University of Minnesota
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
#include "bsphere.h" // for miniball (trimesh2)

using namespace mcl;
using namespace trimesh;

SceneManager::SceneManager() {
	root_bvh=NULL;
	createObject = default_build_object;
	createCamera = default_build_camera;
	createLight = default_build_light;
	createMaterial = default_build_material;
	last_radius=-1.f;
	last_center = trimesh::vec(0,0,0);
}


SceneManager::~SceneManager(){
	objects.clear();
	cameras.clear();
	lights.clear();
	materials.clear();
	object_params.clear();
	material_params.clear();
	camera_params.clear();
	light_params.clear();
}


bool SceneManager::load( std::string filename ){

	//
	//	Load the XML file into mcl::Component
	//

	std::string xmldir = parse::fileDir( filename );
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(filename.c_str());
	if( !result ){
		std::cerr << "\n**SceneManager::load_xml Error: Unable to load " << filename << std::endl;
		return false;
	}

	// Get the node that stores scene info
	pugi::xml_node head_node = doc.first_child();
	while( parse::to_lower(head_node.name()) != "mclscene" && head_node ){ head_node = head_node.next_sibling(); }

	// First, create a mapping for named references.
	// Currently only doing materials and objects
	int num_materials = 0; int num_objects = 0;
	int num_cameras = 0; int num_lights = 0;
	std::unordered_map< std::string, int > material_map;
	std::unordered_map< std::string, int > object_map;
	std::vector<pugi::xml_node> children(head_node.begin(), head_node.end()); // grab all children to iterate with omp

	//
	// Preliminary loop to create the name to index mappings
	//
#pragma omp parallel for reduction(+:num_materials,num_objects,num_cameras,num_lights)
	for( int i=0; i<children.size(); ++i ){
		std::string tag = parse::to_lower(children[i].name());
		std::string name = parse::to_lower(children[i].attribute("name").as_string());
		if( name.size() > 0 ){
			if( tag == "material" ){
#pragma omp critical
				{ material_map[name]=num_materials; }
			}
			if( tag == "object" ){
#pragma omp critical
				{ object_map[name]=num_objects; }
			}
		} // end has a name

		// Increment counters
		if( tag == "material" ){ num_materials++; }
		else if( tag == "object" ){ num_objects++; }
		else if( tag == "camera" ){ num_cameras++; }
		else if( tag == "light" ){ num_lights++; }

	} // end create name maps

	// Reserve space for scene components
	objects.reserve(num_objects);
	object_params.reserve(num_objects);
	materials.reserve(num_materials);
	material_params.reserve(num_materials);
	cameras.reserve(num_cameras);
	camera_params.reserve(num_cameras);
	lights.reserve(num_lights);
	light_params.reserve(num_lights);

	//
	// Now parse scene information and create components
	// Not parallelized to maintain correct file-to-index order
	//
	for( int child=0; child<children.size(); ++child ){

		pugi::xml_node curr_node = children[child];
		std::string type = curr_node.attribute("type").as_string();
		std::string tag = parse::to_lower(curr_node.name());

		if( type.size() == 0 ){
			std::cerr << "\n**SceneManager::load_xml Error: Component \"" << curr_node.name() << "\" need a type." << std::endl;
			return false;
		}

		// Load the parameters
		std::vector<Param> params;
		{
			load_params( params, curr_node );
			// If any parameters are "file" or "texture" give it the path name from the current execution directory
			for( int i=0; i<params.size(); ++i ){
				if( parse::to_lower(params[i].tag) == "file" || parse::to_lower(params[i].tag) == "texture" ){
					params[i].value = xmldir + params[i].as_string();
				}
			}
		} // end load parameters

		// Now build
		{
			//	Build Camera
			if( tag == "camera" ){
				std::shared_ptr<BaseCamera> cam = createCamera( type, params );
				if( cam != NULL ){
					cameras.push_back( cam );
					camera_params.push_back( params );
				}
			} // end build Camera

			//	Build Light
			else if( tag == "light" ){
				std::shared_ptr<BaseLight> light = createLight( type, params );
				if( light != NULL ){
					lights.push_back( light );
					light_params.push_back( params );
				}
			} // end build Light

			//	Build Object
			else if( tag == "object" ){
				std::shared_ptr<BaseObject> obj = createObject( type, params );
				if( obj != NULL ){
					objects.push_back( obj );
					object_params.push_back( params );

					// See if we can figure out the material
					int mat_param_index = param_index("material",params);

					if( mat_param_index >= 0 ){
						std::string material_name = parse::to_lower( params[mat_param_index].as_string() );
						if( material_map.count(material_name) > 0 ){
							obj->set_material( material_map[material_name] );
						} // is a named material
						else {
							std::shared_ptr<BaseMaterial> mat = make_preset_material( material_name );
							if( mat != NULL ){
								int idx = materials.size();
								materials.push_back( mat );
								material_params.push_back( std::vector<Param>() );
								obj->set_material( idx );
							}
						} // is a material preset
					} // end check material
				}
			} // end build object

			//	Build Material
			if( tag == "material" ){
				std::shared_ptr<BaseMaterial> mat = createMaterial( type, params );
				if( mat != NULL ){
					int idx = materials.size();
					materials.push_back( mat );
					material_params.push_back( params );
				}
			} // end build material

		} // end create component

	} // end loop scene info

	//
	//	Success, all done.
	//
	return true;
	
} // end load


void SceneManager::save( std::string xmlfile, int mode ){

	// I really should use pugixml instead of string, however that would
	// increase dependency usage more than I'd like (in the light/object/etc... classes).

	std::stringstream xml;
	std::cout << "Saving scene file: " << std::flush;

	if( mode == 0 ){

		xml << "<?xml version=\"1.0\"?>\n";
		xml << "<mclscene>";

		// Loop over objects
		for( int i=0; i<objects.size(); ++i ){
			xml << "\n" << objects[i]->get_xml( mode );
		}

		// Loop over materials, let the name be the index
		for( int i=0; i<materials.size(); ++i ){
			xml << "\n" << materials[i]->get_xml( mode );
		}

		// Loop over cameras
		for( int i=0; i<cameras.size(); ++i ){
			xml << "\n" << cameras[i]->get_xml( mode );
		}

		// Loop over lights
		for( int i=0; i<lights.size(); ++i ){
			xml << "\n" << lights[i]->get_xml( mode );
		}

		xml << "\n</mclscene>";

	} else {
		std::cerr << "SceneManager::save Error: I don't know that save type" << std::endl;
		return;
	}

	// Save to a file
	std::stringstream filename;
	filename << MCLSCENE_BUILD_DIR << "/" << parse::get_timestamp() << ".xml";
	std::cout << filename.str() << std::endl;
	std::ofstream filestream;
	filestream.open( xmlfile.c_str() );
	filestream << xml.str();
	filestream.close();

} // end save


void SceneManager::build_bvh( std::string split_mode ){

	split_mode = parse::to_lower(split_mode);
	if( root_bvh==NULL ){ root_bvh = std::shared_ptr<BVHNode>( new BVHNode() ); }
	else{ root_bvh.reset( new BVHNode() ); }

	if( split_mode == "spatial" ){
		int num_nodes = BVHBuilder::make_tree_spatial( root_bvh.get(), objects );
	}

	else if( split_mode == "linear" ){
		int num_nodes = BVHBuilder::make_tree_lbvh( root_bvh.get(), objects );
	}

	else{ std::cerr << "**SceneManager::build_bvh Error: I don't know BVH type \"" << split_mode << "\"" << std::endl; }

} // end build bvh


std::shared_ptr<BVHNode> SceneManager::SceneManager::get_bvh( bool recompute, std::string type ){
	if( recompute || root_bvh==NULL ){ build_bvh( type ); }
	return root_bvh;
}


std::shared_ptr<mcl::BaseObject> SceneManager::make_object( std::string type ){

	std::vector<Param> params;
	std::shared_ptr<BaseObject> newObject = createObject( type, params );
	if( newObject == NULL ){ return NULL; }

	// Add it to the SceneManager and return it
	objects.push_back( newObject );
	object_params.push_back( params );

	return newObject;

} // end make object


std::shared_ptr<mcl::BaseLight> SceneManager::make_light( std::string type ){

	std::vector<Param> params;
	std::shared_ptr<BaseLight> newLight = createLight( type, params );
	if( newLight == NULL ){ return NULL; }

	// Add it to the SceneManager and return it
	lights.push_back( newLight );
	light_params.push_back( params );
	return newLight;

} // end make light


std::shared_ptr<mcl::BaseCamera> SceneManager::make_camera( std::string type ){

	std::vector<Param> params;
	std::shared_ptr<BaseCamera> newCam = createCamera( type, params );
	if( newCam == NULL ){ return NULL; }

	// Add it to the SceneManager and return it
	cameras.push_back( newCam );
	camera_params.push_back( params );
	return newCam;

} // end make light


std::shared_ptr<mcl::BaseMaterial> SceneManager::make_material( std::string type ){

	std::vector<Param> params;
	std::shared_ptr<BaseMaterial> newMat = createMaterial( type, params );
	if( newMat == NULL ){ return NULL; }

	// Add it to the SceneManager and return it
	materials.push_back( newMat );
	material_params.push_back( params );
	return newMat;

} // end make light


void SceneManager::make_3pt_lighting( const trimesh::vec &center, float distance ){

	lights.clear();
	light_params.clear();

	// TODO use spotlight instead of point light
	std::shared_ptr<BaseLight> l0 = make_light( "point" );
	std::shared_ptr<BaseLight> l1 = make_light( "point" );
	std::shared_ptr<BaseLight> l2 = make_light( "point" );
	std::shared_ptr<DefaultLight> key = std::dynamic_pointer_cast<DefaultLight>( l0 );
	std::shared_ptr<DefaultLight> fill = std::dynamic_pointer_cast<DefaultLight>( l1 );
	std::shared_ptr<DefaultLight> back = std::dynamic_pointer_cast<DefaultLight>( l2 );

	float half_d = distance/2.f;
	float quart_d = distance/4.f;

	// Set positions
	key->light.position = center + trimesh::vec(-half_d,0.f,distance);
	fill->light.position = center + trimesh::vec(half_d,0.f,distance);
	back->light.position = center + trimesh::vec(0.f,quart_d,-distance);

	// Set intensity
	key->light.intensity = trimesh::vec(.8,.8,.8);
	fill->light.intensity = trimesh::vec(.6,.6,.6);
	back->light.intensity = trimesh::vec(.6,.6,.6);

	// Falloff (none)
	key->light.falloff = trimesh::vec(1.f,0.f,0.f);
	fill->light.falloff = trimesh::vec(1.f,0.f,0.f);
	back->light.falloff = trimesh::vec(1.f,0.f,0.f);

} // end make three point lighting


void SceneManager::get_bsphere( trimesh::vec *center, float *radius, bool recompute ){

	if( last_radius <= 0.f || recompute ){
		trimesh::Miniball<3,float> mb;
		for( int i=0; i<objects.size(); ++i ){
			vec min, max;
			objects[i]->bounds( min, max );
			mb.check_in( min ); mb.check_in( max );
		}
		mb.build();
		last_radius = sqrt(mb.squared_radius());
		last_center = mb.center();
	}

	*center = last_center;
	*radius = last_radius;
}


