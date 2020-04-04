#include "SDKmesh.h"

#include <atlbase.h>
#include <atlconv.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <math.h>
#include <direct.h>
#include <algorithm>  

#include "SeparableSSS.h"
#include "RenderTarget.h"
#include "ShadowMap.h"

#include "Camera.h"
#include "Model.h"
#include "Shader.h"
#include "D3DDevice.h"

enum IMAGE { DRAGON, STATUE };
enum DIRECTION { DIR_FORWARD = 1, DIR_BACKWARD = 2, DIR_LEFT = 4, DIR_RIGHT = 8, DIR_UP = 16, DIR_DOWN = 32, DIR_FORCE_32BIT = 0x7FFFFFFF };

IMAGE image;

ID3D10ShaderResourceView *beckmannSRV;
ID3D10Effect *mainEffect;
ID3D10InputLayout *vertexLayout;

BackbufferRenderTarget *backbufferRT;
RenderTarget *mainRT;
RenderTarget *tmpRT_SRGB;
RenderTarget *tmpRT;
RenderTarget *specularsRT;
RenderTarget *depthRT;
RenderTarget *velocityRT;
DepthStencil *depthStencil;

SeparableSSS *separableSSS;

Quad *quad;
Quad *quad2;

Camera *camera;
ShaderFX *quadShader;

D3DXMATRIX prevViewProj, currViewProj;
int subsampleIndex = 0;

int width = 1280;
int height = 720;
POINT g_OldCursorPos;
bool capture = false;
HWND g_hwnd = NULL;
Model2 *statue;
ID3D10RasterizerState *mNoCullRS;
D3DDevice* d3dDevice;
ID3D10Device* g_pD3DDevice;
IDXGISwapChain*	g_pSwapChain;
bool dragon = true;
void processInput();
void setCursortoMiddle(HWND hwnd);

LRESULT CALLBACK winProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void renderDragon();
void renderStatue();

const int N_LIGHTS = 5;
const int SHADOW_MAP_SIZE = 2048;

struct Light {
	float fov;
	float falloffWidth;
	D3DXVECTOR3 color;
	float attenuation;
	float farPlane;
	float bias;
	ShadowMap *shadowMap;
};
Light lights[N_LIGHTS];


bool sssEnabled = true;
bool translucencyOnly = false;
bool ssssOnly = false;
bool specular = true;


#pragma region Mesh and Material Stuff
struct MeshEntry {

	bool wasPreloaded;
	CDXUTSDKMesh *mesh;
};

struct TextureEntry {

	bool wasPreloaded;
	ID3D10ShaderResourceView *tex;
};

std::map<int, MeshEntry> meshBuffer;
std::map<std::pair<int, std::string>, TextureEntry> textureBuffer;

struct Slot {

	int loadedModelIndex;
	int loadedMaterialIndex;
	MeshEntry *mesh;
	TextureEntry *diffuseMap;
	TextureEntry *normalMap;
	TextureEntry *specularAOSRV;
};

struct Material {

	std::string Name;
	std::string DiffuseFilename;
	std::string NormalFilename;
	std::string SpecularAOFilename;
};

struct Model {

	std::string Name;
	std::vector<Material> materials;
};

std::vector<Model> modelList;
const int slotCount = 1;
Slot slots[slotCount];
int selectedModels[slotCount] = { 0 }; // -1 for empty 
int selectedMaterials[slotCount] = { 0 }; // -1 for none 

void initSlots() {

	slots[0].loadedModelIndex = -1;
	slots[0].loadedMaterialIndex = -1;
	slots[0].mesh = 0;
	slots[0].diffuseMap = 0;
	slots[0].normalMap = 0;
	slots[0].specularAOSRV = 0;
}

Model buildModel(std::string name, Material const * mats, int matCount) {

	Model m;
	m.Name = name;
	m.materials.push_back(mats[0]);

	return m;
}

void initModelList() {

	Material dragon[1] = { "Default", "DiffuseMap.dds", "NormalMap.dds", "SpecularAOMap.dds" };
	modelList.push_back(buildModel("Dragon", dragon, 1));
}


void loadMesh(CDXUTSDKMesh &mesh, ID3D10Device *device, const std::string &name, const std::string &path) {
	HRESULT hr;

	std::string fullpath = name;

	hr = mesh.Create(device, CA2W(fullpath.c_str()), true);
	if (FAILED(hr)) mesh.Create(device, CA2W(("..\\" + fullpath).c_str()), true);
}

HRESULT loadModelAndTexture(ID3D10Device *device, int modelIndex, int materialIndex, bool preload) {

	Model model = modelList[0];

	Material mat;
	if (materialIndex < 0)
		mat = model.materials[0];
	else
		mat = model.materials[materialIndex];

	std::string name = model.Name;
	std::string modelFolder = "Media\\" + name + "\\";

	std::map<int, MeshEntry>::iterator mit = meshBuffer.find(modelIndex);

	if (mit == meshBuffer.end()) {

		MeshEntry me;
		me.wasPreloaded = preload;
		me.mesh = new CDXUTSDKMesh();

		loadMesh(*(me.mesh), device, modelFolder + name + ".sdkmesh", name);

		meshBuffer.insert(std::pair<int, MeshEntry>(modelIndex, me));
	}

	HRESULT hr;

	std::string diffName = mat.DiffuseFilename;
	std::string	diffPath = modelFolder + diffName;


	std::map<std::pair<int, std::string>, TextureEntry>::iterator diffit = textureBuffer.find(make_pair(modelIndex, diffName));
	if (diffit == textureBuffer.end()) {

		TextureEntry diff; diff.wasPreloaded = preload;

		D3DX10_IMAGE_LOAD_INFO loadInfo;
		loadInfo.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		loadInfo.Filter = D3DX10_FILTER_POINT | D3DX10_FILTER_SRGB_IN;
		hr = D3DX10CreateShaderResourceViewFromFile(device, CA2W(diffPath.c_str()), &loadInfo, NULL, &(diff.tex), NULL);
		if (FAILED(hr)) D3DX10CreateShaderResourceViewFromFile(device, CA2W(("..\\" + diffPath).c_str()), &loadInfo, NULL, &(diff.tex), NULL);

		textureBuffer.insert(std::pair<std::pair<int, std::string>, TextureEntry>(make_pair(modelIndex, diffName), diff));
	}

	std::string normName = mat.NormalFilename;
	std::map<std::pair<int, std::string>, TextureEntry>::iterator normit = textureBuffer.find(make_pair(modelIndex, normName));
	if (normit == textureBuffer.end()) {

		TextureEntry norm; norm.wasPreloaded = preload;
		std::string	normPath = modelFolder + normName;

		D3DX10_IMAGE_LOAD_INFO loadInfoNorm;
		loadInfoNorm.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		hr = D3DX10CreateShaderResourceViewFromFile(device, CA2W(normPath.c_str()), &loadInfoNorm, NULL, &norm.tex, NULL);
		if (FAILED(hr)) D3DX10CreateShaderResourceViewFromFile(device, CA2W(("..\\" + normPath).c_str()), &loadInfoNorm, NULL, &norm.tex, NULL);

		textureBuffer.insert(std::pair<std::pair<int, std::string>, TextureEntry>(make_pair(modelIndex, normName), norm));
	}

	std::string specName = mat.SpecularAOFilename;
	std::map<std::pair<int, std::string>, TextureEntry>::iterator specit = textureBuffer.find(make_pair(modelIndex, specName));
	if (specit == textureBuffer.end()) {

		TextureEntry spec; spec.wasPreloaded = preload;
		std::string	specAOPath = modelFolder + specName;

		D3DX10_IMAGE_LOAD_INFO loadInfo;
		loadInfo.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		loadInfo.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		hr = D3DX10CreateShaderResourceViewFromFile(device, CA2W(specAOPath.c_str()), &loadInfo, NULL, &spec.tex, NULL);
		if (FAILED(hr)) D3DX10CreateShaderResourceViewFromFile(device, CA2W(("..\\" + specAOPath).c_str()), &loadInfo, NULL, &spec.tex, NULL);

		textureBuffer.insert(std::pair<std::pair<int, std::string>, TextureEntry>(make_pair(modelIndex, specName), spec));
	}

	return S_OK;
}


void setSlot(ID3D10Device *device, int slotIndex, int modelIndex, int materialIndex) {

	if (modelIndex < 0)
		return;

	loadModelAndTexture(device, modelIndex, materialIndex, false);

	slots[slotIndex].mesh = &meshBuffer[0];

	Model model = modelList[0];

	Material mat;
	std::string diffName;

	mat = model.materials[materialIndex];
	diffName = mat.DiffuseFilename;

	slots[slotIndex].diffuseMap = &textureBuffer[make_pair(modelIndex, diffName)];
	slots[slotIndex].normalMap = &textureBuffer[make_pair(modelIndex, mat.NormalFilename)];
	slots[slotIndex].specularAOSRV = &textureBuffer[make_pair(modelIndex, mat.SpecularAOFilename)];

	slots[slotIndex].loadedModelIndex = modelIndex;
	slots[slotIndex].loadedMaterialIndex = materialIndex;
}
#pragma endregion

void shadowPassDragon(ID3D10Device *device) {

	for (int i = 0; i < 2; i++) {

		lights[i].shadowMap->begin();

		D3DXMATRIX world;
		D3DXMatrixIdentity(&world);

		lights[i].shadowMap->setWorldMatrix((float*)world);

		if (slots[0].loadedModelIndex >= 0)
			slots[0].mesh->mesh->Render(device, lights[i].shadowMap->getTechnique());

		lights[i].shadowMap->end();
	}

}

void shadowPassStatue(ID3D10Device *device) {

	for (int i = 0; i < 4; i++) {
		if (D3DXVec3Length(&lights[i].color) > 0.0f) {
			lights[i].shadowMap->begin();

			D3DXMATRIX world = statue->getTransformationMatrix();

			lights[i].shadowMap->setWorldMatrix((float*)world);

			for (int k = 0; k < statue->mesh.size(); k++) {
				device->IASetVertexBuffers(0, 1, &statue->mesh[k]->pVertexBuffer, &statue->mesh[k]->m_stride, &statue->mesh[k]->m_offset);
				device->IASetIndexBuffer(statue->mesh[k]->pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

				lights[i].shadowMap->getTechnique()->GetPassByIndex(0)->Apply(0);
				device->DrawIndexed(statue->mesh[k]->getNumberOfIndices(), 0, 0);
			}

			lights[i].shadowMap->end();
		}
	}

}

void mainPassDragon(ID3D10Device *device) {
	HRESULT hr;

	device->IASetInputLayout(vertexLayout);
	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Jitter setup:
	D3DXVECTOR2 jitters[] = {
		D3DXVECTOR2(0.25f, -0.25f),
		D3DXVECTOR2(-0.25f,  0.25f)
	};

	D3DXVECTOR2 jitterProjectionSpace = 2.0f * jitters[subsampleIndex];
	jitterProjectionSpace.x /= float(width); jitterProjectionSpace.y /= float(height);

	mainEffect->GetVariableByName("jitter")->AsVector()->SetFloatVector((float *)D3DXVECTOR2(0.0f, 0.0f));

	// Calculate current view-projection matrix:
	currViewProj = camera->getViewMatrix() * camera->getProjectionMatrix();

	// Variables setup:
	mainEffect->GetVariableByName("cameraPosition")->AsVector()->SetFloatVector((float *)(D3DXVECTOR3)camera->getPosition());

	mainEffect->GetVariableByName("translucencyEnabled")->AsScalar()->SetFloat((float)(!ssssOnly));

	for (int i = 0; i < 2; i++) {

		mainEffect->GetVariableByName("lights")->GetElement(i)->GetMemberByName("position")->AsVector()->SetFloatVector((float *)lights[i].shadowMap->m_eye);
		mainEffect->GetVariableByName("lights")->GetElement(i)->GetMemberByName("direction")->AsVector()->SetFloatVector((float *)lights[i].shadowMap->m_viewDir);
		mainEffect->GetVariableByName("lights")->GetElement(i)->GetMemberByName("falloffStart")->AsScalar()->SetFloat(cos(0.5f * lights[i].fov));
		mainEffect->GetVariableByName("lights")->GetElement(i)->GetMemberByName("falloffWidth")->AsScalar()->SetFloat(lights[i].falloffWidth);
		mainEffect->GetVariableByName("lights")->GetElement(i)->GetMemberByName("color")->AsVector()->SetFloatVector((float *)lights[i].color);
		mainEffect->GetVariableByName("lights")->GetElement(i)->GetMemberByName("attenuation")->AsScalar()->SetFloat(lights[i].attenuation);
		mainEffect->GetVariableByName("lights")->GetElement(i)->GetMemberByName("farPlane")->AsScalar()->SetFloat(lights[i].farPlane);
		mainEffect->GetVariableByName("lights")->GetElement(i)->GetMemberByName("bias")->AsScalar()->SetFloat(lights[i].bias);
		mainEffect->GetVariableByName("lights")->GetElement(i)->GetMemberByName("viewProjection")->AsMatrix()->SetMatrix(lights[i].shadowMap->getViewProjectionTextureMatrix());
		mainEffect->GetVariableByName("shadowMaps")->GetElement(i)->AsShaderResource()->SetResource(*lights[i].shadowMap);


	}
	mainEffect->GetTechniqueByName("Render")->GetPassByIndex(0)->Apply(0); // Just to clear some runtime warnings.

																		   // Render target setup:
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	device->ClearDepthStencilView(*depthStencil, D3D10_CLEAR_DEPTH | D3D10_CLEAR_STENCIL, 1.0, 0);
	device->ClearRenderTargetView(*mainRT, clearColor);
	device->ClearRenderTargetView(*depthRT, clearColor);
	device->ClearRenderTargetView(*specularsRT, clearColor);
	device->ClearRenderTargetView(*velocityRT, clearColor);

	ID3D10RenderTargetView *rt[] = { *mainRT, *depthRT, *velocityRT, *specularsRT };
	device->OMSetRenderTargets(4, rt, *depthStencil);

	D3DXMATRIX world;
	D3DXMatrixIdentity(&world);
	D3DXMATRIX currWorldViewProj = world * currViewProj;
	D3DXMATRIX prevWorldViewProj = world * prevViewProj;

	mainEffect->GetVariableByName("currWorldViewProj")->AsMatrix()->SetMatrix((float *)currWorldViewProj);
	mainEffect->GetVariableByName("prevWorldViewProj")->AsMatrix()->SetMatrix((float *)prevWorldViewProj);
	mainEffect->GetVariableByName("world")->AsMatrix()->SetMatrix((float *)world);
	mainEffect->GetVariableByName("worldInverseTranspose")->AsMatrix()->SetMatrixTranspose((float *)world);
	mainEffect->GetVariableByName("id")->AsScalar()->SetInt(1);

	mainEffect->GetVariableByName("diffuseTex")->AsShaderResource()->SetResource(slots[0].diffuseMap->tex);
	mainEffect->GetVariableByName("normalTex")->AsShaderResource()->SetResource(slots[0].normalMap->tex);
	mainEffect->GetVariableByName("specularAOTex")->AsShaderResource()->SetResource(slots[0].specularAOSRV->tex);

	slots[0].mesh->mesh->Render(device, mainEffect->GetTechniqueByName("Render"));


	// Update previous view-projection matrix:
	prevViewProj = currViewProj;


	device->OMSetRenderTargets(1, *mainRT, *depthStencil);
	device->OMSetRenderTargets(0, NULL, NULL);

}

void mainPassStatue(ID3D10Device *device) {
	HRESULT hr;

	device->IASetInputLayout(vertexLayout);
	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Jitter setup:
	D3DXVECTOR2 jitters[] = {
		D3DXVECTOR2(0.25f, -0.25f),
		D3DXVECTOR2(-0.25f,  0.25f)
	};

	D3DXVECTOR2 jitterProjectionSpace = 2.0f * jitters[subsampleIndex];
	jitterProjectionSpace.x /= float(width); jitterProjectionSpace.y /= float(height);

	mainEffect->GetVariableByName("jitter")->AsVector()->SetFloatVector((float *)D3DXVECTOR2(0.0f, 0.0f));

	// Calculate current view-projection matrix:
	currViewProj = camera->getViewMatrix() * camera->getProjectionMatrix();

	// Variables setup:
	mainEffect->GetVariableByName("cameraPosition")->AsVector()->SetFloatVector((float *)(D3DXVECTOR3)camera->getPosition());

	mainEffect->GetVariableByName("translucencyEnabled")->AsScalar()->SetFloat((float)(!ssssOnly));

	for (int i = 0; i < 4; i++) {

		mainEffect->GetVariableByName("lights")->GetElement(i)->GetMemberByName("position")->AsVector()->SetFloatVector((float *)lights[i].shadowMap->m_eye);
		mainEffect->GetVariableByName("lights")->GetElement(i)->GetMemberByName("direction")->AsVector()->SetFloatVector((float *)lights[i].shadowMap->m_viewDir);
		mainEffect->GetVariableByName("lights")->GetElement(i)->GetMemberByName("falloffStart")->AsScalar()->SetFloat(cos(0.5f * lights[i].fov));
		mainEffect->GetVariableByName("lights")->GetElement(i)->GetMemberByName("falloffWidth")->AsScalar()->SetFloat(lights[i].falloffWidth);
		mainEffect->GetVariableByName("lights")->GetElement(i)->GetMemberByName("color")->AsVector()->SetFloatVector((float *)lights[i].color);
		mainEffect->GetVariableByName("lights")->GetElement(i)->GetMemberByName("attenuation")->AsScalar()->SetFloat(lights[i].attenuation);
		mainEffect->GetVariableByName("lights")->GetElement(i)->GetMemberByName("farPlane")->AsScalar()->SetFloat(lights[i].farPlane);
		mainEffect->GetVariableByName("lights")->GetElement(i)->GetMemberByName("bias")->AsScalar()->SetFloat(lights[i].bias);
		mainEffect->GetVariableByName("lights")->GetElement(i)->GetMemberByName("viewProjection")->AsMatrix()->SetMatrix(lights[i].shadowMap->getViewProjectionTextureMatrix());
		mainEffect->GetVariableByName("shadowMaps")->GetElement(i)->AsShaderResource()->SetResource(*lights[i].shadowMap);

	}
	mainEffect->GetTechniqueByName("Render")->GetPassByIndex(0)->Apply(0); // Just to clear some runtime warnings.

																		   // Render target setup:
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	device->ClearDepthStencilView(*depthStencil, D3D10_CLEAR_DEPTH | D3D10_CLEAR_STENCIL, 1.0, 0);
	device->ClearRenderTargetView(*mainRT, clearColor);
	device->ClearRenderTargetView(*depthRT, clearColor);
	device->ClearRenderTargetView(*specularsRT, clearColor);
	device->ClearRenderTargetView(*velocityRT, clearColor);

	ID3D10RenderTargetView *rt[] = { *mainRT, *depthRT, *velocityRT, *specularsRT };
	device->OMSetRenderTargets(4, rt, *depthStencil);

	D3DXMATRIX world = statue->getTransformationMatrix();

	D3DXMATRIX currWorldViewProj = world * currViewProj;
	D3DXMATRIX prevWorldViewProj = world * prevViewProj;

	mainEffect->GetVariableByName("currWorldViewProj")->AsMatrix()->SetMatrix((float *)currWorldViewProj);
	mainEffect->GetVariableByName("prevWorldViewProj")->AsMatrix()->SetMatrix((float *)prevWorldViewProj);
	mainEffect->GetVariableByName("world")->AsMatrix()->SetMatrix((float *)world);
	mainEffect->GetVariableByName("worldInverseTranspose")->AsMatrix()->SetMatrixTranspose((float *)world);
	mainEffect->GetVariableByName("id")->AsScalar()->SetInt(1);

	for (int j = 0; j < statue->mesh.size(); j++) {

		device->IASetVertexBuffers(0, 1, &statue->mesh[j]->pVertexBuffer, &statue->mesh[j]->m_stride, &statue->mesh[j]->m_offset);
		device->IASetIndexBuffer(statue->mesh[j]->pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		mainEffect->GetVariableByName("diffuseTex")->AsShaderResource()->SetResource(statue->mesh[j]->m_pDiffuseTex);
		mainEffect->GetVariableByName("normalTex")->AsShaderResource()->SetResource(statue->mesh[j]->m_pNormalMap);
		mainEffect->GetVariableByName("specularAOTex")->AsShaderResource()->SetResource(statue->mesh[j]->m_pSpecularMap);

		mainEffect->GetTechniqueByName("Render")->GetPassByIndex(0)->Apply(0);
		device->DrawIndexed(statue->mesh[j]->getNumberOfIndices(), 0, 0);
	}

	// Update previous view-projection matrix:
	prevViewProj = currViewProj;

	device->OMSetRenderTargets(1, *mainRT, *depthStencil);
	device->OMSetRenderTargets(0, NULL, NULL);

}


void addSpecular(ID3D10Device *device, RenderTarget *dst) {
	HRESULT hr;
	quad->setInputLayout();
	mainEffect->GetVariableByName("specularsTex")->AsShaderResource()->SetResource(*specularsRT);
	mainEffect->GetTechniqueByName("AddSpecular")->GetPassByIndex(0)->Apply(0);
	device->OMSetRenderTargets(1, *dst, NULL);
	quad->draw();
	device->OMSetRenderTargets(0, NULL, NULL);
}


void setSSSS(bool on) {

	HRESULT hr;
	sssEnabled = on;
	mainEffect->GetVariableByName("sssEnabled")->AsScalar()->SetBool(on);
}


void setupSSS(ID3D10Device *device, int sampleCount = 0) {

	float sssLevel = 180;
	int nSamples = 17;
	bool stencilInitialized = true;
	bool followSurface = true;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	separableSSS = new SeparableSSS(device, width, height, 20, sssLevel, nSamples, stencilInitialized, followSurface, true, format);


}

void setupDragon() {

	D3DXVECTOR3 camPos = D3DXVECTOR3(-1.30592, 0.259203, -2.11601);
	D3DXVECTOR3 target = D3DXVECTOR3(0.00597954, -0.00716528, -0.00461888);
	D3DXVECTOR3 up = D3DXVECTOR3(0.0, 1.0, 0.0);

	camera = new Camera(camPos, target, up);
	camera->perspective(20.0f, width / (FLOAT)height, 0.1f, 100.0f);


	std::vector<D3D10_SHADER_MACRO> defines;
	D3D10_SHADER_MACRO separateSpecularsMacro = { "SEPARATE_SPECULARS", NULL };
	defines.push_back(separateSpecularsMacro);

	D3D10_SHADER_MACRO null = { NULL, NULL };
	defines.push_back(null);

	D3DX10CreateEffectFromFile(L"sss/MainDragon.fx", &defines.front(), NULL, "fx_4_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, g_pD3DDevice, NULL, NULL, &mainEffect, NULL, NULL);

	mainEffect->GetVariableByName("beckmannTex")->AsShaderResource()->SetResource(beckmannSRV);

	float specInt = 1;
	mainEffect->GetVariableByName("specularIntensity")->AsScalar()->SetFloat(specInt);

	float specRough = 0.08;
	mainEffect->GetVariableByName("specularRoughness")->AsScalar()->SetFloat(specRough);

	float specFres = 0.81;
	mainEffect->GetVariableByName("specularFresnel")->AsScalar()->SetFloat(specFres);

	float bump = 1;
	mainEffect->GetVariableByName("bumpiness")->AsScalar()->SetFloat(bump);

	float ambient = 0.12;
	mainEffect->GetVariableByName("ambient")->AsScalar()->SetFloat(ambient);

	float sssWi = 200;
	mainEffect->GetVariableByName("sssWidth")->AsScalar()->SetFloat(sssWi);

	float trans = 0;
	mainEffect->GetVariableByName("translucency")->AsScalar()->SetFloat(trans);

	bool sssEnabled = true;
	mainEffect->GetVariableByName("sssEnabled")->AsScalar()->SetBool(sssEnabled);

	const D3D10_INPUT_ELEMENT_DESC layout[] = {

		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D10_INPUT_PER_VERTEX_DATA, 0 }
	};

	UINT numElements = sizeof(layout) / sizeof(D3D10_INPUT_ELEMENT_DESC);

	D3D10_PASS_DESC passDesc;
	mainEffect->GetTechniqueByName("Render")->GetPassByIndex(0)->GetDesc(&passDesc);
	g_pD3DDevice->CreateInputLayout(layout, numElements, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &vertexLayout);

	g_pD3DDevice->IASetInputLayout(vertexLayout);

	mainEffect->GetTechniqueByName("AddSpecular")->GetPassByIndex(0)->GetDesc(&passDesc);
	quad = new Quad(g_pD3DDevice, passDesc);

	for (int i = 0; i < N_LIGHTS; i++) {
		lights[i].fov = 45.0f * D3DX_PI / 180.f;
		lights[i].falloffWidth = 0.05f;
		lights[i].attenuation = 1.0f / 128.0f;
		lights[i].farPlane = 10.0f;
		lights[i].bias = -0.01f;

		lights[i].shadowMap = new ShadowMap(g_pD3DDevice, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
	}

	lights[0].color.x = 1.2;
	lights[0].color.y = 1.2;
	lights[0].color.z = 1.2;
	lights[0].shadowMap->setProjectionMatrix(45.0, 1.0, 0.1f, 10.0f);

	D3DXVECTOR3 eye0 = D3DXVECTOR3(-0.684557, 0.445025, 1.70124);
	D3DXVECTOR3 target0 = D3DXVECTOR3(0.00305176, 0.0366097, -0.00752223);
	D3DXVECTOR3 up0 = D3DXVECTOR3(0.0, -1.0, 0.0);
	lights[0].shadowMap->setViewMatrix(eye0, target0, up0);

	lights[1].color.x = 1;
	lights[1].color.y = 1;
	lights[1].color.z = 1;

	lights[1].shadowMap->setProjectionMatrix(45.0, 1.0, 0.1f, 10.0f);

	D3DXVECTOR3 eye1 = D3DXVECTOR3(1.24467, 0.995614, -0.182616);
	D3DXVECTOR3 target1 = D3DXVECTOR3(0.0687447, -0.081207, -0.0497149);
	D3DXVECTOR3 up1 = D3DXVECTOR3(0.0, 1.0, 0.0);
	lights[1].shadowMap->setViewMatrix(eye1, target1, up1);

	image = IMAGE::DRAGON;
}

void setupStatue() {

	D3DXVECTOR3 camPos = D3DXVECTOR3(-1.30592, 3.259203, -4.11601);
	D3DXVECTOR3 target = D3DXVECTOR3(0.00597954, 1.00716528, -0.00461888);
	D3DXVECTOR3 up = D3DXVECTOR3(0.0, 1.0, 0.0);

	camera = new Camera(camPos, target, up);
	camera->perspective(20.0f, width / (FLOAT)height, 0.1f, 100.0f);

	std::vector<D3D10_SHADER_MACRO> defines;
	D3D10_SHADER_MACRO separateSpecularsMacro = { "SEPARATE_SPECULARS", NULL };
	defines.push_back(separateSpecularsMacro);

	D3D10_SHADER_MACRO null = { NULL, NULL };
	defines.push_back(null);

	D3DX10CreateEffectFromFile(L"sss/MainStatue.fx", &defines.front(), NULL, "fx_4_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, g_pD3DDevice, NULL, NULL, &mainEffect, NULL, NULL);

	mainEffect->GetVariableByName("beckmannTex")->AsShaderResource()->SetResource(beckmannSRV);

	float specInt = 1;
	mainEffect->GetVariableByName("specularIntensity")->AsScalar()->SetFloat(specInt);

	float specRough = 0.08;
	mainEffect->GetVariableByName("specularRoughness")->AsScalar()->SetFloat(specRough);

	float specFres = 0.81;
	mainEffect->GetVariableByName("specularFresnel")->AsScalar()->SetFloat(specFres);

	float bump = 1;
	mainEffect->GetVariableByName("bumpiness")->AsScalar()->SetFloat(bump);

	float ambient = 0.12;
	mainEffect->GetVariableByName("ambient")->AsScalar()->SetFloat(ambient);

	float sssWi = 200;
	mainEffect->GetVariableByName("sssWidth")->AsScalar()->SetFloat(sssWi);

	float trans = 0;
	mainEffect->GetVariableByName("translucency")->AsScalar()->SetFloat(trans);

	bool sssEnabled = true;
	mainEffect->GetVariableByName("sssEnabled")->AsScalar()->SetBool(sssEnabled);

	const D3D10_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D10_INPUT_PER_VERTEX_DATA, 0 }
	};

	UINT numElements = sizeof(layout) / sizeof(D3D10_INPUT_ELEMENT_DESC);

	D3D10_PASS_DESC passDesc;
	mainEffect->GetTechniqueByName("Render")->GetPassByIndex(0)->GetDesc(&passDesc);
	g_pD3DDevice->CreateInputLayout(layout, numElements, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &vertexLayout);

	g_pD3DDevice->IASetInputLayout(vertexLayout);

	mainEffect->GetTechniqueByName("AddSpecular")->GetPassByIndex(0)->GetDesc(&passDesc);
	quad = new Quad(g_pD3DDevice, passDesc);

	statue = new Model2();
	statue->loadObject("objs/statue/statue.obj");
	statue->scale(0.5, 0.5, 0.5);

	for (int j = 0; j < statue->mesh.size(); j++) {
		statue->mesh[j]->generateTangents();
		statue->mesh[j]->createBuffer(g_pD3DDevice);
		statue->mesh[j]->readMaterial((statue->getModelDirectory() + "/" + statue->getMltPath()).c_str());
		statue->mesh[j]->loadTextureFromMlt(g_pD3DDevice);
	}


	for (int i = 0; i < N_LIGHTS; i++) {
		lights[i].fov = 45.0f * D3DX_PI / 180.f;
		lights[i].falloffWidth = 0.05f;
		lights[i].attenuation = 1.0f / 128.0f;
		lights[i].farPlane = 10.0f;
		lights[i].bias = -0.01f;

		lights[i].shadowMap = new ShadowMap(g_pD3DDevice, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
	}


	lights[0].color.x = 1.2;
	lights[0].color.y = 1.2;
	lights[0].color.z = 1.2;
	lights[0].shadowMap->setProjectionMatrix(45.0, 1.0, 0.1f, 100.0f);

	D3DXVECTOR4 result;
	D3DXVec3Transform(&result, &statue->getCenter(), &statue->getTransformationMatrix());

	D3DXVECTOR3 eye0 = D3DXVECTOR3(0.0, 0.0, -8.0);
	D3DXVECTOR3 target0 = D3DXVECTOR3(result);
	D3DXVECTOR3 up0 = D3DXVECTOR3(0.0, 1.0, 0.0);
	lights[0].shadowMap->setViewMatrix(eye0, target0, up0);


	lights[1].color.x = 0.3;
	lights[1].color.y = 0.3;
	lights[1].color.z = 0.3;

	lights[1].shadowMap->setProjectionMatrix(45.0, 1.0, 0.1f, 10.0f);

	D3DXVECTOR3 eye1 = D3DXVECTOR3(0.0, 0.0, 8.0);
	D3DXVECTOR3 target1 = D3DXVECTOR3(result);
	D3DXVECTOR3 up1 = D3DXVECTOR3(0.0, 1.0, 0.0);
	lights[1].shadowMap->setViewMatrix(eye1, target1, up1);


	lights[2].color.x = 0.3;
	lights[2].color.y = 0.3;
	lights[2].color.z = 0.3;

	lights[2].shadowMap->setProjectionMatrix(45.0, 1.0, 0.1f, 10.0f);

	D3DXVECTOR3 eye2 = D3DXVECTOR3(8.0, 0.0, 0.0);
	D3DXVECTOR3 target2 = D3DXVECTOR3(result);
	D3DXVECTOR3 up2 = D3DXVECTOR3(0.0, 1.0, 0.0);
	lights[2].shadowMap->setViewMatrix(eye2, target2, up2);


	lights[3].color.x = 0.3;
	lights[3].color.y = 0.3;
	lights[3].color.z = 0.3;

	lights[3].shadowMap->setProjectionMatrix(45.0, 1.0, 0.1f, 10.0f);

	image = IMAGE::STATUE;
}

INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {


	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	freopen("CON", "w", stdout);
	SetConsoleTitle(L"Debug console");
	MoveWindow(GetConsoleWindow(), 740, 0, 550, 200, true);


	WNDCLASSEX windowClass;		// window class
	HWND	   hwnd;			//window handle
	MSG		   msg;				// message
	HDC		   hdc;				// device context handle

	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = winProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = hInstance;
	windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);		// default icon
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);			// default arrow
	windowClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);	// white background
	windowClass.lpszMenuName = NULL;									// no menu
	windowClass.lpszClassName = L"WINDOWCLASS";
	windowClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);			// windows logo small icon

																// register the windows class
	if (!RegisterClassEx(&windowClass))
		return 0;

	// class registered, so now create our window
	hwnd = CreateWindowEx(NULL,									// extended style
		L"WINDOWCLASS",						// class name
		L"OBJViewer",							// app name
		WS_OVERLAPPEDWINDOW,
		0, 0,									// x,y coordinate
		width,
		height,								// width, height
		NULL,									// handle to parent
		NULL,									// handle to menu
		hInstance,								// application instance
		NULL);									// no extra params

												// check if window creation failed (hwnd would equal NULL)
	if (!hwnd)
		return 0;

	ShowWindow(hwnd, SW_SHOW);			// display the window
	UpdateWindow(hwnd);

	d3dDevice = new D3DDevice(&hwnd);
	d3dDevice->createSwapChainAndDevice(width, height);
	d3dDevice->createViewports(width, height);
	d3dDevice->initRasterizerState();
	d3dDevice->createRenderTargetsAndDepthBuffer(width, height);
	g_pD3DDevice = d3dDevice->getD3DDevice();
	g_pSwapChain = d3dDevice->getSwapChain();

	ShadowMap::init(g_pD3DDevice);
	setupSSS(g_pD3DDevice);

	initModelList();
	initSlots();

	D3DX10_IMAGE_LOAD_INFO loadInfo;
	loadInfo.BindFlags = D3D10_BIND_SHADER_RESOURCE;
	loadInfo.MipLevels = 1;
	loadInfo.Format = DXGI_FORMAT_R8_UNORM;
	D3DX10CreateShaderResourceViewFromFile(g_pD3DDevice, L"Media/BeckmannMap.dds", &loadInfo, NULL, &beckmannSRV, NULL);

	backbufferRT = new BackbufferRenderTarget(g_pD3DDevice, g_pSwapChain);
	mainRT = new RenderTarget(g_pD3DDevice, width, height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
	tmpRT_SRGB = new RenderTarget(g_pD3DDevice, width, height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
	tmpRT = new RenderTarget(g_pD3DDevice, *tmpRT_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM);
	depthRT = new RenderTarget(g_pD3DDevice, width, height, DXGI_FORMAT_R32_FLOAT);
	specularsRT = new RenderTarget(g_pD3DDevice, width, height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
	velocityRT = new RenderTarget(g_pD3DDevice, width, height, DXGI_FORMAT_R16G16_FLOAT);
	depthStencil = new DepthStencil(g_pD3DDevice, width, height, DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R24_UNORM_X8_TYPELESS);

	quadShader = new ShaderFX("sss/Quad.fx", g_pD3DDevice);
	quadShader->createEffect();
	quadShader->loadTechniquePointer("render");
	quadShader->createInputLayout();
	quad2 = new Quad(g_pD3DDevice);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	//setupDragon();
	setupStatue();

	// main message loop
	while (true) {

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

			if (msg.message == WM_QUIT) break;
			// translate and dispatch message
			TranslateMessage(&msg);
			DispatchMessage(&msg);

		}
		else {

			switch (image) {
			case DRAGON:
				renderDragon();
				break;
			case STATUE:
				renderStatue();
				break;
			}

			processInput();

		} // end If messages waiting
	} // end while*/

	return 0;
}

void setCursortoMiddle(HWND hwnd) {

	RECT rect;
	GetWindowRect(hwnd, &rect);
	SetCursorPos(rect.left + (rect.right - rect.left) / 2, rect.top + (rect.bottom - rect.top) / 2);
}

void processInput() {

	if (capture) {

		static UCHAR pKeyBuffer[256];
		ULONG        Direction = 0;
		POINT        CursorPos;
		float        X = 0.0f, Y = 0.0f, rotationSpeed = 0.05f;

		// Retrieve keyboard state
		if (!GetKeyboardState(pKeyBuffer)) return;

		// Check the relevant keys
		if (pKeyBuffer['W'] & 0xF0) Direction |= DIR_FORWARD;
		if (pKeyBuffer['S'] & 0xF0) Direction |= DIR_BACKWARD;
		if (pKeyBuffer['A'] & 0xF0) Direction |= DIR_LEFT;
		if (pKeyBuffer['D'] & 0xF0) Direction |= DIR_RIGHT;
		if (pKeyBuffer['E'] & 0xF0) Direction |= DIR_UP;
		if (pKeyBuffer['Q'] & 0xF0) Direction |= DIR_DOWN;

		// Hide the mouse pointer
		SetCursor(NULL);

		// Retrieve the cursor position
		GetCursorPos(&CursorPos);

		// Calculate mouse rotational values
		X = (float)(CursorPos.x - g_OldCursorPos.x) * rotationSpeed;
		Y = (float)(CursorPos.y - g_OldCursorPos.y) * rotationSpeed;

		// Reset our cursor position so we can keep going forever :)
		SetCursorPos(g_OldCursorPos.x, g_OldCursorPos.y);

		if (Direction > 0 || X != 0.0f || Y != 0.0f) {

			// Rotate camera
			if (X || Y) {

				camera->rotate(X, Y, 0.0f);
			} // End if any rotation

			if (Direction) {

				float dx = 0, dy = 0, dz = 0, speed = 0.1;

				if (Direction & DIR_FORWARD) dz = speed;
				if (Direction & DIR_BACKWARD) dz = -speed;
				if (Direction & DIR_LEFT) dx = -speed;
				if (Direction & DIR_RIGHT) dx = speed;
				if (Direction & DIR_UP) dy = speed;
				if (Direction & DIR_DOWN) dy = -speed;

				camera->move(dx, dy, dz);
			}
		}// End if any movement
	}// End if Captured
}



LRESULT CALLBACK winProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	static HGLRC hRC;					// rendering context
	static HDC hDC;						// device context
	int width, height;					// window width and height
	POINT pt;
	RECT rect;

	switch (message) {

	case WM_DESTROY: {

		PostQuitMessage(0);
		return 0;
	}

	case WM_CREATE: {

		GetClientRect(hWnd, &rect);
		g_OldCursorPos.x = rect.right / 2;
		g_OldCursorPos.y = rect.bottom / 2;
		pt.x = rect.right / 2;
		pt.y = rect.bottom / 2;
		SetCursorPos(pt.x, pt.y);
		// set the cursor to the middle of the window and capture the window via "SendMessage"
		SendMessage(hWnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(pt.x, pt.y));
		capture = true;
		return 0;
	}break;

	case WM_LBUTTONDOWN: { // Capture the mouse


		setCursortoMiddle(hWnd);
		capture = true;

		return 0;
	} break;

	case WM_KEYDOWN: {

		switch (wParam) {

		case VK_ESCAPE: {
			PostQuitMessage(0);
			return 0;

		}break;
		case VK_SPACE: {
			capture = false;
			return 0;

		}break;
		case 'm':case 'M': {

			sssEnabled = !sssEnabled;

			mainEffect->GetVariableByName("sssEnabled")->AsScalar()->SetBool(sssEnabled);
			break;
		}
		case 'h':case 'H': {

			specular = !specular;
			break;
		}
				 return 0;
		}break;



	default:
		break;
	}
	}

	return (DefWindowProc(hWnd, message, wParam, lParam));
}

void renderDragon() {

	setSlot(g_pD3DDevice, 0, selectedModels[0], selectedMaterials[0]);
	// Shadow Pass
	shadowPassDragon(g_pD3DDevice);
	// Main Pass
	mainPassDragon(g_pD3DDevice);
	// Subsurface Scattering Pass
	if (sssEnabled && !translucencyOnly)
		separableSSS->go(*mainRT, *mainRT, *depthRT, *depthStencil, *specularsRT, 1);

	if (specular) {
		addSpecular(g_pD3DDevice, mainRT);
	}

	g_pD3DDevice->IASetInputLayout(quadShader->m_pInputLayout);
	quadShader->m_pEffect->GetVariableByName("finalTex")->AsShaderResource()->SetResource(*mainRT);
	quadShader->m_pEffect->GetTechniqueByName("render")->GetPassByIndex(0)->Apply(0);
	g_pD3DDevice->OMSetRenderTargets(1, *backbufferRT, NULL);
	quad2->draw();
	g_pD3DDevice->OMSetRenderTargets(0, NULL, NULL);
	g_pD3DDevice->IASetInputLayout(vertexLayout);

	g_pSwapChain->Present(1, 0);
}

void renderStatue() {

	// Shadow Pass
	shadowPassStatue(g_pD3DDevice);
	// Main Pass
	mainPassStatue(g_pD3DDevice);
	// Subsurface Scattering Pass
	if (sssEnabled && !translucencyOnly)
		separableSSS->go(*mainRT, *mainRT, *depthRT, *depthStencil, *specularsRT, 1);

	if (specular) {
		addSpecular(g_pD3DDevice, mainRT);
	}

	g_pD3DDevice->IASetInputLayout(quadShader->m_pInputLayout);
	//quadShader->m_pEffect->GetVariableByName("finalTex")->AsShaderResource()->SetResource(*specularsRT);
	quadShader->m_pEffect->GetVariableByName("finalTex")->AsShaderResource()->SetResource(*mainRT);
	quadShader->m_pEffect->GetTechniqueByName("render")->GetPassByIndex(0)->Apply(0);
	g_pD3DDevice->OMSetRenderTargets(1, *backbufferRT, NULL);
	quad2->draw();
	g_pD3DDevice->OMSetRenderTargets(0, NULL, NULL);
	g_pD3DDevice->IASetInputLayout(vertexLayout);

	g_pSwapChain->Present(1, 0);
}