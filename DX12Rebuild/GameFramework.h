#pragma once

#include "Timer.h"

class CMesh;
class CShader;
class CDescHeapAllocator;
class CTexture;
class CModel;
class CScene;
class CGameScene;
//-----------------------------------
//[아래 코드는 AI로 구현됨]
enum class EGameStage;
//-----------------------------------
//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CUiScene;
class CStageResources;
//-----------------------------------

//-----------------------------------
//[직접 작성한 코드]
class CGameFramework
{
public:
	CGameFramework();
	~CGameFramework();

	bool OnCreate(HINSTANCE hInstance, HWND hWnd);
	void OnDestroy();

	HRESULT CreateSwapChain();
	HRESULT CreateRTVDSVDescHeaps();
	HRESULT CreateDevice();
	HRESULT CreateCommandQueueList();
	void	SetViewportScissorRect(const RECT& rc);

	HRESULT CreateRTV();
	HRESULT CreateDSV();

	HRESULT BuildObjects();
	void ReleaseUploadBuffers();
	void ReleaseObjects();

	void ProcessInput();
	void AnimateObjects();
	void FrameAdvance();

	HRESULT WaitForGPUComplete();

	void ChangeSwapChainState();
	HRESULT ResizeBackBuffers(WPARAM wParam);
	void StopRendering(bool stop = true) { m_bStopRender = stop; }

	void MoveToNextFrame();

	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	POINT m_ptOldCursorPos;

	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	void OpenStageSelect();
	void OpenTutorial();
	void EnterStage(EGameStage stage);
	void RequestExit();
	//-----------------------------------

private:
	HINSTANCE	m_hInstance = NULL;
	HWND		m_hWnd		= NULL;

	int			m_nClientW;
	int			m_nClientH;

	D3D12_VIEWPORT						m_viewport;
	D3D12_RECT							m_scissorRect;

	//Factory/SwapChain/Device
	ComPtr<IDXGIFactory4>				m_cpdxgiFactory;
	ComPtr<IDXGISwapChain3>				m_cpdxgiSwapChain;
	ComPtr<ID3D12Device>				m_cpDevice;

	//MSAA
	bool								m_bMsaaEnable = true;
	UINT								m_nMsaa4xQualityLevels = 0;

	//SwapChain Config/Resource
	static const UINT					m_nSwapChainBuffers = 2;
	UINT								m_nSwapChainBufferIndex = 0;
	ComPtr<ID3D12Resource>				m_cpSwapChainBackBuffers[m_nSwapChainBuffers];
	ComPtr<ID3D12DescriptorHeap>		m_cpRTVDescHeap;
	UINT								m_nRTVDescIncrementSize;
	bool								m_bStopRender = false;

	//DepthStencil
	ComPtr<ID3D12Resource>				m_cpDepthStencilBuffer;
	ComPtr<ID3D12DescriptorHeap>		m_cpDSVDescHeap;
	UINT								m_nDSVDescIncrementSize;

	//CommandQueue/Allocator/List
	ComPtr<ID3D12CommandQueue>			m_cpCommandQueue;
	ComPtr<ID3D12CommandAllocator>		m_cpCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList>	m_cpCommandList;

	//Fence
	ComPtr<ID3D12Fence>					m_cpFence;
	UINT64								m_nFenceValues;
	HANDLE								m_hdFenceEvent;

	//Timer
	CTimer m_timer;

	//Title Text
	std::wstring m_wsTitle;

	//Scene
	ComPtr<ID3D12RootSignature>			m_cpRootSignature;
	std::weak_ptr<CScene>				m_upCurrentScene;
	std::shared_ptr<CGameScene>			m_spTankScene;
//-----------------------------------

	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	HRESULT BuildUi();
	void ResizeUi();
	void ProcessSceneMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	std::shared_ptr<CStageResources> m_spStageResources;

	std::shared_ptr<CGameScene> m_spHelicopterScene;
	std::shared_ptr<CUiScene> m_spMainMenuScene;
	std::shared_ptr<CUiScene> m_spStageSelectScene;
	std::shared_ptr<CUiScene> m_spTutorialScene;
	//-----------------------------------

//-----------------------------------
//[직접 작성한 코드]
	//Texture
	std::unique_ptr<CDescHeapAllocator>	m_upDescHeapAllocator;

	//Debug

	//Test
	
	XMFLOAT4X4	mtx44World;
	ComPtr<ID3D12Resource>				m_cpMtxBuffer;
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	UINT8*								m_mappedMtxBuffer = nullptr;
	//-----------------------------------
};
//-----------------------------------

