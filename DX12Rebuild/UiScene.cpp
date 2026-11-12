//-----------------------------------
//[아래 코드는 AI로 구현됨]
#include "pch.h"
#include "UiScene.h"
#include "GameFramework.h"
#include "Shader.h"

namespace
{
	class COpenStageSelectButton : public CUI
	{
	public:
		using CUI::CUI;

		bool OnClick(CGameFramework& framework) override
		{
			framework.OpenStageSelect();
			return true;
		}
	};

	class COpenTutorialButton final : public CUI
	{
	public:
		using CUI::CUI;

		bool OnClick(CGameFramework& framework) override
		{
			framework.OpenTutorial();
			return true;
		}
	};

	class CStageButton final : public CUI
	{
	public:
		CStageButton(const XMFLOAT4& bounds, const XMFLOAT4& color, EGameStage stage)
			: CUI(bounds, color), m_stage(stage)
		{
		}

		bool OnClick(CGameFramework& framework) override
		{
			framework.EnterStage(m_stage);
			return true;
		}

	private:
		EGameStage m_stage;
	};

	class CExitButton final : public CUI
	{
	public:
		using CUI::CUI;

		bool OnClick(CGameFramework& framework) override
		{
			framework.RequestExit();
			return true;
		}
	};

	class CTutorialReturnButton final : public COpenStageSelectButton
	{
	public:
		using COpenStageSelectButton::COpenStageSelectButton;

		bool HitTest(float, float) const override { return true; }
	};
}

CUI::CUI(const XMFLOAT4& bounds, const XMFLOAT4& color)
	: m_color(color)
{
	SetPosition((bounds.x + bounds.z) * 0.5f - 0.5f, 0.5f - (bounds.y + bounds.w) * 0.5f, 0.5f);
	SetScale(bounds.z - bounds.x, bounds.w - bounds.y, 1.0f);
}

HRESULT CUI::BuildMesh(const ComPtr<ID3D12Device>& cpDevice, const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	MeshData<VertexTexcoord> meshData;
	meshData.vertices =
	{
		{ { -0.5f,  0.5f, 0.0f }, { 1, 1, 1, 1 }, { 0.0f, 0.0f } },
		{ {  0.5f,  0.5f, 0.0f }, { 1, 1, 1, 1 }, { 1.0f, 0.0f } },
		{ {  0.5f, -0.5f, 0.0f }, { 1, 1, 1, 1 }, { 1.0f, 1.0f } },
		{ { -0.5f, -0.5f, 0.0f }, { 1, 1, 1, 1 }, { 0.0f, 1.0f } }
	};
	meshData.indices = { 0, 1, 2, 0, 2, 3 };
	return m_mesh.CreateBuffer(cpDevice, cpCommandList, meshData);
}

void CUI::ReleaseUploadBuffer()
{
	m_mesh.ReleaseUploadBuffer();
}

void CUI::DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	if (!m_bVisible) return;
	cpCommandList->SetGraphicsRoot32BitConstants(1, 4, &m_color, 0);
	const XMFLOAT4X4 world = Matrix4x4::Transpose(m_Transform.getMatrix());
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &world, 0);
	const XMFLOAT4X4 worldInv = Matrix4x4::Inverse(world);
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &worldInv, 16);
	m_mesh.DrawMesh(cpCommandList);
}

bool CUI::HitTest(float x, float y) const
{
	const XMFLOAT4X4 inverse = Matrix4x4::Inverse(m_Transform.getMatrix());
	const XMFLOAT3 local = Vector3::TransformCoord({ x, y, m_Transform.position.z }, inverse);
	return m_bVisible && std::abs(local.x) <= 0.5f && std::abs(local.y) <= 0.5f;
}

CUiScene::CUiScene(const std::shared_ptr<CShader>& spShader)
{
	m_spShader = spShader;
}

HRESULT CUiScene::BuildObjects(
	ComPtr<ID3D12Device>& cpDevice,
	ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
	CDescHeapAllocator* DescHeapAllocator)
{
	m_vUiObjects.clear();
	BuildUi();
	for (const auto& upUi : m_vUiObjects)
	{
		const HRESULT hr = upUi->BuildMesh(cpDevice, cpCommandList);
		if (FAILED(hr)) return hr;
	}
	return S_OK;
}

void CUiScene::ReleaseUploadBuffers()
{
	for (const auto& upUi : m_vUiObjects) upUi->ReleaseUploadBuffer();
}

void CUiScene::ResizeUi(int width, int height)
{
	if (width <= 0 || height <= 0) return;

	m_fCanvasPixels = static_cast<float>(width < height ? width : height);
	m_fViewWidth = static_cast<float>(width) / m_fCanvasPixels;
	m_fViewHeight = static_cast<float>(height) / m_fCanvasPixels;
}

void CUiScene::Click(int x, int y, CGameFramework& framework)
{
	const float uiX = static_cast<float>(x) / m_fCanvasPixels - m_fViewWidth * 0.5f;
	const float uiY = m_fViewHeight * 0.5f - static_cast<float>(y) / m_fCanvasPixels;
	for (const auto& upUi : m_vUiObjects)
	{
		if (upUi->HitTest(uiX, uiY) && upUi->OnClick(framework)) return;
	}
}

void CUiScene::DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	//---------------------------------------
	//[직접 작성한 코드]
	m_spShader->SetGraphicsRootSignature(cpCommandList);
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	m_spShader->SetPipelineState(cpCommandList, ERenderState::UI);
	//-----------------------------------
	//---------------------------------------
	const XMFLOAT4X4 view = Matrix4x4::Identity();
	const XMFLOAT4X4 projection = Matrix4x4::Transpose(Matrix4x4::OrthographicLH(m_fViewWidth, m_fViewHeight, 0.0f, 1.0f));
	const XMFLOAT4 color{ 1.0f, 1.0f, 1.0f, 1.0f };

	const CB_SceneInfo sceneInfo{ view, projection, view };
	m_spShader->SetSceneMatrix(cpCommandList, sceneInfo);
	const float tiling = 1.0f;
	cpCommandList->SetGraphicsRoot32BitConstants(1, 1, &tiling, 4);
	cpCommandList->SetGraphicsRoot32BitConstants(1, 4, &color, 0);
	for (const auto& upUi : m_vUiObjects) upUi->DrawObject(cpCommandList);
}

void CUiScene::AddUi(std::unique_ptr<CUI> upUi)
{
	m_vUiObjects.push_back(std::move(upUi));
}

void CMainMenuScene::BuildUi()
{
	const XMFLOAT4 title{ .20f, .31f, .80f, .41f };
	const XMFLOAT4 play{ .38f, .45f, .62f, .63f };
	const XMFLOAT4 hint{ .36f, .68f, .64f, .75f };
	AddUi(std::make_unique<CUI>(title, XMFLOAT4{ .65f, .88f, 1.0f, 1.0f }));
	AddUi(std::make_unique<COpenStageSelectButton>(play, XMFLOAT4{ .12f, .30f, .46f, 1.0f }));
	AddUi(std::make_unique<CUI>(hint, XMFLOAT4{ .48f, .66f, .76f, 1.0f }));
}

void CStageSelectScene::BuildUi()
{
	AddUi(std::make_unique<CUI>(XMFLOAT4{ .35f, .20f, .65f, .27f }, XMFLOAT4{ .65f, .88f, 1.0f, 1.0f }));

	constexpr float firstCenter = .33188919f;
	constexpr float spacing = .076414005f;
	XMFLOAT4 buttons[6];
	for (int i = 0; i < 6; ++i)
	{
		const float y = firstCenter + i * spacing;
		buttons[i] = { .36f, y - .029f, .64f, y + .029f };
	}

	AddUi(std::make_unique<COpenTutorialButton>(buttons[0], XMFLOAT4{ .16f, .37f, .46f, 1.0f }));
	AddUi(std::make_unique<CStageButton>(buttons[1], XMFLOAT4{ .15f, .31f, .55f, 1.0f }, EGameStage::HelicopterTargets));
	AddUi(std::make_unique<CStageButton>(buttons[2], XMFLOAT4{ .20f, .41f, .27f, 1.0f }, EGameStage::TankBattle));
	AddUi(std::make_unique<CStageButton>(buttons[3], XMFLOAT4{ .37f, .26f, .48f, 1.0f }, EGameStage::HelicopterBattle));
	AddUi(std::make_unique<CStageButton>(buttons[4], XMFLOAT4{ .15f, .45f, .48f, 1.0f }, EGameStage::HelicopterTargets));
	AddUi(std::make_unique<CExitButton>(buttons[5], XMFLOAT4{ .49f, .22f, .23f, 1.0f }));
}

void CPortingTutorialScene::BuildUi()
{
	const XMFLOAT4 card{ .24f, .25f, .76f, .69f };
	const XMFLOAT4 hint{ .37f, .72f, .63f, .79f };
	AddUi(std::make_unique<CUI>(card, XMFLOAT4{ .13f, .29f, .39f, 1.0f }));
	AddUi(std::make_unique<CTutorialReturnButton>(hint, XMFLOAT4{ .15f, .46f, .48f, 1.0f }));
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CPortingTutorialScene::KeyPressed(WPARAM key, CGameFramework& framework)
{
	if (key == VK_ESCAPE) framework.OpenStageSelect();
}
//-----------------------------------
