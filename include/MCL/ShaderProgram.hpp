//
//	Adapted from class of the same name by r3dux:
//	http://r3dux.org/2015/01/a-simple-c-opengl-shader-loader-improved
//

#ifndef MCLSCENE_SHADER_PROG_HPP
#define MCLSCENE_SHADER_PROG_HPP 1
 
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace mcl {

class ShaderProgram {
public:
	// Note: We MUST have a valid rendering context before generating the programId or we'll segfault!
	ShaderProgram() : initialized(false) {
		programId = glCreateProgram();
		glUseProgram(programId);
	}
 
	~ShaderProgram(){ glDeleteProgram(programId); }
 
	// Method to initialize a shader program from shaders provided as files
	inline void initFromFiles(std::string vertexShaderFilename, std::string fragmentShaderFilename);
 
	// Method to initialize a shader program from shaders provided as strings
	inline void initFromStrings(std::string vertexShaderSource, std::string fragmentShaderSource){
		init(vertexShaderSource, fragmentShaderSource);
	}
 
	// Method to enable the shader program
	inline void enable();
 
	// Method to disable the shader
	inline void disable(){ glUseProgram(0); }
 
	// Method to return the bound location of a named attribute
	inline GLuint attribute(const std::string name);
 
	// Method to returns the bound location of a named uniform
	inline GLuint uniform(const std::string name);
 
private:
	GLuint programId;
	GLuint vertexShaderId;
	GLuint fragmentShaderId;
	bool initialized;

	// Map of attributes and their binding locations
	std::unordered_map<std::string, int> attributes;
 
	// Map of uniforms and their binding locations
	std::unordered_map<std::string, int> uniforms;
  
	// Private method to compile a shader of a given type
	inline GLuint compileShader(std::string shaderSource, GLenum shaderType);
 
	// Private method to compile/attach/link/verify the shaders.
	// Note: Rather than returning a boolean as a success/fail status we'll just consider
	// a failure here to be an unrecoverable error and throw a runtime_error.
	inline void init(std::string vertexShaderSource, std::string fragmentShaderSource);
  
}; // End of class


//
//	Implementation
//

// Private method to compile a shader of a given type
GLuint ShaderProgram::compileShader(std::string shaderSource, GLenum shaderType){

	// Generate a shader id
	// Note: Shader id will be non-zero if successfully created.
	GLuint shaderId = glCreateShader(shaderType);
	if( shaderId == 0 ){ throw std::runtime_error("\n**glCreateShader Error"); }

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
	if( shaderStatus == GL_FALSE ){ throw std::runtime_error("\n**glCompileShader Error"); }

	// If everything went well, return the shader id
	return shaderId;
}

// Private method to compile/attach/link/verify the shaders.
// Note: Rather than returning a boolean as a success/fail status we'll just consider
// a failure here to be an unrecoverable error and throw a runtime_error.
void ShaderProgram::init(std::string vertexShaderSource, std::string fragmentShaderSource)
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
	if (programLinkSuccess != GL_TRUE){ throw std::runtime_error("Problem with init"); }

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


void ShaderProgram::initFromFiles( std::string vertexShaderFilename, std::string fragmentShaderFilename ){

	std::string vert_string, frag_string;

	// Load the vertex shader
	std::ifstream vert_in( vertexShaderFilename, std::ios::in | std::ios::binary );
	if( vert_in ){ vert_string = (std::string((std::istreambuf_iterator<char>(vert_in)), std::istreambuf_iterator<char>())); }
	else{ throw std::runtime_error("**ShaderProgram Error: failed to load \""+vertexShaderFilename+"\"" ); }

	// Load the fragement shader
	std::ifstream frag_in( fragmentShaderFilename, std::ios::in | std::ios::binary );
	if( frag_in ){ frag_string = (std::string((std::istreambuf_iterator<char>(frag_in)), std::istreambuf_iterator<char>())); }
	else{ throw std::runtime_error("**ShaderProgram Error: failed to load \""+fragmentShaderFilename+"\"" ); }

	init( vert_string, frag_string );
}


void ShaderProgram::enable(){

	if (initialized){ glUseProgram(programId); }
	else { throw std::runtime_error("Problem with use"); }
}


GLuint ShaderProgram::attribute(const std::string name){

	// Add the attribute to the map table if it doesn't already exist
	if( attributes.count(name)==0 ){
		attributes[name] = glGetAttribLocation( programId, name.c_str() );
		if( attributes[name] == -1 ){ throw std::runtime_error("\n**ShaderProgram::attribute Error"); }
	}
	return attributes[name];
}


GLuint ShaderProgram::uniform(const std::string name){

	// Add the uniform to the map table if it doesn't already exist
	if( uniforms.count(name)==0 ){ 
		uniforms[name] = glGetUniformLocation( programId, name.c_str() );
		if( uniforms[name] == -1 ){ throw std::runtime_error("\n**ShaderProgram::uniform Error"); }
	}
	return uniforms[name];
}



} // end namespace mcl

#endif

/*

// is a valid OpenGL context (i.e. window) - so you may like to declare the ShaderProgram globally, then
// instantiate it later on when you have an OpenGL context.
ShaderProgram *shaderProgram;
 
// ...later on when we have a OpenGL context...
shaderProgram = new ShaderProgram();
 
// To provide the source code for the vertex and fragment shaders you can either initialize from strings or from files:
shaderProgram->initFromStrings(vertexShaderString, fragmentShaderString);
shaderProgram->initFromFiles(vertexShaderFilename, fragmentShaderFilename);
 
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
