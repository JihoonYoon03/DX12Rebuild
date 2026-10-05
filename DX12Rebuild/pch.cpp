#include "pch.h"

UINT gnCbvSrvDescriptorIncrementSize = 0;

HRESULT CreateBufferResource(
	const ComPtr<ID3D12Device>& cpDevice,
	const ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
	ComPtr<ID3D12Resource>& buffer,
	const void* pData,
	size_t dataSize,
    D3D12_HEAP_TYPE heapType,
	D3D12_RESOURCE_STATES resourceStates,
    ComPtr<ID3D12Resource>* pUploadBuffer
	)
{
    D3D12_HEAP_PROPERTIES heapProperties;
    ::ZeroMemory(&heapProperties, sizeof(D3D12_HEAP_PROPERTIES));
    heapProperties.Type = heapType;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resDesc;
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Alignment = 0;
    resDesc.Width = dataSize;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_UNKNOWN;
    resDesc.SampleDesc.Count = 1;
    resDesc.SampleDesc.Quality = 0;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    //리소스 초기 상태 설정
    //디폴트: 생성될 버퍼가 COPY_DEST임
    //업로드: 생성될 버퍼가 GENERIC_READ임
    //리드백: 생성될 버퍼가 COPY_DEST임
    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COPY_DEST;
    if (heapType == D3D12_HEAP_TYPE_UPLOAD) initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    else if (heapType == D3D12_HEAP_TYPE_READBACK) initialState = D3D12_RESOURCE_STATE_COPY_DEST;

    //버퍼 생성
	HRESULT hr = cpDevice->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		initialState,
		nullptr,
		IID_PPV_ARGS(buffer.ReleaseAndGetAddressOf())
	);
	if (FAILED(hr)) { OutputDebugString(L"CreateCommittedResource(): CreateCommittedResource() Failed\n"); return hr; }

    if (pData)
    {
        switch (heapType)
        {
        case D3D12_HEAP_TYPE_DEFAULT:
        {
            //업로드 버퍼 생성
            heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
            hr = cpDevice->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(pUploadBuffer->ReleaseAndGetAddressOf())
            );
            if (FAILED(hr)) { OutputDebugString(L"CreateCommittedResource(): CreateCommittedResource() Failed\n"); return hr; }

            //업로드 버퍼에 데이터 업로드
            D3D12_RANGE range = { 0, 0 };
            UINT8* dataBegin = nullptr;
            hr = (*pUploadBuffer)->Map(0, &range, (void**)&dataBegin);
            if (FAILED(hr)) { OutputDebugString(L"CreateCommittedResource(): CreateCommittedResource() Failed\n"); return hr; }
            memcpy(dataBegin, pData, dataSize);
            (*pUploadBuffer)->Unmap(0, NULL);

            //디폴트 버퍼에 데이터 복사
            cpCommandList->CopyResource(buffer.Get(), pUploadBuffer->Get());

            //리소스 배리어(DEFAULT 버퍼에 대한)
            //이전 상태는 COPY_DEST
            //정점/상수 버퍼 상태로 전환
            D3D12_RESOURCE_BARRIER resBarrier;
            ::ZeroMemory(&resBarrier, sizeof(D3D12_RESOURCE_BARRIER));
            resBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            resBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            resBarrier.Transition.pResource = buffer.Get();
            resBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            resBarrier.Transition.StateBefore = initialState;
            resBarrier.Transition.StateAfter = resourceStates;
            cpCommandList->ResourceBarrier(1, &resBarrier);
            break;
        }

        case D3D12_HEAP_TYPE_UPLOAD:
        {
            D3D12_RANGE range = { 0, 0 };
            UINT8* dataBegin = nullptr;
            hr = buffer->Map(0, &range, (void**)&dataBegin);
            if (FAILED(hr)) { OutputDebugString(L"CreateCommittedResource(): CreateCommittedResource() Failed\n"); return hr; }
            memcpy(dataBegin, pData, dataSize);
            buffer->Unmap(0, NULL);
            break;
        }

        case D3D12_HEAP_TYPE_READBACK:
        {
            break;
        }
        default:
            break;
        }
    }

	return S_OK;
}