#pragma once

#include "GameObject.h"

class CCamera;

//-----------------------------------
//[직접 작성한 코드]
//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CPlayer : public CDamageableObject
//-----------------------------------
{
public:
	CPlayer();
	virtual ~CPlayer();

	void SetCamera(std::shared_ptr<CCamera> spCamera);
	void Update();

	virtual void ProcessInput(DWORD dwInput, float deltaTime);

protected:
	std::shared_ptr<CCamera>	m_spCamera;

};
//-----------------------------------

class CTankPlayer : public CPlayer
{
public:
	//-----------------------------------
	//[직접 작성한 코드]
	CTankPlayer();
	~CTankPlayer();

	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	void SetModel(std::shared_ptr<CModel>& model) override;
	//-----------------------------------

	void ProcessInput(DWORD dwInput, float deltaTime, const CTerrainObject* terrain = nullptr);
	void Move(XMFLOAT3 shift);

	HRESULT CreateCannonTestMesh(const ComPtr<ID3D12Device>& cpDevice,
		const ComPtr<ID3D12GraphicsCommandList>& cpCommandList);
	void ReleaseCannonTestUploadBuffer();
	//-----------------------------------

	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	static void PrepareModel(std::shared_ptr<CModel>& model);
	void UsePreparedTankModel(const std::shared_ptr<CModel>& spModel);
	XMFLOAT3 ApplyControls(DWORD dwInput, float deltaTime, const CTerrainObject* terrain);
	void ResetStagePose(const XMFLOAT3& position, float yaw);
	void FollowStageSlope(const CTerrainObject& terrain);
	XMFLOAT3 GetTurretHeading();
	void GetStageCannonRay(XMFLOAT3& origin, XMFLOAT3& direction);
	bool AimStageCannon(const XMFLOAT3& target, float dt);
	//-----------------------------------
	//-----------------------------------
	//[직접 작성한 코드]
	void DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;

private:
	void CannonTestDraw(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList);
	std::unique_ptr<CMesh> m_upCannonTestMesh;
	//-----------------------------------

	//-----------------------------------
	//[직접 작성한 코드]
	float	m_fVelocity = config::stage::TankDriveSpeed;
	//초당 회전량
	float	m_fTurnSpeed = XMConvertToRadians(config::stage::TankTurnSpeedDegrees);

	//포탑 회전 관련
	float	m_fTurretYaw = 0.0f;
	float	m_fTurretTurnSpeed = XMConvertToRadians(config::stage::TurretManualSpeedDegrees);
	float	m_fCannonPitch = 0.0f;
	float	m_fCannonLiftSpeed = XMConvertToRadians(config::stage::CannonManualSpeedDegrees);
	//-----------------------------------
};

//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CEnemyTank final : public CTankPlayer
{
public:
	CEnemyTank();
	void Reload() override { m_fReload = config::stage::EnemyTankReload; }
	bool TryFire(const XMFLOAT3& target, float dt,
		XMFLOAT3& origin, XMFLOAT3& direction) override;

private:
	float m_fReload = 0.0f;
};
//-----------------------------------
