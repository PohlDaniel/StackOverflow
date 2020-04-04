#include <atlbase.h>
#include <atlconv.h>
#include "Shader.h"


class ID3D10IncludeResource : public ID3D10Include {
public:
	STDMETHOD(Open)(THIS_ D3D_INCLUDE_TYPE, LPCSTR pFileName, LPCVOID, LPCVOID *ppData, UINT *pBytes) {

		std::string file = pFileName;
		std::string directory = "sss/";


		FILE *f = fopen((directory + file).c_str(), "rb");
		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);

		char *buf = (char *)malloc(fsize + 1);
		fread(buf, fsize, 1, f);
		fclose(f);
		*pBytes = fsize;
		*ppData = buf;

		return S_OK;
	}

	STDMETHOD(Close)(THIS_ LPCVOID) {
		return S_OK;
	}
};

ShaderFX::ShaderFX(std::string effectPath, ID3D10Device* pD3DDevice){

	m_pD3DDevice = pD3DDevice;	
	m_effectPath = effectPath;
}

ShaderFX::~ShaderFX(){}


void ShaderFX::createEffect(bool includeresource){

	if (includeresource) {

		std::vector<D3D10_SHADER_MACRO> defines;
		D3D10_SHADER_MACRO null = { NULL, NULL };
		defines.push_back(null);

		ID3D10IncludeResource includeResource;

		if (FAILED(D3DX10CreateEffectFromFile(CA2W(m_effectPath.c_str()), &defines.front(), &includeResource, "fx_4_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, m_pD3DDevice, NULL, NULL, &m_pEffect, NULL, NULL))) {

			std::cout << "Could not create effect with IncludeResource!" << std::endl;
		}
	}else {

		if (FAILED(D3DX10CreateEffectFromFile(CA2W(m_effectPath.c_str()), NULL, NULL, "fx_4_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, m_pD3DDevice, NULL, NULL, &m_pEffect, NULL, NULL))) {

			std::cout << "Could not create effect!" << std::endl;
		}
	}
}

void ShaderFX::loadTechniquePointer(std::string techniqueName){

	m_pTechnique = m_pEffect->GetTechniqueByName(techniqueName.c_str());
	if (m_pTechnique == NULL){
		std::cout << "Could not find technique with name: " << techniqueName << std::endl;
	}
}

void ShaderFX::createInputLayout(){

	const D3D10_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = sizeof(layout) / sizeof(D3D10_INPUT_ELEMENT_DESC);
	D3D10_PASS_DESC PassDesc;
	m_pEffect->GetTechniqueByName("render")->GetPassByIndex(0)->GetDesc(&PassDesc);
	m_pD3DDevice->CreateInputLayout(layout, numElements, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &m_pInputLayout);

}

void ShaderFX::createInputLayout(const D3D10_INPUT_ELEMENT_DESC *pInputElementDescs, const int numInputLayoutElements){

	//create input layout
	D3D10_PASS_DESC PassDesc;
	m_pTechnique->GetPassByIndex(0)->GetDesc(&PassDesc);

	if (FAILED(m_pD3DDevice->CreateInputLayout(&pInputElementDescs[0], numInputLayoutElements, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &m_pInputLayout)))
		std::cout << "Could not create Input Layout!" << std::endl;

	//get technique description
	m_pTechnique->GetDesc(&m_techDesc);
}

void ShaderFX::createInputLayoutDescFromVertexShaderSignature(){

	ID3D10Blob* errorMessage = 0;
	ID3D10Blob* vertexShaderBuffer = 0;
	
	if (FAILED(D3DX10CompileFromFile(CA2W(m_effectPath.c_str()), NULL, NULL, "VS", "vs_4_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, 0, &vertexShaderBuffer, &errorMessage, 0))){

		if (errorMessage != NULL){

			std::cout << "Could not compile vertex shader: " << std::endl;
			errorMessage->Release();
		}
		return;
	}

	// Reflect shader info
	ID3D10ShaderReflection* pVertexShaderReflection = NULL;
	if (FAILED(D3D10ReflectShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &pVertexShaderReflection))){
		
		std::cout << "Could not reflect vertexshader " << std::endl;
	}

	// Get shader info
	D3D10_SHADER_DESC shaderDesc;
	pVertexShaderReflection->GetDesc(&shaderDesc);

	// Read input layout description from shader info
	std::vector<D3D10_INPUT_ELEMENT_DESC> inputLayoutDesc;
	for (int i = 0; i< shaderDesc.InputParameters; i++){

		D3D10_SIGNATURE_PARAMETER_DESC paramDesc;
		pVertexShaderReflection->GetInputParameterDesc(i, &paramDesc);

		// fill out input element desc
		D3D10_INPUT_ELEMENT_DESC elementDesc;
		elementDesc.SemanticName = paramDesc.SemanticName;
		elementDesc.SemanticIndex = paramDesc.SemanticIndex;
		elementDesc.InputSlot = 0;
		elementDesc.AlignedByteOffset = D3D10_APPEND_ALIGNED_ELEMENT;
		elementDesc.InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;

		// determine DXGI format
		if (paramDesc.Mask == 1){

			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;

		}else if (paramDesc.Mask <= 3){

			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;

		}else if (paramDesc.Mask <= 7){
			
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;

		}else if (paramDesc.Mask <= 15){
			
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}

		//save element desc
		inputLayoutDesc.push_back(elementDesc);
	}

	D3D10_PASS_DESC PassDesc;
	m_pTechnique->GetPassByIndex(0)->GetDesc(&PassDesc);
	// Try to create Input Layout
	if (FAILED(m_pD3DDevice->CreateInputLayout(&inputLayoutDesc[0], inputLayoutDesc.size(), PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &m_pInputLayout))){

		std::cout << "Could not create Input Layout!" << std::endl;
	}
	//get technique description
	m_pTechnique->GetDesc(&m_techDesc);

	return;
}

ID3D10Effect* ShaderFX::getEffect(){

	return m_pEffect;
}

void ShaderFX::setFloatScalar(const char* location, const  float value) {

	m_pEffect->GetVariableByName(location)->AsScalar()->SetFloat(value);
}

void ShaderFX::setFloatVector(const char* location, const  D3DXVECTOR4& value){

	m_pEffect->GetVariableByName(location)->AsVector()->SetFloatVector((float*)&value);
}

void ShaderFX::setFloatVector(const char* location, const  D3DXVECTOR3& value, int size) {

	m_pEffect->GetVariableByName(location)->AsVector()->SetRawValue((float*)&value, 0, size);
}

void ShaderFX::setFloatScalarInStruct(const char* structLocation, const char* memeberLocation, const  float value){

	m_pEffect->GetVariableByName(structLocation)->GetMemberByName(memeberLocation)->AsScalar()->SetFloat(value);
}

void ShaderFX::setFloatVectorInStruct(const char* structLocation, const char* memeberLocation, const  D3DXVECTOR4& value){

	m_pEffect->GetVariableByName(structLocation)->GetMemberByName(memeberLocation)->AsVector()->SetFloatVector((float*)&value);
}

void ShaderFX::setFloatVectorInStruct(const char* structLocation, const char* memeberLocation, const  D3DXVECTOR3& value, int size){

	m_pEffect->GetVariableByName(structLocation)->GetMemberByName(memeberLocation)->AsVector()->SetRawValue((float*)&value, 0, size);
}


void ShaderFX::setMatrix(const char* location, const D3DXMATRIXA16& matrix){

	m_pEffect->GetVariableByName(location)->AsMatrix()->SetMatrix((float*)&matrix);

}

void ShaderFX::setTexture(const char* location, ID3D10ShaderResourceView* texture){

	m_pEffect->GetVariableByName(location)->AsShaderResource()->SetResource(texture);
}


void ShaderFX::loadMaterial(const char* structLocation, const Mesh::Material material){

	
	
	m_pEffect->GetVariableByName(structLocation)->GetMemberByName("ambient")->AsVector()->SetFloatVectorArray((float*)material.ambient, 0, 3);
	m_pEffect->GetVariableByName(structLocation)->GetMemberByName("diffuse")->AsVector()->SetFloatVectorArray((float*)material.diffuse, 0, 3);
	m_pEffect->GetVariableByName(structLocation)->GetMemberByName("specular")->AsVector()->SetFloatVectorArray((float*)material.specular, 0, 3);
	m_pEffect->GetVariableByName(structLocation)->GetMemberByName("shininess")->AsScalar()->SetFloat(material.shinies);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Shader::Shader(ID3D10Device* pD3DDevice){

	m_pD3DDevice = pD3DDevice; 

	//m_tmp = 
}

Shader::~Shader(){}

void Shader::compileFromFile(std::string vertexShader, std::string pixelShader, ID3D10Device* pD3DDevice){

	
	ID3D10Blob* errorMessage;

	// Initialize the pointers this function will use to null.
	errorMessage = 0;
	m_vertexShaderBuffer = 0;
	m_pixelShaderBuffer = 0;

	if (FAILED(D3DX10CompileFromFile(CA2W(vertexShader.c_str()), NULL, NULL, "VS", "vs_4_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, 0, &m_vertexShaderBuffer, &errorMessage, 0))){

		if (errorMessage != NULL){

			std::cout << "Could not compile vertex shader: " << std::endl;
			errorMessage->Release();
		}
		return;
	}

	pD3DDevice->CreateVertexShader(m_vertexShaderBuffer->GetBufferPointer(), m_vertexShaderBuffer->GetBufferSize(), &m_vertexShader);

	if (FAILED(D3DX10CompileFromFile(CA2W(pixelShader.c_str()), NULL, NULL, "PS", "ps_4_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, 0, &m_pixelShaderBuffer, &errorMessage, 0))){

		if (errorMessage != NULL){

			std::cout << "Could not compile pixel shader: " << std::endl;
			errorMessage->Release();
		}
		return;

	}

	pD3DDevice->CreatePixelShader(m_pixelShaderBuffer->GetBufferPointer(), m_pixelShaderBuffer->GetBufferSize(), &m_pixelShader);

}

void Shader::createInputLayoutDescFromVertexShaderSignature(){
	
	// Reflect shader info
	ID3D10ShaderReflection* pVertexShaderReflection = NULL;
	if (FAILED(D3D10ReflectShader(m_vertexShaderBuffer->GetBufferPointer(), m_vertexShaderBuffer->GetBufferSize(), &pVertexShaderReflection))){
		
		std::cout << "Could not reflect vertexshader " << std::endl;
	}

	// Get shader info
	D3D10_SHADER_DESC shaderDesc;
	pVertexShaderReflection->GetDesc(&shaderDesc);

	// Read input layout description from shader info
	std::vector<D3D10_INPUT_ELEMENT_DESC> inputLayoutDesc;
	for (int i = 0; i< shaderDesc.InputParameters; i++){

		D3D10_SIGNATURE_PARAMETER_DESC paramDesc;
		pVertexShaderReflection->GetInputParameterDesc(i, &paramDesc);

		// fill out input element desc
		D3D10_INPUT_ELEMENT_DESC elementDesc;
		elementDesc.SemanticName = paramDesc.SemanticName;
		elementDesc.SemanticIndex = paramDesc.SemanticIndex;
		elementDesc.InputSlot = 0;
		elementDesc.AlignedByteOffset = D3D10_APPEND_ALIGNED_ELEMENT;
		elementDesc.InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;

		// determine DXGI format
		if (paramDesc.Mask == 1){

			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;

		}else if (paramDesc.Mask <= 3){

			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;

		}else if (paramDesc.Mask <= 7){

			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;

		}else if (paramDesc.Mask <= 15){

			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}

		//save element desc
		inputLayoutDesc.push_back(elementDesc);
	}

	// Try to create Input Layout
	if (FAILED(m_pD3DDevice->CreateInputLayout(&inputLayoutDesc[0], inputLayoutDesc.size(), m_vertexShaderBuffer->GetBufferPointer(), m_vertexShaderBuffer->GetBufferSize(), &m_pInputLayout))){
		
		std::cout << "Could not create Input Layout!" << std::endl;
	}

	//Free allocation shader reflection memory
	pVertexShaderReflection->Release();
	pVertexShaderReflection = 0;

}



void Shader::createConstBuffer(){

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	D3D10_BUFFER_DESC MVPBufferDesc;
	MVPBufferDesc.Usage = D3D10_USAGE_DYNAMIC;
	MVPBufferDesc.ByteWidth = sizeof(MatrixBuffer);
	MVPBufferDesc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
	MVPBufferDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	MVPBufferDesc.MiscFlags = 0;

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	if (FAILED(m_pD3DDevice->CreateBuffer(&MVPBufferDesc, NULL, &m_MVPBuffer))){

		std::cout << "Could not create constant buffer!" << std::endl;
	}
}

void Shader::createSamplerState(){

	// Create a texture sampler state description.
	D3D10_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D10_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D10_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D10_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D10_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D10_FLOAT32_MAX;

	// Create the texture sampler state.
	if (FAILED(m_pD3DDevice->CreateSamplerState(&samplerDesc, &m_sampleState))){

		std::cout << "Could not create SamplerState!" << std::endl;
	}
	

}

void Shader::bindShader(){

	m_pD3DDevice->VSSetShader(m_vertexShader);
	m_pD3DDevice->PSSetShader(m_pixelShader);
	m_pD3DDevice->PSSetSamplers(0, 1, &m_sampleState);
}

void Shader::loadMatrix(Location location, const D3DXMATRIXA16& matrix){

	D3DXMATRIXA16 mtxTranspose;
	D3DXMatrixTranspose(&mtxTranspose, &matrix);

	switch (location){

		case Model:
			m_MVP.model = mtxTranspose;
		case View:
			m_MVP.view = mtxTranspose;
		case Projection:
			m_MVP.projection = mtxTranspose;
	}

}


bool Shader::mapMVPBuffer(){

	MatrixBuffer* dataPtr;
	// Lock the constant buffer so it can be written to.
	if (FAILED(m_MVPBuffer->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&dataPtr))){
		std::cout << "could not update const buffer" << std::endl;
		return false;
	}

	// Copy the matrices into the constant buffer.
	dataPtr->model = m_MVP.model;
	dataPtr->view = m_MVP.view;
	dataPtr->projection = m_MVP.projection;

	// Unlock the constant buffer.
	m_MVPBuffer->Unmap();

	// Finanly set the constant buffer in the vertex shader with the updated values.
	m_pD3DDevice->VSSetConstantBuffers(0, 1, &m_MVPBuffer);

	return true;
}



void Shader::setTexture(ID3D10ShaderResourceView *texture){
	// Set shader texture resource in the pixel shader
	m_pD3DDevice->PSSetShaderResources(0, 1, &texture);
}