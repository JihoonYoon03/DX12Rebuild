#pragma once
#include "pch.h"

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

		HRESULT hr = CreateCommittedResource(
			cpDevice,
			cpCommandList,
			heapProperties,
			resDesc,
			m_cpVertexUploadBuffer,
			m_cpVertexBuffer, data,
			sizeof(VTYPE) * nVertices
		);
		if (FAILED(hr)) return hr;

		m_VertexBufferView.BufferLocation = m_cpVertexBuffer->GetGPUVirtualAddress();
		m_VertexBufferView.SizeInBytes = sizeof(VTYPE) * nVertices;
		m_VertexBufferView.StrideInBytes = sizeof(VTYPE);
		
		return S_OK;
	}

	//업로드 버퍼 반환
	void ReleaseUploadBuffer();

	void DrawMesh(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList);
};

