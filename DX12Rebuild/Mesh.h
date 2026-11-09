#pragma once
#include "pch.h"
#include "Material.h"

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
	std::vector<XMFLOAT3>		normals;
};

//매쉬에 대한 인덱스, 머터리얼 등 부가정보
//인덱스 범위를 통해 같은 매쉬라도 영역을 나눠 머터리얼 지정 가능
struct SubMeshData
{
	//해당 서브매쉬에 적용할 머터리얼

	MaterialData	materialData;
	uint32_t		materialIndex;
	//해당 서브매쉬가 인덱스 배열 몇 번부터 시작하는가
	uint32_t		startIndex;
	//해당 서브매쉬가 총 몇 개의 인덱스를 갖는가
	uint32_t		indexAmount;
};

struct FrameData
{
	int										index;
	std::string								name;

	//optional로 빈 상태 지정. 매쉬가 없는 경우 등
	std::optional<unsigned int>				parent = std::nullopt;

	//이 프레임의 매쉬가 정점 배열의 몇 번 정점부터 시작되는가
	uint32_t								baseVertex;

	std::vector<SubMeshData>				subMeshes;
	std::vector<MaterialData>				materialDatas;

	XMFLOAT4X4								mtxModel;
};

class CMesh {
public:
	CMesh();
	~CMesh();

	void SetTopology(D3D12_PRIMITIVE_TOPOLOGY type) { m_topology = type; }

	//다양한 정점 타입에 대한 버퍼 생성
	template<class VERTEXTYPE>
	HRESULT CreateBuffer(const ComPtr<ID3D12Device>& cpDevice, const ComPtr<ID3D12GraphicsCommandList>& cpCommandList, const MeshData<VERTEXTYPE>& meshData)
	{
		std::wstring str = std::to_wstring(meshData.indices.size() % 3);

		OutputDebugStringW(str.c_str());
		//데이터 범위에 문제가 있는 경우 (TRIANGLELIST 기준)
		if (m_topology == D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST &&
			(meshData.vertices.empty() || meshData.indices.empty() || meshData.indices.size() % 3 != 0))
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
	void DrawMesh(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList, const uint32_t baseVertex, const SubMeshData& subMesh);

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

};

class CModel
{
private:
	struct ModelData
	{
		std::vector<FrameData>					frames;
		std::vector<MeshData<VertexTexcoord>>	meshData;
	};

public:
	CModel();
	~CModel();

	bool LoadModelData(
		const ComPtr<ID3D12Device>& cpDevice,
		const ComPtr<ID3D12GraphicsCommandList>& cpCommandList, 
		std::string filePath,
		std::shared_ptr<CMaterial>& material,
		std::string* debug = nullptr
	);

	//프레임 별 데이터를 읽어들이는 함수. 재귀적 호출
	bool ReadFrameData(
		ModelData& modelData,
		std::ifstream& file, std::istringstream& iss, std::string& buffer,
		const std::optional<unsigned int> parent = std::nullopt);


	void ReleaseUploadBuffer();

	void SetFrameParent(const std::string& targetFrame, const std::string& parentFrame);
	void SetFrameLocal(const std::string& targetFrame, const Transform& transform);
	void SetFrameLocal(const std::string& targetFrame, const XMFLOAT4X4& mtx);

	//FrameData* GetFrame(const std::string& frameName);
	int GetFrameIndex(const std::string& frameName);
	XMFLOAT4X4 GetFrameMatrix(const std::string& frameName);
	XMFLOAT4X4 GetFrameMatrix(unsigned int index);
	//프레임 별 변환이 적용된 것 까지 포함하여 Get
	XMFLOAT4X4 GetFrameMatrix(const std::string& frameName, std::unordered_map<int, Transform>& umFrameTransform);
	XMFLOAT4X4 GetFrameMatrix(unsigned int index, std::unordered_map<int, Transform>& umFrameTransform);

	const std::vector<FrameData>& getData() { return m_vFrameData; }

	virtual void DrawModel(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
		const XMFLOAT4X4 objWorld = Matrix4x4::Identity());
	//프레임 별 변환 적용
	virtual void DrawModel(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
		std::unordered_map<int, Transform>& umFrameTransform,
		const XMFLOAT4X4 objWorld = Matrix4x4::Identity());

private:
	std::vector<FrameData>					m_vFrameData;
	std::unique_ptr<CMesh>					m_upLandscape;
	std::shared_ptr<CMaterial>				m_spMaterial;

	//아직 미사용
	//ComPtr<ID3D12Resource>				m_cpFrameMatrixBuffer;
	//UINT8*								m_MappedFrameMatrixBuffer;
};