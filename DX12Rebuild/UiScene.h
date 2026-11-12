//-----------------------------------
//[아래 코드는 AI로 구현됨]
#pragma once

#include "Scene.h"
#include "GameObject.h"
#include "Mesh.h"

class CGameFramework;
class CTexture;
class CShader;
//-----------------------------------
//[아래 코드는 AI로 구현됨]
enum class ERenderState;
//-----------------------------------

class CUI : public CObjectBase
{
public:
	CUI(const XMFLOAT4& bounds, const XMFLOAT4& color, const wchar_t* textureFile = nullptr);

	virtual HRESULT BuildMesh(const ComPtr<ID3D12Device>& cpDevice, const ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
		CDescHeapAllocator* allocator = nullptr);
	HRESULT BindTexture(std::shared_ptr<CTexture>& texture);
	virtual void ReleaseUploadBuffer();
	void SetColor(const XMFLOAT4& color) { m_color = color; }
	void SetVisible(bool visible) { m_bVisible = visible; }
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	bool HasTexture() const { return m_spUITexture != nullptr; }
	virtual ERenderState GetRenderState() const;
	//-----------------------------------
	void DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;

	virtual bool HitTest(float x, float y) const;
	virtual bool OnClick(CGameFramework&) { return false; }

private:
	CMesh m_mesh;
	//---------------------------
	//[직접 작성한 코드]
	std::shared_ptr<CTexture>	m_spUITexture;
	//---------------------------
	XMFLOAT4 m_color;
	bool m_bVisible = true;
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	std::wstring m_textureFile;
	//-----------------------------------
};

//-----------------------------------
//[직접 작성한 코드]
class CMaskedUI : public CUI
{
public:
	using CUI::CUI;

	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	CMaskedUI(const XMFLOAT4& bounds, const XMFLOAT4& color,
		const wchar_t* textureFile, const wchar_t* maskTextureFile);
	HRESULT BuildMesh(const ComPtr<ID3D12Device>& cpDevice,
		const ComPtr<ID3D12GraphicsCommandList>& cpCommandList, CDescHeapAllocator* allocator = nullptr) override;
	ERenderState GetRenderState() const override;
	void DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;
	void ReleaseUploadBuffer() override;
	//-----------------------------------

	void SetMaskTexture(std::shared_ptr<CTexture>& maskTexture) { m_spMaskTexture = maskTexture; }
private:
	std::shared_ptr<CTexture>	m_spMaskTexture;
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	std::wstring m_maskTextureFile;
	//-----------------------------------
};

//-----------------------------------

class CUiScene : public CScene
{
public:
	explicit CUiScene(const std::shared_ptr<CShader>& spShader);

	HRESULT BuildObjects(
		ComPtr<ID3D12Device>& cpDevice,
		ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
		CDescHeapAllocator* DescHeapAllocator = nullptr) override;
	void ReleaseUploadBuffers() override;
	void DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;

	void ResizeUi(int width, int height);
	void Click(int x, int y, CGameFramework& framework) override;

protected:
	virtual void BuildUi() = 0;
	void AddUi(std::unique_ptr<CUI> upUi);

private:
	std::vector<std::unique_ptr<CUI>> m_vUiObjects;

	float m_fCanvasPixels = 1.0f;
	float m_fViewWidth = 1.0f;
	float m_fViewHeight = 1.0f;
	//실제 시간값 아님
	float m_fTime = 0.0f;
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
	void KeyPressed(WPARAM key, CGameFramework& framework) override;
};
//-----------------------------------
