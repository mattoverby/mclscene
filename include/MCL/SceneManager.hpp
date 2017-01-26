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

#ifndef MCLSCENE_SCENEMANAGER_H
#define MCLSCENE_SCENEMANAGER_H 1

#include "BVH.hpp"
#include "DefaultBuilders.hpp"

//
//	Loading a scene with SceneManager:
//	1) Replace the build callbacks with your own methods, if needed.
//		See include/MCL/DefaultBuilders.hpp for details.
//	2) Load an XML file with SceneManager::load (see conf/ for examples).
//		OR you can use the make_object, make_light, make_camera, and make_material
//		functions. They return shared pointers of the scene component
//		and you can set the parameters directly.
//
namespace mcl {

class SceneManager {

	public:
		SceneManager();
		~SceneManager();

		//
		// Load a configuration file, can be called multiple times for different files.
		// Additional calls will add stuff to the scene.
		// Returns true on success.
		//
		bool load( std::string filename );

		//
		// Exports to a scene file. Mesh files are saved to the build directory.
		// Note that some of the original scene file information will be lost,
		// E.g. the original mesh after an xform is applied.
		// Mode is:
		//	0 = mclscene
		//
		void save( std::string xmlfile, int mode=0 );

		//
		// Computes bounding volume heirarchy (AABB). Eventually I will add better heuristics.
		// Type is:
		// 	spatial = object median (slower, better balanced)
		//	linear = parallel build w/ morton codes (probably has an error somewhere)
		//
		std::shared_ptr<BVHNode> get_bvh( bool recompute=false, std::string type="spatial" );

		//
		// Computes an exact bounding sphere, excluding cameras and lights.
		// Calls each object's bounds function, so may be costly for dynamic scenes.
		//
		void get_bsphere( Vec3f *center, float *radius, bool recompute=false );

		//
		// Generates a Vertex Pool (pointers to mesh data) and caches the result,
		// subsequent calls will return this cached pool.
		// If dynamic_only is true, only objects marked with the "DYNAMIC" flag
		// will be added to the pool.
		//
		void get_vertex_pool( VertexPool &pool, bool dynamic_only, bool recompute=false );

		//
		// For an initial eye position and scene center, creates a 3pt lighting rig.
		// Camera is assumed to be pointed at center (basically a lookat).
		// The distance between eye and center is used to determined the light locations.
		//
		void make_3pt_lighting( const Vec3f &eye, const Vec3f &center );

		//
		// Scene components.
		// The components are indexed in listed/added order.
		//
		std::vector< std::shared_ptr<BaseObject> > objects;
		std::vector< std::shared_ptr<Camera> > cameras;
		std::vector< std::shared_ptr<Light> > lights;
		std::vector< std::shared_ptr<Material> > materials;

		//
		// In addition to creating the components, the original parameters parsed
		// from the XML file (if any) are stored as vectors. They are listed in order parsed.
		//
		std::vector< std::vector<Param> > object_params;
		std::vector< std::vector<Param> > camera_params;
		std::vector< std::vector<Param> > light_params;
		std::vector< std::vector<Param> > material_params;

		//
		// Functions that create a component and pushes it to the
		// vectors (above). Calls the builder callbacks (below).
		// Uses dynamic_pointer_cast, so use at your own risk.
		//
		template<typename T=BaseObject> std::shared_ptr<T> make_object( std::string type );
		template<typename T=Light> std::shared_ptr<T> make_light( std::string type );
		template<typename T=Camera> std::shared_ptr<T> make_camera( std::string type );
		template<typename T=Material> std::shared_ptr<T> make_material( std::string type );

		//
		// Creator Callbacks, invoked on a "load" or "make_<thing>" call.
		// These can be changed to a custom function.
		// For more details, see include/MCL/DefaultBuilders.hpp
		//
		BuildObjCallback createObject;
		BuildCamCallback createCamera;
		BuildLightCallback createLight;
		BuildMatCallback createMaterial;

	protected:

		// Root bvh is created by build_bvh.
		// build_bvh is called by get_bvh.
		void build_bvh( std::string split_mode );
		std::shared_ptr<BVHNode> root_bvh;

		// Cached bounding sphere stats:
		float last_radius;
		Vec3f last_center;

		// Cached VertexPool
		VertexPool vertex_pool;

}; // end class SceneManager

//
//	Template functions
//

template<typename T> std::shared_ptr<T> SceneManager::make_object( std::string type ){
	std::vector<Param> params;
	std::shared_ptr<BaseObject> newObject = createObject( type, params );
	if( !newObject ){ return NULL; }
	objects.push_back( newObject ); object_params.push_back( params );
	std::shared_ptr<T> casted_ptr = std::dynamic_pointer_cast<T>( newObject );
	if( !casted_ptr ){ return NULL; } return casted_ptr;
} // end make object

template<typename T> std::shared_ptr<T> SceneManager::make_camera( std::string type ){
	std::vector<Param> params;
	std::shared_ptr<Camera> newCam = createCamera( type, params );
	if( !newCam ){ return NULL; }
	cameras.push_back( newCam ); camera_params.push_back( params );
	std::shared_ptr<T> casted_ptr = std::dynamic_pointer_cast<T>( newCam );
	if( !casted_ptr ){ return NULL; } return casted_ptr;
} // end make camera

template<typename T> std::shared_ptr<T> SceneManager::make_light( std::string type ){
	std::vector<Param> params;
	std::shared_ptr<Light> newLight = createLight( type, params );
	if( !newLight ){ return NULL; }
	lights.push_back( newLight ); light_params.push_back( params );
	std::shared_ptr<T> casted_ptr = std::dynamic_pointer_cast<T>( newLight );
	if( !casted_ptr ){ return NULL; } return casted_ptr;
} // end make light

template<typename T> std::shared_ptr<T> SceneManager::make_material( std::string type ){
	std::vector<Param> params;
	std::shared_ptr<Material> newMat = createMaterial( type, params );
	if( !newMat ){ return NULL; }
	materials.push_back( newMat ); material_params.push_back( params );
	std::shared_ptr<T> casted_ptr = std::dynamic_pointer_cast<T>( newMat );
	if( !casted_ptr ){ return NULL; } return casted_ptr;
} // end make material

} // end namespace mcl

#endif
