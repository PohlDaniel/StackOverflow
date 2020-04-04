#include <sstream>
#include "SeparableSSS.h"
using namespace std;

SeparableSSS::SeparableSSS(ID3D10Device *device,
                           int width,
                           int height,
                           float fovy,
                           float sssWidth,
                           int nSamples,
                           bool stencilInitialized,
                           bool followSurface,
                           bool separateStrengthSource,
                           DXGI_FORMAT format) : device(device),
                           sssWidth(sssWidth),
                           nSamples(nSamples),
                           stencilInitialized(stencilInitialized),
                           strength(D3DXVECTOR3(0.48f, 0.41f, 0.28f)),
                           falloff(D3DXVECTOR3(1.0f, 0.37f, 0.3f)) {
  
    // Now, let's load the effect:
	D3DX10CreateEffectFromFile(L"sss/SeparableSSS.fx", NULL, NULL, "fx_4_0", D3D10_SHADER_ENABLE_STRICTNESS | D3D10_SHADER_OPTIMIZATION_LEVEL3, 0, device, NULL, NULL, &effect, NULL, NULL);

    // Setup the fullscreen quad:
    D3D10_PASS_DESC desc;
    effect->GetTechniqueByName("SSS")->GetPassByName("SSSSBlurX")->GetDesc(&desc);
    quad = new Quad(device, desc);

    // Create the temporal render target:
    tmpRT = new RenderTarget(device, width, height, format);

    // Create some handles for techniques and variables:
    technique = effect->GetTechniqueByName("SSS");
    idVariable = effect->GetVariableByName("id")->AsScalar();
    sssWidthVariable = effect->GetVariableByName("sssWidth")->AsScalar();
    kernelVariable = effect->GetVariableByName("kernel")->AsVector();
    colorTexVariable = effect->GetVariableByName("colorTex")->AsShaderResource();
    depthTexVariable = effect->GetVariableByName("depthTex")->AsShaderResource();
    strengthTexVariable = effect->GetVariableByName("strengthTex")->AsShaderResource();	

	// Set additional vars	
	float ar = (float) height / width;
	effect->GetVariableByName("aspectRatio")->AsScalar()->SetFloat(ar);
	effect->GetVariableByName("maxOffsetMm")->AsScalar()->SetFloat(1);

	useImg2DKernel = false;
}

SeparableSSS::~SeparableSSS() {
   
}


void SeparableSSS::go(ID3D10RenderTargetView *mainRTV,
                      ID3D10ShaderResourceView *mainSRV,
                      ID3D10ShaderResourceView *depthSRV,
                      ID3D10DepthStencilView *depthDSV,
                      ID3D10ShaderResourceView *stregthSRV,
                      int id) {
  
    // Save the state:
    SaveViewportsScope saveViewport(device);
    SaveRenderTargetsScope saveRenderTargets(device);
    SaveInputLayoutScope saveInputLayout(device);

    // Clear the temporal render target:
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    device->ClearRenderTargetView(*tmpRT, clearColor);

    // Clear the stencil buffer if it was not available, and thus one must be 
    // initialized on the fly:
    if (!stencilInitialized)
        device->ClearDepthStencilView(depthDSV, D3D10_CLEAR_STENCIL, 1.0, 0);

    // Set variables:
    idVariable->SetInt(id);
    sssWidthVariable->SetFloat(sssWidth);
    depthTexVariable->SetResource(depthSRV);
    strengthTexVariable->SetResource(stregthSRV);

    // Set input layout and viewport:
    quad->setInputLayout();
    tmpRT->setViewport();	

	// Run the horizontal pass:
	colorTexVariable->SetResource(mainSRV);
	technique->GetPassByName("SSSSBlurX")->Apply(0);
	device->OMSetRenderTargets(1, *tmpRT, depthDSV);
	quad->draw();
	device->OMSetRenderTargets(0, NULL, NULL);
	
	// And finish with the vertical one:
	colorTexVariable->SetResource(*tmpRT);
	technique->GetPassByName("SSSSBlurY")->Apply(0);
	device->OMSetRenderTargets(1, &mainRTV, depthDSV);
	quad->draw();
	device->OMSetRenderTargets(0, NULL, NULL);
}

