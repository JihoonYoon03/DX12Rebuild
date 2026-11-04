#include "pch.h"
#include "Scene.h"
#include "GameObject.h"

CScene::CScene()
{
	XMFLOAT3 eye{ 2.5, 1.0, -2.5 };
	XMFLOAT3 look{ 0, 0, 0 };
	XMFLOAT3 up{ 0, 1, 0 };

	m_Camera.SetLookAt(eye, look, up);
	m_Camera.SetPerspective(90, config::FRAME_BUFFER_W / config::FRAME_BUFFER_H, 0.01f, 100.0f);
}

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

void CScene::WindowResized(int width, int height)
{
	if (height != 0) m_Camera.SetAspectRatio((float)width / (float)height);
}

void CScene::ProcessInput(UCHAR* pKeyBuffer, float deltaTime)
{
	DWORD dwDirection = 0;

	//키보드 상태 정보를 반환한다.
	if (::GetKeyboardState(pKeyBuffer))
	{
		if (pKeyBuffer[VK_UP] & 0xF0) dwDirection |= DIR_FORWARD;
		if (pKeyBuffer[VK_DOWN] & 0xF0) dwDirection |= DIR_BACKWARD;
		if (pKeyBuffer[VK_LEFT] & 0xF0) dwDirection |= DIR_LEFT;
		if (pKeyBuffer[VK_RIGHT] & 0xF0) dwDirection |= DIR_RIGHT;
		if (pKeyBuffer[VK_PRIOR] & 0xF0) dwDirection |= DIR_UP;
		if (pKeyBuffer[VK_NEXT] & 0xF0) dwDirection |= DIR_DOWN;
	}
	float cxDelta = 0.0f, cyDelta = 0.0f;
	POINT ptCursorPos;

	//마우스 또는 키 입력이 있으면 플레이어를 이동하거나 회전함
	if ((dwDirection != 0) || (cxDelta != 0.0f) || (cyDelta != 0.0f))
	{
		if (cxDelta || cyDelta)
		{
			//cxDelta = Yaw, cyDelta = Pitch

		}
		//dwDirection 방향으로 이동(속도 벡터 변경). 이동 속력은 50/sec로 가정
		if (dwDirection)
		{
			m_Camera.ProcessInput(dwDirection, deltaTime);
		}
	}
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
	XMFLOAT4X4 view = Matrix4x4::Transpose(m_Camera.GetView());
	XMFLOAT4X4 proj = Matrix4x4::Transpose(m_Camera.GetProj());
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &view, 16);
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &proj, 32);

	for (std::unique_ptr<CGameObject>& upObject : m_vObjects)
	{
		upObject->DrawObject(cpCommandList);
	}
}

