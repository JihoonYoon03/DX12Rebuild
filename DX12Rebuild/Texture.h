#pragma once

class CTexture
{
public:
	CTexture();

	HRESULT CreateTexture(
		CDescHeapAllocator& allocator,
		const ComPtr<ID3D12Device>& cpDevice,
		const ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
		const wchar_t* fileName,
		bool isWIC = false);

	const DescHeapInfo getDescHeapInfo() { return m_SRVDescHeapInfo; }
	D3D12_CPU_DESCRIPTOR_HANDLE getCPUDescHandle() { return m_SRVDescHeapInfo.cpuHandle; }
	D3D12_GPU_DESCRIPTOR_HANDLE getGPUDescHandle() { return m_SRVDescHeapInfo.gpuHandle; }

	void ReleaseUploadBuffer();

private:
	DescHeapInfo			m_SRVDescHeapInfo;
	ComPtr<ID3D12Resource>	m_cpBuffer;
	ComPtr<ID3D12Resource>	m_cpUploadBuffer;
};

//----------------------------------------------
//
// HeightMapImage
// 
//----------------------------------------------
class CHeightMapImage
{
public:
	CHeightMapImage(LPCTSTR pFileName, int nWidth, int nLength);
	~CHeightMapImage();

	float GetHeight(float fx, float fz);

	const std::vector<BYTE>& GetHeightMapPixels() { return m_pHeightMapPixels; }

	int GetHeightMapWidth() const { return m_nWidth; }
	int GetHeightMapLength() const { return m_nLength; }

private:
	std::vector<BYTE>	m_pHeightMapPixels;

	int			m_nWidth;
	int			m_nLength;
};