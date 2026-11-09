#pragma once

#include "GameObject.h"

class CCamera;

class CPlayer : public CModelObject
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

class CTankPlayer : public CPlayer
{
public:
	CTankPlayer();
	~CTankPlayer();

	void SetModel(std::shared_ptr<CModel>& model) override;

	void ProcessInput(DWORD dwInput, float deltaTime, const CTerrainObject* terrain = nullptr);
	void Move(XMFLOAT3 shift);

private:
	float	m_fVelocity = 10.0f;
	//초당 회전량
	float	m_fTurnSpeed = XMConvertToRadians(90.0f);

	//포탑 회전 관련
	float	m_fTurretYaw = 0.0f;
	float	m_fTurretTurnSpeed = XMConvertToRadians(60.0f);
};