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

#ifndef MCLSCENE_RAYINTERSECT_H
#define MCLSCENE_RAYINTERSECT_H 1

#include <memory>
#include "MCL/Projection.hpp"

namespace mcl {

//
//	TODO: variable type (double or float)
//
//	Several common classes and static functions related to ray tracing
//
namespace raycast {

	//
	//	Ray
	//
	template<typename T> class rtRay {
	public:
		rtRay(){ direction=Vec3<T>(0,0,-1); eps=T(1e-5); }
		rtRay( Vec3<T> o, Vec3<T> d, T e=T(1e-5) ){
			origin=o; direction=d; eps=e;
		}
		Vec3<T> origin, direction;
		T eps;
	};
	typedef rtRay<float> Ray; // default type is float

	//
	//	Payload
	//
	template<typename T> class rtPayload {
	public:
		void init( const rtRay<T> &ray ){ t_min=ray.eps; launch_point=ray.origin; }

		rtPayload() : t_min(1e-5), t_max(9999999), closest_dist(9999999), bary(0,0,0) {}
		rtPayload( const rtRay<T> &ray ) : t_max(9999999), closest_dist(9999999), bary(0,0,0) { init(ray); }

		T t_min, t_max;
		Vec3<T> launch_point;
		mutable Vec3<T> bary;
		mutable Vec3<T> n, hit_point;
		mutable int material; // index into SceneManager::materials

		T closest_dist;
		Vec3<T> closest_p;
	};
	typedef rtPayload<float> Payload;


	// Ideal specular reflection
	template<typename T> static Vec3<T> reflect( const Vec3<T> &incident, const Vec3<T> &norm ){
		return ( incident - T(2) * norm * norm.dot( incident ) );
	}

	//
	//	Intersection functions
	//

	// ray -> triangle without early exit
	static inline bool ray_triangle( const Ray *ray, const Vec3f &p0, const Vec3f &p1, const Vec3f &p2,
		const Vec3f &n0, const Vec3f &n1, const Vec3f &n2, Payload *payload );

	// ray -> triangle without smoothed normals
	template<typename T> static inline bool ray_triangle( const rtRay<T> *ray, const Vec3<T> &p0, const Vec3<T> &p1, const Vec3<T> &p2, rtPayload<T> *payload );

	// ray -> axis aligned bounding box
	// Returns true/false only and does not set the payload.
	template<typename T> static inline bool ray_aabb( const rtRay<T> *ray, const Vec3<T> &min, const Vec3<T> &max, const rtPayload<T> *payload );
//	static inline bool ray_aabb_flt( const rtRay<float> *ray, const Vec3<float> &min, const Vec3<float> &max, const rtPayload<float> *payload ){
//		return ray_aabb<float>( ray, min, max, payload );
//	}
//	static inline bool ray_aabb_dbl( const rtRay<double> *ray, const Vec3<double> &min, const Vec3<double> &max, const rtPayload<double> *payload ){
//		return ray_aabb<double>( ray, min, max, payload );
//	}

} // end namespace raycast

//
//	Implementation below
//

// ray -> triangle without early exit
static inline bool mcl::raycast::ray_triangle( const Ray *ray, const Vec3f &p0, const Vec3f &p1, const Vec3f &p2,
	const Vec3f &n0, const Vec3f &n1, const Vec3f &n2, Payload *payload ){

	const Vec3f e0 = p1 - p0;
	const Vec3f e1 = p0 - p2;
	const Vec3f n = e1.cross( e0 );

	const Vec3f e2 = ( 1.0f / n.dot( ray->direction ) ) * ( p0 - ray->origin );
	const Vec3f i  = ray->direction.cross( e2 );

	double beta  = i.dot( e1 );
	double gamma = i.dot( e0 );
	double alpha = 1.f - beta - gamma;

	double t = n.dot( e2 );
	bool hit = ( (t<payload->t_max) & (t>payload->t_min) & (beta>=-ray->eps*0.5f) & (gamma>=-ray->eps*0.5f) & (beta+gamma<=1.f) );

	if( hit ){
		payload->n = alpha*n0 + beta*n1 + gamma*n2;
		payload->t_max = t;
		payload->hit_point = ray->origin + ray->direction*t;
		payload->bary = Vec3f(alpha,beta,gamma);
		return true;
	}

	return false;

} // end  ray -> triangle

// ray -> triangle without smoothed normals
template<typename T> static inline bool mcl::raycast::ray_triangle( const rtRay<T> *ray,
	const Vec3<T> &p0, const Vec3<T> &p1, const Vec3<T> &p2, rtPayload<T> *payload ){

	const Vec3<T> e0 = p1 - p0;
	const Vec3<T> e1 = p0 - p2;
	const Vec3<T> n = e1.cross( e0 );
	const Vec3<T> e2 = ( 1.0 / n.dot( ray->direction ) ) * ( p0 - ray->origin );
	const Vec3<T> i  = ray->direction.cross( e2 );
	T beta  = i.dot( e1 );
	T gamma = i.dot( e0 );
	if( std::abs(beta)<std::numeric_limits<T>::min() ){ beta=T(0); }
	if( std::abs(gamma)<std::numeric_limits<T>::min() ){ gamma=T(0); }
	T alpha = 1.0 - beta - gamma;
	T t = n.dot( e2 );

	bool bary_test = alpha>0.0 && beta>0.0 && gamma>0.0 && (alpha+beta+gamma)<=1.0;
	bool hit = (t<payload->t_max) && (t>payload->t_min);

	if( hit && bary_test ){
		payload->n = n;
		payload->t_max = t;
		payload->hit_point = ray->origin + ray->direction*t;
		payload->bary = Vec3<T>(alpha,beta,gamma);
		return true;
	}

	return false;

} // end  ray -> triangle


// ray -> axis aligned bounding box
template<typename T> static inline bool mcl::raycast::ray_aabb( const rtRay<T> *ray,
	const Vec3<T> &min, const Vec3<T> &max, const rtPayload<T> *payload ){

	T txmin=0.f, txmax=0.f;
	T dirX = 1.f / ray->direction[0];
	if( dirX >= 0.0 ){
		txmin = dirX * ( min[0] - ray->origin[0] );
		txmax = dirX * ( max[0] - ray->origin[0] );
	}
	else{
		txmax = dirX * ( min[0] - ray->origin[0] );
		txmin = dirX * ( max[0] - ray->origin[0] );
	}

	T tymin=0.f, tymax=0.f;
	T dirY = 1.f / ray->direction[1];
	if( ray->direction[1] >= 0.0 ){
		tymin = dirY * ( min[1] - ray->origin[1] );
		tymax = dirY * ( max[1] - ray->origin[1] );
	}
	else{
		tymax = dirY * ( min[1] - ray->origin[1] );
		tymin = dirY * ( max[1] - ray->origin[1] );
	}

	// First check: x/y axis
	if( txmin > tymax || tymin > txmax ){ return false; }

	T tzmin=0.f, tzmax=0.f;
	T dirZ = 1.f / ray->direction[2];
	if( ray->direction[2] >= 0.0 ){
		tzmin = dirZ * ( min[2] - ray->origin[2] );
		tzmax = dirZ * ( max[2] - ray->origin[2] );
	}
	else{
		tzmax = dirZ * ( min[2] - ray->origin[2] );
		tzmin = dirZ * ( max[2] - ray->origin[2] );
	}

	// Second check: z axis
	if( txmin > tzmax || tzmin > txmax ){ return false; }
	if( tymin > tzmax || tzmin > tymax ){ return false; }

	return true;

} // end ray box intersection

} // end namespace mcl

#endif
