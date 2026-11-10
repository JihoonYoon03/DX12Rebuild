//-----------------------------------
//[아래 코드는 AI로 구현됨]
#pragma once

#include "Scene.h"
#include "GameObject.h"
#include "Mesh.h"

class CGameFramework;

class CUI : public CObjectBase
{
public:
	CUI(const XMFLOAT4& bounds, const XMFLOAT4& color);

	HRESULT BuildMesh(const ComPtr<ID3D12Device>& cpDevice, const ComPtr<ID3D12GraphicsCommandList>& cpCommandList);
	void ReleaseUploadBuffer();
	void DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;

	virtual bool HitTest(float x, float y) const;
	virtual bool OnClick(CGameFramework&) { return false; }

private:
	CMesh m_mesh;
	XMFLOAT4 m_color;
};

class CUiScene : public CScene
{
public:
	explicit CUiScene(const std::shared_ptr<CShader>& spShader);

	HRESULT BuildObjects(ComPtr<ID3D12Device>& cpDevice, ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;
	void ReleaseUploadBuffers() override;
	void DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;

	void ResizeUi(int width, int height);
	void Click(int x, int y, CGameFramework& framework);

protected:
	virtual void BuildUi() = 0;
	void AddUi(std::unique_ptr<CUI> upUi);

private:
	std::vector<std::unique_ptr<CUI>> m_vUiObjects;

	float m_fCanvasPixels = 1.0f;
	float m_fViewWidth = 1.0f;
	float m_fViewHeight = 1.0f;
};

class CMainMenuScene : public CUiScene
{
public:
	using CUiScene::CUiScene;

private:
	void BuildUi() override;
};

class CStageSelectScene : public CUiScene
{
public:
	using CUiScene::CUiScene;

private:
	void BuildUi() override;
};

class CPortingTutorialScene : public CUiScene
{
public:
	using CUiScene::CUiScene;

private:
	void BuildUi() override;
};
//-----------------------------------
