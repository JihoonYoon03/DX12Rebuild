#pragma once

class CModelObject;
class CMesh;
class CShader;
class CTextureShader;
class CModel;
class CTerrainObject;
class CCamera;
class CTankPlayer;
class CSRVDescHeapAllocator;

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

	virtual HRESULT InitScene(ComPtr<ID3D12Device>& cpDevice);

	virtual HRESULT BuildObjects(
		ComPtr<ID3D12Device>& cpDevice,
		ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
		CSRVDescHeapAllocator* SRVDespHeapAllocator = nullptr
	);
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
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CGameScene : public CScene
{
public:
	//-----------------------------------
	//[직접 작성한 코드]
	~CGameScene() override;

	HRESULT InitScene(ComPtr<ID3D12Device>& cpDevice) override;

	HRESULT BuildObjects(
		ComPtr<ID3D12Device>& cpDevice,
		ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
		CSRVDescHeapAllocator* SRVDespHeapAllocator = nullptr
	) override;

	void ReleaseUploadBuffers() override;
	virtual void ReleaseObjects();

	void ProcessInput(UCHAR* pKeyBuffer, float deltaTime) override;

	void DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;
	//-----------------------------------
	virtual void ShareStageResources(const CGameScene& source);
	void BeginStage(EGameStage stage);
	void StageKeyPressed(WPARAM key);

	EGameStage GetStage() const { return m_stage; }
	bool StageFinished() const { return m_state == EStageState::Finished; }

protected:
	struct StageResources;
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
	virtual void UpdateStageCamera(const XMFLOAT3& aimPoint) = 0;
	virtual XMFLOAT3 PlayerPosition() const = 0;
	virtual void GetAimRay(XMFLOAT3& origin, XMFLOAT3& direction) const = 0;
	virtual void DrawStageObjects(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList) = 0;

	virtual bool TryAutoAim(const UCHAR*, float, XMFLOAT3&, XMFLOAT3&) { return false; }
	virtual bool IsPlayerProtected() const { return false; }
	virtual bool HasReachedExit() const { return false; }
	virtual float VictoryHoldTime() const { return config::stage::VictoryDelay; }
	virtual void DrawStageHudExtras(const ComPtr<ID3D12GraphicsCommandList>&, float, const XMFLOAT4&) {}

	void UpdateStageView();
	XMFLOAT3 OnTerrain(float x, float z, float offset) const;
	void AddTarget(const XMFLOAT3& position);
	void AddEnemyTank(float x, float z, float playerStartZ);
	void SetLockSettings(float minDot, float radius);
	std::optional<XMFLOAT3> LockedAimPoint() const;
	bool IsPlayerAlive() const { return m_nHealth > 0; }
	void DrawStageBox(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList, const XMFLOAT3& position,
		const XMFLOAT3& size, const XMFLOAT4& color, float yaw = 0.0f);

	std::shared_ptr<StageResources> m_spStageResources;

private:
	enum class EStageState { Playing, Victory, Defeat, Finished };

	struct Enemy
	{
		std::unique_ptr<CTankPlayer> upTank;
		XMFLOAT3 targetPosition{};
		int health;
		float reload = 0.0f;

		XMFLOAT3 Position() const;
	};

	struct Projectile
	{
		XMFLOAT3 position;
		XMFLOAT3 direction;
		float speed;
		float lifetime;
		float clearance;
		float homingDelay = 0.0f;
		float homingTurnSpeed = 0.0f;
		int target = -1;
		bool enemyOwned = false;
	};

	HRESULT BuildStageVisuals(const ComPtr<ID3D12Device>& cpDevice, const ComPtr<ID3D12GraphicsCommandList>& cpCommandList);
	void DrawStage(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList);
	void DrawStageHud(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList);

	XMFLOAT3 PlayerAimPoint() const;
	XMFLOAT3 EnemyAimPoint(const Enemy& enemy) const;
	void UpdateLock(const XMFLOAT3& origin, const XMFLOAT3& direction);
	void UpdateEnemies(float dt);
	void FirePlayer(const XMFLOAT3& origin, const XMFLOAT3& direction);
	void UpdateProjectiles(float dt);
	float TerrainHitFraction(const XMFLOAT3& start, const XMFLOAT3& end, float clearance) const;
	void Win();

	std::vector<Enemy> m_vEnemies;
	std::vector<Projectile> m_vProjectiles;
	const PlayerRules m_rules;
	XMFLOAT3 m_reticleAimPoint{};

	EGameStage m_stage = EGameStage::HelicopterTargets;
	EStageState m_state = EStageState::Playing;
	float m_fResultTimer = 0.0f;

	int m_nHealth = 0;
	float m_fReload = 0.0f;
	int m_nLockedEnemy = -1;
	float m_fLockMinDot = 0.0f;
	float m_fLockRadius = 0.0f;
	bool m_bFireRequested = false;
	bool m_bLockPinned = false;
};

class CTankGameScene final : public CGameScene
{
public:
	CTankGameScene();
	~CTankGameScene() override;
	HRESULT BuildObjects(
		ComPtr<ID3D12Device>& cpDevice,
		ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
		CSRVDescHeapAllocator* SRVDespHeapAllocator = nullptr
	) override;
	void ReleaseUploadBuffers() override;
	void ReleaseObjects() override;

private:
	struct Obstacle
	{
		Transform pose;
		size_t variant;
		BoundingOrientedBox footprint;
	};

	void ResetStage() override;
	void OnStageKeyPressed(WPARAM key) override;
	void UpdatePlayer(const UCHAR* keys, float dt) override;
	void UpdateStageCamera(const XMFLOAT3& aimPoint) override;
	XMFLOAT3 PlayerPosition() const override;
	void GetAimRay(XMFLOAT3& origin, XMFLOAT3& direction) const override;
	bool TryAutoAim(const UCHAR* keys, float dt, XMFLOAT3& origin, XMFLOAT3& direction) override;
	bool IsPlayerProtected() const override { return m_bShield; }
	void DrawStageObjects(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;
	void DrawStageHudExtras(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
		float aspectScale, const XMFLOAT4& lockColor) override;

	//---------------------------------------
	//[직접 작성한 코드]
	std::unique_ptr<CTankPlayer> m_upPlayer;
	//---------------------------------------
	std::vector<Obstacle> m_vObstacles;
	BoundingBox m_tankBounds;
	BoundingBox m_rockBounds[2];
	bool m_bAutoAttack = config::stage::TankAutoAttackAtStart;
	bool m_bShield = false;
};

class CHelicopterGameScene final : public CGameScene
{
public:
	CHelicopterGameScene();
	~CHelicopterGameScene() override;
	void ShareStageResources(const CGameScene& source) override;
	void ReleaseObjects() override;

private:
	void ResetStage() override;
	void OnStageKeyPressed(WPARAM key) override;
	void UpdatePlayer(const UCHAR* keys, float dt) override;
	void UpdateStageCamera(const XMFLOAT3&) override;
	XMFLOAT3 PlayerPosition() const override;
	void GetAimRay(XMFLOAT3& origin, XMFLOAT3& direction) const override;
	bool HasReachedExit() const override;
	float VictoryHoldTime() const override;
	void DrawStageObjects(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;

	bool IsTargetStage() const { return GetStage() == EGameStage::HelicopterTargets; }
	XMFLOAT3 HelicopterForward() const;

	std::unique_ptr<CModelObject> m_upStageHelicopter;
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
