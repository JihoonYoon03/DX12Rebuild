#include "pch.h"
#include "GameFramework.h"

CGameFramework::CGameFramework()
{
}

CGameFramework::~CGameFramework()
{
}

bool CGameFramework::OnCreate(HINSTANCE hInstance, HWND hWnd)
{
	m_hInstance = hInstance;
	m_hWnd = hWnd;

	CreateDevice();
	CreateCommandQueueList();
	CreateRTVDSVDescHeaps();
	CreateSwapChain();
	//CreateRTV();
	CreateDSV();

	BuildObjects();

	return true;
}

void CGameFramework::OnDestroy()
{
	WaitForGPUComplete();

	ReleaseObjects();

	::CloseHandle(m_hdFenceEvent);

	m_cpdxgiSwapChain->SetFullscreenState(false, nullptr);

#ifdef _DEBUG
	ComPtr<IDXGIDebug1> pdxgiDebug;
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(pdxgiDebug.GetAddressOf()));
	HRESULT hr = pdxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
#endif
}

void CGameFramework::CreateSwapChain()
{
	RECT rc;
	::GetClientRect(m_hWnd, &rc);

	m_nClientW = rc.right - rc.left;
	m_nClientH = rc.bottom - rc.top;

	//swapchain
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	::ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
	swapChainDesc.BufferCount = m_nSwapChainBuffers;
	swapChainDesc.BufferDesc.Width = m_nClientW;
	swapChainDesc.BufferDesc.Height = m_nClientH;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = m_hWnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = true;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ComPtr<IDXGISwapChain> SwapChain;
	HRESULT hr = m_cpdxgiFactory->CreateSwapChain(m_cpDevice.Get(), &swapChainDesc, SwapChain.GetAddressOf());
	if (FAILED(hr)) {
		OutputDebugString(L"SwapChainCreationFailed\n");
	}

	hr = SwapChain.As(&m_cpdxgiSwapChain);
	if (FAILED(hr)) {
		OutputDebugString(L"SwapChainCastingFailed\n");
	}

	m_nSwapChainBufferIndex = m_cpdxgiSwapChain->GetCurrentBackBufferIndex();

	hr = m_cpdxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);
	if (FAILED(hr)) {
		OutputDebugString(L"FullscreenShortcutDisablingFailed\n");
	}

#ifndef _WITH_SWAPCHAIN_FULLSCREEN_STATE
	CreateRTV();
#endif
}

void CGameFramework::CreateRTVDSVDescHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = m_nSwapChainBuffers;

	HRESULT hr = m_cpDevice->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(m_cpRTVDescHeap.GetAddressOf()));
	if (FAILED(hr)) {
		OutputDebugString(L"RTVDescHeapCreationFailed\n");
	}
	m_nRTVDescIncrementSize = m_cpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);


	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	hr = m_cpDevice->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(m_cpDSVDescHeap.GetAddressOf()));
	if (FAILED(hr)) {
		OutputDebugString(L"DSVDescHeapCreationFailed\n");
	}
	m_nDSVDescIncrementSize = m_cpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

}

void CGameFramework::CreateDevice()
{
	HRESULT hr;
	UINT nDXGIFactoryFlags = 0;

#ifdef _DEBUG
	ComPtr<ID3D12Debug1> pDebug;
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(pDebug.GetAddressOf()));
	if (pDebug) {
		pDebug->EnableDebugLayer();
	}
	nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	hr = ::CreateDXGIFactory2(nDXGIFactoryFlags, IID_PPV_ARGS(m_cpdxgiFactory.GetAddressOf()));

	if (FAILED(hr)) {
		OutputDebugString(L"FactoryCreationFailed\n");
	}

	ComPtr<IDXGIAdapter1> pd3dAdapter;
	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != m_cpdxgiFactory->EnumAdapters1(i, pd3dAdapter.GetAddressOf()); ++i) {
		DXGI_ADAPTER_DESC1 adapterDesc;
		pd3dAdapter->GetDesc1(&adapterDesc);
		if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(pd3dAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(m_cpDevice.GetAddressOf())))) {
			break;
		}
		else {
			OutputDebugString(L"DeviceCreationFailed\n");
		}
	}

	if (!pd3dAdapter)
	{
		m_cpdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(pd3dAdapter.GetAddressOf()));
		D3D12CreateDevice(pd3dAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(m_cpDevice.GetAddressOf()));
	}

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMsaaQualityLevels;
	d3dMsaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dMsaaQualityLevels.SampleCount = 4;
	d3dMsaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	d3dMsaaQualityLevels.NumQualityLevels = 0;
	m_cpDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &d3dMsaaQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	m_nMsaa4xQualityLevels = d3dMsaaQualityLevels.NumQualityLevels;
	m_bMsaaEnable = m_nMsaa4xQualityLevels ? true : false;

	hr = m_cpDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_cpFence.GetAddressOf()));
	if (FAILED(hr)) {
		OutputDebugString(L"FenceCreationFailed\n");
	}

	m_hdFenceEvent = ::CreateEvent(NULL, false, NULL, NULL);

	gnCbvSrvDescriptorIncrementSize = m_cpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void CGameFramework::CreateCommandQueueList()
{
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc;
	::ZeroMemory(&cmdQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	HRESULT hr = m_cpDevice->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(m_cpCommandQueue.GetAddressOf()));
	if (FAILED(hr)) {
		OutputDebugString(L"CommandQueueCreationFailed\n");
	}
	hr = m_cpDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_cpCommandAllocator.GetAddressOf()));
	if (FAILED(hr)) {
		OutputDebugString(L"CommandAllocatorCreationFailed\n");
	}
	hr = m_cpDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cpCommandAllocator.Get(), NULL, IID_PPV_ARGS(m_cpCommandList.GetAddressOf()));
	if (FAILED(hr)) {
		OutputDebugString(L"CommandListCreationFailed\n");
	}

	hr = m_cpCommandList->Close();
	if (FAILED(hr)) {
		OutputDebugString(L"CommandListCloseFailed\n");
	}
}

void CGameFramework::CreateRTV()
{
}

void CGameFramework::CreateDSV()
{
}

void CGameFramework::BuildObjects()
{
}

void CGameFramework::ReleaseObjects()
{
}

void CGameFramework::ProcessInput()
{
}

void CGameFramework::AnimateObjects()
{
}

void CGameFramework::FrameAdvance()
{
}

void CGameFramework::WaitForGPUComplete()
{
}

void CGameFramework::ChangeSwapChainState()
{
}

void CGameFramework::MoveToNextFrame()
{
}

void CGameFramework::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
}

void CGameFramework::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
}

LRESULT CGameFramework::OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	return LRESULT();
}

