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

	HRESULT InitScene(const ComPtr<ID3D12Device>& cpDevice);

	virtual HRESULT BuildObjects(const GraphicsResources& graphics);

	CObjectBase* AddObject(std::shared_ptr<CObjectBase> object);

	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	void RemoveObject(const CObjectBase* object);
	//-----------------------------------
	void AddObjects(std::shared_ptr<CModelObject>* upObjects, size_t size);
	void AddObjects(std::vector<std::shared_ptr<CModelObject>>& upObjects);

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

	virtual void DrawScene(CFrameConstants& constants);

protected:
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	void DrawObjects(CFrameConstants& constants, bool screenSpace);
	//-----------------------------------
	std::shared_ptr<CCamera>					m_spCamera;
	std::shared_ptr<CShader>					m_spShader;

	std::vector<std::shared_ptr<CObjectBase>>	m_vObjects;

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

	HRESULT BuildObjects(const GraphicsResources& graphics) override;
	void ReleaseUploadBuffers() override;
	void ReleaseObjects() override;

	void DrawScene(CFrameConstants& constants) override;

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

	std::shared_ptr<CModel> LoadModel(const GraphicsResources& graphics, const char* path, const XMFLOAT3& offset = {});

	std::shared_ptr<CModel> FindModel(const char* path) const;

	CObjectBase* AddStageObject(std::shared_ptr<CObjectBase> object);
	void AddEnemyTank(float x, float z, float playerStartZ);

	std::shared_ptr<CMesh> m_upBoxMesh;
	std::unique_ptr<CGameHud> m_upHud;

	std::vector<CDamageableObject*> m_vEnemies;

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

	std::vector<CEnemyTank*> m_vAttackers;

	std::vector<CProjectileObject*> m_vProjectiles;

	std::vector<CObjectBase*> m_vStageObjects;

	// 파일 경로별로 실제 사용한 모델만 보관한다. 오브젝트 인스턴스는 저장하지 않는다.
	std::unordered_map<std::string, std::shared_ptr<CModel>> m_models;

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

	HRESULT BuildObjects(const GraphicsResources& graphics) override;
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

	HRESULT BuildObjects(const GraphicsResources& graphics) override;
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

	HRESULT BuildObjects(const GraphicsResources& graphics) override;

	void ReleaseObjects() override;

	void DrawScene(CFrameConstants& constants) override;

	void ResizeUi(int width, int height);

	void Click(int x, int y, CGameFramework& framework) override;

protected:
	void AddUi(std::shared_ptr<CUI> upUi);

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

	HRESULT BuildObjects(const GraphicsResources& graphics) override;
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

	HRESULT BuildObjects(const GraphicsResources& graphics) override;
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

	HRESULT BuildObjects(const GraphicsResources& graphics) override;

private:
	void KeyPressed(WPARAM key, CGameFramework& framework) override;
};
//-----------------------------------
