#pragma once
struct VertexDiffused
{
	XMFLOAT3 pos;
	XMFLOAT4 color;
};

class CMesh {
private:
	ComPtr<ID3D12Resource>				m_cpVertexUploadBuffer;
	ComPtr<ID3D12Resource>				m_cpVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW			m_VertexBufferView;
	UINT								m_nVertices;
	D3D_PRIMITIVE_TOPOLOGY				m_topology;

public:
	CMesh();
	~CMesh();

	//다양한 정점 타입에 대한 버퍼 생성
	template<class VTYPE>
	HRESULT CreateVertexBuffer(const ComPtr<ID3D12Device>& cpDevice, const ComPtr<ID3D12GraphicsCommandList>& cpCommandList, const VTYPE* data, UINT nVertices)
	{
		if (!data || nVertices == 0) return E_INVALIDARG;

		m_nVertices = nVertices;

		//업로드 버퍼 생성
		D3D12_HEAP_PROPERTIES heapProperties;
		::ZeroMemory(&heapProperties, sizeof(D3D12_HEAP_PROPERTIES));
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProperties.CreationNodeMask = 1;
		heapProperties.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC resDesc;
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Alignment = 0;
		resDesc.Width = sizeof(VTYPE) * nVertices;
		resDesc.Height = 1;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = DXGI_FORMAT_UNKNOWN;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		HRESULT hr = cpDevice->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			NULL,
			IID_PPV_ARGS(m_cpVertexUploadBuffer.ReleaseAndGetAddressOf())
		);
		if (FAILED(hr)) { OutputDebugString(L"CreateVertexBuffer(): CreateCommittedResource() Failed\n"); return hr; }

		D3D12_RANGE readRange = { 0, 0 };
		UINT8* bufBegin;
		hr = m_cpVertexUploadBuffer->Map(0, &readRange, (void**)&bufBegin);
		if (FAILED(hr)) { OutputDebugString(L"CreateVertexBuffer(): Map() Failed\n"); return hr; }
		::memcpy(bufBegin, data, sizeof(VTYPE) * nVertices);
		m_cpVertexUploadBuffer->Unmap(0, NULL);

		//디폴트 버퍼 생성
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		hr = cpDevice->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
			&resDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			NULL,
			IID_PPV_ARGS(m_cpVertexBuffer.ReleaseAndGetAddressOf())
		);
		if (FAILED(hr)) { OutputDebugString(L"CreateVertexBuffer(): CreateCommittedResource() Failed\n"); return hr; }

		cpCommandList->CopyResource(m_cpVertexBuffer.Get(), m_cpVertexUploadBuffer.Get());

		m_VertexBufferView.BufferLocation = m_cpVertexBuffer->GetGPUVirtualAddress();
		m_VertexBufferView.SizeInBytes = sizeof(VTYPE) * nVertices;
		m_VertexBufferView.StrideInBytes = sizeof(VTYPE);

		//리소스 배리어
		D3D12_RESOURCE_BARRIER resBarrier;
		::ZeroMemory(&resBarrier, sizeof(D3D12_RESOURCE_BARRIER));
		resBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		resBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		resBarrier.Transition.pResource = m_cpVertexBuffer.Get();
		resBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		resBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		resBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		cpCommandList->ResourceBarrier(1, &resBarrier);

		return S_OK;
	}

	//업로드 버퍼 반환
	void ReleaseUploadBuffer();

	void DrawMesh(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList);
};

