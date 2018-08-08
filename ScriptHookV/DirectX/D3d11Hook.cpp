// DX11Hook.cpp

#include "D3d11Hook.h"
#include "..\Hooking\Hooking.h"
#include "..\Scripting\ScriptEngine.h"
#include "..\Utility\General.h"
#include "..\Utility\Log.h"

DX11Hook g_D3DHook;

//texture declariations
std::unordered_map<int, std::wstring> CreateTextureArray;
struct ScriptTex { DXTEXTURE2D tex; int id; };
struct ReloadTex { int id; std::wstring name; };
std::vector<ScriptTex> DrawTextureArray;
std::vector<ReloadTex> ReloadArray;
int DrawTextureArrayIndex = 0;
static Vector2 windowSize = Vector2();
static bool windowedState = true;

//====================================================================================================================================================================
// SwapChain Vtable Lookup
LRESULT CALLBACK DXGIMsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { return DefWindowProc(hwnd, uMsg, wParam, lParam); }
void** GetSwapChainVtable()
{
	WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DXGIMsgProc, 0L, 0L, GetModuleHandleA(NULL), NULL, NULL, NULL, NULL, "DX", NULL };
	RegisterClassExA(&wc);
	HWND hWnd = CreateWindowA("DX", NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, NULL, NULL, wc.hInstance, NULL);
	//Temporary device pointers
	ID3D11Device* pTempDevice = nullptr;
	ID3D11DeviceContext* pTempContext = nullptr;
	IDXGISwapChain* pTempSwapChain = nullptr;

	DXGI_SWAP_CHAIN_DESC SwapChainDesc;
	ZeroMemory(&SwapChainDesc, sizeof(SwapChainDesc));

	SwapChainDesc.BufferCount = 1;
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.OutputWindow = hWnd;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.Windowed = ((GetWindowLongPtr(hWnd, GWL_STYLE) & WS_POPUP) != 0) ? false : true;
	SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	D3D_FEATURE_LEVEL requestedLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
	D3D_FEATURE_LEVEL obtainedLevel;
	UINT createFlags = 0;
	// This flag gives you some quite wonderful debug text. Not wonderful for performance, though!
	//createFlags |= D3D11_CREATE_DEVICE_DEBUG;
	if (FAILED(D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		createFlags,
		requestedLevels,
		sizeof(requestedLevels) / sizeof(D3D_FEATURE_LEVEL),
		D3D11_SDK_VERSION,
		&SwapChainDesc,
		&pTempSwapChain,
		&pTempDevice,
		&obtainedLevel,
		&pTempContext)))
	{
		MessageBox(hWnd, "Failed to create directX device and swapchain!", "Error", MB_ICONERROR);
		return NULL;
	}

	void** pVtable = *(void***)pTempSwapChain;

	//Unload temporary devices
	pTempSwapChain->Release();
	pTempContext->Release();
	pTempDevice->Release();
	DestroyWindow(hWnd);
	return pVtable;
}

//====================================================================================================================================================================
// Function hook stubs

// IDXGISwapChain::Present()
PDETOUR_TRAMPOLINE Present;
LPVOID Hook_Present(IDXGISwapChain *chain, UINT SyncInterval, UINT Flags)
{
	BOOL fullscreenState;
	chain->GetFullscreenState(&fullscreenState, nullptr);
	g_D3DHook.m_windowedMode = !fullscreenState;

	if (!g_D3DHook.m_IsResizing && windowedState == g_D3DHook.m_windowedMode)
	{
		if (g_D3DHook.m_pSwapchain == nullptr)
		{
			g_D3DHook.m_pSwapchain = chain;
			g_D3DHook.InitializeDevices();
		}
		else
		{
			g_D3DHook.Draw();
		}
	}
	else if (windowedState != g_D3DHook.m_windowedMode)
	{
		g_D3DHook.m_pRenderTargetTexture->Release();
		g_D3DHook.m_pRenderTargetView->Release();
		g_D3DHook.m_pContext->Release();
		g_D3DHook.m_pDevice->Release();

		windowedState = g_D3DHook.m_windowedMode;
	}

	return RCast(Hook_Present, Present)(chain, SyncInterval, Flags);
}

// IDXGISwapChain::ResizeBuffers()
PDETOUR_TRAMPOLINE ResizeBuffers;
LPVOID Hook_ResizeBuffers(IDXGISwapChain *chain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	g_D3DHook.m_IsResizing = true;

	g_D3DHook.m_pRenderTargetTexture->Release();
	g_D3DHook.m_pRenderTargetView->Release();
	g_D3DHook.m_pContext->Release();
	g_D3DHook.m_pDevice->Release();

	RCast(Hook_ResizeBuffers, ResizeBuffers)(chain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

	g_D3DHook.m_IsResizing = false;

	return nullptr;
}

//====================================================================================================================================================================
//Ensure SwapChain Vtable and perform function hooks

enum eSwapChainVtableIndices : int
{
	SC_QUERYINTERFACE,
	SC_ADDREF,
	SC_RELEASE,
	SC_SETPRIVATEDATA,
	SC_SETPRIVATEDATAINTERFACE,
	SC_GETPRIVATEDATA,
	SC_GETPARENT,
	SC_GETDEVICE,
	SC_PRESENT,
	SC_GETBUFFER,
	SC_SETFULLSCREENSTATE,
	SC_GETFULLSCREENSTATE,
	SC_GETDESC,
	SC_RESIZEBUFFERS,
	SC_RESIZETARGET,
	SC_GETCONTAININGOUTPUT,
	SC_GETFRAMESTATISTICS,
	SC_GETLASTPRESENTCOUNT,
};

bool DX11Hook::InitializeHooks()
{
	HMODULE hD3D11DLL = 0;
	do
	{
		hD3D11DLL = GetModuleHandle("d3d11.dll");
		Sleep(100);
	}	while (!hD3D11DLL);

	if (auto pVtable = GetSwapChainVtable())
	{	
		auto p_Present = pVtable[SC_PRESENT];
		Present = Hooking::CreateDetour(&p_Present, &Hook_Present, "IDXGISwapChainPresent");

		auto p_ResizeBuffers = pVtable[SC_RESIZEBUFFERS];
		ResizeBuffers = Hooking::CreateDetour(&p_ResizeBuffers, &Hook_ResizeBuffers, "IDXGISwapChainResizeBuffers");

		return true;
	}

	return false;
}

//====================================================================================================================================================================
//Device Initialization and drawing functions

void DX11Hook::InitializeDevices()
{
	if (m_pSwapchain)
	{
		//get device and context
		if (SUCCEEDED(m_pSwapchain->GetDevice(__uuidof(ID3D11Device), (void **)&m_pDevice)))
		{
			m_pSwapchain->GetDevice(__uuidof(m_pDevice), (void**)&m_pDevice);
			m_pDevice->GetImmediateContext(&m_pContext);

			m_pCommonState.reset(new CommonStates(m_pDevice));
			m_pSpriteBatch.reset(new SpriteBatch(m_pContext));
			m_pBatch.reset(new PrimitiveBatch<VertexPositionColor>(m_pContext));
			m_stateSaver.reset(new StateSaver());
		}

		// use the back buffer address to create the render target
		if (SUCCEEDED(m_pSwapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&m_pRenderTargetTexture)))
		{
			m_pDevice->CreateRenderTargetView(m_pRenderTargetTexture, NULL, &m_pRenderTargetView);
			m_pRenderTargetTexture->Release();
			ReloadTextures();
		}

		windowSize = GetResolution();
		windowedState = m_windowedMode;
	}
}

void DX11Hook::Draw()
{
	if (m_pSwapchain)
	{
		CreateTextures();

		if (!DrawTextureArray.empty())
		{
			m_pSpriteBatch->Begin(SpriteSortMode::SpriteSortMode_BackToFront, m_pCommonState->NonPremultiplied());
			m_restoreState = m_stateSaver->saveCurrentState(m_pContext);
			m_pBatch->Begin();

			for (auto& t : DrawTextureArray) {
                if (t.tex.bEnabled) t.tex.Draw(*m_pSpriteBatch);
            }

			m_pBatch->End();
			m_pSpriteBatch->End();

			if (m_restoreState)
                m_stateSaver->restoreSavedState();
		}

		for (auto & function : m_PresentCallbacks) {
			m_restoreState = m_stateSaver->saveCurrentState(m_pContext);
			function(m_pSwapchain);
			if (m_restoreState)
				m_stateSaver->restoreSavedState();
		}
	}
}

Vector2 DX11Hook::GetResolution()
{
	D3D11_VIEWPORT vp;
	UINT nvp = 1;
	m_pContext->RSGetViewports(&nvp, &vp);
	return Vector2(vp.Width, vp.Height);
}

//====================================================================================================================================================================
//Custom Texture loading via WIC

void DX11Hook::CreateTextures()
{
	for (auto it = CreateTextureArray.begin(); it != CreateTextureArray.end(); ++it)
	{
		DXTEXTURE2D t;
		if (SUCCEEDED(CreateWICTextureFromFile(m_pDevice, it->second.c_str(), t.resource.GetAddressOf(), t.view.GetAddressOf(), 0)))
		{
			t.resource.As(&t.texture);
			t.texture->GetDesc(&t.desc);
			ScriptTex Temptex;
			Temptex.id = it->first;
			Temptex.tex = t;
			DrawTextureArray.push_back(Temptex);
			ReloadTex tempR;
			tempR.id = it->first;
			tempR.name = it->second;
			ReloadArray.push_back(tempR);
		}
		else
		{
			std::string string = Utility::wstr_to_str(it->second);
			LOG_ERROR("Unable to Load Texture %s = %d", string.c_str(), it->first);
		}
	}

	CreateTextureArray.clear();
}

void DX11Hook::ReloadTextures()
{
	for (auto it : ReloadArray)
	{
		DXTEXTURE2D t;
		if (SUCCEEDED(CreateWICTextureFromFile(m_pDevice, it.name.c_str(), t.resource.GetAddressOf(), t.view.GetAddressOf(), 0)))
		{
			t.resource.As(&t.texture);
			t.texture->GetDesc(&t.desc);
			ScriptTex Temptex;
			Temptex.id = it.id;
			Temptex.tex = t;
			DrawTextureArray.push_back(Temptex);
		}
		else
		{
			std::string string = Utility::wstr_to_str(it.name);
			LOG_ERROR("Unable to Load Texture %s = %d", string.c_str(), it.id);
		}
	}
}

int DX11Hook::CreateTexture(const char *texFileName)
{
	// convert the filename from UTF-8...
	std::string fileNameStr = texFileName;
	std::wstring passedFileName = Utility::str_to_wstr(fileNameStr);
	std::wstring retFileName = passedFileName;

	// then, try finding the requested file.
	bool found = false;

	// absolute path
	if (passedFileName[1] == L':' || passedFileName[0] == '\\')
	{
		LOG_PRINT("File found %s", fileNameStr.c_str());
		found = true;
	}

	// not found at all?
	if (!found)
	{
		LOG_ERROR("File not found %s", fileNameStr.c_str());
		return -1;
	}

	DrawTextureArrayIndex++;
	CreateTextureArray[DrawTextureArrayIndex] = retFileName;
	return DrawTextureArrayIndex;
}

void DX11Hook::DrawTexture(int id, int index, int level, int time, float sizeX, float sizeY, float centerX, float centerY, float posX, float posY, float rotation, float screenHeightScaleFactor, float r, float g, float b, float a)
{
	for (auto& t : DrawTextureArray)
	{
		if (t.id == id)
			t.tex.SetProperties(id, index, level, time, sizeX, sizeY, centerX, centerY, posX, posY, rotation, screenHeightScaleFactor, r, g, b, a);
	}
}

//====================================================================================================================================================================
//State Saver Class

// Construct
StateSaver::StateSaver() :
	m_savedState(false),
	m_featureLevel(D3D_FEATURE_LEVEL_11_0),
	m_pContext(NULL),
	m_primitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED),
	m_pInputLayout(NULL),
	m_pBlendState(NULL),
	m_sampleMask(0xffffffff),
	m_pDepthStencilState(NULL),
	m_stencilRef(0),
	m_pRasterizerState(NULL),
	m_pPSSRV(NULL),
	m_pSamplerState(NULL),
	m_pVS(NULL),
	m_numVSClassInstances(0),
	m_pVSConstantBuffer(NULL),
	m_pGS(NULL),
	m_numGSClassInstances(0),
	m_pGSConstantBuffer(NULL),
	m_pGSSRV(NULL),
	m_pPS(NULL),
	m_numPSClassInstances(0),
	m_pHS(NULL),
	m_numHSClassInstances(0),
	m_pDS(NULL),
	m_numDSClassInstances(0),
	m_pVB(NULL),
	m_vertexStride(0),
	m_vertexOffset(0),
	m_pIndexBuffer(NULL),
	m_indexFormat(DXGI_FORMAT_UNKNOWN),
	m_indexOffset(0)
{
	for (int i = 0; i < 4; ++i)
	{
		m_blendFactor[i] = 0.0f;
	}
	for (int i = 0; i < 256; ++i)
	{
		m_pVSClassInstances[i] = NULL;
		m_pGSClassInstances[i] = NULL;
		m_pPSClassInstances[i] = NULL;
		m_pHSClassInstances[i] = NULL;
		m_pDSClassInstances[i] = NULL;
	}
}


// Destruct
StateSaver::~StateSaver()
{
	releaseSavedState();
}


// Save all states that are changed by the font-wrapper when drawing a string
bool StateSaver::saveCurrentState(ID3D11DeviceContext *pContext)
{
	if (m_savedState) releaseSavedState();
	if (pContext == NULL) return false;

	ID3D11Device *pDevice;
	pContext->GetDevice(&pDevice);
	if (pDevice != NULL) {
		m_featureLevel = pDevice->GetFeatureLevel();
		pDevice->Release();
	}

	pContext->AddRef();
	m_pContext = pContext;

	m_pContext->IAGetPrimitiveTopology(&m_primitiveTopology);
	m_pContext->IAGetInputLayout(&m_pInputLayout);

	m_pContext->OMGetBlendState(&m_pBlendState, m_blendFactor, &m_sampleMask);
	m_pContext->OMGetDepthStencilState(&m_pDepthStencilState, &m_stencilRef);

	m_pContext->RSGetState(&m_pRasterizerState);

	m_numVSClassInstances = 256;
	m_pContext->VSGetShader(&m_pVS, m_pVSClassInstances, &m_numVSClassInstances);
	m_pContext->VSGetConstantBuffers(0, 1, &m_pVSConstantBuffer);

	m_numPSClassInstances = 256;
	m_pContext->PSGetShader(&m_pPS, m_pPSClassInstances, &m_numPSClassInstances);
	m_pContext->PSGetShaderResources(0, 1, &m_pPSSRV);
	pContext->PSGetSamplers(0, 1, &m_pSamplerState);

	if (m_featureLevel >= D3D_FEATURE_LEVEL_10_0)
	{
		m_numGSClassInstances = 256;
		m_pContext->GSGetShader(&m_pGS, m_pGSClassInstances, &m_numGSClassInstances);
		m_pContext->GSGetConstantBuffers(0, 1, &m_pGSConstantBuffer);

		m_pContext->GSGetShaderResources(0, 1, &m_pGSSRV);

		if (m_featureLevel >= D3D_FEATURE_LEVEL_11_0)
		{
			m_numHSClassInstances = 256;
			m_pContext->HSGetShader(&m_pHS, m_pHSClassInstances, &m_numHSClassInstances);

			m_numDSClassInstances = 256;
			m_pContext->DSGetShader(&m_pDS, m_pDSClassInstances, &m_numDSClassInstances);
		}
	}

	m_pContext->IAGetVertexBuffers(0, 1, &m_pVB, &m_vertexStride, &m_vertexOffset);

	m_pContext->IAGetIndexBuffer(&m_pIndexBuffer, &m_indexFormat, &m_indexOffset);

	m_savedState = true;

	return true;
}


// Restore state
bool StateSaver::restoreSavedState()
{
	if (!m_savedState) return false;

	m_pContext->IASetPrimitiveTopology(m_primitiveTopology);
	m_pContext->IASetInputLayout(m_pInputLayout);

	m_pContext->OMSetBlendState(m_pBlendState, m_blendFactor, m_sampleMask);
	m_pContext->OMSetDepthStencilState(m_pDepthStencilState, m_stencilRef);

	m_pContext->RSSetState(m_pRasterizerState);

	m_pContext->VSSetShader(m_pVS, m_pVSClassInstances, m_numVSClassInstances);
	m_pContext->VSSetConstantBuffers(0, 1, &m_pVSConstantBuffer);

	m_pContext->PSSetShader(m_pPS, m_pPSClassInstances, m_numPSClassInstances);
	m_pContext->PSSetShaderResources(0, 1, &m_pPSSRV);
	m_pContext->PSSetSamplers(0, 1, &m_pSamplerState);

	if (m_featureLevel >= D3D_FEATURE_LEVEL_10_0)
	{
		m_pContext->GSSetShader(m_pGS, m_pGSClassInstances, m_numGSClassInstances);
		m_pContext->GSSetConstantBuffers(0, 1, &m_pGSConstantBuffer);

		m_pContext->GSSetShaderResources(0, 1, &m_pGSSRV);

		if (m_featureLevel >= D3D_FEATURE_LEVEL_11_0)
		{
			m_pContext->HSSetShader(m_pHS, m_pHSClassInstances, m_numHSClassInstances);

			m_pContext->DSSetShader(m_pDS, m_pDSClassInstances, m_numDSClassInstances);
		}
	}

	m_pContext->IASetVertexBuffers(0, 1, &m_pVB, &m_vertexStride, &m_vertexOffset);

	m_pContext->IASetIndexBuffer(m_pIndexBuffer, m_indexFormat, m_indexOffset);

	return true;
}


// Release state
void StateSaver::releaseSavedState()
{
	m_primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
	Utility::SafeRelease(m_pInputLayout);
	Utility::SafeRelease(m_pBlendState);
	for (int i = 0; i < 4; ++i)
		m_blendFactor[i] = 0.0f;
	m_sampleMask = 0xffffffff;
	Utility::SafeRelease(m_pDepthStencilState);
	m_stencilRef = 0;
	Utility::SafeRelease(m_pRasterizerState);
	Utility::SafeRelease(m_pPSSRV);
	Utility::SafeRelease(m_pSamplerState);
	Utility::SafeRelease(m_pVS);
	for (UINT i = 0; i < m_numVSClassInstances; ++i)
		Utility::SafeRelease(m_pVSClassInstances[i]);
	m_numVSClassInstances = 0;
	Utility::SafeRelease(m_pVSConstantBuffer);
	Utility::SafeRelease(m_pGS);
	for (UINT i = 0; i < m_numGSClassInstances; ++i)
		Utility::SafeRelease(m_pGSClassInstances[i]);
	m_numGSClassInstances = 0;
	Utility::SafeRelease(m_pGSConstantBuffer);
	Utility::SafeRelease(m_pGSSRV);
	Utility::SafeRelease(m_pPS);
	for (UINT i = 0; i < m_numPSClassInstances; ++i)
		Utility::SafeRelease(m_pPSClassInstances[i]);
	m_numPSClassInstances = 0;
	Utility::SafeRelease(m_pHS);
	for (UINT i = 0; i < m_numHSClassInstances; ++i)
		Utility::SafeRelease(m_pHSClassInstances[i]);
	m_numHSClassInstances = 0;
	Utility::SafeRelease(m_pDS);
	for (UINT i = 0; i < m_numDSClassInstances; ++i)
		Utility::SafeRelease(m_pDSClassInstances[i]);
	m_numDSClassInstances = 0;
	Utility::SafeRelease(m_pVB);
	m_vertexStride = 0;
	m_vertexOffset = 0;
	Utility::SafeRelease(m_pIndexBuffer);
	m_indexFormat = DXGI_FORMAT_UNKNOWN;
	m_indexOffset = 0;

	Utility::SafeRelease(m_pContext);
	m_featureLevel = D3D_FEATURE_LEVEL_11_0;

	m_savedState = false;
}
