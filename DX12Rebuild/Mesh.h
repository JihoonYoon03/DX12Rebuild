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

template <class VERTEXDATA>
struct MeshData
{
	std::vector<VERTEXDATA>		vertices;
	std::vector<uint32_t>		indices;
};

class CMesh {
private:
	ComPtr<ID3D12Resource>				m_cpVertexUploadBuffer;
	ComPtr<ID3D12Resource>				m_cpVertexBuffer;
	ComPtr<ID3D12Resource>				m_cpIndexUploadBuffer;
	ComPtr<ID3D12Resource>				m_cpIndexBuffer;

	D3D12_VERTEX_BUFFER_VIEW			m_VertexBufferView;
	D3D12_INDEX_BUFFER_VIEW				m_IndexBufferView;
	UINT								m_nVertices;
	UINT								m_nIndices;
	D3D_PRIMITIVE_TOPOLOGY				m_topology;

public:
	CMesh();
	~CMesh();

	//다양한 정점 타입에 대한 버퍼 생성
	template<class VERTEXTYPE>
	HRESULT CreateVertexBuffer(const ComPtr<ID3D12Device>& cpDevice, const ComPtr<ID3D12GraphicsCommandList>& cpCommandList, const MeshData<VERTEXTYPE>& meshData)
	{
		std::wstring str = std::to_wstring(meshData.indices.size() % 3);

		OutputDebugStringW(str.c_str());
		//데이터 범위에 문제가 있는 경우
		if (meshData.vertices.empty() || meshData.indices.empty() || meshData.indices.size() % 3 != 0)
			return E_INVALIDARG;

		//인덱스 번호가 정점 개수보다 클 경우
		for (uint32_t index : meshData.indices)
		{
			if (index > meshData.vertices.size() - 1)
				return E_INVALIDARG;
		}


		//정점 버퍼 생성
		m_nVertices = meshData.vertices.size();

		HRESULT hr = CreateBufferResource(
			cpDevice,
			cpCommandList,
			m_cpVertexBuffer,
			meshData.vertices.data(),
			sizeof(VERTEXTYPE) * m_nVertices,
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
			&m_cpVertexUploadBuffer
		);
		if (FAILED(hr)) return hr;

		m_VertexBufferView.BufferLocation = m_cpVertexBuffer->GetGPUVirtualAddress();
		m_VertexBufferView.SizeInBytes = sizeof(VERTEXTYPE) * m_nVertices;
		m_VertexBufferView.StrideInBytes = sizeof(VERTEXTYPE);


		//인덱스 버퍼 생성
		m_nIndices = meshData.indices.size();

		hr = CreateBufferResource(
			cpDevice,
			cpCommandList,
			m_cpIndexBuffer,
			meshData.indices.data(),
			sizeof(uint32_t) * m_nIndices,
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_RESOURCE_STATE_INDEX_BUFFER,
			&m_cpIndexUploadBuffer
		);
		if (FAILED(hr)) return hr;

		m_IndexBufferView.BufferLocation = m_cpIndexBuffer->GetGPUVirtualAddress();
		m_IndexBufferView.SizeInBytes = sizeof(uint32_t) * m_nIndices;
		m_IndexBufferView.Format = DXGI_FORMAT_R32_UINT;

		return S_OK;
	}

	//업로드 버퍼 반환
	void ReleaseUploadBuffer();

	void DrawMesh(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList);
};

