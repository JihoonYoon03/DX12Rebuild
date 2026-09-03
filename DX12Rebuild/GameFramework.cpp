#include "pch.h"
#include "GameFramework.h"

CGameFramework::CGameFramework()
{
	m_nRTVDescIncrementSize = 0;
	m_nDSVDescIncrementSize = 0;

	m_hdFenceEvent = NULL;
	for (int i = 0; i < m_nSwapChainBuffers; ++i) m_nFenceValues[i] = 0;

	m_nClientW = config::FRAME_BUFFER_W;
	m_nClientH = config::FRAME_BUFFER_H;

	m_wsTitle = L"DX12 Base Framework (";
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
	D3D12_CPU_DESCRIPTOR_HANDLE RTVDescHandle = m_cpRTVDescHeap->GetCPUDescriptorHandleForHeapStart();
	HRESULT hr;
	for (UINT i = 0; i < m_nSwapChainBuffers; ++i) {
		hr = m_cpdxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(m_cpSwapChainBackBuffers[i].ReleaseAndGetAddressOf()));
		if (FAILED(hr))
		{
			OutputDebugString(L"SwapChainGetBufferFailed\n");
			continue;
		}
		m_cpDevice->CreateRenderTargetView(m_cpSwapChainBackBuffers[i].Get(), nullptr, RTVDescHandle);
		RTVDescHandle.ptr += m_nRTVDescIncrementSize;
	}
}

void CGameFramework::CreateDSV()
{
	D3D12_RESOURCE_DESC ResourceDesc;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ResourceDesc.Alignment = 0;
	ResourceDesc.Width = m_nClientW;
	ResourceDesc.Height = m_nClientH;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES HeapProperties;
	::ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.CreationNodeMask = 1;
	HeapProperties.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	ClearValue.DepthStencil.Depth = 1.0f;
	ClearValue.DepthStencil.Stencil = 0;

	HRESULT hr = m_cpDevice->CreateCommittedResource(
		&HeapProperties, D3D12_HEAP_FLAG_NONE,
		&ResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&ClearValue, IID_PPV_ARGS(m_cpDepthStencilBuffer.ReleaseAndGetAddressOf()));
	if (FAILED(hr)) {
		OutputDebugString(L"CreateDepthStencilBufferFailed\n");
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DSVDescHandle = m_cpDSVDescHeap->GetCPUDescriptorHandleForHeapStart();
	m_cpDevice->CreateDepthStencilView(m_cpDepthStencilBuffer.Get(), nullptr, DSVDescHandle);
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
	m_timer.Tick(0.0f);

	ProcessInput();

	AnimateObjects();

	HRESULT hr = m_cpCommandAllocator->Reset();
	if (FAILED(hr)) {
		OutputDebugString(L"CommandAllocatorResetFailed\n");
	}
	hr = m_cpCommandList->Reset(m_cpCommandAllocator.Get(), NULL);
	if (FAILED(hr)) {
		OutputDebugString(L"CommandListResetFailed\n");
	}

	D3D12_RESOURCE_BARRIER ResourceBarrier;
	ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarrier.Transition.pResource = m_cpSwapChainBackBuffers[m_nSwapChainBufferIndex].Get();
	ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_cpCommandList->ResourceBarrier(1, &ResourceBarrier);

	D3D12_CPU_DESCRIPTOR_HANDLE RTVDescHandle = m_cpRTVDescHeap->GetCPUDescriptorHandleForHeapStart();
	RTVDescHandle.ptr += (m_nSwapChainBufferIndex * m_nRTVDescIncrementSize);
	D3D12_CPU_DESCRIPTOR_HANDLE DSVDescHandle = m_cpDSVDescHeap->GetCPUDescriptorHandleForHeapStart();
	m_cpCommandList->OMSetRenderTargets(1, &RTVDescHandle, TRUE, &DSVDescHandle);
	m_cpCommandList->ClearRenderTargetView(RTVDescHandle, Colors::Azure, 0, NULL);
	m_cpCommandList->ClearDepthStencilView(DSVDescHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

	//Scene Render Here
	//

	//Player Render Here
	//

	ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	ResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_cpCommandList->ResourceBarrier(1, &ResourceBarrier);

	hr = m_cpCommandList->Close();
	if (FAILED(hr)) {
		OutputDebugString(L"CommandListClosingFailed\n");
	}

	ComPtr<ID3D12CommandList> cpCommandList[] = { m_cpCommandList.Get() };
	m_cpCommandQueue->ExecuteCommandLists(1, cpCommandList->GetAddressOf());
	WaitForGPUComplete();

	m_cpdxgiSwapChain->Present(0, 0);

	MoveToNextFrame();

	m_timer.GetFrameRate(m_wsTitle);
	::SetWindowText(m_hWnd, m_wsTitle.c_str());
}

void CGameFramework::WaitForGPUComplete()
{
	UINT64 FenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];

	HRESULT hr = m_cpCommandQueue->Signal(m_cpFence.Get(), FenceValue);
	if (FAILED(hr)) {
		OutputDebugString(L"FenceValueSettingFailed\n");
	}
	else {
		if (m_cpFence->GetCompletedValue() < FenceValue) {
			hr = m_cpFence->SetEventOnCompletion(FenceValue, m_hdFenceEvent);
			if (FAILED(hr)) {
				OutputDebugString(L"FenceEventSetFailed\n");
			}
			::WaitForSingleObject(m_hdFenceEvent, INFINITE);
		}
	}
}

void CGameFramework::ChangeSwapChainState()
{
}

void CGameFramework::MoveToNextFrame()
{
	m_nSwapChainBufferIndex = m_cpdxgiSwapChain->GetCurrentBackBufferIndex();

	UINT64 FenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hr = m_cpCommandQueue->Signal(m_cpFence.Get(), FenceValue);
	if (m_cpFence->GetCompletedValue() < FenceValue) {
		hr = m_cpFence->SetEventOnCompletion(FenceValue, m_hdFenceEvent);
		if (FAILED(hr)) {
			OutputDebugString(L"FenceEventSetFailed\n");
		}
		::WaitForSingleObject(m_hdFenceEvent, INFINITE);
	}
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

