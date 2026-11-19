#pragma once

#include "Mesh.h"
#include "Weapon.h"

class CHeightMapImage;
class CTexture;
class CCamera;
class CScene;
class CGameFramework;
enum class ERenderState;

// -------------------------------------------
//
// ObjectBase
//
// -------------------------------------------
class CObjectBase
{
public:
	CObjectBase();
	virtual ~CObjectBase();

	void SetScale(const float x, const float y, const float z);
	void SetScale(const XMFLOAT3& scale);
	void SetScale(const float scale);

	void SetRotation(const float x, const float y, const float z);
	void SetRotation(const XMFLOAT3& rotation);

	void SetPosition(const float x, const float y, const float z);
	void SetPosition(const XMFLOAT3& position);

	XMFLOAT3 GetPosition() const { return m_Transform.position; }
	const Transform& GetTransform() const { return m_Transform; }

	virtual void DrawObject(CFrameConstants& constants) = 0;
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	virtual ERenderState GetRenderState() const;
	virtual bool IsScreenSpace() const { return false; }

	virtual void ReleaseUploadBuffer() {}
	//-----------------------------------

protected:
	Transform	m_Transform;

	bool BindWorld(CFrameConstants& constants) const;
};

// -------------------------------------------
//
// ModelObject
//
// -------------------------------------------
class CModelObject : public CObjectBase
{
public:
	//모델 오브젝트는 CModel에서 CBV를 관리함
	CModelObject();
	virtual ~CModelObject();

	virtual void SetModel(std::shared_ptr<CModel>& model) { m_spModel = model; }

	void SetFrameTransform(const std::string& name, const Transform& transform);
	void SetFrameTransform(const std::string& name, const XMFLOAT4X4& transform);

	void DrawObject(CFrameConstants& constants) override;

	void ReleaseModelUploadBuffer();
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	void ReleaseUploadBuffer() override { ReleaseModelUploadBuffer(); }
	//-----------------------------------

protected:
	std::shared_ptr<CModel>				m_spModel;

	//프레임 별로 적용되는 변환
	std::unordered_map<int, XMFLOAT4X4>	m_umFrameTransform;
};

// -------------------------------------------
//
// TerrainObject
//
// -------------------------------------------
class CTerrainObject : public CObjectBase
{
public:
	CTerrainObject();
	~CTerrainObject();

	void SetHeightMap(std::shared_ptr<CHeightMapImage>& pHeightMapImage);

	HRESULT GenerateTerrain(const GraphicsResources& graphics, bool normalMapping = false);

	float GetHeightAt(float x, float z) const;
	float GetHeightAt(const XMFLOAT3& pos) const;

	void DrawObject(CFrameConstants& constants) override;

	void ReleaseUploadBuffer() override;

	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	ERenderState GetRenderState() const override;

	float GetHitFraction(const XMFLOAT3& start, const XMFLOAT3& end, float clearance) const;
	//-----------------------------------

private:
	int									m_nColSlice;
	int									m_nRowSlice;
	float								m_fTiling;

	std::shared_ptr<CHeightMapImage>	m_spHeightMapImage;

	std::unique_ptr<CMesh>				m_upTerrain;

	std::unique_ptr<CTexture>			m_upTerrainTex;
	std::unique_ptr<CTexture>			m_upNormalMap;
};

// -------------------------------------------
//
// SkyboxObject
//
// -------------------------------------------
class CSkyboxObject : public CObjectBase
{
public:
	CSkyboxObject();
	~CSkyboxObject() override;

	HRESULT GenerateSkyBox(const GraphicsResources& graphics);

	void DrawObject(CFrameConstants& constants) override;

	void ReleaseUploadBuffer() override;

	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	ERenderState GetRenderState() const override;
	//-----------------------------------

private:
	std::vector<std::unique_ptr<CMesh>> m_vPlane;

	std::vector<std::unique_ptr<CTexture>> m_vSkyTextures;
};

// -------------------------------------------
//
// MeshObject
//
// -------------------------------------------
//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CMeshObject : public CObjectBase
{
public:
	CMeshObject(CMesh& mesh, const XMFLOAT4& color);

	void SetColor(const XMFLOAT4& color) { m_color = color; }
	void SetVisible(bool visible) { m_bVisible = visible; }

	ERenderState GetRenderState() const override;
	void DrawObject(CFrameConstants& constants) override;

protected:
	CMesh* m_pMesh; // 공용 메시를 참조하며 소유하지 않는다.

	XMFLOAT4 m_color;

	bool m_bVisible = true;
};
//-----------------------------------

// -------------------------------------------
//
// DamageableObject
//
// -------------------------------------------
//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CDamageableObject : public CModelObject
{
public:
	void SetCombatInfo(int health, float aimHeight, float hitRadius, float homingHitRadius, float markerHeight, float hitHeight = 0.0f);

	virtual void ApplyDamage() { if (m_nHealth > 0) --m_nHealth; }
	void ResetHealth() { m_nHealth = m_nMaxHealth; m_bVisible = true; }
	int GetMaxHealth() const { return m_nMaxHealth; }
	int GetHealth() const { return m_nHealth; }
	bool IsAlive() const { return m_nHealth > 0; }

	void SetVisible(bool visible) { m_bVisible = visible; }
	bool IsVisible() const { return m_bVisible && IsAlive(); }

	void DrawObject(CFrameConstants& constants) override;

	virtual XMFLOAT3 GetAimPoint() const;
	virtual XMFLOAT3 GetHitPoint() const;
	virtual float GetHitFraction(const XMFLOAT3& start, const XMFLOAT3& end, bool homing) const;
	float GetHitRadius(bool homing) const { return homing ? m_fHomingHitRadius : m_fHitRadius; }

	float GetMarkerHeight() const { return m_fMarkerHeight; }

private:
	bool m_bVisible = true;

	int m_nHealth = 1;
	int m_nMaxHealth = 1;

	float m_fAimHeight = 0.0f;
	float m_fHitHeight = 0.0f;

	float m_fHitRadius = 0.0f;
	float m_fHomingHitRadius = 0.0f;

	float m_fMarkerHeight = 0.0f;
};
//-----------------------------------

// -------------------------------------------
//
// TargetObject
//
// -------------------------------------------
//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CTargetObject final : public CDamageableObject
{
public:
	explicit CTargetObject(CMesh& mesh);

	ERenderState GetRenderState() const override;
	void DrawObject(CFrameConstants& constants) override;

private:
	CMeshObject m_visual;
};
//-----------------------------------

// -------------------------------------------
//
// ProjectileData
//
// -------------------------------------------
//-----------------------------------
//[아래 코드는 AI로 구현됨]
struct ProjectileData
{
	XMFLOAT3 direction;
	float speed;
	float lifetime;
	float clearance;

	float homingDelay = 0.0f;
	float homingTurnSpeed = 0.0f;

	CDamageableObject* target = nullptr; // 씬 소유 대상. 발사체보다 오래 유지한다.
	bool enemyOwned = false;
};
//-----------------------------------

// -------------------------------------------
//
// ProjectileObject
//
// -------------------------------------------
//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CProjectileObject final : public CMeshObject
{
public:
	CProjectileObject(CMesh& mesh, const XMFLOAT3& position, const ProjectileData& data);

	void Advance(float dt);
	void Expire() { m_flight.lifetime = 0.0f; }
	bool IsExpired() const { return m_flight.lifetime <= 0.0f; }

	bool IsEnemyOwned() const { return m_flight.enemyOwned; }
	bool IsHoming() const { return m_flight.target != nullptr; }
	float GetClearance() const { return m_flight.clearance; }

private:
	ProjectileData m_flight;
};
//-----------------------------------

// -------------------------------------------
//
// ObstacleObject
//
// -------------------------------------------
//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CObstacleObject final : public CModelObject
{
public:
	void SetModel(std::shared_ptr<CModel>& model) override;

	BoundingOrientedBox GetFootprint() const;

private:
	BoundingBox m_bounds;
};
//-----------------------------------

// -------------------------------------------
//
// VehicleObject
//
// -------------------------------------------
class CVehicleObject : public CDamageableObject
{
public:
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	explicit CVehicleObject(const WeaponSettings& weapon);
	//-----------------------------------

	void SetCamera(const std::shared_ptr<CCamera>& camera) { m_spCamera = camera; }

	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	void ResetCombat();

	void KeyPressed(WPARAM key);
	virtual void ProcessControls(const UCHAR* keys, float dt, const CTerrainObject& terrain) {}

	virtual void UpdateCamera(const CTerrainObject& terrain) {}

	virtual void GetAimRay(XMFLOAT3& origin, XMFLOAT3& direction) const = 0;

	void UpdateTargeting(const std::vector<CDamageableObject*>& targets, const CTerrainObject& terrain);

	bool TryPlayerFire(float dt, XMFLOAT3& origin, ProjectileData& projectile);
	bool FireWeapon(XMFLOAT3& origin, ProjectileData& projectile);

	CDamageableObject* GetLockedTarget() const { return m_pLockedTarget; }
	bool IsLockPinned() const { return m_bLockPinned; }
	void ClearTargeting() { m_pLockedTarget = nullptr; m_bLockPinned = false; }

	virtual bool IsAutoAttacking() const { return false; }

	virtual bool IsProtected() const { return false; }
	virtual bool ShowsShield() const { return false; }

	void ApplyDamage() override;

protected:
	void SetTargeting(float minDot, float radius);

	virtual void ResetControls() {}
	virtual void OnKeyPressed(WPARAM key) {}

	virtual bool AimAutomatically(float dt) { return false; }
	//-----------------------------------

	std::shared_ptr<CCamera> m_spCamera;

	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	CWeapon m_weapon;

private:
	CDamageableObject* m_pLockedTarget = nullptr; // 씬이 소유하며 스테이지 종료 전에 초기화한다.

	float m_fLockMinDot = 0.0f;
	float m_fLockRadius = 0.0f;

	bool m_bLockPinned = false;

	bool m_bFireRequested = false;
	//-----------------------------------
};

// -------------------------------------------
//
// TankObject
//
// -------------------------------------------
class CTankObject : public CVehicleObject
{
public:
	CTankObject();
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	explicit CTankObject(const WeaponSettings& weapon);
	//-----------------------------------

	void SetModel(std::shared_ptr<CModel>& model) override;
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	void UsePreparedTankModel(const std::shared_ptr<CModel>& model);

	XMFLOAT3 GetAimPoint() const override;
	XMFLOAT3 GetHitPoint() const override;
	float GetHitFraction(const XMFLOAT3& start, const XMFLOAT3& end, bool homing) const override;
	BoundingOrientedBox GetFootprint(const Transform& pose) const;

	void ResetStagePose(const XMFLOAT3& position, float yaw);
	void FollowStageSlope(const CTerrainObject& terrain);

	XMFLOAT3 GetTurretHeading() const;
	//-----------------------------------
	void GetStageCannonRay(XMFLOAT3& origin, XMFLOAT3& direction) const;
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	void GetAimRay(XMFLOAT3& origin, XMFLOAT3& direction) const override;
	bool AimStageCannon(const XMFLOAT3& target, float dt);
	//-----------------------------------

	void DrawObject(CFrameConstants& constants) override;

protected:
	//-----------------------------------
	//[직접 작성한 코드]
	XMFLOAT3 ApplyControls(DWORD input, float dt, const CTerrainObject* terrain);
	void Move(XMFLOAT3 shift);

	float m_fVelocity = config::stage::TankDriveSpeed;
	float m_fTurnSpeed = XMConvertToRadians(config::stage::TankTurnSpeedDegrees);

	float m_fTurretYaw = 0.0f;
	float m_fTurretTurnSpeed = XMConvertToRadians(config::stage::TurretManualSpeedDegrees);

	float m_fCannonPitch = 0.0f;
	float m_fCannonLiftSpeed = XMConvertToRadians(config::stage::CannonManualSpeedDegrees);
	//-----------------------------------

private:
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	std::optional<BoundingBox> m_hitBounds;
	//-----------------------------------
};

// -------------------------------------------
//
// EnemyTank
//
// -------------------------------------------
//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CEnemyTank final : public CTankObject
{
public:
	CEnemyTank();

	bool UpdateAttack(const XMFLOAT3& target, float dt, const CTerrainObject& terrain);
};
//-----------------------------------

// -------------------------------------------
//
// UI
//
// -------------------------------------------
class CUI : public CObjectBase
{
public:
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	CUI(
		const XMFLOAT4& bounds, 
		const XMFLOAT4& color, 
		const wchar_t* textureFile = nullptr);

	virtual HRESULT BuildMesh(const GraphicsResources& graphics);
	//-----------------------------------
	//[직접 작성한 코드]
	HRESULT BindTexture(std::shared_ptr<CTexture>& texture);
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	void ReleaseUploadBuffer() override;

	void SetColor(const XMFLOAT4& color) { m_color = color; }
	void SetVisible(bool visible) { m_bVisible = visible; }

	bool HasTexture() const { return m_spUITexture != nullptr; }

	ERenderState GetRenderState() const override;
	bool IsScreenSpace() const override { return true; }
	void DrawObject(CFrameConstants& constants) override;

	virtual bool HitTest(float x, float y) const;
	virtual bool OnClick(CGameFramework&) { return false; }
	//-----------------------------------

private:
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	CMesh m_mesh;
	//---------------------------
	//[직접 작성한 코드]
	std::shared_ptr<CTexture>	m_spUITexture;

	//---------------------------
	//[아래 코드는 AI로 구현됨]
	XMFLOAT4 m_color;
	bool m_bVisible = true;

	std::wstring m_textureFile;
	//-----------------------------------
};

// -------------------------------------------
//
// MaskedUI
//
// -------------------------------------------
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

	HRESULT BuildMesh(const GraphicsResources& graphics) override;

	ERenderState GetRenderState() const override;
	void DrawObject(CFrameConstants& constants) override;

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

// -------------------------------------------
//
// GameHud
//
// -------------------------------------------
//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CGameHud
{
public:
	HRESULT Build(CScene& scene, int maxHealth, const GraphicsResources& graphics);

	void Update(const CVehicleObject& player, bool victory, bool defeat, float aspectScale);

private:
	enum ESlot { Aim, AutoAttack, Shield, HealthBegin };

	void SetPlane(size_t slot, float x, float y, float width, float height, const XMFLOAT4& color);

	std::vector<CUI*> m_ui; // 소유권은 CScene::m_vObjects에 있다.
};
//-----------------------------------
