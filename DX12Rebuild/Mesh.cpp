#include "pch.h"
#include "Mesh.h"

//map으로 태그 별 데이터 저장하는 람다 함수 지정해주기. 파싱 편의성을 위해
std::unordered_map<std::string, std::function<bool(std::istringstream&, FrameData&)>> parseByTag
= {
	//Frame 속성 파싱 관련. Children의 재귀호출 제외
	{ "<Frame>:",
	[](std::istringstream& iss, FrameData& frame) {
		if (iss >> frame.index && iss >> frame.name)
			return false;
		else
			return true;
	}},

	{ "<TransformMatrix>:",
	[](std::istringstream& iss, FrameData& frame) {
		float n;
			for (int i = 0; i < 16; ++i) {
				if (iss >> n)
					frame.mtxTransform.m[i / 4][i % 4] = n;
				else
					return true;
			}
		return false;
	}},

	{ "<Mesh>:",
	[](std::istringstream& iss, FrameData& frame) {
		frame.meshData.emplace();
		return false;
	}},

	{ "<Materials>:",
	[](std::istringstream& iss, FrameData& frame) {
		int numMaterial;
		if (iss >> numMaterial)
		{
			if (numMaterial < 0)
				return true;

			frame.materials.reserve(numMaterial);
			return false;
		}
		return true;
	}},

	//Mesh 속성 파싱 관련
	{ "<Bounds>:",
	[](std::istringstream& iss, FrameData& frame) {
		return false;
	}},

	{ "<Positions>:",
	[](std::istringstream& iss, FrameData& frame) {
		int numVertex;
		if (iss >> numVertex)
		{
			//정점 개수 이상할 시
			if (numVertex < 0)
				return true;

			frame.meshData->vertices.reserve(numVertex);

			XMFLOAT3 pos;
			for (int i = 0; i < numVertex; ++i)
			{
				if (iss >> pos.x >> pos.y >> pos.z)
					frame.meshData->vertices.push_back({ pos });
				// 개수 카운트보다 데이터 읽기가 빨리 끝난 경우
				else
					return true;
			}
			return false;
		}
		//개수 파싱 실패 시 리턴 true(종료)
		return true;
	}},

	{ "<Colors>:",
	[](std::istringstream& iss, FrameData& frame) {
		int numColors;
		bool colored = false;
		if (iss >> numColors)
		{
			if (numColors > 0)
				colored = true;
			else if (numColors < 0)
				return true;

			for (VertexTexcoord& vtx : frame.meshData->vertices)
			{
				XMFLOAT4 color;
				if (colored)
				{
					if (iss >> color.x >> color.y >> color.z >> color.w)
						vtx.color = color;
					else
						return true;
				}
				else
				{
					vtx.color = XMFLOAT4(1, 1, 1, 1);
				}
			}
			return false;
		}
		return true;
	}},

	{ "<Normals>:",
	[](std::istringstream& iss, FrameData& frame) {
		int numVertex;
		if (iss >> numVertex)
		{
			if (numVertex < 0)
				return true;

			frame.meshData->normals.reserve(numVertex);

			XMFLOAT3 normal;
			for (int i = 0; i < numVertex; ++i)
			{
				if (iss >> normal.x >> normal.y >> normal.z)
					frame.meshData->normals.push_back({ normal });
				else
					return true;
			}
			return false;
		}
		return true;
	}},

	{ "<SubMeshes>:",
	[](std::istringstream& iss, FrameData& frame) {
		int numSubMesh;
		if (iss >> numSubMesh)
		{
			if (numSubMesh < 0)
				return true;

			frame.subMeshes.reserve(numSubMesh);

			return false;
		}
		return true;
	}},

	{ "<SubMesh>:",
	[](std::istringstream& iss, FrameData& frame) {
		SubMesh sMesh;
		if (iss >> sMesh.materialSlot >> sMesh.indexAmount)
		{
			if (frame.subMeshes.empty())
			{
				sMesh.startIndex = 0;
			}
			else
			{
				//서브매쉬는 인덱스 버퍼에서 읽을 영역만 지정함.
				//인덱스 버퍼에는 실제 모델 데이터 상에서 서브매쉬가 지정하는 인덱스 번호들을
				//서브매쉬 순서대로 순차적으로 전부 indices 벡터에 집어넣음
				sMesh.startIndex = frame.subMeshes.back().startIndex
					+ frame.subMeshes.back().indexAmount;
			}
			frame.subMeshes.push_back(sMesh);

			uint32_t index;
			for (int i = 0; i < frame.subMeshes.back().indexAmount; ++i)
			{
				if (iss >> index && index < frame.meshData->vertices.size())
					frame.meshData->indices.push_back(index);
				else
					return true;
			}
			return false;
		}
		return true;
	}},


	//Material 속성 파싱 관련
	{ "<Material>:",
	[](std::istringstream& iss, FrameData& frame) {
		frame.materials.push_back({});
		return false;
	}},

	{ "<AlbedoColor>:",
	[](std::istringstream& iss, FrameData& frame) {
		Material& mat = frame.materials.back();
		if (iss >> mat.albedoColor.x >> mat.albedoColor.y >> mat.albedoColor.z >> mat.albedoColor.w)
			return false;
		else
			return true;
	}},

	{ "<EmissiveColor>:",
	[](std::istringstream& iss, FrameData& frame) {
		Material& mat = frame.materials.back();
		if (iss >> mat.emissiveColor.x >> mat.emissiveColor.y >> mat.emissiveColor.z >> mat.emissiveColor.w)
			return false;
		else
			return true;
	}},

	{ "<Glossiness>:",
	[](std::istringstream& iss, FrameData& frame) {
		Material& mat = frame.materials.back();
		if (iss >> mat.glossiness)
			return false;
		else
			return true;
	}},

	{ "<Metallic>:",
	[](std::istringstream& iss, FrameData& frame) {
		Material& mat = frame.materials.back();
		if (iss >> mat.metallic)
			return false;
		else
			return true;
	}},

	{ "<SpecularHighlight>:",
	[](std::istringstream& iss, FrameData& frame) {
		Material& mat = frame.materials.back();
		if (iss >> mat.SpecularHighlight)
			return false;
		else
			return true;
	}},

	{ "<GlossyReflection>:",
	[](std::istringstream& iss, FrameData& frame) {
		Material& mat = frame.materials.back();
		if (iss >> mat.GlossyReflection)
			return false;
		else
			return true;
	}}
};

CMesh::CMesh()
{
	::ZeroMemory(&m_VertexBufferView, sizeof(D3D12_VERTEX_BUFFER_VIEW));
	::ZeroMemory(&m_IndexBufferView, sizeof(D3D12_INDEX_BUFFER_VIEW));
	m_nVertices = 0;
	m_nIndices = 0;
	m_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

CMesh::~CMesh()
{}

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

	ModelData model;

	//<Hierarchy>: 행 날리기
	std::string line;
	std::getline(file, line);

	std::istringstream iss;
	std::string buffer;
	if (ReadFrameData(model, file, iss, buffer))
	{
		m_modelData = model;
		return true;
	}
	else
	{
		if (debug) *debug = "Data Parse Error";
		return false;
	}
}

bool CModel::ReadFrameData(
	ModelData& modelData,
	std::ifstream& file, std::istringstream& iss, std::string& buffer,
	const std::optional<unsigned int> parent)
{
	modelData.frames.push_back({});
	int curIndex = modelData.frames.back().index = modelData.frames.size() - 1;

	if (parent.has_value())
	{
		modelData.frames[curIndex].parent = parent;
	}

	std::string data;
	while (1)
	{
		//버퍼에 라인 불러오고, iss로 파싱하기 위해 flag clear 후 str로 새 문자열 세팅
		if (std::getline(file, buffer))
		{
			iss.clear();
			iss.str(buffer);
			iss >> data;
			if (data == "<Children>:")
			{
				int numChild;
				if (iss >> numChild && numChild >= 0)
				{
					for (int i = 0; i < numChild; ++i)
					{
						//파싱 에러
						if (!ReadFrameData(modelData, file, iss, buffer, curIndex))
							return false;
					}
				}
				else
				{
					return false;
				}
			}
			else if (data == "</Frame>")
			{
				return true;
			}
			else
			{
				auto iter = parseByTag.find(data);
				if (iter != parseByTag.end())
				{
					bool ends = iter->second(iss, modelData.frames[curIndex]);

					//에러 발생
					if (ends) return false;
				}
			}
		}
		else
		{
			//파일 getline 실패 시
			return false;
		}
	}

	return false;
}