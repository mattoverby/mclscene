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

//#include "MCL/SceneManager.hpp"
#include "MCL/SceneLoader.hpp"
#include "bsphere.h" // for miniball (trimesh2)

using namespace mcl;
using namespace trimesh;

SceneManager::SceneManager() {
//	root_bvh=NULL;
	last_radius=-1.f;
	last_center = Vec3f(0,0,0);
}


SceneManager::~SceneManager(){ clear(); }


void SceneManager::clear(){
	objects.clear();
	cameras.clear();
	lights.clear();
	materials.clear();
	object_params.clear();
}


bool SceneManager::load( std::string filename ){
	return load_mclscene( filename, this );
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
		for( size_t i=0; i<objects.size(); ++i ){
			xml << "\n" << objects[i]->get_xml( mode );
			int mat = objects[i]->material;
			if( mat >= 0 && mat < (int)objects.size() ){
				std::stringstream ss; ss << "mat" << mat;
				// TODO export
			}
		}

		// Loop over materials, let the name be the index
		for( size_t i=0; i<materials.size(); ++i ){
			xml << "\n" << materials[i]->get_xml( mode );
		}

		// Loop over cameras
		for( size_t i=0; i<cameras.size(); ++i ){
			xml << "\n" << cameras[i]->get_xml( mode );
		}

		// Loop over lights
		for( size_t i=0; i<lights.size(); ++i ){
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

/*
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
*/

void SceneManager::make_3pt_lighting( const Vec3f &eye, const Vec3f &center ){

	Vec3f up(0,1,0);
	Vec3f w = eye-center;
	// Since we're setting no falloff, lights can be far away
	float distance = w.norm()*100.f;
	w.normalize();
	Vec3f u = up.cross(w);
	Vec3f v = w.cross(u);

	lights.clear();

	std::vector<Param> params;
	std::shared_ptr<Light> key = parse_light( "spot", params );
	std::shared_ptr<Light> fill = parse_light( "spot", params );
	std::shared_ptr<Light> back = parse_light( "spot", params );
	lights.push_back( key );
	lights.push_back( fill );
	lights.push_back( back );

	float half_d = distance/2.f;
//	float quart_d = distance/4.f;

	// Set positions
	key->app.position = center + w*distance + v*half_d - u*distance;
	fill->app.position = center + w*distance + u*distance;
	back->app.position = center - w*distance + v*distance;

	// Directions (not used for point lights though)
	key->app.direction = center-key->app.position;
	fill->app.direction = center-fill->app.position;
	back->app.direction = center-back->app.position;
	key->app.direction.normalize();
	fill->app.direction.normalize();
	back->app.direction.normalize();

	// Set intensity
	key->app.intensity = Vec3f(.6,.6,.6);
	fill->app.intensity = Vec3f(.3,.3,.3);
	back->app.intensity = Vec3f(.3,.3,.3);

	// Falloff (none)
	key->app.falloff = Vec3f(1.f,0.f,0.f);
	fill->app.falloff = Vec3f(1.f,0.f,0.f);
	back->app.falloff = Vec3f(1.f,0.f,0.f);

} // end make three point lighting


void SceneManager::get_bsphere( Vec3f *center, float *radius, bool recompute ){

	if( last_radius <= 0.f || recompute ){
		trimesh::Miniball<3,float> mb;
		for( size_t i=0; i<objects.size(); ++i ){
			Vec3f min, max;
			objects[i]->get_bounds( min, max );
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



