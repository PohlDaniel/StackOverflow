#ifndef _MODEL_H
#define _MODEL_H

#include <iostream>
#include <ctime>
#include <vector>
#include <array>
#include <fstream>
#include <map>
#include <string>
#include <algorithm>

#include "ModelMatrix.h"

#include "d3d10.h"
#include "d3dx10.h"



#define RANDOM_COLOR 0xFF000000 | ((rand() * 0xFFFFFF) / RAND_MAX)

class Mesh;
class Model2{

	

public:

	Model2();
	~Model2();

	const D3DXVECTOR3 &getCenter() const;
	const D3DXMATRIXA16 &getTransformationMatrix() const;
	const D3DXMATRIXA16 &getInvTransformationMatrix() const;

	void setRotPos(const D3DXVECTOR3 &axis, float degrees, float dx, float dy, float dz);
	void setRotXYZPos(const D3DXVECTOR3 &axisX, float degreesX,
		const D3DXVECTOR3 &axisY, float degreesY,
		const D3DXVECTOR3 &axisZ, float degreesZ,
		float dx, float dy, float dz);

	void rotate(const D3DXVECTOR3 &axis, float degrees);
	void translate(float dx, float dy, float dz);
	void scale(float a, float b, float c);

	//create and release methods
	void createBuffer(ID3D10Device* pD3DDevice);
	
	//size values
	unsigned int m_size, m_numVertices, m_numIndices, m_stride, m_offset, m_numberOfBytes;

	//vertex and index buffers
	ID3D10Buffer* pVertexBuffer;
	ID3D10Buffer* pIndexBuffer;

	bool loadObject(const char* filename);
	bool loadObject(const char* a_filename, D3DXVECTOR3 &rotate, float degree, D3DXVECTOR3& translate, float scale);

	int getNumberOfIndices() const;

	std::string getMltPath();
	std::string getModelDirectory();

	bool m_hasTextureCoords;
	bool m_hasNormals;
	bool m_hasTangents;

	std::vector<Mesh*> mesh;

	

private:
	int addVertex(int hash, float *pVertex, int stride);
	int m_numberOfMeshes;
	int m_numberOfTriangles;
	std::string m_mltPath;
	std::string m_modelDirectory;
	bool m_hasMaterial;
	D3DXVECTOR3 m_center;
	ModelMatrix *m_modelMatrix;

	std::vector <float> m_vertexBuffer;
	std::vector<unsigned int> m_indexBuffer;
	std::map<int, std::vector<int>> m_vertexCache;

	
};

class Mesh{

	friend Model2;
	
public:

	struct Material{

		float ambient[3];
		float diffuse[3];
		float specular[3];
		float shinies;
		std::string diffuseTexPath;
		std::string bumpMapPath;
		std::string displacementMapPath;
		std::string specularMapPath;
	};

	
	Mesh(std::string mltName, int numberTriangles, Model2* model);
	Mesh(int numberTriangles, Model2* model);
	~Mesh();

	void generateNormals();
	void generateTangents();
	bool readMaterial(const char* filename);
	int getNumberOfIndices() const;
	int getNumberOfVertices() const;
	int getNumberOfTriangles() const;
	std::string getMltName();
	Material getMaterial();

	const D3DXVECTOR3 &getColor() const;
	void setMaterial(const D3DXVECTOR3 &ambient, const D3DXVECTOR3 &diffuse, const D3DXVECTOR3 &specular, float shinies);
	void createBuffer(ID3D10Device* pD3DDevice);
	void determineInpuDescD3D10();
	std::vector<D3D10_INPUT_ELEMENT_DESC> getInpuDescD3D10();
	int getNumInputLayoutElements();

	void loadTextureFromMlt(ID3D10Device* pD3DDevice);

	//size values
	unsigned int m_size, m_numVertices, m_numIndices, m_stride, m_offset, m_numberOfBytes;

	//vertex and index buffers
	ID3D10Buffer* pVertexBuffer;
	ID3D10Buffer* pIndexBuffer;

	std::vector<D3D10_INPUT_ELEMENT_DESC> m_vertexInputDescD3D10;

	//textures
	ID3D10ShaderResourceView *m_pDiffuseTex;
	ID3D10ShaderResourceView *m_pNormalMap;
	ID3D10ShaderResourceView *m_pDisplacementMap;
	ID3D10ShaderResourceView *m_pSpecularMap;

private:

	Model2* m_model;

	bool mltCompare(std::string* mltName);
	int addVertex(int hash, const float *pVertex, int numberOfBytes);

	std::vector <float> m_vertexBuffer;
	std::vector<unsigned int> m_indexBuffer;
	std::map<int, std::vector<int>> m_vertexCache;

	bool m_hasTextureCoords, m_hasNormals, m_hasTangents;
	int m_numberTriangles, stride;
	
	D3DXVECTOR3 m_color;
	std::string m_mltName;
	Material m_material;

	
};

#endif 