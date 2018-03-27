#ifndef _DXHOOK_H_
#define _DXHOOK_H_

#include "..\ScriptHookV.h"
#include <d3d11.h>
#include <d3d11_1.h>

#include <Model.h>
#include <CommonStates.h>
#include <DDSTextureLoader.h>
#include <DirectXHelpers.h>
#include <Effects.h>
#include <GamePad.h>
#include <GeometricPrimitive.h>
#include <PrimitiveBatch.h>
#include <ScreenGrab.h>
#include <SimpleMath.h>
#include <SpriteFont.h>
#include <SpriteBatch.h>
#include <VertexTypes.h>
#include <WICTextureLoader.h>

using namespace DirectX;
using namespace Microsoft::WRL;

typedef void(*PresentCallback)(void *);

class StateSaver
{
	// Public functions
public:
	StateSaver();
	~StateSaver();

	bool saveCurrentState(ID3D11DeviceContext *pContext);
	bool restoreSavedState();
	void releaseSavedState();

	// Internal data
private:
	bool						m_savedState;
	D3D_FEATURE_LEVEL			m_featureLevel;
	ID3D11DeviceContext			*m_pContext;
	D3D11_PRIMITIVE_TOPOLOGY	m_primitiveTopology;
	ID3D11InputLayout			*m_pInputLayout;
	ID3D11BlendState			*m_pBlendState;
	FLOAT						m_blendFactor[4];
	UINT						m_sampleMask;
	ID3D11DepthStencilState		*m_pDepthStencilState;
	UINT						m_stencilRef;
	ID3D11RasterizerState		*m_pRasterizerState;
	ID3D11ShaderResourceView	*m_pPSSRV;
	ID3D11SamplerState			*m_pSamplerState;
	ID3D11VertexShader			*m_pVS;
	ID3D11ClassInstance			*m_pVSClassInstances[256];
	UINT						m_numVSClassInstances;
	ID3D11Buffer				*m_pVSConstantBuffer;
	ID3D11GeometryShader		*m_pGS;
	ID3D11ClassInstance			*m_pGSClassInstances[256];
	UINT						m_numGSClassInstances;
	ID3D11Buffer				*m_pGSConstantBuffer;
	ID3D11ShaderResourceView	*m_pGSSRV;
	ID3D11PixelShader			*m_pPS;
	ID3D11ClassInstance			*m_pPSClassInstances[256];
	UINT						m_numPSClassInstances;
	ID3D11HullShader			*m_pHS;
	ID3D11ClassInstance			*m_pHSClassInstances[256];
	UINT						m_numHSClassInstances;
	ID3D11DomainShader			*m_pDS;
	ID3D11ClassInstance			*m_pDSClassInstances[256];
	UINT						m_numDSClassInstances;
	ID3D11Buffer				*m_pVB;
	UINT						m_vertexStride;
	UINT						m_vertexOffset;
	ID3D11Buffer				*m_pIndexBuffer;
	DXGI_FORMAT					m_indexFormat;
	UINT						m_indexOffset;

	StateSaver(const StateSaver&);
};

class DX11Hook
{
public:
    DX11Hook()
    { }

	void Draw();
	bool InitializeHooks();
	void InitializeDevices();

	Vector2 GetResolution();
	IDXGISwapChain* GetSwapChain() { return m_pSwapchain;}
	void SetSwapChain(IDXGISwapChain* _swapChain) { m_pSwapchain = _swapChain; }

	int CreateTexture(const char *texFileName);
	void DrawTexture(int id, int index, int level, int time, float sizeX, float sizeY, float centerX, float centerY, float posX, float posY, float rotation, float screenHeightScaleFactor, float r, float g, float b, float a);
    bool AddCallback(PresentCallback callback) { return m_PresentCallbacks.insert(callback).second; }
    bool RemoveCallback(PresentCallback callback) { return m_PresentCallbacks.erase(callback) != 0; }
	bool IsResizing() { return m_IsResizing; }
	void SetResizing(bool value) { m_IsResizing = value; }

private:
	void CreateTextures();
	void ReloadTextures();
protected:
	IDXGISwapChain*				m_pSwapchain = nullptr;
    ID3D11Device*				m_pDevice = nullptr;
    ID3D11DeviceContext*		m_pContext = nullptr;
	ID3D11RenderTargetView*		m_pRenderTargetView = nullptr;
	ID3D11Texture2D*			m_pRenderTargetTexture = nullptr;
    std::set<PresentCallback>	m_PresentCallbacks;

    std::unique_ptr<PrimitiveBatch<VertexPositionColor>> m_pBatch;
	std::unique_ptr<SpriteBatch> m_pSpriteBatch;
	std::unique_ptr<CommonStates> m_pCommonState;
	std::unique_ptr<StateSaver> m_stateSaver;
	bool m_restoreState = false;
	bool m_IsResizing = false;
};

extern DX11Hook g_D3DHook;

class DXTEXTURE2D
{
private:
	int id;
	int index;
	int level;
	int time;
	float sizeX, sizeY;
	float centerX, centerY;
	float posX, posY;
	float rotation;
	float screenHeightScaleFactor; // Not used
	float r, g, b, a;
public:
	bool bEnabled;
	DWORD64 disableTime;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	view;
	Microsoft::WRL::ComPtr<ID3D11Resource>				resource;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>				texture;
	CD3D11_TEXTURE2D_DESC								desc;

	DXTEXTURE2D() : bEnabled(false), disableTime(0)
	{
		desc.Width = 256;
		desc.Height = 256;
		desc.MipLevels = 0;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET);
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	}
	float scale_image(const float width, const float height, const float scalex, const float scaley)
	{
		float ret = 1.0f;
		if (width > height)
			ret = width * scalex;
		if (height > width)
			ret = height * scaley;
		return ret;
	}
	void Draw(DirectX::SpriteBatch& pSpriteBatch)
	{
		DirectX::XMVECTOR colour = { r,g,b,a };
		pSpriteBatch.Draw(view.Get(),
			DirectX::XMFLOAT2(posX * g_D3DHook.GetResolution().x, posY * g_D3DHook.GetResolution().y),
			nullptr,
			colour,
			(rotation * 2 * float(PI)),
			DirectX::XMFLOAT2(centerX * desc.Width, centerY * desc.Height),
			scale_image(g_D3DHook.GetResolution().x / desc.Width, g_D3DHook.GetResolution().y / desc.Height, sizeX * 2, sizeY * 2),
			DirectX::SpriteEffects::SpriteEffects_None);
			//float(9 - (std::abs(index) % 10)) / 10);
		if (GetTickCount64() > disableTime) bEnabled = false;
	}

	void SetProperties(int id_, int index_, int level_, int time_, float sizeX_, float sizeY_, float centerX_, float centerY_, float posX_, float posY_, float rotation_, float screenHeightScaleFactor_, float r_, float g_, float b_, float a_)
	{
		bEnabled = true;
		id = id_;
		index = index_;
		level = level_;
		time = time_;
		sizeX = sizeX_;
		sizeY = sizeY_;
		centerX = centerX_;
		centerY = centerY_;
		posX = posX_;
		posY = posY_;
		rotation = rotation_;
		screenHeightScaleFactor = screenHeightScaleFactor_;
		r = r_;
		g = g_;
		b = b_;
		a = a_;
		disableTime = GetTickCount64() + time;
	}
};

#endif //_DXHOOK_H_
