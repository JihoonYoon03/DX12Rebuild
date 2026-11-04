#pragma once

class CGameObject;

class CScene
{
public:
	CScene();
	~CScene();

	void AddObject(std::unique_ptr<CGameObject>& upObject);
	void AddObjects(std::unique_ptr<CGameObject>* upObjects, size_t size);
	void AddObjects(std::vector<std::unique_ptr<CGameObject>>& upObjects);

	//void ReleaseObjectsUploadBuffer();

	void DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList);

private:
	std::vector<std::unique_ptr<CGameObject>>	m_vObjects;
};

