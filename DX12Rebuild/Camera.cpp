#include "pch.h"
#include "Camera.h"

CCamera::CCamera()
{
	SetView();
	SetProj();
}

CCamera::~CCamera()
{}

void CCamera::Move(const XMFLOAT3& shift)
{
	m_xmf3Pos = Vector3::Add(m_xmf3Pos, shift);
	m_xmf3LookAt = Vector3::Add(m_xmf3LookAt, shift);
	SetView();
}

void CCamera::SetLookAt(const XMFLOAT3& pos, const XMFLOAT3& lookAt, const XMFLOAT3& up)
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
