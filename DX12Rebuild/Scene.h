#pragma once
#include "Camera.h"

class CGameObject;
class CMesh;

class CScene
{
public:
	CScene();
	virtual ~CScene();

	virtual HRESULT BuildObjects(ComPtr<ID3D12Device>& cpDevice, ComPtr<ID3D12GraphicsCommandList>& cpCommandList);
	void AddObject(std::unique_ptr<CGameObject>& upObject);
	void AddObjects(std::unique_ptr<CGameObject>* upObjects, size_t size);
	void AddObjects(std::vector<std::unique_ptr<CGameObject>>& upObjects);

	CCamera& GetCamera() { return m_Camera; }
	void WindowResized(int width, int height);

	virtual void ProcessInput(UCHAR* pKeyBuffer, float deltaTime);

	//void ReleaseObjectsUploadBuffer();

	virtual void DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList);

private:
	CCamera		m_Camera;
	std::vector<std::unique_ptr<CGameObject>>	m_vObjects;
};

class CGameScene : public CScene
{
public:
	CGameScene();
	~CGameScene();
	HRESULT BuildObjects(ComPtr<ID3D12Device>& cpDevice, ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;

	void ReleaseUploadBuffers();

	void ReleaseObjects();

	void DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList);
private:
	std::shared_ptr<CShader>	m_spShader;
	std::shared_ptr<CModel>		m_spModel;

	std::unique_ptr<CMesh>		m_upMesh;
};