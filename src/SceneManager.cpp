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
	last_center = Vec3f(0,0,0);
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
//#pragma omp parallel for reduction(+:num_materials,num_objects,num_cameras,num_lights)
	for( int i=0; i<children.size(); ++i ){
		std::string tag = parse::to_lower(children[i].name());
		std::string name = parse::to_lower(children[i].attribute("name").as_string());
		if( name.size() > 0 ){
			if( tag == "material" ){
//#pragma omp critical
				{ material_map[name]=num_materials; }
			}
			if( tag == "object" ){
//#pragma omp critical
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
				std::shared_ptr<Camera> cam = createCamera( type, params );
				if( cam != NULL ){
					cameras.push_back( cam );
					camera_params.push_back( params );
				}
			} // end build Camera

			//	Build Light
			else if( tag == "light" ){
				std::shared_ptr<Light> light = createLight( type, params );
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
							obj->app.material = material_map[material_name];
						} // is a named material
						else {
							std::shared_ptr<Material> mat = make_preset_material( material_name );
							if( mat != NULL ){
								int idx = materials.size();
								materials.push_back( mat );
								material_params.push_back( std::vector<Param>() );
								obj->app.material = idx;
							}
						} // is a material preset
					} // end check material
				}
			} // end build object

			//	Build Material
			if( tag == "material" ){
				std::shared_ptr<Material> mat = createMaterial( type, params );
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
			int mat = objects[i]->app.material;
			if( mat >= 0 && mat < objects.size() ){
				std::stringstream ss; ss << "mat" << mat;
				// TODO export
			}
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


void SceneManager::get_vertex_pool( VertexPool &pool, bool dynamic_only, bool recompute ){

	if( !recompute && vertex_pool.valid ){ pool = vertex_pool; return; }
	vertex_pool.clear();

	// Make a new vertex pool
	for( int i=0; i<objects.size(); ++i ){
		if( dynamic_only && !(objects[i]->app.dynamic) ){ continue; }
		BaseObject::AppData *app = &objects[i]->app;

		vertex_pool.vertices.push_back( app->vertices );
		vertex_pool.num_vertices.push_back( app->num_vertices );
		vertex_pool.normals.push_back( app->normals );
		vertex_pool.num_normals.push_back( app->num_normals );
		vertex_pool.colors.push_back( app->colors );
		vertex_pool.num_colors.push_back( app->num_colors );
		vertex_pool.texcoords.push_back( app->texcoords );
		vertex_pool.num_texcoords.push_back( app->num_texcoords );
		vertex_pool.faces.push_back( app->faces );
		vertex_pool.num_faces.push_back( app->num_faces );
		vertex_pool.index.push_back( i );

	} // end loop objects

	pool = vertex_pool;

} // end get vertex pool


void SceneManager::make_3pt_lighting( const Vec3f &eye, const Vec3f &center ){

	Vec3f up(0,1,0);
	Vec3f w = eye-center;
	// Since we're setting no falloff, lights can be far away
	float distance = w.norm()*6.f;
	w.normalize();
	Vec3f u = up.cross(w);
	Vec3f v = w.cross(u);

	lights.clear();
	light_params.clear();

	std::shared_ptr<Light> key = make_light<Light>( "spot" );
	std::shared_ptr<Light> fill = make_light<Light>( "spot" );
	std::shared_ptr<Light> back = make_light<Light>( "spot" );

	float half_d = distance/2.f;
	float quart_d = distance/4.f;

	// Set positions
	key->app.position = center + w*distance + v*half_d - u*distance;
	fill->app.position = center + w*distance + u*distance;
	back->app.position = center - w*distance + v*distance;

	// Directions (not used for point lights though)
	key->app.direction = center-key->app.position;
	fill->app.direction = center-fill->app.position;
	back->app.direction = center-back->app.position;

	// Set intensity
	key->app.intensity = Vec3f(.8,.8,.8);
	fill->app.intensity = Vec3f(.6,.6,.6);
	back->app.intensity = Vec3f(.6,.6,.6);

	// Falloff (none)
	key->app.falloff = Vec3f(1.f,0.f,0.f);
	fill->app.falloff = Vec3f(1.f,0.f,0.f);
	back->app.falloff = Vec3f(1.f,0.f,0.f);

} // end make three point lighting


void SceneManager::get_bsphere( Vec3f *center, float *radius, bool recompute ){

	if( last_radius <= 0.f || recompute ){
		trimesh::Miniball<3,float> mb;
		for( int i=0; i<objects.size(); ++i ){
			Vec3f min, max;
			objects[i]->bounds( min, max );
			mb.check_in( trimesh::vec(min[0],min[1],min[2]) ); mb.check_in( trimesh::vec(max[0],max[1],max[2]) );
		}
		mb.build();
		last_radius = sqrt(mb.squared_radius());
		trimesh::vec curr_center = mb.center();
		last_center = Vec3f( curr_center[0], curr_center[1], curr_center[2] );
	}

	*center = last_center;
	*radius = last_radius;
}



