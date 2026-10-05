#include "pch.h"

UINT gnCbvSrvDescriptorIncrementSize = 0;

HRESULT CreateCommittedResource(
	const ComPtr<ID3D12Device>& cpDevice,
	const ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
	D3D12_HEAP_PROPERTIES& heapProperties,
	const D3D12_RESOURCE_DESC& resDesc,
	ComPtr<ID3D12Resource>& uploadBuffer,
	ComPtr<ID3D12Resource>& buffer,
	const void* pData,
	size_t dataSize,
	D3D12_HEAP_FLAGS d3dHeapFlags,
	D3D12_RESOURCE_STATES d3dResourceStates
	)
{
	HRESULT hr = cpDevice->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		NULL,
		IID_PPV_ARGS(uploadBuffer.ReleaseAndGetAddressOf())
	);
	if (FAILED(hr)) { OutputDebugString(L"CreateCommittedResource(): CreateCommittedResource() Failed\n"); return hr; }

	D3D12_RANGE readRange = { 0, 0 };
	UINT8* bufBegin;
	hr = uploadBuffer->Map(0, &readRange, (void**)&bufBegin);
	if (FAILED(hr)) { OutputDebugString(L"CreateCommittedResource(): Map() Failed\n"); return hr; }
	::memcpy(bufBegin, pData, dataSize);
	uploadBuffer->Unmap(0, NULL);

	//디폴트 버퍼 생성
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	hr = cpDevice->CreateCommittedResource(
		&heapProperties,
		d3dHeapFlags,
		&resDesc,
		d3dResourceStates,
		NULL,
		IID_PPV_ARGS(buffer.ReleaseAndGetAddressOf())
	);
	if (FAILED(hr)) { OutputDebugString(L"CreateCommittedResource(): CreateCommittedResource() Failed\n"); return hr; }

	cpCommandList->CopyResource(buffer.Get(), uploadBuffer.Get());

	//리소스 배리어
	D3D12_RESOURCE_BARRIER resBarrier;
	::ZeroMemory(&resBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	resBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	resBarrier.Transition.pResource = buffer.Get();
	resBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	resBarrier.Transition.StateBefore = d3dResourceStates;
	resBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	cpCommandList->ResourceBarrier(1, &resBarrier);

	return S_OK;
}