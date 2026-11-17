#pragma once

#include "GameObject.h"

// -------------------------------------------
//
// TankPlayer
//
// -------------------------------------------
class CTankPlayer final : public CTankObject
{
public:
	CTankPlayer();
	~CTankPlayer();

	void ProcessInput(DWORD input, float dt, const CTerrainObject* terrain = nullptr);
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	void ProcessControls(const UCHAR* keys, float dt, const CTerrainObject& terrain) override;

	void UpdateCamera(const CTerrainObject& terrain) override;

	bool IsAutoAttacking() const override { return m_bAutoAttack; }

	bool IsProtected() const override { return m_bShield; }
	bool ShowsShield() const override { return true; }
	//-----------------------------------

	//-----------------------------------
	//[직접 작성한 코드]
	HRESULT CreateCannonTestMesh(const ComPtr<ID3D12Device>& device,
		const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void ReleaseCannonTestUploadBuffer();

	//-----------------------------------
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	void ReleaseUploadBuffer() override;
	//-----------------------------------

private:
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	void ResetControls() override;
	void OnKeyPressed(WPARAM key) override;

	bool AimAutomatically(float dt) override;

	bool m_bAutoAttack = config::stage::TankAutoAttackAtStart;

	bool m_bShield = false;
	bool m_bManualAim = false;
	//-----------------------------------

	//-----------------------------------
	//[직접 작성한 코드]
	void CannonTestDraw(const ComPtr<ID3D12GraphicsCommandList>& commandList);

	std::unique_ptr<CMesh> m_upCannonTestMesh;
	//-----------------------------------
};

// -------------------------------------------
//
// HelicopterPlayer
//
// -------------------------------------------
//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CHelicopterPlayer final : public CVehicleObject
{
public:
	CHelicopterPlayer();

	void SetModel(std::shared_ptr<CModel>& model) override;

	void ResetStagePose(const XMFLOAT3& position, bool targetStage);

	void ProcessControls(const UCHAR* keys, float dt, const CTerrainObject& terrain) override;

	void UpdateCamera(const CTerrainObject& terrain) override;

	void GetAimRay(XMFLOAT3& origin, XMFLOAT3& direction) const override;

private:
	void ResetControls() override;
	void OnKeyPressed(WPARAM key) override;

	XMFLOAT3 Forward() const;

	void UpdatePose();

	float m_fFlightPitch = 0.0f;

	float m_fRotorTime = 0.0f;

	bool m_bFirstPerson = false;
	bool m_bAllowFirstPerson = false;

	XMFLOAT3 m_mainRotorAxis{ 0, 1, 0 };
	XMFLOAT3 m_mainRotorOffset{};
	XMFLOAT3 m_mainHubOffset{};

	XMFLOAT3 m_tailRotorAxis{ 1, 0, 0 };
	XMFLOAT3 m_tailRotorOffset{};
};
//-----------------------------------
