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

#ifndef MCLSCENE_CAMERA_H
#define MCLSCENE_CAMERA_H 1

#include <Vec.h>
#include <XForm.h>
#include <string>

namespace mcl {

	static trimesh::fxform make_view(Vec3f eye, Vec3f u, Vec3f v, Vec3f w){
		trimesh::fxform r;
		for(size_t i=0; i<3; ++i){
			r[4*i] = u[i];
			r[4*i+1] = v[i];
			r[4*i+2] = w[i];
		}
		r[12] = -eye.dot(u);
		r[13] = -eye.dot(v);
		r[14] = -eye.dot(w);
		return r;
	}


//
//	Camera base class
//
class Camera {
public:
	virtual ~Camera(){}

	// Return eye world location
	virtual Vec3f get_position() = 0;

	// Returns a string containing xml code for saving to a scenefile.
	virtual std::string get_xml( int mode ){ return ""; }

	// Zoom (move forward/backward)
	virtual void zoom( float dz ){}

	// Rotate (input is left-drag x and y amount from screen)
	virtual void rotate( float dx, float dy ){}

	// Pan (input is right-drag x and y amount from screen)
	virtual void pan( float dx, float dy ){}

	// Update the view matrix
	virtual void update_view(){}

	// Update the projection matrix
	virtual void update_proj( float aspect_ratio ){}

	// Used by mcl::Application
	// This data is used by the mclscene OpenGL renderer
	struct AppData {
		trimesh::fxform view, projection;
	} app ;
};


class Trackball : public Camera {
public:
	Vec3f lookat, eye, u, v, w;
	float rotx, roty, panx, pany;
	float fov_deg;
	Vec2f clipping; // clipping plane for proj. matrix

	Trackball( Vec3f eye_, Vec3f lookat_ ) : eye(eye_), lookat(lookat_),
		rotx(0.f), roty(0.f),
		panx(0.f), pany(0.f),
		fov_deg(30.f),
		clipping(0.1f,1000.f) {
		update_basis();
	}

	Vec3f get_position(){ return eye; }

	void update_basis(){
		Vec3f up(0,1,0);
		Vec3f dir = lookat - eye;
		dir.normalize();
		w = dir*-1.f;
		u = up.cross(w);
		v = w.cross(u);
		update_view();
	}

	void zoom( float dz ){
		eye -= w*dz;
	}

	void rotate( float dx, float dy ){
		rotx -= dx;
		roty -= dy;
	}

	void pan( float dx, float dy ){
		panx += dx;
		pany += dy;
	}

	void update_view(){
		using namespace trimesh;

		// Update rotation
		fxform xf = fxform::rot(rotx,v) * fxform::rot(roty,u);
		rotx = 0.f;
		roty = 0.f;
		u = xf * u;
		v = xf * v;
		w = xf * w;

		lookat += v*pany;
		lookat -= u*panx;
		pany = 0.f;
		panx = 0.f;

		float rad = (lookat-eye).norm();
		eye = w*rad + lookat;

		this->app.view = make_view( eye, u, v, w );
	}

	void update_proj( float aspect ){
		this->app.projection = trimesh::fxform::persp( fov_deg, aspect, clipping[0], clipping[1] );
	}
};

} // end namespace mcl

#endif
