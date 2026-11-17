#pragma once

class CObjectBase;
class CModelObject;
class CTerrainObject;
class CMeshObject;
class CObstacleObject;
class CDamageableObject;
class CVehicleObject;
class CEnemyTank;
class CProjectileObject;
class CTankPlayer;
class CHelicopterPlayer;
class CGameHud;
class CUI;
class CMesh;
class CModel;
class CShader;
class CCamera;
class CGameFramework;
class CDescHeapAllocator;
struct ProjectileData;

//-----------------------------------
//[아래 코드는 AI로 구현됨]
enum class EGameStage
{
	HelicopterTargets,
	TankBattle,
	HelicopterBattle
};
//-----------------------------------

// -------------------------------------------
//
// Scene
//
// -------------------------------------------
//-----------------------------------
//[직접 작성한 코드]
class CScene
{
public:
	CScene();
	virtual ~CScene();

	virtual HRESULT InitScene(ComPtr<ID3D12Device>& cpDevice,
		ComPtr<ID3D12GraphicsCommandList>& cpCommandList, CDescHeapAllocator& descHeapAllocator);

	virtual HRESULT BuildObjects(
		ComPtr<ID3D12Device>& cpDevice,
		ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
		CDescHeapAllocator* DescHeapAllocator = nullptr
	);

	CObjectBase* AddObject(std::unique_ptr<CObjectBase> object);

	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	void RemoveObject(const CObjectBase* object);
	//-----------------------------------
	void AddObjects(std::unique_ptr<CModelObject>* upObjects, size_t size);
	void AddObjects(std::vector<std::unique_ptr<CModelObject>>& upObjects);

	std::shared_ptr<CCamera> GetCamera() { return m_spCamera; }
	void WindowResized(int width, int height);

	virtual void ProcessInput(UCHAR* pKeyBuffer, float deltaTime);
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	virtual void Update(float deltaTime) {}
	virtual void KeyPressed(WPARAM key, CGameFramework& framework) {}
	virtual void Click(int x, int y, CGameFramework& framework) {}

	virtual void CheckStageTransition(CGameFramework& framework) {}
	//-----------------------------------

	virtual void ReleaseUploadBuffers();
	virtual void ReleaseObjects();

	virtual void DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList);

protected:
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	void DrawObjects(const ComPtr<ID3D12GraphicsCommandList>& commandList, bool screenSpace);
	//-----------------------------------
	std::shared_ptr<CCamera>					m_spCamera;
	std::shared_ptr<CShader>					m_spShader;

	std::vector<std::unique_ptr<CObjectBase>>	m_vObjects;
};
//-----------------------------------

// -------------------------------------------
//
// GameScene
//
// -------------------------------------------
class CGameScene : public CScene
{
public:
	CGameScene();
	~CGameScene() override;

	HRESULT BuildObjects(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& list,
		CDescHeapAllocator* allocator = nullptr) override;
	void ReleaseUploadBuffers() override;
	void ReleaseObjects() override;

	void DrawScene(ComPtr<ID3D12GraphicsCommandList>& list) override;

	void ProcessInput(UCHAR* keys, float dt) override;
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	void Update(float dt) override;

	void KeyPressed(WPARAM key, CGameFramework& framework) override;
	void CheckStageTransition(CGameFramework& framework) override;

	void BeginStage(EGameStage stage);

	EGameStage GetStage() const { return m_stage; }
	bool StageFinished() const { return m_state == EStageState::Finished; }
	//-----------------------------------

protected:
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	virtual void ResetStage() = 0;
	virtual void ResolvePlayerMovement(const Transform& previous) {}

	virtual bool HasReachedExit() const { return false; }
	virtual float VictoryHoldTime() const { return config::stage::VictoryDelay; }

	void SetPlayer(CVehicleObject* player);
	const CTerrainObject& Terrain() const { return *m_pTerrain; }
	XMFLOAT3 OnTerrain(float x, float z, float offset) const;

	HRESULT BuildHud(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& list,
		CDescHeapAllocator& allocator);

	std::shared_ptr<CModel> LoadModel(const ComPtr<ID3D12Device>& device,
		const ComPtr<ID3D12GraphicsCommandList>& list, const char* path, const XMFLOAT3& offset = {});
	std::shared_ptr<CModel> LoadTankModel(const ComPtr<ID3D12Device>& device,
		const ComPtr<ID3D12GraphicsCommandList>& list);
	std::shared_ptr<CModel> FindModel(const char* path) const;

	CObjectBase* AddStageObject(std::unique_ptr<CObjectBase> object);
	void AddTarget(const XMFLOAT3& position);
	void AddEnemyTank(float x, float z, float playerStartZ);
	CMeshObject* AddStageBox(const XMFLOAT3& position, const XMFLOAT3& size, const XMFLOAT4& color);

private:
	enum class EStageState { Playing, Victory, Defeat, Finished };

	void UpdateEnemies(float dt);

	void UpdateProjectiles(float dt);
	void SpawnProjectile(const XMFLOAT3& position, const ProjectileData& data);

	void ClearProjectiles();

	void ClearStageObjects();

	void UpdateLockMarker();

	void Win();
	//-----------------------------------

	// 모든 게임 오브젝트의 소유자는 CScene::m_vObjects이다.
	CTerrainObject* m_pTerrain = nullptr;
	CVehicleObject* m_pPlayer = nullptr;
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	CMeshObject* m_pLockMarker = nullptr;

	std::vector<CDamageableObject*> m_vEnemies;
	std::vector<CEnemyTank*> m_vAttackers;

	std::vector<CProjectileObject*> m_vProjectiles;

	std::vector<CObjectBase*> m_vStageObjects;

	// 파일 경로별로 실제 사용한 모델만 보관한다. 오브젝트 인스턴스는 저장하지 않는다.
	std::unordered_map<std::string, std::shared_ptr<CModel>> m_models;

	std::unique_ptr<CMesh> m_upBoxMesh;

	std::unique_ptr<CGameHud> m_upHud;

	UCHAR m_keys[256]{};

	EGameStage m_stage = EGameStage::HelicopterTargets;
	EStageState m_state = EStageState::Playing;
	float m_fResultTimer = 0.0f;
	//-----------------------------------
};

// -------------------------------------------
//
// TankGameScene
//
// -------------------------------------------
//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CTankGameScene final : public CGameScene
{
public:
	~CTankGameScene() override;

	HRESULT BuildObjects(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& list,
		CDescHeapAllocator* allocator = nullptr) override;
	void ReleaseObjects() override;

private:
	void ResetStage() override;

	void ResolvePlayerMovement(const Transform& previous) override;

	CTankPlayer* m_pPlayer = nullptr;

	std::vector<CObstacleObject*> m_vObstacles;
};
//-----------------------------------

// -------------------------------------------
//
// HelicopterGameScene
//
// -------------------------------------------
//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CHelicopterGameScene final : public CGameScene
{
public:
	~CHelicopterGameScene() override;

	HRESULT BuildObjects(ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& list,
		CDescHeapAllocator* allocator = nullptr) override;
	void ReleaseObjects() override;

private:
	void ResetStage() override;

	bool HasReachedExit() const override;
	float VictoryHoldTime() const override;

	bool IsTargetStage() const { return GetStage() == EGameStage::HelicopterTargets; }

	CHelicopterPlayer* m_pPlayer = nullptr;

	CMeshObject* m_pExitMarker = nullptr;
};
//-----------------------------------

// -------------------------------------------
//
// UiScene
//
// -------------------------------------------
//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CUiScene : public CScene
{
public:
	explicit CUiScene(const std::shared_ptr<CShader>& spShader);

	HRESULT BuildObjects(
		ComPtr<ID3D12Device>& cpDevice,
		ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
		CDescHeapAllocator* DescHeapAllocator = nullptr) override;

	void ReleaseObjects() override;

	void DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;

	void ResizeUi(int width, int height);

	void Click(int x, int y, CGameFramework& framework) override;

protected:
	virtual void BuildUi() = 0;

	void AddUi(std::unique_ptr<CUI> upUi);

private:
	std::vector<CUI*> m_vUiObjects; // m_vObjects가 소유하며 클릭 처리에서만 참조한다.

	float m_fCanvasPixels = 1.0f;
	float m_fViewWidth = 1.0f;
	float m_fViewHeight = 1.0f;

	//실제 시간값 아님
	float m_fTime = 0.0f;
};
//-----------------------------------

// -------------------------------------------
//
// MainMenuScene
//
// -------------------------------------------
//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CMainMenuScene : public CUiScene
{
public:
	using CUiScene::CUiScene;

private:
	void BuildUi() override;
};
//-----------------------------------

// -------------------------------------------
//
// StageSelectScene
//
// -------------------------------------------
//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CStageSelectScene : public CUiScene
{
public:
	using CUiScene::CUiScene;

private:
	void BuildUi() override;
};
//-----------------------------------

// -------------------------------------------
//
// PortingTutorialScene
//
// -------------------------------------------
//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CPortingTutorialScene : public CUiScene
{
public:
	using CUiScene::CUiScene;

private:
	void BuildUi() override;

	void KeyPressed(WPARAM key, CGameFramework& framework) override;
};
//-----------------------------------
