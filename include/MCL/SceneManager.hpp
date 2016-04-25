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

#include "MetaTypes.hpp"
#include "bsphere.h" // in trimesh2
#include "BVH.hpp"

namespace mcl {
///
///	When loading a scene, you can either load from an XML
///	file or specify contents directory (with function calls).
///
class SceneManager {

	public:
		SceneManager() {}
		~SceneManager() {}

		// Computes the world bounding
		trimesh::box3 get_bbox( bool recompute=false );
		trimesh::TriMesh::BSphere get_bsphere( bool recompute=false );

		// Load a configuration file, can be called multiple times for different files.
		// Returns true on success
		bool load( std::string xmlfile );

		// Build all objects as a mesh (which can be indexed by object_map)
		void build_meshes();
		std::vector< std::shared_ptr<trimesh::TriMesh> > meshes;

		// Builds bounding volume heirarchies for each individual mesh
		// (which can be indexed by object_map).
		void build_bvh();
		std::shared_ptr<BVHNode> get_bvh( bool recompute=false );
		std::vector< std::shared_ptr<BVHNode> > mesh_bvh;

		// Vectors of scene components. These are just meta types
		// that describe what was read in the XML file.
		std::vector< CameraComponent > cameras;
		std::vector< ObjectComponent > objects;
		std::vector< LightComponent > lights;
		std::vector< MaterialComponent > materials;

		// Because each component has a unique name, we can
		// use maps to access the data directly.
		// name -> index in the above vectors
		std::unordered_map< std::string, int > camera_map;
		std::unordered_map< std::string, int > object_map;
		std::unordered_map< std::string, int > light_map;
		std::unordered_map< std::string, int > material_map;

	protected:
		// Root bvh is created by build_bvh
		std::shared_ptr<BVHNode> root_bvh;

		// Builds both bounding sphere and bounding box
		void build_boundary();
		trimesh::box3 bbox;
		trimesh::TriMesh::BSphere bsphere;

}; // end class SceneManager

} // end namespace mcl

#endif
