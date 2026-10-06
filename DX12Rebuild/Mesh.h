#pragma once
#include "pch.h"

struct VertexDiffused
{
	XMFLOAT3 pos;
	XMFLOAT4 color;
};

struct VertexTexcoord
{
	XMFLOAT3 pos;
	XMFLOAT4 color;
	XMFLOAT2 uv;
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


		HRESULT hr = CreateBufferResource(
			cpDevice,
			cpCommandList,
			m_cpVertexBuffer,
			data,
			sizeof(VTYPE) * nVertices,
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
			&m_cpVertexUploadBuffer
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

