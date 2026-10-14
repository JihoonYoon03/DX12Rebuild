#pragma once

struct SRVDescHeapInfo
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	UINT						index;
};

//=========================================================================================
//=========================================================================================
class CSRVDescHeapAllocator
{
public:
	CSRVDescHeapAllocator();
	HRESULT CreateSRVDescHeap(const ComPtr<ID3D12Device>& cpDevice);

	HRESULT AllocateHeapSlot(SRVDescHeapInfo& srvDescHeapInfo);

	ID3D12DescriptorHeap* const* GetDescriptorHeap() { return m_cpHeap.GetAddressOf(); }

private:
	ComPtr<ID3D12DescriptorHeap>	m_cpHeap;
	UINT							m_nSlots;
	UINT							m_nNextIndex;
	UINT							m_nDescHeapIncrementSize;

};

//=========================================================================================
//=========================================================================================
class CTexture
{
public:
	CTexture();

	HRESULT CreateTexture(
		CSRVDescHeapAllocator& allocator,
		const ComPtr<ID3D12Device>& cpDevice,
		const ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
		const wchar_t* fileName);

	const SRVDescHeapInfo getDescHeapInfo() { return m_SRVDescHeapInfo; }
	D3D12_CPU_DESCRIPTOR_HANDLE getCPUDescHandle() { return m_SRVDescHeapInfo.cpuHandle; }
	D3D12_GPU_DESCRIPTOR_HANDLE getGPUDescHandle() { return m_SRVDescHeapInfo.gpuHandle; }

	void ReleaseUploadBuffer();

private:
	SRVDescHeapInfo			m_SRVDescHeapInfo;
	ComPtr<ID3D12Resource>	m_cpBuffer;
	ComPtr<ID3D12Resource>	m_cpUploadBuffer;
};