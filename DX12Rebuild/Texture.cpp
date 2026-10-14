#include "pch.h"
#include "Texture.h"

CSRVDescHeapAllocator::CSRVDescHeapAllocator()
{
	m_nSlots = config::MAX_TEXTURE_SLOTS;
	m_nNextIndex = 0;
	m_nDescHeapIncrementSize = 0;
}

HRESULT CSRVDescHeapAllocator::CreateSRVDescHeap(const ComPtr<ID3D12Device>& cpDevice)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = m_nSlots;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;

	HRESULT hr = cpDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_cpHeap.GetAddressOf()));
	if (FAILED(hr)) return hr;

	m_nDescHeapIncrementSize = cpDevice->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	return S_OK;
}

HRESULT CSRVDescHeapAllocator::AllocateHeapSlot(SRVDescHeapInfo& DescHeapInfo)
{
	SRVDescHeapInfo info;

	info.cpuHandle = m_cpHeap->GetCPUDescriptorHandleForHeapStart();
	info.gpuHandle = m_cpHeap->GetGPUDescriptorHandleForHeapStart();

	if (m_nNextIndex == m_nSlots) return E_FAIL;

	info.cpuHandle.ptr += m_nDescHeapIncrementSize * m_nNextIndex;
	info.gpuHandle.ptr += m_nDescHeapIncrementSize * m_nNextIndex;
	info.index = m_nNextIndex++;

	DescHeapInfo = info;

	return S_OK;
}

//=========================================================================================
//=========================================================================================
CTexture::CTexture()
{
}

HRESULT CTexture::CreateTexture(
	CSRVDescHeapAllocator& allocator,
	const ComPtr<ID3D12Device>& cpDevice,
	const ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
	const wchar_t* fileName)
{
	std::unique_ptr<uint8_t[]> ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> subData;
	DDS_ALPHA_MODE ddsAlpha = DDS_ALPHA_MODE_UNKNOWN;
	bool bIsCubeMap = false;

	//m_cpBuffer에 디폴트 텍스처 리소스 생성
	//DDS 이미지 데이터를 GPU에 복사한 상태는 아님
	HRESULT hr = LoadDDSTextureFromFileEx(
		cpDevice.Get(),
		fileName,
		0,
		D3D12_RESOURCE_FLAG_NONE,
		DDS_LOADER_DEFAULT,
		m_cpBuffer.GetAddressOf(),
		ddsData,
		subData,
		&ddsAlpha,
		&bIsCubeMap);
	if (FAILED(hr)) return hr;

	if (subData.empty()) return E_FAIL;
	//GPU 복사용 배치를 고려한 UPLOAD 버퍼 크기 계산
	//밉맵, 압축 등 요소 때문에 API 사용
	UINT64 reqSize = GetRequiredIntermediateSize(m_cpBuffer.Get(), 0, subData.size());

	//UPLOAD BUFFER
	hr = CreateBufferResource(
		cpDevice,
		cpCommandList,
		m_cpUploadBuffer,
		nullptr,
		reqSize,
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_STATE_GENERIC_READ);
	if (FAILED(hr)) return hr;

	//텍스처 COMMON->COPY_DEST 전환
	D3D12_RESOURCE_BARRIER resBarrier;
	resBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	resBarrier.Transition.pResource = m_cpBuffer.Get();
	resBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	resBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	resBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	cpCommandList->ResourceBarrier(1, &resBarrier);

	//CPU데이터를 UPLOAD 버퍼에 배치 -> 텍스처 복사 명령 기록
	UINT64 result = UpdateSubresources(
		cpCommandList.Get(),
		m_cpBuffer.Get(),
		m_cpUploadBuffer.Get(),
		0,
		0,
		subData.size(),
		subData.data());
	if (result == 0) return E_FAIL;

	//텍스처 COPY_DEST->PIXEL_SHADER_RESOURCE 전환
	resBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	resBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	cpCommandList->ResourceBarrier(1, &resBarrier);

	//SRV 디스크립터 힙 공간 할당받기
	hr = allocator.AllocateHeapSlot(m_SRVDescHeapInfo);
	if (FAILED(hr)) return hr;

	cpDevice->CreateShaderResourceView(m_cpBuffer.Get(), nullptr, m_SRVDescHeapInfo.cpuHandle);

	return S_OK;
}

void CTexture::ReleaseUploadBuffer()
{
	m_cpUploadBuffer.Reset();
}

