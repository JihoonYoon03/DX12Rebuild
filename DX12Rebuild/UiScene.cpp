//-----------------------------------
//[아래 코드는 AI로 구현됨]
#include "pch.h"
#include "UiScene.h"
#include "Shader.h"

CUiScene::CUiScene(const std::shared_ptr<CShader>& spShader)
{
	m_spShader = spShader;
}

HRESULT CUiScene::BuildObjects(ComPtr<ID3D12Device>& cpDevice, ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	MeshData<VertexTexcoord> meshData;
	m_vPanels.clear();
	BuildUi(meshData);
	return m_uiMesh.CreateBuffer(cpDevice, cpCommandList, meshData);
}

void CUiScene::ReleaseUploadBuffers()
{
	m_uiMesh.ReleaseUploadBuffer();
}

void CUiScene::ResizeUi(int width, int height)
{
	if (width <= 0 || height <= 0) return;

	m_fCanvasPixels = static_cast<float>(width < height ? width : height);
	m_fViewWidth = static_cast<float>(width) / m_fCanvasPixels;
	m_fViewHeight = static_cast<float>(height) / m_fCanvasPixels;
}

EUiAction CUiScene::HitTest(int x, int y) const
{
	const float uiX = static_cast<float>(x) / m_fCanvasPixels + (1.0f - m_fViewWidth) * 0.5f;
	const float uiY = static_cast<float>(y) / m_fCanvasPixels + (1.0f - m_fViewHeight) * 0.5f;

	for (const UiPanel& panel : m_vPanels)
	{
		if (panel.action != EUiAction::None &&
			uiX >= panel.left && uiX <= panel.right && uiY >= panel.top && uiY <= panel.bottom)
		{
			return panel.action;
		}
	}
	return EUiAction::None;
}

void CUiScene::DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	m_spShader->SetPipelineState(cpCommandList);

	const XMFLOAT4X4 identity = Matrix4x4::Identity();
	const XMFLOAT4X4 projection = Matrix4x4::Transpose(Matrix4x4::OrthographicLH(m_fViewWidth, m_fViewHeight, 0.0f, 1.0f));
	const XMFLOAT4 color{ 1.0f, 1.0f, 1.0f, 1.0f };

	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &identity, 0);
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &identity, 16);
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &projection, 32);
	cpCommandList->SetGraphicsRoot32BitConstants(2, 4, &color, 0);
	SubMeshData panel{};
	panel.indexAmount = 6;
	for (uint32_t i = 0; i < m_vPanels.size(); ++i)
	{
		// Each panel is its own draw, so its texture can be bound here later.
		panel.startIndex = i * panel.indexAmount;
		m_uiMesh.DrawMesh(cpCommandList, 0, panel);
	}
}

void CUiScene::AddPanel(MeshData<VertexTexcoord>& meshData, float left, float top, float right, float bottom,
	const XMFLOAT4& color, EUiAction action)
{
	m_vPanels.push_back({ left, top, right, bottom, action });

	const auto base = static_cast<uint32_t>(meshData.vertices.size());
	constexpr float depth = 0.5f;
	meshData.vertices.push_back({ { left - 0.5f, 0.5f - top, depth }, color, { 0.0f, 0.0f } });
	meshData.vertices.push_back({ { right - 0.5f, 0.5f - top, depth }, color, { 1.0f, 0.0f } });
	meshData.vertices.push_back({ { right - 0.5f, 0.5f - bottom, depth }, color, { 1.0f, 1.0f } });
	meshData.vertices.push_back({ { left - 0.5f, 0.5f - bottom, depth }, color, { 0.0f, 1.0f } });
	meshData.indices.insert(meshData.indices.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
}

void CMainMenuScene::BuildUi(MeshData<VertexTexcoord>& meshData)
{
	AddPanel(meshData, 0.20f, 0.31f, 0.80f, 0.41f, { 0.65f, 0.88f, 1.0f, 1.0f }); // Title
	AddPanel(meshData, 0.38f, 0.45f, 0.62f, 0.63f, { 0.12f, 0.30f, 0.46f, 1.0f }, EUiAction::OpenStageSelect);
	AddPanel(meshData, 0.36f, 0.68f, 0.64f, 0.75f, { 0.48f, 0.66f, 0.76f, 1.0f }); // Play hint
}

void CStageSelectScene::BuildUi(MeshData<VertexTexcoord>& meshData)
{
	//-----------------------------------
	//[직접 작성한 코드]
	constexpr EUiAction actions[] =
	{
		EUiAction::OpenTutorial, EUiAction::Level1, EUiAction::Level2,
		EUiAction::Level3, EUiAction::Level1, EUiAction::Exit
	};
	//-----------------------------------
	const XMFLOAT4 colors[] =
	{
		{ 0.16f, 0.37f, 0.46f, 1.0f }, { 0.15f, 0.31f, 0.55f, 1.0f },
		{ 0.20f, 0.41f, 0.27f, 1.0f }, { 0.37f, 0.26f, 0.48f, 1.0f },
		{ 0.15f, 0.45f, 0.48f, 1.0f }, { 0.49f, 0.22f, 0.23f, 1.0f }
	};
	AddPanel(meshData, 0.35f, 0.20f, 0.65f, 0.27f, { 0.65f, 0.88f, 1.0f, 1.0f }); // Title
	for (size_t i = 0; i < sizeof(actions) / sizeof(actions[0]); ++i)
	{
		//-----------------------------------
		//[직접 작성한 코드]
		const float y = 0.5f - (1.65f - 0.75f * static_cast<float>(i)) * 0.10188534f;
		//-----------------------------------
		AddPanel(meshData, 0.36f, y - 0.029f, 0.64f, y + 0.029f, colors[i], actions[i]);
	}
}

void CPortingTutorialScene::BuildUi(MeshData<VertexTexcoord>& meshData)
{
	AddPanel(meshData, 0.24f, 0.25f, 0.76f, 0.69f, { 0.13f, 0.29f, 0.39f, 1.0f }); // Tutorial card
	AddPanel(meshData, 0.37f, 0.72f, 0.63f, 0.79f, { 0.15f, 0.46f, 0.48f, 1.0f }); // Return hint
}
//-----------------------------------
