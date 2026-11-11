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

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CModelObject::SetFrameTransform(const std::string& name, const Transform& transform)
{
	SetFrameTransform(name, transform.getMatrix());
}

void CModelObject::SetFrameTransform(const std::string& name, const XMFLOAT4X4& transform)
{
	int index = m_spModel->GetFrameIndex(name);
	if (index == -1) return;

	m_umFrameTransform[index] = transform;
}
//-----------------------------------

void CModelObject::DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	if (m_spModel) m_spModel->DrawModel(cpCommandList, m_umFrameTransform, m_Transform.getMatrix());
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
{
}

CTerrainObject::~CTerrainObject()
{}

void CTerrainObject::SetHeightMap(std::shared_ptr<CHeightMapImage>& pHeightMapImage)
{
	m_spHeightMapImage = std::move(pHeightMapImage);
	m_nColSlice = m_spHeightMapImage->GetHeightMapWidth();
	m_nRowSlice = m_spHeightMapImage->GetHeightMapLength();
	//나누는 수치는 나중에 머터리얼 도입 시 텍스쳐 사이즈와 연동
	m_fTiling = m_nColSlice / 10;
}

HRESULT CTerrainObject::GenerateTerrain(
	ComPtr<ID3D12Device>& cpDevice,
	ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
	CDescHeapAllocator* DescHeapAllocator,
	bool normalMapping)
{
	if (m_spHeightMapImage->GetHeightMapPixels().empty()) return E_FAIL;

	HRESULT hr;
	if (DescHeapAllocator)
	{
		m_upTerrainTex = std::make_unique<CTexture>();
		hr = m_upTerrainTex->CreateTexture(*DescHeapAllocator, cpDevice, cpCommandList, L"../Resources/grass.dds");
		if (FAILED(hr))
		{
			m_upTerrainTex.reset();
			return hr;
		}

		if (normalMapping)
		{
			m_upNormalMap = std::make_unique<CTexture>();
			HRESULT hr = m_upNormalMap->CreateTexture(*DescHeapAllocator, cpDevice, cpCommandList, L"../Resources/TerrainNormal.dds");
			if (FAILED(hr))
			{
				m_upNormalMap.reset();
				return hr;
			}
		}
	}

	MeshData<VertexTexNormal> terrain;
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
					(float(x) / (m_nColSlice - 1), float(z) / (m_nRowSlice - 1)),

					XMFLOAT3
					(0.0f, 1.0f, 0.0f)
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
	hr = m_upTerrain->CreateBuffer<VertexTexNormal>(cpDevice, cpCommandList, terrain);
	if (FAILED(hr))
	{
		OutputDebugString(L"CTerrainObject::GenerateTerrain() Failed to create Buffer\n");
		return hr;
	}

	return S_OK;
}

void CTerrainObject::ReleaseUploadBuffer()
{
	m_upTerrain->ReleaseUploadBuffer();
	if (m_upTerrainTex) m_upTerrainTex->ReleaseUploadBuffer();
}

float CTerrainObject::GetHeightAt(float x, float z) const
{
	//지형 좌표계로 변환
	XMFLOAT3 local(x, 0.0f, z);
	local = Vector3::TransformCoord(local, Matrix4x4::Inverse(m_Transform.getMatrix()));

	//지형의 인덱스로 변환
	float pixelX = local.x + (m_nColSlice - 1) * 0.5f;
	float pixelZ = local.z + (m_nRowSlice - 1) * 0.5f;

	return m_spHeightMapImage->GetHeight(pixelX, pixelZ)
		* m_Transform.scale.y + m_Transform.position.y;
}

float CTerrainObject::GetHeightAt(const XMFLOAT3& pos) const
{
	return GetHeightAt(pos.x, pos.z);
}

void CTerrainObject::DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	cpCommandList->SetGraphicsRoot32BitConstants(1, 1, &m_fTiling, 4);
	XMFLOAT4X4 world = Matrix4x4::Transpose(m_Transform.getMatrix());
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &world, 0);
	XMFLOAT4X4 worldInv = Matrix4x4::Inverse(world);
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &worldInv, 16);

	//텍스쳐 설정
	if (m_upTerrainTex)
		cpCommandList->SetGraphicsRootDescriptorTable(3, m_upTerrainTex->getGPUDescHandle());

	//노멀맵 설정
	if (m_upNormalMap)
		cpCommandList->SetGraphicsRootDescriptorTable(4, m_upNormalMap->getGPUDescHandle());

	m_upTerrain->DrawMesh(cpCommandList);
}


// -------------------------------------------
//
// SkyboxObject
//
// -------------------------------------------
CSkyboxObject::CSkyboxObject() = default;
CSkyboxObject::~CSkyboxObject() = default;

HRESULT CSkyboxObject::GenerateSkyBox(
	const ComPtr<ID3D12Device>& cpDevice,
	const ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
	CDescHeapAllocator& allocator)
{
	const wchar_t* filenames[] =
	{
		L"../Resources/SkyBox_Front_0.dds",
		L"../Resources/SkyBox_Back_0.dds",
		L"../Resources/SkyBox_Left_0.dds",
		L"../Resources/SkyBox_Right_0.dds",
		L"../Resources/SkyBox_Top_0.dds",
		L"../Resources/SkyBox_Bottom_0.dds"
	};
	const XMFLOAT2 uv[] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
	const XMFLOAT4 white{ 1, 1, 1, 1 };
	const float extent = config::stage::SkyboxExtent;

	m_vSkyTextures.resize(std::size(filenames));
	m_vPlane.resize(std::size(filenames));
	for (size_t i = 0; i < std::size(filenames); ++i)
	{
		m_vSkyTextures[i] = std::make_unique<CTexture>();
		HRESULT hr = m_vSkyTextures[i]->CreateTexture(allocator, cpDevice, cpCommandList, filenames[i]);
		if (FAILED(hr))
		{
			OutputDebugStringW(L"Skybox texture load failed: ");
			OutputDebugStringW(filenames[i]);
			OutputDebugStringW(L"\n");
			return hr;
		}

		//-----------------------------------
		//[아래 코드는 AI로 구현됨]
		//평면 데이터 생성
		MeshData<VertexTexcoord> plane;
		const float sign = i % 2 == 0 ? 1.0f : -1.0f;
		for (const auto& texcoord : uv)
		{
			const float u = texcoord.x == 0.0f ? -extent : extent;
			const float v = texcoord.y == 0.0f ? extent : -extent;
			const XMFLOAT3 pos[] =
			{
				{ sign * u, v, sign * extent },
				{ -sign * extent, v, sign * u },
				{ u, sign * extent, -sign * v }
			};
			plane.vertices.push_back({ pos[i / 2], white, texcoord });
		}
		plane.indices = { 0, 1, 2, 0, 2, 3 };
		//-----------------------------------

		m_vPlane[i] = std::make_unique<CMesh>();
		hr = m_vPlane[i]->CreateBuffer(cpDevice, cpCommandList, plane);
		if (FAILED(hr)) return hr;
	}

	return S_OK;
}

void CSkyboxObject::DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	const XMFLOAT4X4 world = Matrix4x4::Transpose(m_Transform.getMatrix());
	const float tiling = 1.0f;
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &world, 0);
	const XMFLOAT4X4 worldInv = Matrix4x4::Inverse(world);
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &worldInv, 16);
	cpCommandList->SetGraphicsRoot32BitConstants(1, 1, &tiling, 4);

	for (size_t i = 0; i < m_vPlane.size(); ++i)
	{
		cpCommandList->SetGraphicsRootDescriptorTable(3, m_vSkyTextures[i]->getGPUDescHandle());
		m_vPlane[i]->DrawMesh(cpCommandList);
	}
}

void CSkyboxObject::ReleaseUploadBuffer()
{
	for (auto& plane : m_vPlane)
	{
		plane->ReleaseUploadBuffer();
	}
	for (auto& texture : m_vSkyTextures)
	{
		texture->ReleaseUploadBuffer();
	}
}
