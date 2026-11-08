#pragma once

class CModelObject;
class CMesh;
class CShader;
class CTextureShader;
class CModel;
class CTerrainObject;
class CCamera;
class CTankPlayer;

class CScene
{
public:
	CScene();
	virtual ~CScene();

	virtual HRESULT InitScene(ComPtr<ID3D12Device>& cpDevice, ComPtr<ID3D12RootSignature>& cpRootSignature);

	virtual HRESULT BuildObjects(ComPtr<ID3D12Device>& cpDevice, ComPtr<ID3D12GraphicsCommandList>& cpCommandList);
	void AddObject(std::unique_ptr<CModelObject>& upObject);
	void AddObjects(std::unique_ptr<CModelObject>* upObjects, size_t size);
	void AddObjects(std::vector<std::unique_ptr<CModelObject>>& upObjects);

	std::shared_ptr<CCamera> GetCamera() { return m_spCamera; }
	void WindowResized(int width, int height);

	virtual void ProcessInput(UCHAR* pKeyBuffer, float deltaTime);

	virtual void ReleaseUploadBuffers();
	void ReleaseObjects();

	virtual void DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList);

protected:
	std::shared_ptr<CCamera>					m_spCamera;
	std::shared_ptr<CShader>					m_spShader;
	std::vector<std::unique_ptr<CModelObject>>	m_vObjects;
};

class CGameScene : public CScene
{
public:
	CGameScene();
	~CGameScene();
	HRESULT InitScene(ComPtr<ID3D12Device>& cpDevice, ComPtr<ID3D12RootSignature>& cpRootSignature);
	
	HRESULT BuildObjects(ComPtr<ID3D12Device>& cpDevice, ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;

	void ProcessInput(UCHAR* pKeyBuffer, float deltaTime);

	void ReleaseUploadBuffers() override;
	void ReleaseObjects();

	void DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList);
private:
	CTankPlayer*					m_pPlayer;
	std::shared_ptr<CModel>			m_spTankModel;
	std::shared_ptr<CTextureShader> m_spTexShader;

	std::unique_ptr<CTerrainObject>	m_upTerrainObj;
};