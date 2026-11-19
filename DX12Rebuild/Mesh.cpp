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
using FrameReader = bool (*)(std::istringstream&, FrameData&, MeshData<VertexTexNormal>&);
const std::unordered_map<std::string, FrameReader> parseByTag
= {
	//Frame 속성 파싱 관련. Children의 재귀호출 제외
	{ "<Frame>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		unsigned int fileIndex;
		if (iss >> fileIndex >> frame.name)
			return false;
		else
			return true;
	}},

	{ "<TransformMatrix>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		float n;
			for (int i = 0; i < 16; ++i) {
				if (iss >> n)
					frame.local.m[i / 4][i % 4] = n;
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

			frame.materialDatas.resize(numMaterial);
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
		XMFLOAT3& center = bounds.Center;
		XMFLOAT3& extents = bounds.Extents;
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
		if (!(iss >> frame.materialIndex)) return true;
		if (frame.materialIndex >= frame.materialDatas.size()) return true;
		return false;
	}},

	{ "<AlbedoColor>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		if (frame.materialIndex >= frame.materialDatas.size()) return true;
		CB_Material& mat = frame.materialDatas[frame.materialIndex];
		if (iss >> mat.xmf4AlbedoColor.x >> mat.xmf4AlbedoColor.y >> mat.xmf4AlbedoColor.z >> mat.xmf4AlbedoColor.w)
			return false;
		else
			return true;
	}},

	{ "<EmissiveColor>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		if (frame.materialIndex >= frame.materialDatas.size()) return true;
		CB_Material& mat = frame.materialDatas[frame.materialIndex];
		if (iss >> mat.xmf4EmissiveColor.x >> mat.xmf4EmissiveColor.y >> mat.xmf4EmissiveColor.z >> mat.xmf4EmissiveColor.w)
			return false;
		else
			return true;
	}},

	{ "<Glossiness>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		if (frame.materialIndex >= frame.materialDatas.size()) return true;
		CB_Material& mat = frame.materialDatas[frame.materialIndex];
		if (iss >> mat.fGlossiness)
			return false;
		else
			return true;
	}},

	{ "<Metallic>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		if (frame.materialIndex >= frame.materialDatas.size()) return true;
		CB_Material& mat = frame.materialDatas[frame.materialIndex];
		if (iss >> mat.fMetallic)
			return false;
		else
			return true;
	}},

	{ "<SpecularHighlight>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		if (frame.materialIndex >= frame.materialDatas.size()) return true;
		CB_Material& mat = frame.materialDatas[frame.materialIndex];
		if (iss >> mat.fSpecularHighlight)
			return false;
		else
			return true;
	}},

	{ "<GlossyReflection>:",
	[](std::istringstream& iss, FrameData& frame, MeshData<VertexTexNormal>& meshData) {
		if (frame.materialIndex >= frame.materialDatas.size()) return true;
		CB_Material& mat = frame.materialDatas[frame.materialIndex];
		if (iss >> mat.fGlossyReflection)
			return false;
		else
			return true;
	}}
};

//=======================================================================================
//=======================================================================================

//-----------------------------------
//[아래 코드는 AI로 구현됨]
bool CModel::LoadModelData(
	const GraphicsResources& graphics,
	const char* filePath,
	const XMFLOAT3& offset,
	const std::vector<ModelFrameTag>& frameTags,
	std::string* debug)
{
	std::ifstream file(filePath);
	if (!file)
	{
		if (debug) *debug = "Failed to open file";
		return false;
	}

	ModelData model;
	std::istringstream iss;
	std::string buffer;
	std::getline(file, buffer);
	if (!ReadFrameData(model, file, iss, buffer))
	{
		if (debug) *debug = "Data parse error";
		return false;
	}

	// Resolve the file hierarchy while loading.
	const XMFLOAT4X4 rootOffset = Matrix4x4::Translation(offset);

	std::vector<XMFLOAT4X4> restMatrices(model.frames.size());
	std::unordered_map<std::string, size_t> fileFrames;
	for (size_t i = 0; i < model.frames.size(); ++i)
	{
		const FrameData& frame = model.frames[i];
		const XMFLOAT4X4& parent = frame.parent ? restMatrices[*frame.parent] : rootOffset;
		restMatrices[i] = Matrix4x4::Multiply(frame.local, parent);
		fileFrames.emplace(frame.name, i);
	}

	// Retain the body and only the frame tags used for animation.
	std::vector<ModelPart> parts{ { "", Matrix4x4::Identity(), 0, {} } };
	std::vector<XMFLOAT4X4> partInverse{ Matrix4x4::Identity() };
	std::vector<size_t> frameParts(model.frames.size(), 0);
	std::unordered_map<std::string, size_t> animatedParts;
	for (const ModelFrameTag& tag : frameTags)
	{
		if (!tag.animated) continue;

		const std::unordered_map<std::string, size_t>::const_iterator frame = fileFrames.find(tag.name);
		if (frame == fileFrames.end() || animatedParts.count(tag.name))
		{
			if (debug) *debug = std::string("Invalid animation frame: ") + tag.name;
			return false;
		}

		size_t parent = 0;
		if (tag.parent)
		{
			const std::unordered_map<std::string, size_t>::const_iterator found = animatedParts.find(tag.parent);
			if (found == animatedParts.end())
			{
				if (debug) *debug = std::string("Invalid animation parent for: ") + tag.name;
				return false;
			}
			parent = found->second;
		}

		const size_t partIndex = parts.size();
		const XMFLOAT4X4& rest = restMatrices[frame->second];
		const XMFLOAT4X4 local = Matrix4x4::Multiply(rest, partInverse[parent]);
		parts.push_back({ tag.name, local, parent, {} });
		partInverse.push_back(Matrix4x4::Inverse(rest));
		animatedParts.emplace(tag.name, partIndex);
		frameParts[frame->second] = partIndex;
	}

	// Extra parts of the turret follow it without their own animated transform.
	for (const ModelFrameTag& tag : frameTags)
	{
		if (tag.animated) continue;

		const std::unordered_map<std::string, size_t>::const_iterator frame = fileFrames.find(tag.name);
		if (frame == fileFrames.end()) continue;

		const std::unordered_map<std::string, size_t>::const_iterator parent =
			tag.parent ? animatedParts.find(tag.parent) : animatedParts.end();
		if (parent == animatedParts.end())
		{
			if (debug) *debug = std::string("Invalid animation parent for: ") + tag.name;
			return false;
		}
		frameParts[frame->second] = parent->second;
	}

	// Bake static transforms into vertices and collect indices by part and material.
	MeshData<VertexTexNormal> combinedMesh;
	std::vector<CB_Material> materials;
	struct PartBatch
	{
		uint32_t materialIndex;
		std::vector<uint32_t> indices;
	};
	std::vector<std::vector<PartBatch>> batches(parts.size());
	std::optional<BoundingBox> modelBounds;
	static_assert(sizeof(CB_Material) == sizeof(float) * 14);

	for (size_t i = 0; i < model.frames.size(); ++i)
	{
		const FrameData& frame = model.frames[i];
		if (frameParts[i] == 0 && frame.parent) frameParts[i] = frameParts[*frame.parent];

		const MeshData<VertexTexNormal>& mesh = model.meshData[i];
		if (mesh.indices.empty()) continue;

		const size_t part = frameParts[i];
		const XMFLOAT4X4 local = Matrix4x4::Multiply(restMatrices[i], partInverse[part]);
		const XMFLOAT4X4 normal = Matrix4x4::Transpose(Matrix4x4::Inverse(local));
		const uint32_t baseVertex = static_cast<uint32_t>(combinedMesh.vertices.size());
		for (VertexTexNormal vertex : mesh.vertices)
		{
			vertex.pos = Vector3::TransformCoord(vertex.pos, local);
			vertex.normal = Vector3::TransformNormal(vertex.normal, normal);
			combinedMesh.vertices.push_back(vertex);
		}

		BoundingBox bounds;
		if (frame.localBounds && Vector3::LengthSquared(frame.localBounds->Extents) != 0.0f)
			bounds = *frame.localBounds;
		else
			BoundingBox::CreateFromPoints(bounds, mesh.vertices.size(), &mesh.vertices.front().pos, sizeof(VertexTexNormal));

		bounds = Collision::TransformBox(bounds, restMatrices[i]);
		if (modelBounds) BoundingBox::CreateMerged(*modelBounds, *modelBounds, bounds);
		else modelBounds = bounds;

		std::vector<PartBatch>& partBatches = batches[part];
		for (const SubMeshData& subMesh : frame.subMeshes)
		{
			if (subMesh.indexAmount == 0) continue;

			CB_Material data{};
			if (subMesh.materialIndex < frame.materialDatas.size()) data = frame.materialDatas[subMesh.materialIndex];

			size_t slot = 0;
			while (slot < materials.size() && ::memcmp(&materials[slot], &data, sizeof(data)) != 0) ++slot;
			if (slot == materials.size()) materials.push_back(data);
			size_t batch = 0;
			while (batch < partBatches.size() && partBatches[batch].materialIndex != slot) ++batch;
			if (batch == partBatches.size()) partBatches.push_back({ static_cast<uint32_t>(slot), {} });

			std::vector<uint32_t>& indices = partBatches[batch].indices;
			for (uint32_t index = 0; index < subMesh.indexAmount; ++index)
				indices.push_back(baseVertex + mesh.indices[subMesh.startIndex + index]);
		}
	}

	for (size_t i = 0; i < parts.size(); ++i)
	{
		for (const PartBatch& batch : batches[i])
		{
			parts[i].subMeshes.push_back({
				batch.materialIndex,
				static_cast<uint32_t>(combinedMesh.indices.size()),
				static_cast<uint32_t>(batch.indices.size())
			});
			combinedMesh.indices.insert(combinedMesh.indices.end(), batch.indices.begin(), batch.indices.end());
		}
	}

	std::unique_ptr<CMesh> mesh = std::make_unique<CMesh>();
	if (FAILED(mesh->CreateBuffer(graphics.device, graphics.commandList, combinedMesh)))
	{
		if (debug) *debug = "Failed to create model mesh buffers";
		return false;
	}

	m_upMesh = std::move(mesh);
	m_parts = std::move(parts);
	m_bounds = modelBounds;

	m_materials.clear();
	m_materials.reserve(materials.size());
	for (const CB_Material& material : materials) m_materials.emplace_back(material);

	return true;
}
//-----------------------------------

bool CModel::ReadFrameData(
	ModelData& modelData,
	std::ifstream& file, std::istringstream& iss, std::string& buffer,
	std::optional<unsigned int> parent)
{
	modelData.frames.push_back({});
	modelData.meshData.push_back({});
	const unsigned int curIndex = static_cast<unsigned int>(modelData.frames.size() - 1);

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
			if (!(iss >> data)) continue;
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
				const std::unordered_map<std::string, FrameReader>::const_iterator iter = parseByTag.find(data);
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

void CModel::ReleaseUploadBuffer()
{
	if (m_upMesh) m_upMesh->ReleaseUploadBuffer();
}

int CModel::GetFrameIndex(const std::string& frameName) const
{
	for (size_t i = 1; i < m_parts.size(); ++i)
	{
		if (m_parts[i].name == frameName) return static_cast<int>(i);
	}
	return -1;
}

XMFLOAT4X4 CModel::GetFrameMatrix(const std::string& frameName) const
{
	return GetFrameMatrix(frameName, {});
}

XMFLOAT4X4 CModel::GetFrameMatrix(const std::string& frameName,
	const std::unordered_map<int, XMFLOAT4X4>& frameTransforms) const
{
	const int index = GetFrameIndex(frameName);
	return index < 0 ? Matrix4x4::Identity() : GetPartMatrix(static_cast<size_t>(index), frameTransforms);
}

//-----------------------------------
//[아래 코드는 AI로 구현됨]
XMFLOAT4X4 CModel::GetPartMatrix(size_t index, const std::unordered_map<int, XMFLOAT4X4>& frameTransforms) const
{
	const ModelPart& part = m_parts[index];
	XMFLOAT4X4 local = part.local;
	const std::unordered_map<int, XMFLOAT4X4>::const_iterator transform = frameTransforms.find(static_cast<int>(index));
	if (transform != frameTransforms.end())
	{
		Transform pivot;
		pivot.position = { local._41, local._42, local._43 };
		const XMFLOAT4X4 pivotMatrix = pivot.getMatrix();
		const XMFLOAT4X4 rotation = Matrix4x4::Multiply(
			Matrix4x4::Multiply(Matrix4x4::Inverse(pivotMatrix), transform->second), pivotMatrix);
		local = Matrix4x4::Multiply(local, rotation);
	}

	if (part.parent != 0)
		local = Matrix4x4::Multiply(local, GetPartMatrix(part.parent, frameTransforms));
	return local;
}

void CModel::DrawModel(CFrameConstants& constants, const std::unordered_map<int, XMFLOAT4X4>& frameTransforms,
	const XMFLOAT4X4& objWorld)
{
	if (!m_upMesh) return;

	const ComPtr<ID3D12GraphicsCommandList>& cpCommandList = constants.GetCommandList();
	for (size_t i = 0; i < m_parts.size(); ++i)
	{
		const ModelPart& part = m_parts[i];
		if (part.subMeshes.empty()) continue;

		CB_GameObjInfo objInfo;
		objInfo.xmf4x4World = Matrix4x4::Transpose(Matrix4x4::Multiply(GetPartMatrix(i, frameTransforms), objWorld));
		objInfo.xmf4x4WorldInv = Matrix4x4::Inverse(objInfo.xmf4x4World);
		if (!constants.Bind(0, objInfo)) return;

		for (const SubMeshData& subMesh : part.subMeshes)
		{
			if (!m_materials[subMesh.materialIndex].BindMaterialData(constants)) return;
			m_upMesh->DrawMesh(cpCommandList, 0, subMesh);
		}
	}
}
//-----------------------------------
