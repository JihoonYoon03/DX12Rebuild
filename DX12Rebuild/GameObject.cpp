#include "pch.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Texture.h"

CObjectBase::CObjectBase()
{}

CObjectBase::~CObjectBase()
{}

void CObjectBase::SetScale(const float x, const float y, const float z)
{
	m_Transform.scale.x = x;
	m_Transform.scale.y = y;
	m_Transform.scale.z = z;
}

void CObjectBase::SetScale(const XMFLOAT3& scale)
{
	SetScale(scale.x, scale.y, scale.z);
}

void CObjectBase::SetScale(const float scale)
{
	SetScale(scale, scale, scale);
}

void CObjectBase::SetRotation(const float x, const float y, const float z)
{
	m_Transform.rotation.x = x;
	m_Transform.rotation.y = y;
	m_Transform.rotation.z = z;
}

void CObjectBase::SetRotation(const XMFLOAT3& rotation)
{
	SetRotation(rotation.x, rotation.y, rotation.z);
}

void CObjectBase::SetPosition(const float x, const float y, const float z)
{
	m_Transform.position.x = x;
	m_Transform.position.y = y;
	m_Transform.position.z = z;
}

void CObjectBase::SetPosition(const XMFLOAT3& position)
{
	SetPosition(position.x, position.y, position.z);
}

// -------------------------------------------
// 
// ModelObject
// 
// -------------------------------------------
CModelObject::CModelObject()
	: CObjectBase()
{
	m_spModel = std::make_shared<CModel>();
}

CModelObject::~CModelObject()
{}

void CModelObject::DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	if (m_spModel) m_spModel->DrawModel(cpCommandList, m_Transform.getMatrix());
}

void CModelObject::ReleaseModelUploadBuffer()
{
	m_spModel->ReleaseUploadBuffer();
}

// -------------------------------------------
// 
// TerrainObject
// 
// -------------------------------------------
CTerrainObject::CTerrainObject()
	: CObjectBase()
{}

CTerrainObject::~CTerrainObject()
{}

void CTerrainObject::SetHeightMap(std::shared_ptr<CHeightMapImage>& pHeightMapImage)
{
	m_spHeightMapImage = std::move(pHeightMapImage);
	m_nColSlice = m_spHeightMapImage->GetHeightMapWidth();
	m_nRowSlice = m_spHeightMapImage->GetHeightMapLength();
}

HRESULT CTerrainObject::GenerateTerrain(ComPtr<ID3D12Device>& cpDevice, ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	MeshData<VertexTexcoord> terrain;
	float interval = 1.0f;
	float halfInterval = interval / 2;

	//정점 정보 설정 구간
	for (int z = 0; z < m_nRowSlice; ++z)
	{
		for (int x = 0; x < m_nColSlice; ++x)
		{
			float y = 0.0f;
			
			if (m_spHeightMapImage)
				y = m_spHeightMapImage->GetHeight(x, z);

			terrain.vertices.push_back(
				{
					//halfInterval은 지형 중심이 원점이 되도록 하기 위함
					XMFLOAT3
					((x - ((m_nColSlice - 1) * halfInterval)) * interval,
					y,
					(z - ((m_nRowSlice - 1) * halfInterval)) * interval),

					XMFLOAT4
					(1.0f, 1.0f, 1.0f, 1.0f),

					XMFLOAT2
					(float(x) / (m_nColSlice - 1), float(z) / (m_nRowSlice - 1))
				}
			);
		}
	}

	//각 칸의 인덱스 설정 구간
	//인덱스는 정점 사이 칸을 순회하므로 -1 해줘야 함
	for (int j = 0, z = 0; z < m_nRowSlice - 1; ++z)
	{
		//홀수 번째 줄
		if (z % 2 == 0)
		{
			//정점 인덱스 번호는 기존대로 col만큼 처리해줘야 함
			for (int x = 0; x < m_nColSlice; ++x)
			{
				//첫 정점 제외, 각 칸 줄의 왼쪽 아래 시작점 한 번씩 추가
				if ((x == 0) && (z > 0))
					terrain.indices.push_back(x + (z * m_nColSlice));
				//아랫줄의 정점 인덱스
				terrain.indices.push_back(x + (z * m_nColSlice));
				//한 칸 위 정점 인덱스
				terrain.indices.push_back((x + (z * m_nColSlice)) + m_nColSlice);
			}
		}
		//짝수 번째 줄
		else
		{
			for (int x = m_nColSlice - 1; x >= 0; --x)
			{
				//맨 오른쪽 정점일 때, 각 칸 줄 오른쪽 아래 정점 1번씩 추가
				if (x == m_nColSlice - 1)
					terrain.indices.push_back(x + (z * m_nColSlice));
				//나머진 위와 동일
				terrain.indices.push_back(x + (z * m_nColSlice));
				terrain.indices.push_back((x + (z * m_nColSlice)) + m_nColSlice);
			}
		}
	}

	m_upTerrain = std::make_unique<CMesh>();
	m_upTerrain->SetTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	if (FAILED(m_upTerrain->CreateBuffer<VertexTexcoord>(cpDevice, cpCommandList, terrain)))
	{
		OutputDebugString(L"CTerrainObject::GenerateTerrain() Failed to create Buffer\n");
		return E_FAIL;
	}

	return S_OK;
}

void CTerrainObject::ReleaseUploadBuffer()
{
	m_upTerrain->ReleaseUploadBuffer();
}

float CTerrainObject::GetHeightAt(float x, float z)
{
	return m_spHeightMapImage->GetHeight(x, z);
}

float CTerrainObject::GetHeightAt(XMFLOAT3 pos)
{
	return GetHeightAt(pos.x, pos.z);
}

void CTerrainObject::DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	XMFLOAT4X4 world = Matrix4x4::Transpose(m_Transform.getMatrix());
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &world, 0);
	m_upTerrain->DrawMesh(cpCommandList);
}