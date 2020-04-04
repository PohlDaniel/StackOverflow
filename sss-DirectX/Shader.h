#ifndef __shaderH__
#define __shaderH__

#include <iostream>
#include <sstream>
#include <vector>
#include "d3d10.h"
#include "d3dx10.h"
#include "D3dx9math.h"

#include "Model.h"

class ShaderFX{

public:
	ShaderFX(std::string effectPath , ID3D10Device* pD3DDevice);
	~ShaderFX();

	void setMatrix(const char* location, const D3DXMATRIXA16& matrix);
	void setFloatScalar(const char* location, const  float value);
	void setFloatVector(const char* location, const  D3DXVECTOR3& value, int size);
	void setFloatVector(const char* location, const  D3DXVECTOR4& value);

	void setFloatScalarInStruct(const char* location, const char* memeberLocation, const  float value);
	void setFloatVectorInStruct(const char* structLocation, const char* memeberLocation, const  D3DXVECTOR3& value, int size);
	void setFloatVectorInStruct(const char* structLocation, const char* memeberLocation, const  D3DXVECTOR4& value);


	void setTexture(const char* location, ID3D10ShaderResourceView* texture);
	
	void loadMaterial(const char* structLocation, const Mesh::Material material);


	void createEffect(bool includeresource = false);
	void loadTechniquePointer(std::string techniqueName);
	void createInputLayout();

	void createInputLayout(const D3D10_INPUT_ELEMENT_DESC *pInputElementDescs, const int numInputLayoutElements);
	void createInputLayoutDescFromVertexShaderSignature();

	ID3D10Effect* getEffect();

	ID3D10Device* m_pD3DDevice;
	ID3D10InputLayout*	m_pInputLayout;
	//effects, techniques and technique description
	ID3D10Effect* m_pEffect;
	ID3D10EffectTechnique* m_pTechnique;
	D3D10_TECHNIQUE_DESC m_techDesc;

private:
	std::string m_effectPath;
};

class Shader{

public:


	struct MatrixBuffer{

		D3DXMATRIXA16 model;
		D3DXMATRIXA16 view;
		D3DXMATRIXA16 projection;
	};

	enum Location { Model, View, Projection };

	Shader(ID3D10Device* pD3DDevice);
	~Shader();

	void loadMatrix(Location location, const D3DXMATRIXA16& matrix);
	void setTexture(ID3D10ShaderResourceView *texture);
	void compileFromFile(std::string vertexShader, std::string pixelShader, ID3D10Device* pD3DDevice);
	bool mapMVPBuffer();
	void bindShader();
	void createConstBuffer();
	void createSamplerState();
	void createInputLayoutDescFromVertexShaderSignature();

	ID3D10InputLayout*	m_pInputLayout;

private:

	ID3D10Blob* m_vertexShaderBuffer;
	ID3D10Blob* m_pixelShaderBuffer;
	ID3D10Device* m_pD3DDevice;
	
	ID3D10VertexShader* m_vertexShader;
	ID3D10PixelShader* m_pixelShader;

	ID3D10Buffer* m_MVPBuffer;
	MatrixBuffer  m_MVP;

	ID3D10SamplerState* m_sampleState;


};

#endif // __shaderH__