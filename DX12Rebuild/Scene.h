#pragma once
#include "Camera.h"

class CGameObject;

class CScene
{
public:
	CScene();
	~CScene();

	void AddObject(std::unique_ptr<CGameObject>& upObject);
	void AddObjects(std::unique_ptr<CGameObject>* upObjects, size_t size);
	void AddObjects(std::vector<std::unique_ptr<CGameObject>>& upObjects);

	CCamera& GetCamera() { return m_Camera; }
	void WindowResized(int width, int height);

	virtual void ProcessInput(UCHAR* pKeyBuffer, float deltaTime);

	//void ReleaseObjectsUploadBuffer();

	void DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList);

private:
	CCamera		m_Camera;
	std::vector<std::unique_ptr<CGameObject>>	m_vObjects;
};