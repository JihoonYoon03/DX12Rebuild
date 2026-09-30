#include "pch.h"
#include "Mesh.h"

CMesh::CMesh()
{
	::ZeroMemory(&m_VertexBufferView, sizeof(D3D12_VERTEX_BUFFER_VIEW));
	m_nVertices = 0;
	m_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

CMesh::~CMesh()
{
}

void CMesh::ReleaseUploadBuffer()
{
	m_cpVertexUploadBuffer.Reset();
}

void CMesh::DrawMesh(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	cpCommandList->IASetPrimitiveTopology(m_topology);
	cpCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	cpCommandList->DrawInstanced(m_nVertices, 1, 0, 0);
}