#include "pch.h"
#include "Scene.h"
#include "GameObject.h"

CScene::CScene()
{}

CScene::~CScene()
{}

void CScene::AddObject(std::unique_ptr<CGameObject>& upObject)
{
	m_vObjects.push_back(std::move(upObject));
}

void CScene::AddObjects(std::unique_ptr<CGameObject>* upObjects, size_t size)
{
	for (int i = 0; i < size; ++i)
	{
		AddObject(upObjects[i]);
	}
}

void CScene::AddObjects(std::vector<std::unique_ptr<CGameObject>>& upObjects)
{
	//소유권 이전
	m_vObjects.insert(m_vObjects.end(),
		std::make_move_iterator(upObjects.begin()),
		std::make_move_iterator(upObjects.end()));
}

//void CScene::ReleaseObjectsUploadBuffer()
//{
//	for (std::unique_ptr<CGameObject>& upObject : m_vObjects)
//	{
//		upObject->ReleaseModelUploadBuffer();
//	}
//}

void CScene::DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	for (std::unique_ptr<CGameObject>& upObject : m_vObjects)
	{
		upObject->DrawObject(cpCommandList);
	}
}

