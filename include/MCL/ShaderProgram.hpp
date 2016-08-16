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
//
//	Adapted from class of the same name by r3dux:
//	http://r3dux.org/2015/01/a-simple-c-opengl-shader-loader-improved
//

#ifndef MCLSCENE_SHADER_PROG_HPP
#define MCLSCENE_SHADER_PROG_HPP 1
 
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
 
class ShaderProgram
{
private:
	// Shader program and individual shader Ids
	GLuint programId;
	GLuint vertexShaderId;
	GLuint fragmentShaderId;

	// Map of attributes and their binding locations
	std::unordered_map<std::string, int> attributeMap;
 
	// Map of uniforms and their binding locations
	std::unordered_map<std::string, int> uniformMap;
 
	// Has this shader program been initialized?
	bool initialized;
 
	// ---------- PRIVATE METHODS ----------
 
	// Private method to compile a shader of a given type
	GLuint compileShader(std::string shaderSource, GLenum shaderType)
	{
		std::string shaderTypeString;
		switch (shaderType)
		{
			case GL_VERTEX_SHADER:
				shaderTypeString = "GL_VERTEX_SHADER";
				break;
			case GL_FRAGMENT_SHADER:
				shaderTypeString = "GL_FRAGMENT_SHADER";
				break;
			case GL_GEOMETRY_SHADER:
				throw std::runtime_error("Geometry shaders are unsupported at this time.");
				break;
			default:
				throw std::runtime_error("Bad shader type enum in compileShader.");
				break;
		}
 
		// Generate a shader id
		// Note: Shader id will be non-zero if successfully created.
		GLuint shaderId = glCreateShader(shaderType);
		if (shaderId == 0)
		{
			// Display the shader log via a runtime_error
			throw std::runtime_error("Problem with compileShader");
		}
 
		// Get the source string as a pointer to an array of characters
		const char *shaderSourceChars = shaderSource.c_str();
 
		// Attach the GLSL source code to the shader
		// Params: GLuint shader, GLsizei count, const GLchar **string, const GLint *length
		// Note: The pointer to an array of source chars will be null terminated, so we don't need to specify the length and can instead use NULL.
		glShaderSource(shaderId, 1, &shaderSourceChars, NULL);
 
		// Compile the shader
		glCompileShader(shaderId);
 
		// Check the compilation status and throw a runtime_error if shader compilation failed
		GLint shaderStatus;
		glGetShaderiv(shaderId, GL_COMPILE_STATUS, &shaderStatus);
		if (shaderStatus == GL_FALSE)
		{
			throw std::runtime_error("Problem with compileShader");
		}
 
		// If everything went well, return the shader id
		return shaderId;
	}
 
	// Private method to compile/attach/link/verify the shaders.
	// Note: Rather than returning a boolean as a success/fail status we'll just consider
	// a failure here to be an unrecoverable error and throw a runtime_error.
	void init(std::string vertexShaderSource, std::string fragmentShaderSource)
	{
		// Compile the shaders and return their id values
		vertexShaderId   = compileShader(vertexShaderSource,   GL_VERTEX_SHADER);
		fragmentShaderId = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
 
		// Attach the compiled shaders to the shader program
		glAttachShader(programId, vertexShaderId);
		glAttachShader(programId, fragmentShaderId);
 
		// Link the shader program - details are placed in the program info log
		glLinkProgram(programId);
 
		// Once the shader program has the shaders attached and linked, the shaders are no longer required.
		// If the linking failed, then we're going to abort anyway so we still detach the shaders.
		glDetachShader(programId, vertexShaderId);
		glDetachShader(programId, fragmentShaderId);
 
		// Check the program link status and throw a runtime_error if program linkage failed.
		GLint programLinkSuccess = GL_FALSE;
		glGetProgramiv(programId, GL_LINK_STATUS, &programLinkSuccess);
		if (programLinkSuccess != GL_TRUE)
		{
			throw std::runtime_error("Problem with init");
		}
 
		// Validate the shader program
		glValidateProgram(programId);
 
		// Check the validation status and throw a runtime_error if program validation failed
		GLint programValidatationStatus;
		glGetProgramiv(programId, GL_VALIDATE_STATUS, &programValidatationStatus);
		if (programValidatationStatus != GL_TRUE)
		{
			throw std::runtime_error("Problem with init");
		}
 
		// Finally, the shader program is initialized
		initialized = true;
	}
 
	// Private method to load the shader source code from a file
	std::string loadShaderFromFile(const std::string filename)
	{
		// Create an input filestream and attempt to open the specified file
		std::ifstream file( filename.c_str() );
 
		// If we couldn't open the file we'll bail out
		if ( !file.good() )
		{
			throw std::runtime_error("Failed to open file: " + filename);
		}
 
		// Otherwise, create a string stream...
		std::stringstream stream;
 
		// ...and dump the contents of the file into it.
		stream << file.rdbuf();
 
		// Now that we've read the file we can close it
		file.close();
 
		// Finally, convert the stringstream into a string and return it
		return stream.str();
	}
  
public:
	// Constructor
	ShaderProgram()
	{
		// We start in a non-initialized state - calling initFromFiles() or initFromStrings() will
		// initialise us.
		initialized = false;
 
		// Generate a unique Id / handle for the shader program
		// Note: We MUST have a valid rendering context before generating the programId or we'll segfault!
		programId = glCreateProgram();
		glUseProgram(programId);
	}
 
	// Destructor
	~ShaderProgram()
	{
		// Delete the shader program from the graphics card memory to
		// free all the resources it's been using
		glDeleteProgram(programId);
	}
 
	// Method to initialise a shader program from shaders provided as files
	void initFromFiles(std::string vertexShaderFilename, std::string fragmentShaderFilename)
	{
		// Get the shader file contents as strings
		std::string vertexShaderSource   = loadShaderFromFile(vertexShaderFilename);
		std::string fragmentShaderSource = loadShaderFromFile(fragmentShaderFilename);
 
		init(vertexShaderSource, fragmentShaderSource);
	}
 
	// Method to initialise a shader program from shaders provided as strings
	void initFromStrings(std::string vertexShaderSource, std::string fragmentShaderSource)
	{
		init(vertexShaderSource, fragmentShaderSource);
	}
 
	// Method to enable the shader program - we'll suggest this for inlining
	inline void enable()
	{
		// Santity check that we're initialized and ready to go...
		if (initialized)
		{
			glUseProgram(programId);
		}
		else
		{
			throw std::runtime_error("Problem with use");
		}
	}
 
	// Method to disable the shader - we'll also suggest this for inlining
	inline void disable()
	{
		glUseProgram(0);
	}
 
	// Method to return the bound location of a named attribute, or -1 if the attribute was not found
	GLuint attribute(const std::string attributeName)
	{
		// You could do this method with the single line:
		//
		//		return attributeMap[attribute];
		//
		// BUT, if you did, and you asked it for a named attribute which didn't exist
		// like: attributeMap["FakeAttrib"] then the method would return an invalid
		// value which will likely cause the program to segfault. So we're making sure
		// the attribute asked for exists, and if it doesn't then we alert the user & bail.
 
		// Create an iterator to look through our attribute map (only create iterator on first run -
		// reuse it for all further calls).
		static std::unordered_map<std::string, int>::const_iterator attributeIter;
 
		// Try to find the named attribute
		attributeIter = attributeMap.find(attributeName);
 
		// Not found? Bail.
		if ( attributeIter == attributeMap.end() )
		{
			throw std::runtime_error("Problem with attribute");
		}
 
		// Otherwise return the attribute location from the attribute map
		return attributeMap[attributeName];
	}
 
	// Method to returns the bound location of a named uniform
	GLuint uniform(const std::string uniformName)
	{
		// Note: You could do this method with the single line:
		//
		// 		return uniformLocList[uniform];
		//
		// But we're not doing that. Explanation in the attribute() method above.
 
		// Create an iterator to look through our uniform map (only create iterator on first run -
		// reuse it for all further calls).
		static std::unordered_map<std::string, int>::const_iterator uniformIter;
 
		// Try to find the named uniform
		uniformIter = uniformMap.find(uniformName);
 
		// Found it? Great - pass it back! Didn't find it? Alert user and halt.
		if ( uniformIter == uniformMap.end() )
		{
			throw std::runtime_error("Problem with uniform");
		}
 
		// Otherwise return the attribute location from the uniform map
		return uniformMap[uniformName];
	}
 
	// Method to add an attribute to the shader and return the bound location
	int addAttribute(const std::string attributeName)
	{
		// Add the attribute location value for the attributeName key
		attributeMap[attributeName] = glGetAttribLocation( programId, attributeName.c_str() );
 
		// Check to ensure that the shader contains an attribute with this name
		if (attributeMap[attributeName] == -1)
		{
			throw std::runtime_error("Problem with addAttribute");
		}
 
		// Return the attribute location
		return attributeMap[attributeName];
	}
 
	// Method to add a uniform to the shader and return the bound location
	int addUniform(const std::string uniformName)
	{
		// Add the uniform location value for the uniformName key
		uniformMap[uniformName] = glGetUniformLocation( programId, uniformName.c_str() );
 
		// Check to ensure that the shader contains a uniform with this name
		if (uniformMap[uniformName] == -1)
		{
			throw std::runtime_error("Problem with addUniform");
		}
 
		// Return the uniform location
		return uniformMap[uniformName];
	}
 
}; // End of class

#endif

/*

// is a valid OpenGL context (i.e. window) - so you may like to declare the ShaderProgram globally, then
// instantiate it later on when you have an OpenGL context.
ShaderProgram *shaderProgram;
 
// ...later on when we have a OpenGL context...
shaderProgram = new ShaderProgram();
 
// To provide the source code for the vertex and fragment shaders you can either initialise from strings or from files:
shaderProgram->initFromStrings(vertexShaderString, fragmentShaderString);
shaderProgram->initFromFiles(vertexShaderFilename, fragmentShaderFilename);
 
// Add attributes to suit
shaderProgram->addAttribute("vertexLocation");
shaderProgram->addAttribute("vertexColour");
 
// Add uniforms to suit
shaderProgram->addUniform("mvpMatrix");
 
// Enable attributes
// 1.) Create and bind to a vertex array object (VAO)...
// 2.) Create and bind to a vertex buffer object (VBO) and specify your vertex attrib pointers
// 3.) Provide geometry data then unbind from the VBO
// --- Now you can enable vertex attributes with, for example:
             glEnableVertexAttribArray( shaderProgram->attribute("vertexLocation") );
             glEnableVertexAttribArray( shaderProgram->attribute("vertexColour") );
// 5.) Unbind from VAO
 
// Finally, to draw the geometry using the shader program you can use code similar to this:
// Note: This code assumes you're using GLM for your matrices, modify as appropriate if not. Also
// note that to use glm::value_ptr you must "#include <glm/gtc/type_ptr.hpp>"
shaderProgram->use();
             glUniformMatrix4fv(shaderProgram->uniform("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix) );
             <YOUR-DRAW-CALL-HERE> i.e. glDrawArrays(GL_TRIANGLES, 0, 100) etc.
shaderProgram->disable();

*/
