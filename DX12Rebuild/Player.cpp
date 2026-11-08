#include "pch.h"
#include "Player.h"
#include "Camera.h"
#include "GameObject.h"


CPlayer::CPlayer()
{}

CPlayer::~CPlayer()
{}

void CPlayer::SetCamera(std::shared_ptr<CCamera> spCamera)
{
	m_spCamera = spCamera;
}

void CPlayer::Update()
{
}

void CPlayer::ProcessInput(DWORD dwInput, float deltaTime)
{
	XMFLOAT3 Right	{ 1.0f, 0.0f, 0.0f };
	XMFLOAT3 Up		{ 0.0f, 1.0f, 0.0f };
	XMFLOAT3 Look	{ 0.0f, 0.0f, 1.0f };
	XMFLOAT3 xmf3Shift = XMFLOAT3(0.0f, 0.0f, 0.0f);
	if (dwInput & DIR_FORWARD) xmf3Shift = Vector3::Add(xmf3Shift, Look, 1.0f);
	if (dwInput & DIR_BACKWARD) xmf3Shift = Vector3::Add(xmf3Shift, Look, -1.0f);

	if (dwInput & DIR_RIGHT) xmf3Shift = Vector3::Add(xmf3Shift, Right, 1.0f);
	if (dwInput & DIR_LEFT) xmf3Shift = Vector3::Add(xmf3Shift, Right, -1.0f);

	if (dwInput & DIR_UP) xmf3Shift = Vector3::Add(xmf3Shift, Up, 1.0f);
	if (dwInput & DIR_DOWN) xmf3Shift = Vector3::Add(xmf3Shift, Up, -1.0f);

	//상수값은 나중에 카메라 속도로 분리
	float moveDistance = 10.0f * deltaTime;
	xmf3Shift = Vector3::Normalize(xmf3Shift);
	xmf3Shift = Vector3::Multiply(moveDistance, xmf3Shift);

	//Custom
}



//--------------------------------
// 
// CTankPlayer
// 
//--------------------------------
CTankPlayer::CTankPlayer()
{
}

CTankPlayer::~CTankPlayer()
{
}

void CTankPlayer::ProcessInput(DWORD dwInput, float deltaTime, const CTerrainObject* terrain)
{
	XMFLOAT3 Right{ 1.0f, 0.0f, 0.0f };
	//XMFLOAT3 Up{ 0.0f, 1.0f, 0.0f };
	XMFLOAT3 Look{ 0.0f, 0.0f, 1.0f };
	XMFLOAT3 xmf3Shift = XMFLOAT3(0.0f, 0.0f, 0.0f);
	if (dwInput & DIR_FORWARD) xmf3Shift = Vector3::Add(xmf3Shift, Look, 1.0f);
	if (dwInput & DIR_BACKWARD) xmf3Shift = Vector3::Add(xmf3Shift, Look, -1.0f);

	if (dwInput & DIR_RIGHT) xmf3Shift = Vector3::Add(xmf3Shift, Right, 1.0f);
	if (dwInput & DIR_LEFT) xmf3Shift = Vector3::Add(xmf3Shift, Right, -1.0f);

	//if (dwInput & DIR_UP) xmf3Shift = Vector3::Add(xmf3Shift, Up, 1.0f);
	//if (dwInput & DIR_DOWN) xmf3Shift = Vector3::Add(xmf3Shift, Up, -1.0f);

	float moveDistance = m_fVelocity * deltaTime;
	xmf3Shift = Vector3::Normalize(xmf3Shift);
	xmf3Shift = Vector3::Multiply(moveDistance, xmf3Shift);
	if (terrain)
	{
		float terrainHeight = terrain->GetHeightAt(Vector3::Add(m_Transform.position, xmf3Shift)) + 1.0f;
		xmf3Shift.y = terrainHeight - m_Transform.position.y;
	}

	Move(xmf3Shift);
	m_spCamera->Move(xmf3Shift);
}

void CTankPlayer::Move(XMFLOAT3 shift)
{
	m_Transform.position = Vector3::Add(m_Transform.position, shift);
}