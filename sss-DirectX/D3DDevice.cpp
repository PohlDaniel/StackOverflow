#include "D3DDevice.h"

D3DDevice::D3DDevice(HWND* hWnd) {

	m_hWnd = hWnd;
}

D3DDevice::~D3DDevice() {

}

ID3D10Device* D3DDevice::getD3DDevice() {

	return m_pD3DDevice;
}

IDXGISwapChain* D3DDevice::getSwapChain() {

	return m_pSwapChain;
}

void D3DDevice::createSwapChainAndDevice(UINT width, UINT height) {

	// set up the SwapChain description
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	//set buffer dimensions and format	
	swapChainDesc.BufferDesc.Width = width;
	swapChainDesc.BufferDesc.Height = height;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.Flags = 0;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	//set refresh rate
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;

	//sampling settings
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.SampleDesc.Count = 1;

	//output window handle
	swapChainDesc.OutputWindow = *m_hWnd;
	swapChainDesc.Windowed = true;


	//Create the D3D device
	//--------------------------------------------------------------
	if (FAILED(D3D10CreateDeviceAndSwapChain(NULL,
		D3D10_DRIVER_TYPE_HARDWARE,
		NULL,
		0,
		D3D10_SDK_VERSION,
		&swapChainDesc,
		&m_pSwapChain,
		&m_pD3DDevice))) {

		std::cout << "D3D device and swapchain creation failed" << std::endl;
	}

}

void D3DDevice::createViewports(UINT width, UINT height) {

	D3D10_VIEWPORT viewPort;

	//create viewport structure	
	viewPort.Width = width;
	viewPort.Height = height;
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;

	m_pD3DDevice->RSSetViewports(1, &viewPort);
}

void D3DDevice::initRasterizerState() {


	//set up rasterizer	description
	D3D10_RASTERIZER_DESC rasterizerState;
	rasterizerState.CullMode = D3D10_CULL_BACK;
	rasterizerState.FillMode = D3D10_FILL_SOLID;
	rasterizerState.FrontCounterClockwise = false;
	rasterizerState.DepthBias = false;
	rasterizerState.DepthBiasClamp = 0;
	rasterizerState.SlopeScaledDepthBias = 0;
	rasterizerState.DepthClipEnable = true;
	rasterizerState.ScissorEnable = false;
	rasterizerState.MultisampleEnable = false;
	rasterizerState.AntialiasedLineEnable = true;

	ID3D10RasterizerState* pRS;

	m_pD3DDevice->CreateRasterizerState(&rasterizerState, &pRS);
	m_pD3DDevice->RSSetState(pRS);

}


void D3DDevice::createRenderTargetsAndDepthBuffer(UINT width, UINT height) {


	ID3D10Texture2D* pBackBuffer;
	ID3D10Texture2D* pDepthStencil;



	//try to get the back buffer
	if (FAILED(m_pSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID*)&pBackBuffer))) std::cout << "Could not get back buffer" << std::endl;

	//try to create render target view
	if (FAILED(m_pD3DDevice->CreateRenderTargetView(pBackBuffer, NULL, &pRenderTargetView))) std::cout << "Could not create render target view" << std::endl;

	pBackBuffer->Release();

	//create depth stencil texture
	D3D10_TEXTURE2D_DESC descDepth;
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D32_FLOAT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D10_USAGE_DEFAULT;
	descDepth.BindFlags = D3D10_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;



	if (FAILED(m_pD3DDevice->CreateTexture2D(&descDepth, NULL, &pDepthStencil)))  std::cout << "Could not create depth stencil texture" << std::endl;

	// Create the depth stencil view
	D3D10_DEPTH_STENCIL_VIEW_DESC descDSV;
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;

	if (FAILED(m_pD3DDevice->CreateDepthStencilView(pDepthStencil, &descDSV, &pDepthStencilView))) std::cout << "Could not create depth stencil view" << std::endl;

	//set render targets
	m_pD3DDevice->OMSetRenderTargets(1, &pRenderTargetView, pDepthStencilView);
}

void D3DDevice::clear() {

	m_pD3DDevice->ClearRenderTargetView(pRenderTargetView, D3DXCOLOR(0.3f, 0.3f, 0.3f, 1.0f));
	m_pD3DDevice->ClearDepthStencilView(pDepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0);
}