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

	build_meshes();
	for( int i=0; i<meshes.size(); ++i ){
		for( int j=0; j<meshes[i]->vertices.size(); ++j ){
			bbox += meshes[i]->vertices.at(j);
		}
		mb.check_in( meshes[i]->vertices.begin(), meshes[i]->vertices.end() );
	}

	mb.build();
	bsphere.center = mb.center();
	bsphere.r = sqrt(mb.squared_radius());
	bsphere.valid = true;

}


void SceneManager::build_meshes(){

	if( meshes.size() == objects.size() ){ return; }

	meshes.clear();
	meshes.reserve( objects.size() );
	for( int i=0; i<objects.size(); ++i ){

		ObjectComponent *obj = &objects[i];
		std::string ltype = parse::to_lower(obj->type);

		std::shared_ptr<BaseObject> built_obj;
		if( ltype == "sphere" ){ built_obj = std::shared_ptr<BaseObject>( new Sphere() ); }
		else if( ltype == "box" ){ built_obj = std::shared_ptr<BaseObject>( new Box() ); }
		else if( ltype == "plane" ){ built_obj = std::shared_ptr<BaseObject>( new Plane() ); }
		else if( ltype == "trimesh" ){ built_obj = std::shared_ptr<BaseObject>( new TriangleMesh() ); }
		else if( ltype == "tetmesh" ){ built_obj = std::shared_ptr<BaseObject>( new TetMesh() ); }
		else{ std::cerr << "I should really use builder callbacks..." << std::endl; exit(0); }

		built_obj->init( obj->param_vec );
		built_obj->apply_xform( obj->x_form );

		// Now that we've build the object we can get its triangle mesh
		const std::shared_ptr<TriMesh> tmesh = built_obj->get_TriMesh();
		meshes.push_back( std::make_shared<TriMesh>(*tmesh) ); // make a copy
		meshes.back()->need_normals();
		meshes.back()->need_tstrips();
	}
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
		std::string type = curr_node.attribute("type").as_string();
		if( name.size() == 0 || type.size() == 0 ){
			std::cerr << "\n**SceneManager Error: Component \"" << curr_node.name() << "\" need a name and type." << std::endl;
			return false;
		}

		//
		//	Parse Camera
		//
		if( parse::to_lower(curr_node.name()) == "camera" ){

			CameraComponent cam;
			cam.name = name;
			cam.type = type;

			// Set defaults
			cam.add_param( Param( "type", "perspective", "string" ) );
			cam.add_param( Param( "position", "0 0 0", "vec3" ) );
			cam.add_param( Param( "direction", "0 0 -1", "vec3" ) );
			cam.add_param( Param( "up", "0 1 0", "vec3" ) );

			// Load parameters
			if( !cam.load_params( curr_node ) ){ return false; }

			// Check the parameters and store the data
			if( !cam.check_params() ){ printf("\n**SceneManager Error: Camera check_params\n"); return false; }
			camera_map[ name ] = cameras.size();
			cameras.push_back( cam );

		} // end parse camera

		//
		//	Parse Light
		//
		if( parse::to_lower(curr_node.name()) == "light" ){

			LightComponent light;
			light.name = name;
			light.type = type;

			// Set defaults
			light.add_param( Param( "type", "point", "string" ) );
			light.add_param( Param( "position", "0 0 0", "vec3" ) );
			light.add_param( Param( "intensity", "1 1 1", "vec3" ) );

			// Load parameters
			if( !light.load_params( curr_node ) ){ return false; }

			// Check the parameters and store the data
			if( !light.check_params() ){ printf("\n**SceneManager Error: Light check_params\n"); return false; }
			light_map[ name ] = lights.size();
			lights.push_back( light );

		} // end parse light

		//
		//	Parse Material
		//
		if( parse::to_lower(curr_node.name()) == "material" ){

			MaterialComponent mat;
			mat.name = name;
			mat.type = type;

			// Set defaults
			mat.add_param( Param( "type", "diffuse", "string" ) );

			// Load parameters
			if( !mat.load_params( curr_node ) ){ return false; }

			// Check the parameters and store the data
			if( !mat.check_params() ){ printf("\n**SceneManager Error: Material check_params\n"); return false; }
			material_map[ name ] = materials.size();
			materials.push_back( mat );

		} // end parse material

		//
		//	Parse Object
		//
		if( parse::to_lower(curr_node.name()) == "object" ){

			ObjectComponent object;
			object.name = name;
			object.type = type;

			// Set defaults
			object.add_param( Param( "type", "none", "string" ) );

			// Load parameters
			if( !object.load_params( curr_node ) ){ return false; }

			// If there is a file, append the full path
			if( object.param_map.count("file")>0 ){
				std::string file = object.param_vec[ object.param_map["file"] ].as_string();
				std::string full_path = xmldir + file;
				object.param_vec[ object.param_map["file"] ].value = full_path;
			}

			// Check the parameters and store the data
			if( !object.check_params() ){ printf("\n**SceneManager Error: Object check_params\n"); return false; }
			object_map[ name ] = objects.size();
			objects.push_back( object );

		} // end parse object

	} // end loop scene info

	return true;

} // end load xml file


void SceneManager::build_bvh(){

	// Need meshes
	build_meshes();
	mesh_bvh.clear();

	// Create all of the mesh bvhs
	for( int i=0; i<meshes.size(); ++i ){
		std::shared_ptr<BVHNode> bvh = make_tree( meshes[i].get() );
		mesh_bvh.push_back( bvh );
	}

	root_bvh = make_tree( mesh_bvh );

}


std::shared_ptr<BVHNode> SceneManager::get_bvh( bool recompute ){
	if( recompute || mesh_bvh.size()==0 ){ build_bvh(); }
	return root_bvh;
}

