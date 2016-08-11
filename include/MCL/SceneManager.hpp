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

#ifndef MCLSCENE_SCENEMANAGER_H
#define MCLSCENE_SCENEMANAGER_H 1

#include "BVH.hpp"
#include "DefaultBuilders.hpp"

namespace mcl {

class SceneManager {

	public:
		SceneManager();

		//
		// Load a configuration file, can be called multiple times for different files.
		// Additional calls will add (or replace, if same name) stuff to the scene.
		// Returns true on success
		//
		bool load( std::string filename );

		//
		// Exports to a scene file. Mesh files are saved to the build directory.
		// Note that some of the original scene file information will be lost
		// (i.e. names)
		// Mode is:
		//	0 = mclscene
		//
		void save( std::string xmlfile, int mode=0 );

		//
		// Computes bounding volume heirarchy (AABB)
		// Type is:
		// 	spatial = object median (slower)
		//	linear = parallel build w/ morton codes
		//
		std::shared_ptr<BVHNode> get_bvh( bool recompute=false, std::string type="linear" );

		//
		// Vectors and (duplicate) maps of scene components
		//
		std::vector< std::shared_ptr<BaseObject> > objects;
		std::vector< std::shared_ptr<BaseMaterial> > materials;
		std::vector< std::shared_ptr<BaseCamera> > cameras;
		std::vector< std::shared_ptr<BaseLight> > lights;
		std::unordered_map< std::string, std::shared_ptr<BaseObject> > objects_map; // name -> object
		std::unordered_map< std::string, std::shared_ptr<BaseMaterial> > materials_map; // name -> material
		std::unordered_map< std::string, std::shared_ptr<BaseCamera> > cameras_map; // name -> camera
		std::unordered_map< std::string, std::shared_ptr<BaseLight> > lights_map; // name -> light

		//
		// Creator Callbacks, invoked on a "load" or "scene::create_<thing>" call.
		//
		BuildObjCallback createObject;
		BuildCamCallback createCamera;
		BuildLightCallback createLight;
		BuildMatCallback createMaterial;

	protected:

		// Root bvh is created by build_bvh
		void build_bvh( int split_mode ); // 0=object median, 1=linear (parallel)
		std::shared_ptr<BVHNode> root_bvh;

}; // end class SceneManager

} // end namespace mcl

#endif
