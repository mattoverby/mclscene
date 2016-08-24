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

#ifndef MCLSCENE_RENDERUTILS_HPP
#define MCLSCENE_RENDERUTILS_HPP 1

#include <png.h>
#include <SFML/Graphics.hpp>

namespace mcl {


//////////////////////
/// Definitions
//////////////////////


namespace Draw {

	// Replacement for gluPerspective
	static inline void perspectiveGL(double fovy,double aspect, double zNear, double zFar);

	// Replacement for gluLookat
	static inline void LookAt(const double p_EyeX, const double p_EyeY, const double p_EyeZ,
		const double p_CenterX, const double p_CenterY, const double p_CenterZ);

	// Draws a sphere the bad way. Best to make this a display list...
	static inline void drawSphere( float x, float y, float z, float r, int dimensions );

	// From: https://stackoverflow.com/questions/1700211/to-dynamically-increment-from-blue-to-red-using-c
	static inline float blend(float a, float b, float alpha){ return (1.f - alpha) * a + alpha * b; }

	// gradient should be 0-1. blended needs to be a 3-element array
	// From https://stackoverflow.com/questions/1700211/to-dynamically-increment-from-blue-to-red-using-c
	static inline void colorBlend( float *blended, float a[3], float b[3], float gradient );

	// Rotate a 3D point p about axis a
	static inline void rotatePoint( float *p, const float *a, const float &angle);

	// Swap pixel locations
	static inline void swap( unsigned char &p1, unsigned char &p2 );

	// Flip storage order of image rows
	static inline void flip_image (int w, int h, unsigned char *pixels);

	// Write an image buffer to a PNG file
	static inline void save_png (const char *filename, int width, int height,
		       unsigned char *pixels, bool has_alpha=false);

}; // end namespace Draw


}; // end namespace mcl


//////////////////////
/// Implementations Below
//////////////////////

// From:
// http://en.sfml-dev.org/forums/index.php?topic=137.0
static inline void mcl::Draw::perspectiveGL(double fovy,double aspect, double zNear, double zFar){
	// Start in projection mode.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	double xmin, xmax, ymin, ymax;
	ymax = zNear * tan(fovy * M_PI / 360.0);
	ymin = -ymax;
	xmin = ymin * aspect;
	xmax = ymax * aspect;
	glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}


// From:
// http://forums.codeguru.com/showthread.php?396078-gluLookAt
static inline void mcl::Draw::LookAt(const double p_EyeX, const double p_EyeY, const double p_EyeZ, const double p_CenterX,
	const double p_CenterY, const double p_CenterZ){
	double l_X = p_EyeX - p_CenterX;
	double l_Y = p_EyeY - p_CenterY;
	double l_Z = p_EyeZ - p_CenterZ;

	if(l_X == l_Y && l_Y == l_Z && l_Z == 0.0f)
		return;

	if(l_X == l_Z && l_Z == 0.0f)
	{
		if (l_Y < 0.0f)
			glRotatef(-90.0f, 1, 0, 0);
		else
			glRotatef(90.0f, 1, 0, 0);
		glTranslatef(-l_X, -l_Y, -l_Z);
		return;
	}
  
	double l_rX = 0.0f;
	double l_rY = 0.0f;
  
	double l_hA = (l_X == 0.0f) ? l_Z : hypot(l_X, l_Z);
	double l_hB;
	if(l_Z == 0.0f)
		l_hB = hypot(l_X, l_Y);
	else
		l_hB = (l_Y == 0.0f) ? l_hA : hypot(l_Y, l_hA);
	  
	l_rX = asin(l_Y / l_hB) * (180 / M_PI);
	l_rY = asin(l_X / l_hA) * (180 / M_PI);

	glRotated(l_rX, 1, 0, 0);
	if(l_Z < 0.0f)
		l_rY += 180.0f;
	else
		l_rY = 360.0f - l_rY;

	glRotated(l_rY, 0, 1, 0);
	glTranslated(-p_EyeX, -p_EyeY, -p_EyeZ);
}



static inline void mcl::Draw::drawSphere( float x, float y, float z, float r, int dimensions ) {

	int lats = dimensions;
	int longs = dimensions;

	// Drawing a sphere from
	// https://stackoverflow.com/questions/10294345/texture-coordinates-for-rendering-a-3d-sphere

	glPushMatrix();
	glTranslatef( x, y, z );

	double dTheta=180/dimensions, dLon=360/dimensions, degToRad=3.141592665885/180 ;
	for(double lat =0; lat <=180; lat+=dTheta)
	{

		glBegin( GL_QUAD_STRIP ) ;
		for(double lon =0 ; lon <=360 ; lon+=dLon)
		{  

			//Vertex 1
			x = r*cos(lon * degToRad) * sin(lat * degToRad) ;
			y = r*sin(lon * degToRad) * sin(lat * degToRad) ;
			z = r*cos(lat * degToRad) ;
			glNormal3d( x, y, z) ;
			glTexCoord2d(lon/360-0.25, lat/180);
			glVertex3d( x, y, z ) ;


			//Vetex 2
			x = r*cos(lon * degToRad) * sin( (lat + dTheta)* degToRad) ;
			y = r*sin(lon * degToRad) * sin((lat + dTheta) * degToRad) ;
			z = r*cos( (lat + dTheta) * degToRad ) ;
			glNormal3d( x, y, z ) ;
			glTexCoord2d(lon/360-0.25, (lat + dTheta-1)/(180)); 
			glVertex3d( x, y, z ) ;


			//Vertex 3
			x = r*cos((lon + dLon) * degToRad) * sin((lat) * degToRad) ;
			y = r*sin((lon + dLon) * degToRad) * sin((lat) * degToRad) ;
			z = r*cos((lat) * degToRad ) ;
			glNormal3d( x, y, z ) ;
			glTexCoord2d((lon + dLon)/(360)-0.25 ,(lat)/180);
			glVertex3d( x, y, z ) ;


			//Vertex 4
			x = r*cos((lon + dLon) * degToRad) * sin((lat + dTheta)* degToRad) ;
			y = r*sin((lon + dLon)* degToRad) * sin((lat + dTheta)* degToRad) ;
			z = r*cos((lat + dTheta)* degToRad ) ;
			glNormal3d( x, y, z ) ;
			glTexCoord2d((lon + dLon)/360-0.25, (lat + dTheta)/(180));
			glVertex3d( x, y, z ) ;


		}
		glEnd();
	}

	glPopMatrix();

} // end draw sphere


static inline void mcl::Draw::colorBlend( float *blended, float a[3], float b[3], float gradient ){
	if( gradient > 1.f ){ gradient = 1.f; }
	if( gradient < 0.f ){ gradient = 0.f; }
	blended[0] = blend( a[0], b[0], gradient );
	blended[1] = blend( a[1], b[1], gradient );
	blended[2] = blend( a[2], b[2], gradient );
}

static inline void mcl::Draw::rotatePoint( float *p, const float *a, const float &angle ){

	float pos[3];
	pos[0] = p[0];
	pos[1] = p[1];
	pos[2] = p[2];

	float axis[3];
	axis[0] = a[0];
	axis[1] = a[1];
	axis[2] = a[2];

	float c = cosf((float)angle);
	float s = sinf((float)angle);

	float rotMat[4][4];

	// Setup the rotation matrix, this matrix is based off of the rotation matrix used in glRotatef.
	rotMat[0][0] = axis[0] * axis[0] * (1 - c) + c;           rotMat[0][1] = axis[0] * axis[1] * (1 - c) - axis[2] * s; rotMat[0][2] = axis[0] * axis[2] * (1 - c) + axis[1] * s; rotMat[0][3] = 0;
	rotMat[1][0] = axis[1] * axis[0] * (1 - c) + axis[2] * s; rotMat[1][1] = axis[1] * axis[1] * (1 - c) + c;           rotMat[1][2] = axis[1] * axis[2] * (1 - c) - axis[0] * s; rotMat[1][3] = 0;
	rotMat[2][0] = axis[0] * axis[2] * (1 - c) - axis[1] * s; rotMat[2][1] = axis[1] * axis[2] * (1 - c) + axis[0] * s; rotMat[2][2] = axis[2] * axis[2] * (1 - c) + c;           rotMat[2][3] = 0;
	rotMat[3][0] = 0;                       rotMat[3][1] = 0;                       rotMat[3][2] = 0;                       rotMat[3][3] = 1;

	// Multiply the rotation matrix with the position vector.
	float tmp[3];
	tmp[0] = rotMat[0][0] * pos[0] + rotMat[0][1] * pos[1] + rotMat[0][2] * pos[2] + rotMat[0][3];
	tmp[1] = rotMat[1][0] * pos[0] + rotMat[1][1] * pos[1] + rotMat[1][2] * pos[2] + rotMat[1][3];
	tmp[2] = rotMat[2][0] * pos[0] + rotMat[2][1] * pos[1] + rotMat[2][2] * pos[2] + rotMat[2][3];
  
	// Update p
	p[0] = tmp[0];
	p[1] = tmp[1];
	p[2] = tmp[2];	
}

// Swap pixel locations
static inline void mcl::Draw::swap( unsigned char &p1, unsigned char &p2 ){
	unsigned char temp = p1;
	p1 = p2;
	p2 = temp;
}

// Flip storage order of image rows
static inline void mcl::Draw::flip_image (int w, int h, unsigned char *pixels) {

    for (int j = 0; j < h/2; j++)
	for (int i = 0; i < w; i++)
	    for (int c = 0; c < 3; c++)
	        swap(pixels[(i+w*j)*3+c], pixels[(i+w*(h-1-j))*3+c]);

}

// Write an image buffer to a PNG file
static inline void mcl::Draw::save_png (const char *filename, int width, int height,
	       unsigned char *pixels, bool has_alpha) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
	printf("Couldn't open file %s for writing.\n", filename);
	return;
    }
    // initialize the PNG structures
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
	                                          NULL, NULL);
    if (!png_ptr) {
	printf("Couldn't create a PNG write structure.\n");
	fclose(file);
	return;
    }
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
	printf("Couldn't create a PNG info structure.\n");
	png_destroy_write_struct(&png_ptr, NULL);
	fclose(file);
	return;
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
	printf("Had a problem writing %s.\n", filename);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(file);
	return;
    }
    png_init_io(png_ptr, file);
    png_set_IHDR(png_ptr, info_ptr, width, height, 8,
	         has_alpha ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB,
	         PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
	         PNG_FILTER_TYPE_DEFAULT);
    // set the pixel data
    int channels = has_alpha ? 4 : 3;
    png_bytep* row_pointers = (png_bytep*) new unsigned char*[height];
    for (int y = 0; y < height; y++)
	row_pointers[y] = (png_bytep) &pixels[y*width*channels];
    png_set_rows(png_ptr, info_ptr, row_pointers);
    // write the file
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    // clean up
    delete[] row_pointers;
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(file);
}

#endif
