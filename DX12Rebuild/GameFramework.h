#pragma once

#include "Timer.h"

class CGameFramework
{
public:
	CGameFramework();
	~CGameFramework();

	bool OnCreate(HINSTANCE hInstance, HWND hWnd);
	void OnDestroy();

	void CreateSwapChain();
	void CreateRTVDSVDescHeaps();
	void CreateDevice();
	void CreateCommandQueueList();

	void CreateRTV();
	void CreateDSV();

	void BuildObjects();
	void ReleaseObjects();

	void ProcessInput();
	void AnimateObjects();
	void FrameAdvance();

	void WaitForGPUComplete();

	void ChangeSwapChainState();

	void MoveToNextFrame();

	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	POINT m_ptOldCursorPos;

private:
	HINSTANCE	m_hInstance;
	HWND		m_hWnd;

	int			m_nClientW;
	int			m_nClientH;

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

	//DepthStencil
	ComPtr<ID3D12Resource>				m_cpDepthStencilBuffer;
	ComPtr<ID3D12DescriptorHeap>		m_cpDSVDescHeap;
	UINT								m_nDSVDescIncrementSize;

	//CommandQueue/Allocator/List
	ComPtr<ID3D12CommandQueue>			m_cpCommandQueue;
	ComPtr<ID3D12CommandAllocator>		m_cpCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList>	m_cpCommandList;

	//PSO
	ComPtr<ID3D12PipelineState>			m_cpPipelineState;                                               

	//Fence
	ComPtr<ID3D12Fence>					m_cpFence;
	UINT64								m_nFenceValue[m_nSwapChainBuffers];
	HANDLE								m_hdFenceEvent;

	//Timer
	CTimer m_timer;

	//Title Text
	std::wstring title;

	//Scene

	
};

