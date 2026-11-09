#include "pch.h"
#include "Scene.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"
#include "Player.h"
#include "Camera.h"

CScene::CScene()
{
	XMFLOAT3 eye{ 2.5, 1.0, -2.5 };
	XMFLOAT3 look{ 0, 0, 0 };
	XMFLOAT3 up{ 0, 1, 0 };

	m_spCamera = std::make_shared<CCamera>();
	m_spCamera->SetLookAt(eye, look, up);
	m_spCamera->SetPerspective(90, config::FRAME_BUFFER_W / config::FRAME_BUFFER_H, 0.01f, 100.0f);
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

	return S_OK;
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
	if (height != 0) m_spCamera->SetAspectRatio((float)width / (float)height);
}

void CScene::ProcessInput(UCHAR* pKeyBuffer, float deltaTime)
{
}

void CScene::ReleaseUploadBuffers()
{
}

void CScene::ReleaseObjects()
{
	m_vObjects.clear();
}

void CScene::DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	m_spShader->SetPipelineState(cpCommandList);
	XMFLOAT4X4 view = Matrix4x4::Transpose(m_spCamera->GetView());
	XMFLOAT4X4 proj = Matrix4x4::Transpose(m_spCamera->GetProj());
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

	return S_OK;
}

HRESULT CGameScene::BuildObjects(ComPtr<ID3D12Device>& cpDevice, ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	HRESULT hr;

	//----------------------------------
	// 
	// Terrain
	// 
	//----------------------------------
	m_upTerrainObj = std::make_unique<CTerrainObject>();
	std::shared_ptr<CHeightMapImage> heightmap = std::make_shared<CHeightMapImage>(L"../Resources/Cheongsando.raw", 505, 505);
	m_upTerrainObj->SetHeightMap(heightmap);
	hr = m_upTerrainObj->GenerateTerrain(cpDevice, cpCommandList);
	//스케일에 정점 사이간 실제 배율 몇m인지 체크해서 넣으면 됨. '가 원래 소수점
	m_upTerrainObj->SetScale(1.578'37f, 0.075'39f, 1.578'37f);


	//----------------------------------
	// 
	// Model Data
	// 
	//----------------------------------
	m_spModel = std::make_shared<CModel>();
	std::shared_ptr<CMaterial> material = std::make_shared<CMaterial>(m_spShader);
	if (!m_spModel->LoadModelData(cpDevice, cpCommandList, "../Resources/AbramsTank.txt", material))
	{
		return E_FAIL;
	}

	//----------------------------------
	// 
	// Player
	// 
	//----------------------------------
	std::unique_ptr<CTankPlayer> upPlayer = std::make_unique<CTankPlayer>();
	upPlayer->SetModel(m_spModel);
	upPlayer->SetCamera(m_spCamera);
	upPlayer->SetPosition(0.0f, m_upTerrainObj->GetHeightAt(0.0f, 0.0f) + 1.0f, 0.0f);

	const HRESULT cannonTestResult = upPlayer->CreateCannonTestMesh(cpDevice, cpCommandList);
	if (FAILED(cannonTestResult)) return cannonTestResult;

	m_pPlayer = upPlayer.get();

	//----------------------------------
	// 
	// Objs
	// 
	//----------------------------------
	m_vObjects.push_back(std::move(upPlayer));

	std::unique_ptr<CModelObject> upObj[1];
	upObj[0] = std::make_unique<CModelObject>();
	upObj[0]->SetModel(m_spModel);

	upObj[0]->SetPosition(10.0f, m_upTerrainObj->GetHeightAt(10.0f, 10.0f) + 1.0f, 10.0f);
	
	AddObjects(upObj, 1);

	//----------------------------------
	// 
	// Camera
	// 
	//----------------------------------
	m_spCamera->SetPerspective(90, config::FRAME_BUFFER_W / config::FRAME_BUFFER_H, 0.01f, 100.0f);
	XMFLOAT3 playerPos = m_pPlayer->GetPosition();

	XMFLOAT3 eye{
		playerPos.x,
		playerPos.y + 1.0f,
		playerPos.z - 3.5f
	};

	XMFLOAT3 look{
		playerPos.x,
		playerPos.y + 1.0f,
		playerPos.z
	};

	m_spCamera->SetLookAt(eye, look, XMFLOAT3{ 0, 1, 0 });

	if (FAILED(hr))
	{
		OutputDebugString(L"CGameScene::BuildObjects() Failed to Create Buffer\n");
		return hr;
	}

	return S_OK;
}

void CGameScene::ProcessInput(UCHAR* pKeyBuffer, float deltaTime)
{
	DWORD dwDirection = 0;

	//키보드 상태 정보를 반환한다.
	if (::GetKeyboardState(pKeyBuffer))
	{
		if (pKeyBuffer[VK_UP] & 0xF0) dwDirection |= DIR_FORWARD;
		if (pKeyBuffer[VK_DOWN] & 0xF0) dwDirection |= DIR_BACKWARD;
		if (pKeyBuffer[VK_LEFT] & 0xF0) dwDirection |= DIR_LEFT;
		if (pKeyBuffer[VK_RIGHT] & 0xF0) dwDirection |= DIR_RIGHT;
		if (pKeyBuffer['W'] & 0xF0) dwDirection |= CANNON_UP;
		if (pKeyBuffer['S'] & 0xF0) dwDirection |= CANNON_DOWN;
		if (pKeyBuffer['A'] & 0xF0) dwDirection |= TURRET_LEFT;
		if (pKeyBuffer['D'] & 0xF0) dwDirection |=  TURRET_RIGHT;
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
		if (m_pPlayer && dwDirection)
		{
			m_pPlayer->ProcessInput(dwDirection, deltaTime, m_upTerrainObj.get());
		}
	}
}

void CGameScene::ReleaseUploadBuffers()
{
	m_upTerrainObj->ReleaseUploadBuffer();
	m_spModel->ReleaseUploadBuffer();
	if (m_pPlayer) m_pPlayer->ReleaseCannonTestUploadBuffer();
}

void CGameScene::ReleaseObjects()
{
	m_upTerrainObj.reset();
	m_spModel.reset();
	m_spShader.reset();
}

void CGameScene::DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	m_spShader->SetPipelineState(cpCommandList);
	XMFLOAT4X4 view = Matrix4x4::Transpose(m_spCamera->GetView());
	XMFLOAT4X4 proj = Matrix4x4::Transpose(m_spCamera->GetProj());
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &view, 16);
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &proj, 32);

	//Terrain
	m_spTexShader->SetPipelineState(cpCommandList);
	XMFLOAT4X4 temp = Matrix4x4::Transpose(Matrix4x4::Identity());
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &temp, 0);
	XMFLOAT4 color{ 1, 1, 1, 1 };
	cpCommandList->SetGraphicsRoot32BitConstants(2, 4, &color, 0);
	m_upTerrainObj->DrawObject(cpCommandList);

	for (std::unique_ptr<CModelObject>& upObject : m_vObjects)
	{
		upObject->DrawObject(cpCommandList);
	}
}