#ifndef __shaderH__
#define __shaderH__

#include <iostream>
#include <fstream>
#include <sstream>

#include "Extension.h"
#include "Vector.h"
#include "Model.h"





struct LightSource{
	Vector3f ambient;
	Vector3f diffuse;
	Vector3f specular;
	Vector3f position;
};


class Shader{

public:
	
	Shader(std::string vertex, std::string fragment);
	Shader(Shader* shader);
	virtual ~Shader();

	
	GLuint positionID, texCoordID, normalID, tangentID, bitangentID;
	GLuint m_program;

	void loadSampler(std::string location, int sampler);
	void loadMatrix(std::string location, const Matrix4f matrix);
	void loadMatrix(std::string location, const Matrix4f matrix, int count);
	void loadVector(std::string location,  Vector2f vector, int count);
	void loadVector(std::string location, Vector3f vector, int count);
	void loadVector(std::string location, Vector3f vector);
	void loadVector(std::string location, Vector2f vector);
	void loadFloat2(std::string location, float value[2]);
	void loadFloat(std::string location, float value);
	void loadBool(std::string location, bool value);
	
	virtual void bindAttributes(Mesh *a_mesh, GLuint texture1, GLuint texture2 = NULL);
	virtual void unbindAttributes(Mesh *a_mesh);
	
	

	void loadMaterial(const Mesh::Material material);
	void loadLightSource(LightSource &lightsource, int index);
	void loadLightSources(std::vector<LightSource> lights);
	
	unsigned int normal;
	unsigned int depth;
	unsigned int irradiance;

protected:

	GLuint createProgram(std::string vertex, std::string fragment);
	void getAttributeGetAttribLocation();
	void loadSampler();

	void readTextFile(const char *pszFilename, std::string &buffer);
	GLuint loadShaderProgram(GLenum type, const char *pszFilename);
	GLuint compileShader(GLenum type, const char *pszSource);
	GLuint linkShaders(GLuint vertShader, GLuint fragShader);

	void cleanup();

};

class SkyboxShader : public Shader{

public:

	SkyboxShader(std::string vertex, std::string fragment, GLuint cubemap);
	~SkyboxShader();

	
	
	void bindAttributes();
	void unbindAttributes();

private:

	GLuint m_cubemap;
};

class EnvironmentMap : public Shader{

public:

	EnvironmentMap(std::string vertex, std::string fragment, GLuint cubemap);
	EnvironmentMap(EnvironmentMap* shader);
	~EnvironmentMap();

	

	void bindAttributes(Mesh *a_mesh, GLuint texture);
	void unbindAttributes(Mesh *a_mesh);

private:

	GLuint m_cubemap;
};

class DepthShader : public Shader{

public:

	DepthShader(std::string vertex, std::string fragment);
	~DepthShader();

	void bindAttributes(Mesh *a_mesh, GLuint texture);
	void unbindAttributes(Mesh *a_mesh);

};


class TextureShader : public Shader{

public:

	TextureShader(std::string vertex, std::string fragment);
	~TextureShader();

	void bindAttributes(GLuint texture);
	void unbindAttributes();

	

};


class SubsurfaceShader : public Shader {

public:

	SubsurfaceShader(std::string vertex, std::string fragment);
	~SubsurfaceShader();

	void bindAttributes(GLuint colorTexture, GLuint depthTexture, GLuint strengthTexture);
	void unbindAttributes();
};


class SpecularShader : public Shader {

public:

	SpecularShader(std::string vertex, std::string fragment);
	~SpecularShader();

	void bindAttributes(GLuint colorTexture);
	void unbindAttributes();
};

#endif // __shaderH__