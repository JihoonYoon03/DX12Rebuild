#include "pch.h"
#include "Mesh.h"

CMesh::CMesh()
{
	::ZeroMemory(&m_VertexBufferView, sizeof(D3D12_VERTEX_BUFFER_VIEW));
	::ZeroMemory(&m_IndexBufferView, sizeof(D3D12_INDEX_BUFFER_VIEW));
	m_nVertices = 0;
	m_nIndices = 0;
	m_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

CMesh::~CMesh()
{
}

void CMesh::ReleaseUploadBuffer()
{
	m_cpVertexUploadBuffer.Reset();
	m_cpIndexUploadBuffer.Reset();
}

void CMesh::DrawMesh(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	cpCommandList->IASetPrimitiveTopology(m_topology);
	cpCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	cpCommandList->IASetIndexBuffer(&m_IndexBufferView);
	cpCommandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
}


//=======================================================================================
//=======================================================================================

bool CModel::LoadModelData(std::string filePath, std::string* debug)
{
	std::ifstream file{ filePath };
	if (file.fail())
	{
		if (debug) *debug = "Failed to open File";
		return false;
	}

	//<Hierarchy>: 행 날리기
	std::string line;
	std::getline(file, line);

	std::istringstream iss;
	std::string buffer;
	ReadFrameData(file, iss, buffer);

	return true;
}

bool CModel::ReadFrameData(std::ifstream& file, std::istringstream& iss, std::string& buffer,
	const std::optional<unsigned int> parent)
{
	std::getline(file, buffer);

	iss.clear();
	iss.str(buffer);

	NodeData node;
	iss >> node.index;
	iss >> node.name;
	node.parent = parent;
	ReadMeshBlock(file, iss, buffer, node);
	ReadMaterialBlock(file, iss, buffer, node);

	data.nodes.push_back(node);

	return false;
}

bool CModel::ReadMeshBlock(std::ifstream& file, std::istringstream& iss, std::string& buffer, NodeData& node)
{
	std::string data;
	while (1)
	{
		std::getline(file, buffer);
		iss.clear();
		iss.str(buffer);
		iss >> data;
		if (data == "<TransformMatrix>:")
		{
			int n;
			for (int i = 0; i < 16; ++i) {
				iss >> n;
				node.mtxTransform.m[i % 4][i / 4] = n;
			}
		}
		else if (data == "<Children>:")
		{
			//재귀호출
		}
	}
	return false;
}

bool CModel::ReadMaterialBlock(std::ifstream& file, std::istringstream& iss, std::string& buffer, NodeData& node)
{
	return false;
}
