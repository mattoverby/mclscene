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

#ifndef MCLSCENE_DEFAULTBUILDERS_H
#define MCLSCENE_DEFAULTBUILDERS_H 1

#include <functional> // for std::function
#include "TriMeshBuilder.h"
#include "TetMesh.hpp"
#include "TriangleMesh.hpp"
#include "PointCloud.hpp"
#include "Material.hpp"
#include "Light.hpp"
#include "Camera.hpp"
#include "../../deps/pugixml/pugixml.hpp"

namespace mcl {


//
//	The builder types
//
typedef std::function<std::shared_ptr<Camera> ( std::string type, std::vector<Param> &params )> BuildCamCallback;
typedef std::function<std::shared_ptr<BaseObject> ( std::string type, std::vector<Param> &params )> BuildObjCallback;
typedef std::function<std::shared_ptr<Light> ( std::string type, std::vector<Param> &params )> BuildLightCallback;
typedef std::function<std::shared_ptr<Material> ( std::string type, std::vector<Param> &params )> BuildMatCallback;


//
//	Default Object Builder: Everything is a trimesh or tetmesh.
//
static std::shared_ptr<BaseObject> default_build_object( std::string type, std::vector<Param> &params ){

	using namespace trimesh;
	type = parse::to_lower(type);

	//
	//	First build the transform and other common params
	//
	xform x_form;

	for( int i=0; i<params.size(); ++i ){
		std::string tag = parse::to_lower(params[i].tag);
		if( tag=="translate" ){ x_form = params[i].as_xform() * x_form; }
		else if( tag=="scale" ){ x_form = params[i].as_xform() * x_form; }
		else if( tag=="rotate" ){ x_form = params[i].as_xform() * x_form; }

	}
	

	std::shared_ptr<BaseObject> new_obj = NULL;


	//
	//	Sphere
	//
	if( type == "sphere" ){
		new_obj = std::shared_ptr<BaseObject>( new TriangleMesh() );
		double radius = 1.0;
		vec center(0,0,0);
		int tessellation = 1;

		for( int i=0; i<params.size(); ++i ){
			if( parse::to_lower(params[i].tag)=="radius" ){ radius=params[i].as_double(); }
			else if( parse::to_lower(params[i].tag)=="center" ){ center=params[i].as_vec3(); }
			else if( parse::to_lower(params[i].tag)=="tess" ){ tessellation=params[i].as_int(); }
		}

		make_sphere_polar( new_obj->app.mesh, tessellation, tessellation );

		// Now scale it by the radius
		xform s_xf = trimesh::xform::scale(radius,radius,radius);
		apply_xform(new_obj->app.mesh, s_xf);

		// Translate so center is correct
		xform t_xf = trimesh::xform::trans(center[0],center[1],center[2]);
		apply_xform(new_obj->app.mesh, t_xf);

	} // end build sphere


	//
	//	Box
	//
	else if( type == "box" || type == "cube" ){
		new_obj = std::shared_ptr<BaseObject>( new TriangleMesh() );

		//
		//	For some reason the make_cube function is broken???
		//	Just use the make beam with 1 chunk for now.
		//

		int tess = 3;
		int chunks = 1;
		for( int i=0; i<params.size(); ++i ){
			if( parse::to_lower(params[i].tag)=="tess" ){ tess=params[i].as_int(); }
		}
		make_beam( new_obj->app.mesh, tess, chunks );

	} // end build box


	//
	//	Plane, 2 or more triangles
	//
	else if( type == "plane" ){

		new_obj = std::shared_ptr<BaseObject>( new TriangleMesh() );

		int width = 10;
		int length = 10;
		double noise = 0.0;

		for( int i=0; i<params.size(); ++i ){
			if( parse::to_lower(params[i].tag)=="width" ){ width=params[i].as_int(); }
			else if( parse::to_lower(params[i].tag)=="length" ){ length=params[i].as_int(); }
			else if( parse::to_lower(params[i].tag)=="noise" ){ noise=params[i].as_double(); }
		}

		make_sym_plane( new_obj->app.mesh, width, length );
		if( noise > 0.0 ){ trimesh::noisify( new_obj->app.mesh, noise ); }

	} // end build plane


	//
	//	Beam
	//
	else if( type == "beam" ){

		new_obj = std::shared_ptr<BaseObject>( new TriangleMesh() );

		int tess = 3;
		int chunks = 5;

		for( int i=0; i<params.size(); ++i ){
			if( parse::to_lower(params[i].tag)=="tess" ){ tess=params[i].as_int(); }
			else if( parse::to_lower(params[i].tag)=="chunks" ){ chunks=params[i].as_int(); }
		}


		make_beam( new_obj->app.mesh, tess, chunks );

	} // end build beam

	//
	//	Cylinder
	//
	else if( type == "cylinder" ){

		new_obj = std::shared_ptr<BaseObject>( new TriangleMesh() );

		float radius = 1.f;
		int tess_l=10, tess_c=10;

		for( int i=0; i<params.size(); ++i ){
			if( parse::to_lower(params[i].tag)=="tess_l" ){ tess_l=params[i].as_int(); }
			if( parse::to_lower(params[i].tag)=="tess_c" ){ tess_c=params[i].as_int(); }
			else if( parse::to_lower(params[i].tag)=="radius" ){ radius=params[i].as_float(); }
		}

		trimesh::make_ccyl( new_obj->app.mesh, tess_l, tess_c, radius );

	} // end build cylinder



	//
	//	Torus
	//
	else if( type == "torus" ){

		new_obj = std::shared_ptr<BaseObject>( new TriangleMesh() );

		int tess_th=50, tess_ph=20;
		float inner_rad = 0.25f;
		float outer_rad = 1.f; // doesn't do anything?

		for( int i=0; i<params.size(); ++i ){
			if( parse::to_lower(params[i].tag)=="tess_th" ){ tess_th=params[i].as_int(); }
			else if( parse::to_lower(params[i].tag)=="tess_ph" ){ tess_ph=params[i].as_int(); }
			else if( parse::to_lower(params[i].tag)=="inner_radius" ){ inner_rad=params[i].as_float(); }
		}

		trimesh::make_torus( new_obj->app.mesh, tess_th, tess_ph, inner_rad, outer_rad );

	}


	//
	//	Triangle Mesh, 2 or more triangles
	//
	else if( type == "trimesh" || type == "trianglemesh" ){

		std::shared_ptr<TriangleMesh> mesh( new TriangleMesh() );

		std::string filename = "";
		for( int i=0; i<params.size(); ++i ){
			if( parse::to_lower(params[i].tag)=="file" ){ filename=params[i].as_string(); }
		}

		// Try to load the trimesh
		if( filename.size() ){
			if( !mesh->load( filename ) ){ printf("\n**TriMesh Error: failed to load file %s\n", filename.c_str()); }
		}

		new_obj = std::shared_ptr<BaseObject>( mesh );

	} // end build trimesh


	//
	//	Tet Mesh
	//
	else if( type == "tetmesh" ){

		std::shared_ptr<TetMesh> mesh( new TetMesh() );
		std::string filename = "";
		for( int i=0; i<params.size(); ++i ){
			if( parse::to_lower(params[i].tag)=="file" ){ filename=params[i].as_string(); }
		}

		if( filename.size() ){
			if( !mesh->load( filename ) ){ printf("\n**TetMesh Error: failed to load file %s\n", filename.c_str()); }
		}

		new_obj = std::shared_ptr<BaseObject>( mesh );

	} // end build tet mesh


	//
	//	Point Cloud
	//
	else if( type == "pointcloud" ){

		std::shared_ptr<PointCloud> cloud( new PointCloud() );
		std::string filename = "";
		bool fill = false;
		for( int i=0; i<params.size(); ++i ){
			if( parse::to_lower(params[i].tag)=="file" ){ filename=params[i].as_string(); }
			if( parse::to_lower(params[i].tag)=="fill" ){ fill=params[i].as_bool(); }
		}

		if( filename.size() ){
			if( !cloud->load( filename, fill ) ){ printf("\n**PointCloud Error: failed to load file %s\n", filename.c_str()); }
		}

		new_obj = std::shared_ptr<BaseObject>( cloud );

	} // end build particle cloud


	//
	//	Unknown
	//
	else{
		std::cerr << "**Error: I don't know how to create an object of type " << type << std::endl;
	}


	if( new_obj != NULL ){
		// If the new object has a trimesh, update its
		// required information.
		if( new_obj->app.mesh != NULL ){
			new_obj->app.mesh->need_normals();
			new_obj->app.mesh->need_tstrips();
		}
		new_obj->apply_xform( x_form );
		return new_obj;
	}


	//
	//	Unknown
	//
	return NULL;

} // end object builder


//
//	Default Material Builder
//
static std::shared_ptr<Material> default_build_material( std::string type, std::vector<Param> &params ){

	type = parse::to_lower(type);

	if( type == "blinnphong" || type == "default" ){

		std::shared_ptr<Material> mat( new Material() );

		// Loop again for a change in params
		for( int i=0; i<params.size(); ++i ){

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
			else if( tag=="mode" || tag=="mode" ){ mat->app.mode=(unsigned int)params[i].as_int(); }

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
static std::shared_ptr<Light> default_build_light( std::string type, std::vector<Param> &params ){

	type = parse::to_lower(type);
	std::shared_ptr<Light> light( new Light() );

	//
	//	OpenGL Light
	//
	if( type == "point" ){
		light->app.type = 0;
		for( int i=0; i<params.size(); ++i ){
			std::string tag = parse::to_lower(params[i].tag);
			if( tag=="intensity" || tag=="color" ){
				params[i].fix_color();
				light->app.intensity=params[i].as_vec3();
			}
			else if( tag=="position" ){ light->app.position=params[i].as_vec3(); }
			else if( tag=="falloff" ){ light->app.falloff=params[i].as_vec3(); }
		}
		return light;
	}
	else if( type == "directional" ){
		light->app.type = 1;
		light->app.falloff = trimesh::vec(1,0,0); // no falloff on directional lights
		for( int i=0; i<params.size(); ++i ){
			std::string tag = parse::to_lower(params[i].tag);
			if( tag=="intensity" || tag=="color" ){
				params[i].fix_color();
				light->app.intensity=params[i].as_vec3();
			}
			else if( tag=="direction" ){
				params[i].normalize();
				light->app.direction=params[i].as_vec3();
			}
		}
		return light;
	}
	else if( type == "spot" ){
		light->app.type = 2;
		for( int i=0; i<params.size(); ++i ){
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
static std::shared_ptr<Camera> default_build_camera( std::string type, std::vector<Param> &params ){

	type = parse::to_lower(type);

	//
	//	Default (TODO: params)
	//
	if( type == "default" ){
		std::shared_ptr<Camera> cam( new Camera() );
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
static std::shared_ptr<Material> make_preset_material( std::string preset ){// MaterialPreset m ){
	using namespace trimesh;

	enum class MaterialPreset {
		Emerald, Jade, Obsidian, Pearl, Ruby, Turquoise, // gems
		Brass, Bronze, Chrome, Copper, Gold, Silver, // metals
		BlackPlastic, CyanPlastic, GreenPlastic, RedPlastic, WhitePlastic, YellowPlastic, // plastic
		BlackRubber, CyanRubber, GreenRubber, RedRubber, WhiteRubber, YellowRubber, // rubber
		Invisible, Unknown // Special
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

	// Special
	else if( preset=="invisible" ){ m = ( MaterialPreset::Invisible ); }

	//
	//	Create the material
	//

	switch( m ){

	// Gemstones
	case MaterialPreset::Emerald:
		r= std::shared_ptr<Material>( new Material( vec(0.0215, 0.1745, 0.0215), vec(0.07568, 0.61424, 0.07568), vec(0.633, 0.727811, 0.633), 0.6 ) ); break;
	case MaterialPreset::Jade:
		r= std::shared_ptr<Material>( new Material( vec(0.135, 0.2225, 0.1575), vec(0.54, 0.89, 0.63), vec(0.316228, 0.316228, 0.316228), 0.1 ) ); break;
	case MaterialPreset::Obsidian:
		r= std::shared_ptr<Material>( new Material( vec(0.05375, 0.05, 0.06625), vec(0.18275, 0.17, 0.22525), vec(0.332741, 0.328634, 0.346435), 0.3 ) ); break;
	case MaterialPreset::Pearl:
		r= std::shared_ptr<Material>( new Material( vec(0.25, 0.20725, 0.20725), vec(1.0, 0.829, 0.829), vec(0.296648, 0.296648, 0.296648), 0.088 ) ); break;
	case MaterialPreset::Ruby:
		r= std::shared_ptr<Material>( new Material( vec(0.1745, 0.01175, 0.01175), vec(0.61424, 0.04136, 0.04136), vec(0.727811, 0.626959, 0.626959), 0.6 ) ); break;
	case MaterialPreset::Turquoise:
		r= std::shared_ptr<Material>( new Material( vec(0.1, 0.18725, 0.1745), vec(0.396, 0.74151, 0.69102), vec(0.297254, 0.30829, 0.306678), 0.1 ) ); break;

	// Metals
	case MaterialPreset::Brass:
		r= std::shared_ptr<Material>( new Material( vec(0.329412, 0.223529, 0.027451), vec(0.780392, 0.568627, 0.113725), vec(0.992157, 0.941176, 0.807843), 0.21794872 ) ); break;
	case MaterialPreset::Bronze:
		r= std::shared_ptr<Material>( new Material( vec(0.2125, 0.1275, 0.054), vec(0.714, 0.4284, 0.18144), vec(0.393548, 0.271906, 0.166721), 0.2 ) ); break;
	case MaterialPreset::Chrome:
		r= std::shared_ptr<Material>( new Material( vec(0.25, 0.25, 0.25), vec(0.4, 0.4, 0.4), vec(0.774597, 0.774597, 0.774597), 0.6 ) ); break;
	case MaterialPreset::Copper:
		r= std::shared_ptr<Material>( new Material( vec(0.19125, 0.0735, 0.0225), vec(0.7038, 0.27048, 0.0828), vec(0.256777, 0.137622, 0.086014), 0.6 ) ); break;
	case MaterialPreset::Gold:
		r= std::shared_ptr<Material>( new Material( vec(0.24725, 0.1995, 0.0745), vec(0.75164, 0.60648, 0.22648), vec(0.628281, 0.555802, 0.366065), 0.4 ) ); break;
	case MaterialPreset::Silver:
		r= std::shared_ptr<Material>( new Material( vec(0.19225, 0.19225, 0.19225), vec(0.50754, 0.50754, 0.50754), vec(0.508273, 0.508273, 0.508273), 0.4 ) ); break;

	// Plastics
	case MaterialPreset::BlackPlastic:
		r= std::shared_ptr<Material>( new Material( vec(0.0, 0.0, 0.0), vec(0.01, 0.01, 0.01), vec(0.50, 0.50, 0.50), 0.25 ) ); break;
	case MaterialPreset::CyanPlastic:
		r= std::shared_ptr<Material>( new Material( vec(0.0, 0.1, 0.06), vec(0.0, 0.50980392, 0.50980392), vec(0.50196078, 0.50196078, 0.50196078), 0.25 ) ); break;
	case MaterialPreset::GreenPlastic:
		r= std::shared_ptr<Material>( new Material( vec(0.0, 0.0, 0.0), vec(0.1, 0.35, 0.1), vec(0.45, 0.55, 0.45), 0.25 ) ); break;
	case MaterialPreset::RedPlastic:
		r= std::shared_ptr<Material>( new Material( vec(0.0, 0.0, 0.0), vec(0.5, 0.0, 0.0), vec(0.7, 0.6, 0.6), 0.25 ) ); break;
	case MaterialPreset::WhitePlastic:
		r= std::shared_ptr<Material>( new Material( vec(0.0, 0.0, 0.0), vec(0.55, 0.55, 0.55), vec(0.70, 0.70, 0.70), 0.25 ) ); break;
	case MaterialPreset::YellowPlastic:
		r= std::shared_ptr<Material>( new Material( vec(0.0, 0.0, 0.0), vec(0.5, 0.5, 0.0), vec(0.60, 0.60, 0.50), 0.25 ) ); break;

	// Rubbers
	case MaterialPreset::BlackRubber:
		r= std::shared_ptr<Material>( new Material( vec(0.02, 0.02, 0.02), vec(0.01, 0.01, 0.01), vec(0.4, 0.4, 0.4), 0.078125 ) ); break;
	case MaterialPreset::CyanRubber:
		r= std::shared_ptr<Material>( new Material( vec(0.0, 0.05, 0.05), vec(0.4, 0.5, 0.5), vec(0.04, 0.7, 0.7), 0.078125 ) ); break;
	case MaterialPreset::GreenRubber:
		r= std::shared_ptr<Material>( new Material( vec(0.0, 0.05, 0.0), vec(0.4, 0.5, 0.4), vec(0.04, 0.7, 0.04), 0.078125 ) ); break;
	case MaterialPreset::RedRubber:
		r= std::shared_ptr<Material>( new Material( vec(0.05, 0.0, 0.0), vec(0.5, 0.4, 0.4), vec(0.7, 0.04, 0.04), 0.078125 ) ); break;
	case MaterialPreset::WhiteRubber:
		r= std::shared_ptr<Material>( new Material( vec(0.05, 0.05, 0.05), vec(0.5, 0.5, 0.5), vec(0.7, 0.7, 0.7), 0.078125 ) ); break;
	case MaterialPreset::YellowRubber:
		r= std::shared_ptr<Material>( new Material( vec(0.05, 0.05, 0.0), vec(0.5, 0.5, 0.4), vec(0.7, 0.7, 0.04), 0.078125 ) ); break;

	// Special
	case MaterialPreset::Invisible:
		r= std::shared_ptr<Material>( new Material() ); r->app.mode = 2; break;

	default: break;

	} // end switch preset

	if( r != NULL ){ r->app.shini *= 128.f; }
	else{ std::cerr << "**make_preset_material Error: Unknown material preset" << std::endl; }
	return r;

} // end material preset



} // end namespace mcl

#endif
