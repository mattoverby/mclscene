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
#include "MCL/Object.hpp"
#include "MCL/Camera.hpp"
#include "MCL/Material.hpp"
#include "MCL/Light.hpp"


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
		// Clears all scene data.
		//
		void clear();

		//
		// Computes bounding volume heirarchy (AABB). Eventually I will add better heuristics.
		// Type is:
		// 	spatial = object median (slower, better balanced)
		//	linear = parallel build w/ morton codes (probably has an error somewhere)
		//
//		std::shared_ptr<BVHNode> get_bvh( bool recompute=false, std::string type="spatial" );

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
//		void get_vertex_pool( VertexPool &pool, bool dynamic_only, bool recompute=false );

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
		// from the XML file (if any) are stored as vectors.
		// They are listed in order parsed.
		//
		std::vector< std::vector<Param> > object_params;

	protected:

		// Root bvh is created by build_bvh.
		// build_bvh is called by get_bvh.
//		void build_bvh( std::string split_mode );
//		std::shared_ptr<BVHNode> root_bvh;

		// Cached bounding sphere stats:
		float last_radius;
		Vec3f last_center;

		// Cached VertexPool
//		VertexPool vertex_pool;

}; // end class SceneManager

} // end namespace mcl

#endif
