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

void CMesh::DrawMesh(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList, const uint32_t baseVertex, const SubMeshData& subMesh)
{
	cpCommandList->IASetPrimitiveTopology(m_topology);
	cpCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	cpCommandList->IASetIndexBuffer(&m_IndexBufferView);
	cpCommandList->DrawIndexedInstanced(subMesh.indexAmount, 1, subMesh.startIndex, baseVertex, 0);
}


//=======================================================================================
//=======================================================================================
//map으로 태그 별 데이터 저장하는 람다 함수 지정해주기. 파싱 편의성을 위해
std::unordered_map<std::string, std::function<bool(std::istringstream&, FrameData&, MeshData<VertexTexNormal>&)>> parseByTag
= {
	//Frame 속성 파싱 관련. Children의 재귀호출 제외
	{ "<Frame>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		if (iss >> frame.index && iss >> frame.name)
			return false;
		else
			return true;
	}},

	{ "<TransformMatrix>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		float n;
			for (int i = 0; i < 16; ++i) {
				if (iss >> n)
					frame.mtxModel.m[i / 4][i % 4] = n;
				else
					return true;
			}
		return false;
	}},

	{ "<Mesh>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		return false;
	}},

	{ "<Materials>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		int numMaterial;
		if (iss >> numMaterial)
		{
			if (numMaterial < 0)
				return true;

			frame.materialDatas.reserve(numMaterial);
			return false;
		}
		return true;
	}},

	//Mesh 속성 파싱 관련
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	{ "<Bounds>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>&) {
		BoundingBox bounds;
		auto& center = bounds.Center;
		auto& extents = bounds.Extents;
		if (iss >> center.x >> center.y >> center.z >> extents.x >> extents.y >> extents.z &&
			std::isfinite(center.x) && std::isfinite(center.y) && std::isfinite(center.z) &&
			std::isfinite(extents.x) && std::isfinite(extents.y) && std::isfinite(extents.z) &&
			extents.x >= 0.0f && extents.y >= 0.0f && extents.z >= 0.0f)
		{
			frame.localBounds = bounds;
		}
		return false;
	}},
	//-----------------------------------

	{ "<Positions>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		int numVertex;
		if (iss >> numVertex)
		{
			//정점 개수 이상할 시
			if (numVertex < 0)
				return true;

			meshData.vertices.reserve(numVertex);

			XMFLOAT3 pos;
			for (int i = 0; i < numVertex; ++i)
			{
				if (iss >> pos.x >> pos.y >> pos.z)
					meshData.vertices.push_back({ pos });
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
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		int numColors;
		bool colored = false;
		if (iss >> numColors)
		{
			if (numColors > 0)
				colored = true;
			else if (numColors < 0)
				return true;

			for (VertexTexNormal& vtx : meshData.vertices)
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
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		int numVertex;
		if (iss >> numVertex)
		{
			if (numVertex < 0)
				return true;

			XMFLOAT3 normal;
			for (int i = 0; i < numVertex; ++i)
			{
				if (iss >> normal.x >> normal.y >> normal.z)
					meshData.vertices[i].normal = normal;
				else
					return true;
			}
			return false;
		}
		return true;
	}},

	{ "<SubMeshes>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
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
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		SubMeshData sMesh;
		if (iss >> sMesh.materialIndex >> sMesh.indexAmount)
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
				if (iss >> index && index < meshData.vertices.size())
					meshData.indices.push_back(index);
				else
					return true;
			}
			return false;
		}
		return true;
	}},


	//Material 속성 파싱 관련
	{ "<Material>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		frame.materialDatas.push_back({});
		return false;
	}},

	{ "<AlbedoColor>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		MaterialData& mat = frame.materialDatas.back();
		if (iss >> mat.albedoColor.x >> mat.albedoColor.y >> mat.albedoColor.z >> mat.albedoColor.w)
			return false;
		else
			return true;
	}},

	{ "<EmissiveColor>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		MaterialData& mat = frame.materialDatas.back();
		if (iss >> mat.emissiveColor.x >> mat.emissiveColor.y >> mat.emissiveColor.z >> mat.emissiveColor.w)
			return false;
		else
			return true;
	}},

	{ "<Glossiness>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		MaterialData& mat = frame.materialDatas.back();
		if (iss >> mat.glossiness)
			return false;
		else
			return true;
	}},

	{ "<Metallic>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		MaterialData& mat = frame.materialDatas.back();
		if (iss >> mat.metallic)
			return false;
		else
			return true;
	}},

	{ "<SpecularHighlight>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		MaterialData& mat = frame.materialDatas.back();
		if (iss >> mat.SpecularHighlight)
			return false;
		else
			return true;
	}},

	{ "<GlossyReflection>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		MaterialData& mat = frame.materialDatas.back();
		if (iss >> mat.GlossyReflection)
			return false;
		else
			return true;
	}}
};

//=======================================================================================
//=======================================================================================

CModel::CModel()
{}

CModel::~CModel()
{}

bool CModel::LoadModelData(
	const ComPtr<ID3D12Device>& cpDevice,
	const ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
	std::string filePath,
	std::shared_ptr<CMaterial>& material,
	std::string* debug
)
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
		//읽은 데이터 하나로 붙이기.
		MeshData<VertexTexNormal> combinedMesh;
		
		for (int i = 0; i < model.frames.size(); ++i)
		{
			//해당 프레임 매쉬의 정점 버퍼 시작 위치 저장
			model.frames[i].baseVertex = combinedMesh.vertices.size();
			for (auto& subMesh : model.frames[i].subMeshes)
			{
				//서브매쉬의 인덱스 배열 시작 위치도 갱신
				subMesh.startIndex += combinedMesh.indices.size();
			}
			if (!model.meshData[i].vertices.empty())
			{
				//-----------------------------------
				//[아래 코드는 AI로 구현됨]
				auto& bounds = model.frames[i].localBounds;
				const auto& vertices = model.meshData[i].vertices;
				if (!bounds || Vector3::LengthSquared(bounds->Extents) == 0.0f)
				{
					bounds.emplace();
					BoundingBox::CreateFromPoints(*bounds, vertices.size(), &vertices.front().pos, sizeof(VertexTexcoord));
				}
				//-----------------------------------
				combinedMesh.vertices.insert(combinedMesh.vertices.end(), model.meshData[i].vertices.begin(), model.meshData[i].vertices.end());
				combinedMesh.indices.insert(combinedMesh.indices.end(), model.meshData[i].indices.begin(), model.meshData[i].indices.end());
			}
		}

		if (combinedMesh.vertices.empty()) return false;

		std::unique_ptr<CMesh> upMesh = std::make_unique<CMesh>();
		if (FAILED(upMesh->CreateBuffer(cpDevice, cpCommandList, combinedMesh))) return false;
		m_spMaterial = std::move(material);
		m_upMesh = std::move(upMesh);
		m_vFrameData = std::move(model.frames);

		//행렬 업로드용 버퍼 생성
		// 아직 미사용
		//::CreateBufferResource(
		//	cpDevice,
		//	cpCommandList,
		//	m_cpFrameMatrixBuffer,
		//	nullptr,
		//	m_modelData.frames.size() * sizeof(XMFLOAT4X4)
		//);

		//D3D12_RANGE range{ 0, 0 };
		//if (FAILED(m_cpFrameMatrixBuffer->Map(0, &range, (void**)m_MappedFrameMatrixBuffer))) return false;

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
	modelData.meshData.push_back({});
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
					//함수 호출
					bool ends = iter->second(iss, modelData.frames[curIndex], modelData.meshData[curIndex]);

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

//-----------------------------------
//[아래 코드는 AI로 구현됨]
std::optional<BoundingBox> CModel::GetModelBounds()
{
	std::optional<BoundingBox> result;
	for (unsigned int i = 0; i < m_vFrameData.size(); ++i)
	{
		const auto& frame = m_vFrameData[i];
		if (!frame.localBounds || std::none_of(frame.subMeshes.begin(), frame.subMeshes.end(),
			[](const SubMeshData& subMesh) { return subMesh.indexAmount > 0; })) continue;

		const BoundingBox bounds = Collision::TransformBox(*frame.localBounds, GetFrameMatrix(i));
		if (result) BoundingBox::CreateMerged(*result, *result, bounds);
		else result = bounds;
	}
	return result;
}
//-----------------------------------

void CModel::ReleaseUploadBuffer()
{
	if (m_upMesh) m_upMesh->ReleaseUploadBuffer();
}

void CModel::SetFrameParent(const std::string& targetFrame, const std::string& parentFrame)
{
	auto target = std::find_if(m_vFrameData.begin(), m_vFrameData.end(),
		[targetFrame](FrameData& frame) {
			return frame.name == targetFrame;
		});

	auto parent = std::find_if(m_vFrameData.begin(), m_vFrameData.end(),
		[parentFrame](FrameData& frame) {
			return frame.name == parentFrame;
		});

	if (target != m_vFrameData.end() && parent != m_vFrameData.end())
		target->parent = parent->index;
}

void CModel::SetFrameLocal(const std::string& targetFrame, const Transform& transform)
{
	SetFrameLocal(targetFrame, transform.getMatrix());
}

void CModel::SetFrameLocal(const std::string& targetFrame, const XMFLOAT4X4& mtx)
{
	auto target = std::find_if(m_vFrameData.begin(), m_vFrameData.end(),
		[targetFrame](FrameData& frame) {
			return frame.name == targetFrame;
		});

	if (target != m_vFrameData.end())
	{
		target->mtxModel = mtx;
	}
}

//FrameData* CModel::GetFrame(const std::string& frameName)
//{
//	auto target = std::find_if(m_vFrameData.begin(), m_vFrameData.end(),
//		[frameName](FrameData& frame) {
//			return frame.name == frameName;
//		});
//
//	if (target != m_vFrameData.end())
//		return &(*target);
//	else
//		return nullptr;
//}

int CModel::GetFrameIndex(const std::string& frameName)
{
	auto target = std::find_if(m_vFrameData.begin(), m_vFrameData.end(),
		[frameName](FrameData& frame) {
			return frame.name == frameName;
		});

	if (target != m_vFrameData.end())
		return target->index;
	
	//실패
	return -1;
}

XMFLOAT4X4 CModel::GetFrameMatrix(const std::string& frameName)
{
	int index = GetFrameIndex(frameName);

	if (index != -1)
		return GetFrameMatrix(index);
	else
		return Matrix4x4::Identity();
}

XMFLOAT4X4 CModel::GetFrameMatrix(unsigned int index)
{
	XMFLOAT4X4 retValue;

	if (index >= m_vFrameData.size()) return Matrix4x4::Identity();

	FrameData& current = m_vFrameData[index];
	retValue = current.mtxModel;

	//부모 인덱스 정보가 있는 경우
	if (current.parent.has_value())
	{
		XMFLOAT4X4 mtxParent = GetFrameMatrix(current.parent.value());
		//재귀호출
		retValue = Matrix4x4::Multiply(retValue, mtxParent);
	}

	return retValue;
}

XMFLOAT4X4 CModel::GetFrameMatrix(const std::string& frameName, const std::unordered_map<int, XMFLOAT4X4>& umFrameTransform)
{
	return GetFrameMatrix(GetFrameIndex(frameName), umFrameTransform);
}

XMFLOAT4X4 CModel::GetFrameMatrix(unsigned int index, const std::unordered_map<int, XMFLOAT4X4>& umFrameTransform)
{
	if (index >= m_vFrameData.size()) return Matrix4x4::Identity();

	//프레임에 적용되는 행렬 있는지 확인
	FrameData& current = m_vFrameData[index];
	auto iter = umFrameTransform.find(m_vFrameData[index].index);
	XMFLOAT4X4 retval = current.mtxModel;

	if (iter != umFrameTransform.end())
	{
		Transform pivot;
		pivot.position = XMFLOAT3{ retval._41, retval._42, retval._43 };
		//프레임별 변환 적용
		XMFLOAT4X4 frame = Matrix4x4::Multiply(
			Matrix4x4::Multiply(Matrix4x4::Inverse(pivot.getMatrix()), iter->second),
			pivot.getMatrix());
		retval = Matrix4x4::Multiply(retval, frame);
	}

	//부모 인덱스 정보가 있는 경우
	if (current.parent.has_value())
	{
		XMFLOAT4X4 mtxParent = GetFrameMatrix(current.parent.value(), umFrameTransform);
		//재귀호출
		retval = Matrix4x4::Multiply(retval, mtxParent);
	}

	return retval;
}

//아직 미사용
//void CModel::SetFramesData(const size_t index)
//{
//	if (!m_modelData.frames[index].meshData.has_value() ||
//		m_modelData.frames[index].meshData->vertices.size() == 0 ||
//		m_modelData.frames[index].meshData->indices.size() == 0)
//		return;
//
//	XMFLOAT4X4 world = GetFrameMatrix(index);
//	world = Matrix4x4::Transpose(world);
//	//root constant 256바이트 정렬. 나중에 최적화 필요
//	::memcpy(m_MappedFrameMatrixBuffer + index * 256, &world, sizeof(XMFLOAT4X4));
//}

void CModel::DrawModel(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList, const XMFLOAT4X4 objWorld)
{
	if (!m_upMesh) return;

	//비효율적 렌더링 구조 개선 필요
	m_spMaterial->BindMaterial(cpCommandList);

	for (int i = 0; i < m_vFrameData.size(); ++i)
	{

		XMFLOAT4X4 world = Matrix4x4::Transpose(Matrix4x4::Multiply(GetFrameMatrix(i), objWorld));
		cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &world, 0);
		const XMFLOAT4X4 worldInv = Matrix4x4::Inverse(world);
		cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &worldInv, 16);

		for (auto& subMesh : m_vFrameData[i].subMeshes)
		{
			//현재 프레임의 머터리얼 데이터에서 선택 후 적용
			MaterialData materialData{};
			if (subMesh.materialIndex < m_vFrameData[i].materialDatas.size())
				materialData = m_vFrameData[i].materialDatas[subMesh.materialIndex];
			else
				materialData.albedoColor = XMFLOAT4(1, 1, 1, 1);
			m_spMaterial->SetMaterialData(cpCommandList, materialData);
			m_upMesh->DrawMesh(cpCommandList, m_vFrameData[i].baseVertex, subMesh);
		}
	}
}

void CModel::DrawModel(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList, const std::unordered_map<int, XMFLOAT4X4>& umFrameTransform, const XMFLOAT4X4 objWorld)
{
	if (!m_upMesh) return;

	//비효율적 렌더링 구조 개선 필요
	m_spMaterial->BindMaterial(cpCommandList);

	for (int i = 0; i < m_vFrameData.size(); ++i)
	{
		XMFLOAT4X4 model;
		if (umFrameTransform.empty())
		{
			//기본 로컬 행렬
			model = GetFrameMatrix(i);
		}
		else
		{
			//프레임 별 변환까지 적용
			model = GetFrameMatrix(i, umFrameTransform);
		}

		XMFLOAT4X4 world = Matrix4x4::Transpose(Matrix4x4::Multiply(model, objWorld));
		cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &world, 0);
		const XMFLOAT4X4 worldInv = Matrix4x4::Inverse(world);
		cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &worldInv, 16);

		for (auto& subMesh : m_vFrameData[i].subMeshes)
		{
			//현재 프레임의 머터리얼 데이터에서 선택 후 적용
			MaterialData materialData{};
			if (subMesh.materialIndex < m_vFrameData[i].materialDatas.size())
				materialData = m_vFrameData[i].materialDatas[subMesh.materialIndex];
			else
				materialData.albedoColor = XMFLOAT4(1, 1, 1, 1);
			m_spMaterial->SetMaterialData(cpCommandList, materialData);
			m_upMesh->DrawMesh(cpCommandList, m_vFrameData[i].baseVertex, subMesh);
		}
	}
}
