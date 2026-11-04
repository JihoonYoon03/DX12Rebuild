#include "pch.h"
#include "GameObject.h"
#include "Mesh.h"


CModelObject::CModelObject()
{
	m_spModel = std::make_shared<CModel>();
	m_Transform.rotation = XMFLOAT3{ 0, 0, 0 };
	m_Transform.position = XMFLOAT3{ 0, 0, 0 };
}

CModelObject::~CModelObject()
{}

void CModelObject::SetScale(const float x, const float y, const float z)
{
	m_Transform.scale.x = x;
	m_Transform.scale.y = y;
	m_Transform.scale.z = z;
}

void CModelObject::SetScale(const XMFLOAT3& scale)
{
	SetScale(scale.x, scale.y, scale.z);
}

void CModelObject::SetRotation(const float x, const float y, const float z)
{
	m_Transform.rotation.x = x;
	m_Transform.rotation.y = y;
	m_Transform.rotation.z = z;
}

void CModelObject::SetRotation(const XMFLOAT3& rotation)
{
	SetRotation(rotation.x, rotation.y, rotation.z);
}

void CModelObject::SetPosition(const float x, const float y, const float z)
{
	m_Transform.position.x = x;
	m_Transform.position.y = y;
	m_Transform.position.z = z;
}

void CModelObject::SetPosition(const XMFLOAT3& position)
{
	SetPosition(position.x, position.y, position.z);
}

void CModelObject::DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	if (m_spModel) m_spModel->DrawModel(cpCommandList, m_Transform.getMatrix());
}

void CModelObject::ReleaseModelUploadBuffer()
{
	m_spModel->ReleaseUploadBuffer();
}