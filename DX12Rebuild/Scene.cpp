#include "pch.h"
#include "Scene.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Shader.h"

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

HRESULT CScene::InitScene(ComPtr<ID3D12Device>& cpDevice, ComPtr<ID3D12RootSignature>& cpRootSignature)
{
	HRESULT hr;
	m_spShader = std::make_shared<CShader>();
	hr = m_spShader->CreateShader(cpDevice, cpRootSignature);
	if (FAILED(hr))
	{
		OutputDebugString(L"CScene::InitScene() Failed to create Shader\n");
		return hr;
	}
}

HRESULT CScene::BuildObjects(ComPtr<ID3D12Device>& cpDevice, ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	return S_OK;
}

void CScene::AddObject(std::unique_ptr<CModelObject>& upObject)
{
	m_vObjects.push_back(std::move(upObject));
}

void CScene::AddObjects(std::unique_ptr<CModelObject>* upObjects, size_t size)
{
	for (int i = 0; i < size; ++i)
	{
		AddObject(upObjects[i]);
	}
}

void CScene::AddObjects(std::vector<std::unique_ptr<CModelObject>>& upObjects)
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
//	for (std::unique_ptr<CModelObject>& upObject : m_vObjects)
//	{
//		upObject->ReleaseModelUploadBuffer();
//	}
//}

void CScene::DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	m_spShader->SetPipelineState(cpCommandList);
	XMFLOAT4X4 view = Matrix4x4::Transpose(m_Camera.GetView());
	XMFLOAT4X4 proj = Matrix4x4::Transpose(m_Camera.GetProj());
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &view, 16);
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &proj, 32);

	for (std::unique_ptr<CModelObject>& upObject : m_vObjects)
	{
		upObject->DrawObject(cpCommandList);
	}
}


//===============================================================================================
//===============================================================================================

CGameScene::CGameScene()
	: CScene()
{
}

CGameScene::~CGameScene()
{}

HRESULT CGameScene::InitScene(ComPtr<ID3D12Device>& cpDevice, ComPtr<ID3D12RootSignature>& cpRootSignature)
{
	HRESULT hr;
	m_spShader = std::make_shared<CShader>();
	hr = m_spShader->CreateShader(cpDevice, cpRootSignature);
	if (FAILED(hr))
	{
		OutputDebugString(L"CScene::InitScene() Failed to create Shader\n");
		return hr;
	}

	m_spTexShader = std::make_shared<CTextureShader>();
	hr = m_spTexShader->CreateShader(cpDevice, cpRootSignature);
	if (FAILED(hr))
	{
		OutputDebugString(L"CScene::InitScene() Failed to create Shader\n");
		return hr;
	}
}

HRESULT CGameScene::BuildObjects(ComPtr<ID3D12Device>& cpDevice, ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	HRESULT hr;

	m_spModel = std::make_shared<CModel>();
	std::shared_ptr<CMaterial> material = std::make_shared<CMaterial>(m_spShader);
	if (!m_spModel->LoadModelData(cpDevice, cpCommandList, "../Resources/AbramsTank.txt", material))
	{
		return E_FAIL;
	}

	std::unique_ptr<CModelObject> upObj[2];
	upObj[0] = std::make_unique<CModelObject>();
	upObj[1] = std::make_unique<CModelObject>();
	upObj[0]->SetModel(m_spModel);
	upObj[1]->SetModel(m_spModel);

	upObj[0]->SetRotation(0, XMConvertToRadians(90), 0);
	upObj[1]->SetPosition(2, 0, 2);
	
	AddObjects(upObj, 2);


	MeshData<VertexTexcoord> triangle = {
		{
			{ XMFLOAT3(-5.0f, -2.0f, 5.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(5.0f, -2.0f, 5.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
			{ XMFLOAT3(5.0f, -2.0f, -5.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
			{ XMFLOAT3(-5.0f, -2.0f, -5.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) }
		},

		{
			0, 1, 2,
			0, 2, 3
		}
	};

	m_upLandscape = std::make_unique<CMesh>();
	hr = m_upLandscape->CreateBuffer<VertexTexcoord>(cpDevice, cpCommandList, triangle);
	if (FAILED(hr))
	{
		OutputDebugString(L"CGameScene::BuildObjects() Failed to Create Buffer\n");
		return hr;
	}

	return S_OK;
}

void CGameScene::ReleaseUploadBuffers()
{
	m_upLandscape->ReleaseUploadBuffer();
	m_spModel->ReleaseUploadBuffer();
}

void CGameScene::ReleaseObjects()
{
	m_upLandscape.reset();
	m_spModel.reset();
	m_spShader.reset();
}

void CGameScene::DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	m_spShader->SetPipelineState(cpCommandList);
	XMFLOAT4X4 view = Matrix4x4::Transpose(m_Camera.GetView());
	XMFLOAT4X4 proj = Matrix4x4::Transpose(m_Camera.GetProj());
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &view, 16);
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &proj, 32);

	//Landscape
	m_spTexShader->SetPipelineState(cpCommandList);
	XMFLOAT4X4 temp = Matrix4x4::Transpose(Matrix4x4::Identity());
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &temp, 0);
	XMFLOAT4 color{ 1, 1, 1, 1 };
	cpCommandList->SetGraphicsRoot32BitConstants(2, 4, &color, 0);
	m_upLandscape->DrawMesh(cpCommandList);

	for (std::unique_ptr<CModelObject>& upObject : m_vObjects)
	{
		upObject->DrawObject(cpCommandList);
	}
}