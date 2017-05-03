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


#ifndef MCLSCENE_SCENELOADER_H
#define MCLSCENE_SCENELOADER_H 1

#include "MCL/SceneManager.hpp"
#include "MCL/ShapeFactory.hpp"
#include "../../deps/pugixml/pugixml.hpp"

namespace mcl {


	//
	//	Load an mclscene file, returns true on success
	//
	static inline bool load_mclscene( std::string filename, SceneManager *scene );

	//
	//	Helper functions
	//
	static inline std::shared_ptr<BaseObject> parse_object( std::string type, std::vector<Param> &params );
	static inline std::shared_ptr<Material> parse_material( std::string type, std::vector<Param> &params );
	static inline std::shared_ptr<Camera> parse_camera( std::string type, std::vector<Param> &params );
	static inline std::shared_ptr<Light> parse_light( std::string type, std::vector<Param> &params );
	static inline std::shared_ptr<Material> parse_preset_material( std::string preset );

} // end namespace mcl

//
//	Implementation
//

static inline bool mcl::load_mclscene( std::string filename, SceneManager *scene ){

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
	size_t n_child = children.size();
	for( size_t i=0; i<n_child; ++i ){
		std::string tag = parse::to_lower(children[i].name());
		std::string name = parse::to_lower(children[i].attribute("name").as_string());
		if( name.size() > 0 ){
			if( tag == "material" ){ material_map[name]=num_materials; }
			if( tag == "object" ){ object_map[name]=num_objects; }
		} // end has a name

		// Increment counters
		if( tag == "material" ){ num_materials++; }
		else if( tag == "object" ){ num_objects++; }
		else if( tag == "camera" ){ num_cameras++; }
		else if( tag == "light" ){ num_lights++; }

	} // end create name maps

	// Reserve space for scene components
	scene->objects.reserve(num_objects);
	scene->object_params.reserve(num_objects);
	scene->materials.reserve(num_materials);
	scene->cameras.reserve(num_cameras);
	scene->lights.reserve(num_lights);

	//
	// Now parse scene information and create components
	// Not parallelized to maintain correct file-to-index order
	//
	for( size_t child=0; child<n_child; ++child ){

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
			for( size_t i=0; i<params.size(); ++i ){
				if( parse::to_lower(params[i].tag) == "file" || parse::to_lower(params[i].tag) == "texture" ){
					params[i].value = xmldir + params[i].as_string();
				}
			}
		} // end load parameters

		// Now build
		{
			//	Build Camera
			if( tag == "camera" ){
				std::shared_ptr<Camera> cam = parse_camera( type, params );
				if( cam != NULL ){
					scene->cameras.push_back( cam );
//					scene->camera_params.push_back( params );
				}
			} // end build Camera

			//	Build Light
			else if( tag == "light" ){
				std::shared_ptr<Light> light = parse_light( type, params );
				if( light != NULL ){
					scene->lights.push_back( light );
//					scene->light_params.push_back( params );
				}
			} // end build Light

			//	Build Object
			else if( tag == "object" ){
				std::shared_ptr<BaseObject> obj = parse_object( type, params );
				if( obj != NULL ){
					scene->objects.push_back( obj );
					scene->object_params.push_back( params );

					// See if we can figure out the material
					int mat_param_index = param_index("material",params);

					if( mat_param_index >= 0 ){
						std::string material_name = parse::to_lower( params[mat_param_index].as_string() );
						if( material_map.count(material_name) > 0 ){
							obj->material = material_map[material_name];
						} // is a named material
						else if( material_name == "invisible" ){} // handled by parse object
						else {
							std::shared_ptr<Material> mat = parse_preset_material( material_name );
							if( mat != NULL ){
								int idx = scene->materials.size();
								scene->materials.push_back( mat );
								obj->material = idx;
							}
						} // is a material preset
					} // end check material

				}
			} // end build object

			//	Build Material
			if( tag == "material" ){
				std::shared_ptr<Material> mat = parse_material( type, params );
				if( mat != NULL ){
					scene->materials.push_back( mat );
				}
			} // end build material

		} // end create component

	} // end loop scene info

	//
	//	Success, all done.
	//
	return true;
}


//
//	Default Object Builder
//
static inline std::shared_ptr<mcl::BaseObject> mcl::parse_object( std::string type, std::vector<mcl::Param> &params ){

	type = parse::to_lower(type);

	//
	//	First build the transform and other common params
	//
	int n_params = params.size();
	bool flat_shading = false;
	int subdivide_mesh = 0;
	bool invis_material = false;
	bool wireframe = false;
	int tess = 3;
	trimesh::xform x_form;
	std::string filename = "";
	for( int i=0; i<n_params; ++i ){
		std::string tag = parse::to_lower(params[i].tag);
		if( tag=="translate" ){ x_form = params[i].as_xform() * x_form; }
		else if( tag=="scale" ){ x_form = params[i].as_xform() * x_form; }
		else if( tag=="rotate" ){ x_form = params[i].as_xform() * x_form; }
		else if( tag=="subdivide" || tag=="subdivide_mesh" ){ subdivide_mesh = std::abs(params[i].as_int()); }
		else if( tag=="wireframe" ){ wireframe = params[i].as_bool(); }
		else if( tag=="flat" || tag=="flat_shading" ){ flat_shading = params[i].as_bool(); }
		else if( tag=="tess" ){ tess=params[i].as_int(); }
		else if( tag=="file" || tag=="filename" ){ filename=params[i].as_string(); }
		else if( tag=="material" ){
			if( parse::to_lower(params[i].as_string())=="invisible" ){ invis_material = true; }
		}
	}
	
	std::shared_ptr<BaseObject> new_obj = NULL;


	//
	//	Sphere
	//
	if( type == "sphere" ){

		double radius = 1.0;
		Vec3f center(0,0,0);
		int tess = 3;

		for( int i=0; i<n_params; ++i ){
			if( parse::to_lower(params[i].tag)=="radius" ){ radius=params[i].as_double(); }
			else if( parse::to_lower(params[i].tag)=="center" ){ center=params[i].as_vec3(); }
			else if( parse::to_lower(params[i].tag)=="tess" ){ tess=params[i].as_int(); }
		}

		new_obj = factory::make_sphere( center, radius, tess );

	} // end build sphere


	//
	//	Box
	//
	else if( type == "box" || type == "cube" ){
		new_obj = factory::make_beam( 1, tess );
		flat_shading = true;
	} // end build box


	//
	//	Plane, 2 or more triangles
	//
	else if( type == "plane" ){
		int tess_x = 10;
		int tess_y = 10;
		for( int i=0; i<n_params; ++i ){
			std::string tag = parse::to_lower(params[i].tag);
			if( tag=="width" || tag=="tess_x" ){ tess_x=params[i].as_int(); }
			else if( tag=="length" || tag=="tess_y" ){ tess_y=params[i].as_int(); }
		}
		new_obj = factory::make_plane( tess_x, tess_y );
	} // end build plane


	//
	//	Beam
	//
	else if( type == "beam" ){
		int chunks = 5;
		for( int i=0; i<n_params; ++i ){
			if( parse::to_lower(params[i].tag)=="chunks" ){ chunks=params[i].as_int(); }
		}
		new_obj = factory::make_beam( chunks, tess );
		flat_shading = true;
	} // end build beam


	//
	//	Cylinder
	//
	else if( type == "cylinder" ){
		float radius = 1.f;
		int tess_l=10, tess_c=10;
		for( int i=0; i<n_params; ++i ){
			if( parse::to_lower(params[i].tag)=="tess_l" ){ tess_l=params[i].as_int(); }
			if( parse::to_lower(params[i].tag)=="tess_c" ){ tess_c=params[i].as_int(); }
			else if( parse::to_lower(params[i].tag)=="radius" ){ radius=params[i].as_float(); }
		}
		new_obj = factory::make_cyl( tess_l, tess_c, radius );
	} // end build cylinder


	//
	//	Torus
	//
	else if( type == "torus" ){
		float inner_rad = 0.25f;
		float outer_rad = 1.f; // doesn't do anything?
		for( int i=0; i<n_params; ++i ){
			if( parse::to_lower(params[i].tag)=="inner_radius" ){ inner_rad=params[i].as_float(); }
		}
		new_obj = factory::make_torus( tess, inner_rad, outer_rad );
	}


	//
	//	Triangle Mesh
	//
	else if( type == "trimesh" || type == "trianglemesh" ){
		std::shared_ptr<TriangleMesh> mesh( new TriangleMesh() );
		if( !mesh->load( filename ) ){ printf("\n**TriMesh Error: failed to load file %s\n", filename.c_str()); }
		if( wireframe ){ mesh->need_edges(); }
		new_obj = std::shared_ptr<BaseObject>( mesh );
	} // end build trimesh


	//
	//	Tet Mesh
	//
	else if( type == "tetmesh" ){
		std::shared_ptr<TetMesh> mesh( new TetMesh() );
		if( !mesh->load( filename ) ){ printf("\n**TetMesh Error: failed to load file %s\n", filename.c_str()); }
		if( wireframe ){ mesh->need_edges(); }
		new_obj = std::shared_ptr<BaseObject>( mesh );
	} // end build tet mesh


	//
	//	Apply regular params
	//
	if( new_obj != NULL ){
		new_obj->apply_xform( x_form );
		if( flat_shading ){ new_obj->flags = new_obj->flags | BaseObject::FLAT; }
		if( subdivide_mesh ){ new_obj->flags = new_obj->flags | BaseObject::SUBDIVIDE; }
		if( wireframe ){ new_obj->flags = new_obj->flags | BaseObject::WIREFRAME; }
		if( invis_material ){ new_obj->flags = new_obj->flags | BaseObject::INVISIBLE; }
		return new_obj;
	} else {
		std::cerr << "**Error: I don't know how to create an object of type " << type << std::endl;
	}


	//
	//	Unknown
	//
	return NULL;

} // end parse object


//
//	Default Material Builder
//
static inline std::shared_ptr<mcl::Material> mcl::parse_material( std::string type, std::vector<mcl::Param> &params ){

	type = parse::to_lower(type);

	if( type == "blinnphong" || type == "default" ){

		std::shared_ptr<Material> mat( new Material() );

		// Loop again for a change in params
		for( size_t i=0; i<params.size(); ++i ){

			std::string tag = parse::to_lower(params[i].tag);

			if( tag=="ambient" ){
				params[i].fix_color();
				mat->app.amb=params[i].as_vec3();
			}
			else if( tag=="diffuse" || tag=="color" ){
				params[i].fix_color();
				mat->app.diff=params[i].as_vec3();
			}
			else if( tag=="specular" ){
				params[i].fix_color();
				mat->app.spec=params[i].as_vec3();
			}
			else if( tag=="texture" ){ mat->app.texture = params[i].as_string(); }
			else if( tag=="shininess" || tag=="exponent" ){ mat->app.shini=params[i].as_int(); }
			else if( tag=="red_back" ){
				if( params[i].as_int()>0 ){ mat->flags = mat->flags | Material::RED_BACKFACE; }
			}
		}
		std::shared_ptr<Material> new_mat( mat );
		return new_mat;
	}
	else{
		std::cerr << "**Error: I don't know how to create a material of type " << type << std::endl;
	}

	//
	//	Unknown
	//
	return NULL;

} // end build material


//
//	Default Light Builder
//
static inline std::shared_ptr<mcl::Light> mcl::parse_light( std::string type, std::vector<mcl::Param> &params ){

	type = parse::to_lower(type);
	std::shared_ptr<Light> light( new Light() );

	//
	//	Common light parameters
	//
	for( size_t i=0; i<params.size(); ++i ){
		std::string tag = parse::to_lower(params[i].tag);
		if( tag=="intensity" || tag=="color" ){
			params[i].fix_color();
			light->app.intensity=params[i].as_vec3();
		}
		else if( tag=="direction" ){
			params[i].normalize();
			light->app.direction=params[i].as_vec3();
		}
		else if( tag=="position" ){ light->app.position=params[i].as_vec3(); }
		else if( tag=="falloff" ){ light->app.falloff=params[i].as_vec3(); }
		else if( tag=="angle" ){ light->app.angle=params[i].as_double(); }
	}


	//
	//	OpenGL Light
	//
	if( type == "point" ){
		light->app.type = 0;
		return light;
	}
	else if( type == "directional" ){
		light->app.type = 1;
		light->app.falloff = Vec3f(1,0,0); // no falloff on directional lights
		return light;
	}
	else if( type == "spot" ){
		light->app.type = 2;
		return light;
	}
	else{
		std::cerr << "**Error: I don't know how to create a light of type " << type << std::endl;
		return NULL;
	}

	//
	//	Unknown
	//
	return light;

} // end build light


//
//	Default Camera
//
static inline std::shared_ptr<mcl::Camera> mcl::parse_camera( std::string type, std::vector<mcl::Param> &params ){

	type = parse::to_lower(type);
	Vec3f eye(0,0,1), direction(0,0,-1), lookat(0,0,0);

	//
	//	Common camera parameters
	//
	for( size_t i=0; i<params.size(); ++i ){
		std::string tag = parse::to_lower(params[i].tag);
		if( tag=="eye" || tag=="position" ){ eye=params[i].as_vec3(); }
		else if( tag=="direction" ){
			params[i].normalize();
			direction=params[i].as_vec3();
		}
		else if( tag=="lookat" ){ lookat=params[i].as_vec3(); }
	}


	//
	//	Trackball camera
	//
	if( type == "trackball" ){
		std::shared_ptr<Camera> cam( new Trackball( eye, lookat ) );
		return cam;
	}

	else{
		std::cerr << "**Error: I don't know how to create a camera of type " << type << std::endl;
	}

	//
	//	Unknown
	//
	return NULL;

} // end build camera




//
//	OpenGL Material Presets
//	See: http://devernay.free.fr/cours/opengl/materials.html
//
//	Create a blinn phong material from preset values.
//	Calling this function does NOT add them to the SceneManager data vectors.
//
static inline std::shared_ptr<mcl::Material> mcl::parse_preset_material( std::string preset ){

	//
	//	This used to be multiple functions and these types were global.
	//	I decided to join them into one function, but instead of refactoring I just
	//	kept the original if elses. That's why the logic is a bit odd in this function.
	//
	enum class MaterialPreset {
		Emerald, Jade, Obsidian, Pearl, Ruby, Turquoise, // gems
		Brass, Bronze, Chrome, Copper, Gold, Silver, // metals
		BlackPlastic, CyanPlastic, GreenPlastic, RedPlastic, WhitePlastic, YellowPlastic, // plastic
		BlackRubber, CyanRubber, GreenRubber, RedRubber, WhiteRubber, YellowRubber, // rubber
		Invisible, Cloth, Unknown // Special
	};

	std::shared_ptr<Material> r = NULL;
	MaterialPreset m = MaterialPreset::Unknown;
	preset = parse::to_lower(preset);

	// Gemstones
	if( preset=="emerald"){ m = ( MaterialPreset::Emerald ); }
	else if( preset=="jade"){ m = ( MaterialPreset::Jade ); }
	else if( preset=="obsidian"){ m = ( MaterialPreset::Obsidian ); }
	else if( preset=="pearl"){ m = ( MaterialPreset::Pearl ); }
	else if( preset=="ruby"){ m = ( MaterialPreset::Ruby ); }
	else if( preset=="turquoise"){ m = ( MaterialPreset::Turquoise ); }

	// Metals
	else if( preset=="brass"){ m = ( MaterialPreset::Brass ); }
	else if( preset=="bronze"){ m = ( MaterialPreset::Bronze ); }
	else if( preset=="chrome"){ m = ( MaterialPreset::Chrome ); }
	else if( preset=="copper"){ m = ( MaterialPreset::Copper ); }
	else if( preset=="gold"){ m = ( MaterialPreset::Gold ); }
	else if( preset=="silver"){ m = ( MaterialPreset::Silver ); }

	// Plastics
	else if( preset=="blackplastic"){ m = ( MaterialPreset::BlackPlastic ); }
	else if( preset=="cyanplastic"){ m = ( MaterialPreset::CyanPlastic ); }
	else if( preset=="greenplastic"){ m = ( MaterialPreset::GreenPlastic ); }
	else if( preset=="redplastic"){ m = ( MaterialPreset::RedPlastic ); }
	else if( preset=="whiteplastic"){ m = ( MaterialPreset::WhitePlastic ); }
	else if( preset=="yellowplastic"){ m = ( MaterialPreset::YellowPlastic ); }

	// Rubber
	else if( preset=="blackrubber"){ m = ( MaterialPreset::BlackRubber ); }
	else if( preset=="cyanrubber"){ m = ( MaterialPreset::CyanRubber ); }
	else if( preset=="greenrubber"){ m = ( MaterialPreset::GreenRubber ); }
	else if( preset=="redrubber"){ m = ( MaterialPreset::RedRubber ); }
	else if( preset=="whiterubber"){ m = ( MaterialPreset::WhiteRubber ); }
	else if( preset=="yellowrubber"){ m = ( MaterialPreset::YellowRubber ); }

	//
	//	Create the material
	//

	// Cloth
	else if ( preset=="cloth"){ m = (MaterialPreset::Cloth); }

	switch( m ){

	// Gemstones
	case MaterialPreset::Emerald:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.0215, 0.1745, 0.0215), Vec3f(0.07568, 0.61424, 0.07568), Vec3f(0.633, 0.727811, 0.633), 0.6 ) ); break;
	case MaterialPreset::Jade:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.135, 0.2225, 0.1575), Vec3f(0.54, 0.89, 0.63), Vec3f(0.316228, 0.316228, 0.316228), 0.1 ) ); break;
	case MaterialPreset::Obsidian:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.05375, 0.05, 0.06625), Vec3f(0.18275, 0.17, 0.22525), Vec3f(0.332741, 0.328634, 0.346435), 0.3 ) ); break;
	case MaterialPreset::Pearl:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.25, 0.20725, 0.20725), Vec3f(1.0, 0.829, 0.829), Vec3f(0.296648, 0.296648, 0.296648), 0.088 ) ); break;
	case MaterialPreset::Ruby:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.1745, 0.01175, 0.01175), Vec3f(0.61424, 0.04136, 0.04136), Vec3f(0.727811, 0.626959, 0.626959), 0.6 ) ); break;
	case MaterialPreset::Turquoise:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.1, 0.18725, 0.1745), Vec3f(0.396, 0.74151, 0.69102), Vec3f(0.297254, 0.30829, 0.306678), 0.1 ) ); break;

	// Metals
	case MaterialPreset::Brass:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.329412, 0.223529, 0.027451), Vec3f(0.780392, 0.568627, 0.113725), Vec3f(0.992157, 0.941176, 0.807843), 0.21794872 ) ); break;
	case MaterialPreset::Bronze:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.2125, 0.1275, 0.054), Vec3f(0.714, 0.4284, 0.18144), Vec3f(0.393548, 0.271906, 0.166721), 0.2 ) ); break;
	case MaterialPreset::Chrome:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.25, 0.25, 0.25), Vec3f(0.4, 0.4, 0.4), Vec3f(0.774597, 0.774597, 0.774597), 0.6 ) ); break;
	case MaterialPreset::Copper:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.19125, 0.0735, 0.0225), Vec3f(0.7038, 0.27048, 0.0828), Vec3f(0.256777, 0.137622, 0.086014), 0.6 ) ); break;
	case MaterialPreset::Gold:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.24725, 0.1995, 0.0745), Vec3f(0.75164, 0.60648, 0.22648), Vec3f(0.628281, 0.555802, 0.366065), 0.4 ) ); break;
	case MaterialPreset::Silver:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.19225, 0.19225, 0.19225), Vec3f(0.50754, 0.50754, 0.50754), Vec3f(0.508273, 0.508273, 0.508273), 0.4 ) ); break;

	// Plastics
	case MaterialPreset::BlackPlastic:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.0, 0.0, 0.0), Vec3f(0.01, 0.01, 0.01), Vec3f(0.50, 0.50, 0.50), 0.25 ) ); break;
	case MaterialPreset::CyanPlastic:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.0, 0.1, 0.06), Vec3f(0.0, 0.50980392, 0.50980392), Vec3f(0.50196078, 0.50196078, 0.50196078), 0.25 ) ); break;
	case MaterialPreset::GreenPlastic:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.0, 0.0, 0.0), Vec3f(0.1, 0.35, 0.1), Vec3f(0.45, 0.55, 0.45), 0.25 ) ); break;
	case MaterialPreset::RedPlastic:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.0, 0.0, 0.0), Vec3f(0.5, 0.0, 0.0), Vec3f(0.7, 0.6, 0.6), 0.25 ) ); break;
	case MaterialPreset::WhitePlastic:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.0, 0.0, 0.0), Vec3f(0.55, 0.55, 0.55), Vec3f(0.70, 0.70, 0.70), 0.25 ) ); break;
	case MaterialPreset::YellowPlastic:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.0, 0.0, 0.0), Vec3f(0.5, 0.5, 0.0), Vec3f(0.60, 0.60, 0.50), 0.25 ) ); break;

	// Rubbers
	case MaterialPreset::BlackRubber:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.02, 0.02, 0.02), Vec3f(0.01, 0.01, 0.01), Vec3f(0.4, 0.4, 0.4), 0.078125 ) ); break;
	case MaterialPreset::CyanRubber:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.0, 0.05, 0.05), Vec3f(0.4, 0.5, 0.5), Vec3f(0.04, 0.7, 0.7), 0.078125 ) ); break;
	case MaterialPreset::GreenRubber:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.0, 0.05, 0.0), Vec3f(0.4, 0.5, 0.4), Vec3f(0.04, 0.7, 0.04), 0.078125 ) ); break;
	case MaterialPreset::RedRubber:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.05, 0.0, 0.0), Vec3f(0.5, 0.4, 0.4), Vec3f(0.7, 0.04, 0.04), 0.078125 ) ); break;
	case MaterialPreset::WhiteRubber:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.05, 0.05, 0.05), Vec3f(0.5, 0.5, 0.5), Vec3f(0.7, 0.7, 0.7), 0.078125 ) ); break;
	case MaterialPreset::YellowRubber:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.05, 0.05, 0.0), Vec3f(0.5, 0.5, 0.4), Vec3f(0.7, 0.7, 0.04), 0.078125 ) ); break;
	case MaterialPreset::Cloth:
		r= std::shared_ptr<Material>( new Material( Vec3f(0.25, 0.20725, 0.20725), Vec3f(1.0, 0.829, 0.829), Vec3f(0., 0., 0.), 0.088 ) ); break;

	default: break;

	} // end switch preset

	if( r == NULL ){ std::cerr << "**make_preset_material Error: Unknown material preset" << std::endl; }
	return r;

} // end material preset

#endif

