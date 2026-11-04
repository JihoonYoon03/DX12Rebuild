#include "pch.h"
#include "Camera.h"

CCamera::CCamera()
{
	SetView();
	SetProj();
}

CCamera::~CCamera()
{}

void CCamera::SetLookAt(XMFLOAT3 pos, XMFLOAT3 lookAt, XMFLOAT3 up)
{
	m_xmf3Pos = pos;
	m_xmf3LookAt = lookAt;
	m_xmf3Up = up;
	SetView();
}

void CCamera::SetPerspective(float fovAngle, float aspect, float Near, float Far)
{
	m_fFovRadian = XMConvertToRadians(fovAngle);
	m_fAspectRatio = aspect;
	m_fNear = Near;
	m_fFar = Far;
	SetProj();
}

void CCamera::SetAspectRatio(float aspect)
{
	m_fAspectRatio = aspect;
	SetProj();
}

void CCamera::ProcessInput(DWORD direction, float deltaTime)
{
	//이동속도
	float moveDir = 1.0f;
	XMFLOAT3 xmf3Shift = XMFLOAT3(0.0f, 0.0f, 0.0f);
	if (direction & DIR_FORWARD) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Look, moveDir);
	if (direction & DIR_BACKWARD) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Look, -moveDir);

	if (direction & DIR_RIGHT) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Right, moveDir);
	if (direction & DIR_LEFT) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Right, -moveDir);

	if (direction & DIR_UP) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Up, moveDir);
	if (direction & DIR_DOWN) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Up, -moveDir);

	//상수값은 나중에 카메라 속도로 분리
	float moveDistance = 10.0f * deltaTime;
	xmf3Shift = Vector3::Normalize(xmf3Shift);
	xmf3Shift = Vector3::Multiply(moveDistance, xmf3Shift);

	Move(xmf3Shift);
}

void CCamera::Move(XMFLOAT3 shift)
{
	m_xmf3Pos = Vector3::Add(m_xmf3Pos, shift);
	m_xmf3LookAt = Vector3::Add(m_xmf3LookAt, shift);
	SetView();
}

void CCamera::SetView()
{
	m_xmf3Look = Vector3::DirectionVector(m_xmf3LookAt, m_xmf3Pos);
	m_xmf3Right = Vector3::CrossProduct(m_xmf3Up, m_xmf3Look);

	m_xmf4x4View = Matrix4x4::LookAtLH(m_xmf3Pos, m_xmf3LookAt, m_xmf3Up);
}

void CCamera::SetProj()
{
	m_xmf4x4Proj = Matrix4x4::PerspectiveFovLH(m_fFovRadian, m_fAspectRatio, m_fNear, m_fFar);
}
