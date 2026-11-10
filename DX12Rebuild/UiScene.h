//-----------------------------------
//[아래 코드는 AI로 구현됨]
#pragma once

#include "Scene.h"
#include "Mesh.h"

enum class EUiAction
{
	None,
	OpenStageSelect,
	OpenTutorial,
	Level1,
	Level2,
	Level3,
	Exit
};

class CUiScene : public CScene
{
public:
	explicit CUiScene(const std::shared_ptr<CShader>& spShader);

	HRESULT BuildObjects(ComPtr<ID3D12Device>& cpDevice, ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;
	void ReleaseUploadBuffers() override;
	void DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;

	void ResizeUi(int width, int height);
	EUiAction HitTest(int x, int y) const;

protected:
	virtual void BuildUi(MeshData<VertexTexcoord>& meshData) = 0;
	void AddPanel(MeshData<VertexTexcoord>& meshData, float left, float top, float right, float bottom,
		const XMFLOAT4& color, EUiAction action = EUiAction::None);

private:
	struct UiPanel
	{
		float left;
		float top;
		float right;
		float bottom;
		EUiAction action;
	};

	CMesh					m_uiMesh;
	std::vector<UiPanel>	m_vPanels;
	float					m_fCanvasPixels = 1.0f;
	float					m_fViewWidth = 1.0f;
	float					m_fViewHeight = 1.0f;
};

class CMainMenuScene : public CUiScene
{
public:
	using CUiScene::CUiScene;

private:
	void BuildUi(MeshData<VertexTexcoord>& meshData) override;
};

class CStageSelectScene : public CUiScene
{
public:
	using CUiScene::CUiScene;

private:
	void BuildUi(MeshData<VertexTexcoord>& meshData) override;
};

class CPortingTutorialScene : public CUiScene
{
public:
	using CUiScene::CUiScene;

private:
	void BuildUi(MeshData<VertexTexcoord>& meshData) override;
};
//-----------------------------------
