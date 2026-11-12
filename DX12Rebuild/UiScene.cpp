//-----------------------------------
//[아래 코드는 AI로 구현됨]
#include "pch.h"
#include "UiScene.h"
#include "GameFramework.h"
#include "Shader.h"
#include "Texture.h"

namespace
{
	class COpenStageSelectButton : public CMaskedUI
	{
	public:
		using CMaskedUI::CMaskedUI;

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
		CStageButton(const XMFLOAT4& bounds, const XMFLOAT4& color, EGameStage stage, const wchar_t* textureFile)
			: CUI(bounds, color, textureFile), m_stage(stage)
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

	class CTutorialReturnButton final : public CUI
	{
	public:
		using CUI::CUI;

		bool OnClick(CGameFramework& framework) override
		{
			framework.OpenStageSelect();
			return true;
		}

		bool HitTest(float, float) const override { return true; }
	};
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
CUI::CUI(const XMFLOAT4& bounds, const XMFLOAT4& color, const wchar_t* textureFile)
	: m_color(color), m_textureFile(textureFile ? textureFile : L"")
{
	SetPosition((bounds.x + bounds.z) * 0.5f - 0.5f, 0.5f - (bounds.y + bounds.w) * 0.5f, 0.5f);
	SetScale(bounds.z - bounds.x, bounds.w - bounds.y, 1.0f);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
HRESULT CUI::BuildMesh(const ComPtr<ID3D12Device>& cpDevice, const ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
	CDescHeapAllocator* allocator)
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
	HRESULT hr = m_mesh.CreateBuffer(cpDevice, cpCommandList, meshData);
	if (FAILED(hr)) return hr;
	if (m_textureFile.empty()) return S_OK;
	if (!allocator) return E_INVALIDARG;

	auto texture = std::make_shared<CTexture>();
	hr = texture->CreateTexture(*allocator, cpDevice, cpCommandList, m_textureFile.c_str(), true);
	if (FAILED(hr))
	{
		OutputDebugStringW(m_textureFile.c_str());
		OutputDebugStringW(L": UI texture load failed\n");
		return hr;
	}
	return BindTexture(texture);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
HRESULT CUI::BindTexture(std::shared_ptr<CTexture>& texture)
{
	if (!texture) return E_INVALIDARG;
	m_spUITexture = texture;
	return S_OK;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CUI::ReleaseUploadBuffer()
{
	m_mesh.ReleaseUploadBuffer();
	if (m_spUITexture) m_spUITexture->ReleaseUploadBuffer();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CUI::DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	if (!m_bVisible) return;
	cpCommandList->SetGraphicsRoot32BitConstants(1, 4, &m_color, 0);
	const XMFLOAT4X4 world = Matrix4x4::Transpose(m_Transform.getMatrix());
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &world, 0);
	const XMFLOAT4X4 worldInv = Matrix4x4::Inverse(world);
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &worldInv, 16);

	if (m_spUITexture)
	{
		cpCommandList->SetGraphicsRootDescriptorTable(3, m_spUITexture->getGPUDescHandle());
	}

	m_mesh.DrawMesh(cpCommandList);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
bool CUI::HitTest(float x, float y) const
{
	const XMFLOAT4X4 inverse = Matrix4x4::Inverse(m_Transform.getMatrix());
	const XMFLOAT3 local = Vector3::TransformCoord({ x, y, m_Transform.position.z }, inverse);
	return m_bVisible && std::abs(local.x) <= 0.5f && std::abs(local.y) <= 0.5f;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
CUiScene::CUiScene(const std::shared_ptr<CShader>& spShader)
{
	m_spShader = spShader;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
HRESULT CUiScene::BuildObjects(
	ComPtr<ID3D12Device>& cpDevice,
	ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
	CDescHeapAllocator* DescHeapAllocator)
{
	m_vUiObjects.clear();
	BuildUi();
	for (const auto& upUi : m_vUiObjects)
	{
		const HRESULT hr = upUi->BuildMesh(cpDevice, cpCommandList, DescHeapAllocator);
		if (FAILED(hr)) return hr;
	}
	return S_OK;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CUiScene::ReleaseUploadBuffers()
{
	for (const auto& upUi : m_vUiObjects) upUi->ReleaseUploadBuffer();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CUiScene::ResizeUi(int width, int height)
{
	if (width <= 0 || height <= 0) return;

	m_fCanvasPixels = static_cast<float>(width < height ? width : height);
	m_fViewWidth = static_cast<float>(width) / m_fCanvasPixels;
	m_fViewHeight = static_cast<float>(height) / m_fCanvasPixels;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CUiScene::Click(int x, int y, CGameFramework& framework)
{
	const float uiX = static_cast<float>(x) / m_fCanvasPixels - m_fViewWidth * 0.5f;
	const float uiY = m_fViewHeight * 0.5f - static_cast<float>(y) / m_fCanvasPixels;
	for (const auto& upUi : m_vUiObjects)
	{
		if (upUi->HitTest(uiX, uiY) && upUi->OnClick(framework)) return;
	}
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CUiScene::DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	//---------------------------------------
	//[직접 작성한 코드]
	m_spShader->SetGraphicsRootSignature(cpCommandList);
	//---------------------------------------
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	m_spShader->SetPipelineState(cpCommandList, ERenderState::UI);
	//-----------------------------------
	const XMFLOAT4X4 view = Matrix4x4::Identity();
	const XMFLOAT4X4 projection = Matrix4x4::Transpose(Matrix4x4::OrthographicLH(m_fViewWidth, m_fViewHeight, 0.0f, 1.0f));
	const XMFLOAT4 color{ 1.0f, 1.0f, 1.0f, 1.0f };

	//---------------------------------------
	//[직접 작성한 코드]
	//실제 시간값이 아직 아님에 주의
	m_fTime += 0.001f;
	//---------------------------------------
	const CB_SceneInfo sceneInfo{ view, projection, view, m_fTime };
	m_spShader->SetSceneMatrix(cpCommandList, sceneInfo);
	const float tiling = 1.0f;
	cpCommandList->SetGraphicsRoot32BitConstants(1, 1, &tiling, 4);
	cpCommandList->SetGraphicsRoot32BitConstants(1, 4, &color, 0);
	for (const auto& upUi : m_vUiObjects)
	{
		m_spShader->SetPipelineState(cpCommandList, upUi->GetRenderState());
		upUi->DrawObject(cpCommandList);
	}
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CUiScene::AddUi(std::unique_ptr<CUI> upUi)
{
	m_vUiObjects.push_back(std::move(upUi));
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CMainMenuScene::BuildUi()
{
	const XMFLOAT4 white{ 1, 1, 1, 1 };
	const float titleHeight = .60f * 513.0f / 3300.0f;
	const float playHeight = .24f * 273.0f / 660.0f;
	const XMFLOAT4 title{ .20f, .36f - titleHeight * .5f, .80f, .36f + titleHeight * .5f };
	const XMFLOAT4 play{ .38f, .54f - playHeight * .5f, .62f, .54f + playHeight * .5f };
	AddUi(std::make_unique<CUI>(title, white, L"../Resources/UI/Title.png"));
	AddUi(std::make_unique<COpenStageSelectButton>(play, white,
		L"../Resources/UI/PlayTex.png", L"../Resources/UI/PlayMask2.png"));
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CStageSelectScene::BuildUi()
{
	const XMFLOAT4 white{ 1, 1, 1, 1 };
	const float headingHeight = .30f * 463.0f / 1908.0f;
	AddUi(std::make_unique<CUI>(XMFLOAT4{ .35f, .235f - headingHeight * .5f, .65f, .235f + headingHeight * .5f },
		white, L"../Resources/UI/SelectStage.png"));

	constexpr float firstCenter = .33188919f;
	constexpr float spacing = .076414005f;
	// PNG 원본 크기에 공통 배율을 적용해 글자와 이미지의 비율을 보존한다.
	const XMFLOAT2 imageSizes[]
	{
		{ 1827, 463 }, // Howtoplay
		{ 1827, 276 }, // TargetPractice
		{ 1827, 463 }, // TankBattle
		{ 2194, 463 }, // HelicopterBattle
		{ 1549, 463 }, // StartGame
		{ 690, 463 }   // Exit
	};
	constexpr float imageScale = .07f / 463.0f;
	XMFLOAT4 buttons[6];
	for (int i = 0; i < 6; ++i)
	{
		const float y = firstCenter + i * spacing;
		const float halfWidth = imageSizes[i].x * imageScale * .5f;
		const float halfHeight = imageSizes[i].y * imageScale * .5f;
		buttons[i] = { .5f - halfWidth, y - halfHeight, .5f + halfWidth, y + halfHeight };
	}
	AddUi(std::make_unique<COpenTutorialButton>(buttons[0], white, L"../Resources/UI/Howtoplay.png"));
	AddUi(std::make_unique<CStageButton>(buttons[1], white, EGameStage::HelicopterTargets, L"../Resources/UI/TargetPractice.png"));
	AddUi(std::make_unique<CStageButton>(buttons[2], white, EGameStage::TankBattle, L"../Resources/UI/TankBattle.png"));
	AddUi(std::make_unique<CStageButton>(buttons[3], white, EGameStage::HelicopterBattle, L"../Resources/UI/HelicopterBattle.png"));
	AddUi(std::make_unique<CStageButton>(buttons[4], white, EGameStage::HelicopterTargets, L"../Resources/UI/StartGame.png"));
	AddUi(std::make_unique<CExitButton>(buttons[5], white, L"../Resources/UI/Exit.png"));
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CPortingTutorialScene::BuildUi()
{
	const XMFLOAT4 white{ 1, 1, 1, 1 };
	const float cardHeight = .82f;
	const float cardWidth = cardHeight * 939.0f / 1814.0f;
	const float returnWidth = .70f;
	const float returnHeight = returnWidth * 174.0f / 1404.0f;
	const XMFLOAT4 card{ .5f - cardWidth * .5f, .06f, .5f + cardWidth * .5f, .06f + cardHeight };
	const XMFLOAT4 hint{ .5f - returnWidth * .5f, .94f - returnHeight * .5f,
		.5f + returnWidth * .5f, .94f + returnHeight * .5f };
	AddUi(std::make_unique<CUI>(card, white, L"../Resources/UI/Tutorial.png"));
	AddUi(std::make_unique<CTutorialReturnButton>(hint, white, L"../Resources/UI/TutorialReturn.png"));
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CPortingTutorialScene::KeyPressed(WPARAM key, CGameFramework& framework)
{
	if (key == VK_ESCAPE) framework.OpenStageSelect();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
ERenderState CUI::GetRenderState() const
{
	return HasTexture() ? ERenderState::UI : ERenderState::Unlit;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
CMaskedUI::CMaskedUI(const XMFLOAT4& bounds, const XMFLOAT4& color,
	const wchar_t* textureFile, const wchar_t* maskTextureFile)
	: CUI(bounds, color, textureFile), m_maskTextureFile(maskTextureFile ? maskTextureFile : L"")
{
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
HRESULT CMaskedUI::BuildMesh(const ComPtr<ID3D12Device>& cpDevice,
	const ComPtr<ID3D12GraphicsCommandList>& cpCommandList, CDescHeapAllocator* allocator)
{
	HRESULT hr = CUI::BuildMesh(cpDevice, cpCommandList, allocator);
	if (FAILED(hr)) return hr;
	if (m_spMaskTexture) return S_OK;
	if (!allocator || m_maskTextureFile.empty()) return E_INVALIDARG;

	auto mask = std::make_shared<CTexture>();
	hr = mask->CreateTexture(*allocator, cpDevice, cpCommandList, m_maskTextureFile.c_str(), true);
	if (FAILED(hr))
	{
		OutputDebugStringW(m_maskTextureFile.c_str());
		OutputDebugStringW(L": UI mask texture load failed\n");
		return hr;
	}
	SetMaskTexture(mask);
	return S_OK;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
ERenderState CMaskedUI::GetRenderState() const
{
	return ERenderState::MaskedUI;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CMaskedUI::DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	if (!HasTexture() || !m_spMaskTexture) return;
	cpCommandList->SetGraphicsRootDescriptorTable(5, m_spMaskTexture->getGPUDescHandle());
	CUI::DrawObject(cpCommandList);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CMaskedUI::ReleaseUploadBuffer()
{
	CUI::ReleaseUploadBuffer();
	if (m_spMaskTexture) m_spMaskTexture->ReleaseUploadBuffer();
}
//-----------------------------------
