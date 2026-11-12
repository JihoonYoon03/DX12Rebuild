#pragma once

//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CObjectBase;
class CSkyboxObject;
class CModelObject;
class CGameFramework;
class CDamageableObject;
class CProjectileObject;
class CUI;
struct ProjectileData;
//-----------------------------------
class CMesh;
class CShader;
class CTextureShader;
class CModel;
class CTerrainObject;
class CCamera;
class CTankPlayer;
class CDescHeapAllocator;

//-----------------------------------
//[아래 코드는 AI로 구현됨]
enum class EGameStage
{
	HelicopterTargets,
	TankBattle,
	HelicopterBattle
};
//-----------------------------------

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
	void AddObject(std::unique_ptr<CModelObject>& upObject);
	void AddObjects(std::unique_ptr<CModelObject>* upObjects, size_t size);
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	void AddObjects(std::vector<std::unique_ptr<CModelObject>>& upObjects);
	//-----------------------------------

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
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	virtual void ReleaseObjects();
	//-----------------------------------

	virtual void DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList);

protected:
	std::shared_ptr<CCamera>					m_spCamera;
	std::shared_ptr<CShader>					m_spShader;
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	std::vector<std::unique_ptr<CObjectBase>>	m_vObjects;
	//-----------------------------------
};
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CStageResources
{
public:
	CStageResources();
	~CStageResources();
	HRESULT Build(ComPtr<ID3D12Device>& cpDevice, ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
		CDescHeapAllocator& allocator);
	void ReleaseUploadBuffers();

	std::shared_ptr<CShader> spShader;
	std::unique_ptr<CTerrainObject> upTerrain;
	std::shared_ptr<CModel> spTank;
	std::shared_ptr<CModel> spApache;
	std::shared_ptr<CModel> spRocks[2];
	std::unique_ptr<CSkyboxObject> upSkybox;
	std::unique_ptr<CMesh> upBox;

private:
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	HRESULT BuildVisuals(const ComPtr<ID3D12Device>& cpDevice, const ComPtr<ID3D12GraphicsCommandList>& cpCommandList);
	//-----------------------------------
};
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CGameScene : public CScene
{
public:
	//-----------------------------------
	//[직접 작성한 코드]
	~CGameScene() override;
	void DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;
	//-----------------------------------

	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	HRESULT BuildObjects(ComPtr<ID3D12Device>& cpDevice,
		ComPtr<ID3D12GraphicsCommandList>& cpCommandList, CDescHeapAllocator* DescHeapAllocator = nullptr) override;
	void ReleaseUploadBuffers() override;
	void ReleaseObjects() override;
	void ProcessInput(UCHAR* pKeyBuffer, float deltaTime) override;
	void SetStageResources(const std::shared_ptr<CStageResources>& resources);
	void Update(float dt) override;
	void KeyPressed(WPARAM key, CGameFramework& framework) override;
	void CheckStageTransition(CGameFramework& framework) override;
	void BeginStage(EGameStage stage);
	void StageKeyPressed(WPARAM key);

	EGameStage GetStage() const { return m_stage; }
	bool StageFinished() const { return m_state == EStageState::Finished; }

protected:
	struct PlayerRules
	{
		int maxHealth;
		float aimHeight;
		float hitRadius;
		float reload;
		float shotSpeed;
		float shotLifetime;
		float shotClearance;
		float muzzleOffset = 0.0f;
		float homingDelay = 0.0f;
		float homingTurnSpeed = 0.0f;
		bool homing = false;
	};

	explicit CGameScene(const PlayerRules& rules);
	virtual void ResetStage() = 0;
	virtual void OnStageKeyPressed(WPARAM key) = 0;
	virtual void UpdatePlayer(const UCHAR* keys, float dt) = 0;
	virtual void UpdateStageCamera(const XMFLOAT3& aimDirection) = 0;
	XMFLOAT3 PlayerPosition() const;
	virtual void GetAimRay(XMFLOAT3& origin, XMFLOAT3& direction) const = 0;
	virtual void DrawStageObjects(const ComPtr<ID3D12GraphicsCommandList>&) {}

	virtual bool TryAutoAim(const UCHAR*, float, XMFLOAT3&, XMFLOAT3&) { return false; }
	virtual bool IsPlayerProtected() const { return false; }
	virtual bool HasReachedExit() const { return false; }
	virtual float VictoryHoldTime() const { return config::stage::VictoryDelay; }
	virtual void UpdateHudExtras(float, const XMFLOAT4&) {}

	enum EHudSlot { AimHud, AutoAttackHud, ShieldHud, HealthHudBegin };
	void UpdateStageView();
	void SetHudPlane(size_t index, float x, float y, float width, float height, const XMFLOAT4& color);
	XMFLOAT3 OnTerrain(float x, float z, float offset) const;
	void AddTarget(const XMFLOAT3& position);
	void AddEnemyTank(float x, float z, float playerStartZ);
	void SetLockSettings(float minDot, float radius);
	std::optional<XMFLOAT3> LockedAimPoint() const;
	bool IsPlayerAlive() const;
	virtual CDamageableObject& PlayerObject() const = 0;
	void DrawStageBox(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList, const XMFLOAT3& position,
		const XMFLOAT3& size, const XMFLOAT4& color, float yaw = 0.0f);

	std::shared_ptr<CStageResources> m_spStageResources;

private:
	enum class EStageState { Playing, Victory, Defeat, Finished };

	void DrawStage(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList);
	void DrawStageHud(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList);
	void SpawnProjectile(const XMFLOAT3& position, const ProjectileData& data);

	XMFLOAT3 PlayerAimPoint() const;
	void UpdateLock(const XMFLOAT3& origin, const XMFLOAT3& direction);
	void UpdateEnemies(float dt);
	void FirePlayer(const XMFLOAT3& origin, const XMFLOAT3& direction);
	void UpdateProjectiles(float dt);
	float TerrainHitFraction(const XMFLOAT3& start, const XMFLOAT3& end, float clearance) const;
	void Win();

	std::vector<CDamageableObject*> m_vEnemies; // m_vObjects가 소유하는 객체의 참조.
	std::vector<std::unique_ptr<CProjectileObject>> m_vProjectiles;
	std::vector<std::unique_ptr<CUI>> m_vHud;
	UCHAR m_keys[256]{};
	const PlayerRules m_rules;

	EGameStage m_stage = EGameStage::HelicopterTargets;
	EStageState m_state = EStageState::Playing;
	float m_fResultTimer = 0.0f;

	float m_fReload = 0.0f;
	int m_nLockedEnemy = -1;
	float m_fLockMinDot = 0.0f;
	float m_fLockRadius = 0.0f;
	bool m_bFireRequested = false;
	bool m_bLockPinned = false;
};
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CTankGameScene final : public CGameScene
{
public:
	CTankGameScene();
	~CTankGameScene() override;
	HRESULT BuildObjects(
		ComPtr<ID3D12Device>& cpDevice,
		ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
		CDescHeapAllocator* DescHeapAllocator = nullptr
	) override;
	void ReleaseUploadBuffers() override;
	void ReleaseObjects() override;

private:
	struct Obstacle
	{
		BoundingOrientedBox footprint;
	};

	void ResetStage() override;
	void OnStageKeyPressed(WPARAM key) override;
	void UpdatePlayer(const UCHAR* keys, float dt) override;
	void UpdateStageCamera(const XMFLOAT3& aimDirection) override;
	CDamageableObject& PlayerObject() const override;
	void GetAimRay(XMFLOAT3& origin, XMFLOAT3& direction) const override;
	bool TryAutoAim(const UCHAR* keys, float dt, XMFLOAT3& origin, XMFLOAT3& direction) override;
	bool IsPlayerProtected() const override { return m_bShield; }
	void UpdateHudExtras(float aspectScale, const XMFLOAT4& reticleColor) override;

	CTankPlayer* m_pPlayer = nullptr; // m_vObjects가 소유한다.
	std::vector<Obstacle> m_vObstacles;
	BoundingBox m_tankBounds;
	BoundingBox m_rockBounds[2];
	bool m_bAutoAttack = config::stage::TankAutoAttackAtStart;
	bool m_bShield = false;
};
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CHelicopterGameScene final : public CGameScene
{
public:
	CHelicopterGameScene();
	~CHelicopterGameScene() override;
	HRESULT BuildObjects(ComPtr<ID3D12Device>& cpDevice, ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
		CDescHeapAllocator* DescHeapAllocator = nullptr) override;
	void ReleaseObjects() override;

private:
	void ResetStage() override;
	void OnStageKeyPressed(WPARAM key) override;
	void UpdatePlayer(const UCHAR* keys, float dt) override;
	void UpdateStageCamera(const XMFLOAT3&) override;
	CDamageableObject& PlayerObject() const override;
	void GetAimRay(XMFLOAT3& origin, XMFLOAT3& direction) const override;
	bool HasReachedExit() const override;
	float VictoryHoldTime() const override;
	void DrawStageObjects(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;

	bool IsTargetStage() const { return GetStage() == EGameStage::HelicopterTargets; }
	XMFLOAT3 HelicopterForward() const;
	void UpdateHelicopterPose();

	CDamageableObject* m_pStageHelicopter = nullptr; // m_vObjects가 소유한다.
	Transform m_helicopterPose;
	XMFLOAT3 m_exitPosition{};
	XMFLOAT3 m_mainRotorAxis{ 0.0f, 1.0f, 0.0f };
	XMFLOAT3 m_mainRotorOffset{};
	XMFLOAT3 m_mainHubOffset{};
	XMFLOAT3 m_tailRotorAxis{ 1.0f, 0.0f, 0.0f };
	XMFLOAT3 m_tailRotorOffset{};
	float m_fRotorTime = 0.0f;
	bool m_bStageFirstPerson = false;
};
//-----------------------------------
