#include "pch.h"
#include "Scene.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"
#include "Player.h"
#include "Camera.h"
//-----------------------------------
//[아래 코드는 AI로 구현됨]
#include "UiScene.h"
#include "GameFramework.h"
//-----------------------------------

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

HRESULT CScene::InitScene(ComPtr<ID3D12Device>& cpDevice,
		ComPtr<ID3D12GraphicsCommandList>& cpCommandList, CDescHeapAllocator& descHeapAllocator)
{
	HRESULT hr;
	m_spShader = std::make_shared<CShader>();
	hr = m_spShader->CreateShader(cpDevice, cpCommandList, descHeapAllocator);
	if (FAILED(hr))
	{
		OutputDebugString(L"CScene::InitScene() Failed to create Shader\n");
		return hr;
	}

	return S_OK;
}

HRESULT CScene::BuildObjects(
	ComPtr<ID3D12Device>& cpDevice,
	ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
	CDescHeapAllocator* DescHeapAllocator)
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
	for (auto& object : upObjects) AddObject(object);
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
	m_spShader->SetGraphicsRootSignature(cpCommandList);
	m_spShader->SetPipelineState(cpCommandList, ERenderState::Default);
	
	//기본 타일링 수치 설정
	float defaultTiling = 1.0f;
	cpCommandList->SetGraphicsRoot32BitConstants(1, 1, &defaultTiling, 4);

	CB_SceneInfo sceneInfo;
	sceneInfo.view = Matrix4x4::Transpose(m_spCamera->GetView());
	sceneInfo.proj = Matrix4x4::Transpose(m_spCamera->GetProj());
	sceneInfo.viewSky = m_spCamera->GetView();
	sceneInfo.viewSky._41 = sceneInfo.viewSky._42 = sceneInfo.viewSky._43 = 0.0f;
	sceneInfo.viewSky = Matrix4x4::Transpose(sceneInfo.viewSky);
	m_spShader->SetSceneMatrix(cpCommandList, sceneInfo);

	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	for (auto& upObject : m_vObjects)
	{
		upObject->DrawObject(cpCommandList);
	}
	//-----------------------------------
}

//===============================================================================================
//===============================================================================================

//-----------------------------------
//[아래 코드는 AI로 구현됨]
namespace
{
	bool Down(const UCHAR* keys, int key)
	{
		return (keys[key] & 0x80) != 0;
	}

	DWORD ReadTankControls(const UCHAR* keys)
	{
		DWORD input = 0;
		if (Down(keys, 'W')) input |= DIR_FORWARD;
		if (Down(keys, 'S')) input |= DIR_BACKWARD;
		if (Down(keys, 'A')) input |= DIR_LEFT;
		if (Down(keys, 'D')) input |= DIR_RIGHT;
		if (Down(keys, VK_UP)) input |= CANNON_UP;
		if (Down(keys, VK_DOWN)) input |= CANNON_DOWN;
		if (Down(keys, VK_LEFT)) input |= TURRET_LEFT;
		if (Down(keys, VK_RIGHT)) input |= TURRET_RIGHT;
		return input;
	}
}

using namespace config::stage;

CStageResources::CStageResources() = default;
CStageResources::~CStageResources() = default;

CGameScene::CGameScene(const PlayerRules& rules)
	: m_rules(rules)
{
}

//-----------------------------------
//[직접 작성한 코드]
CGameScene::~CGameScene()
{}


//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
HRESULT CStageResources::Build(ComPtr<ID3D12Device>& cpDevice,
	ComPtr<ID3D12GraphicsCommandList>& cpCommandList, CDescHeapAllocator& allocator)
{
	spShader = std::make_shared<CShader>();
	HRESULT shaderResult = spShader->CreateShader(cpDevice, cpCommandList, allocator);
	if (FAILED(shaderResult)) return shaderResult;
	//-----------------------------------
	//[직접 작성한 코드]
	upTerrain = std::make_unique<CTerrainObject>();
	auto spHeightmap = std::make_shared<CHeightMapImage>(L"../Resources/Cheongsando.raw", TerrainSamples, TerrainSamples);
	upTerrain->SetHeightMap(spHeightmap);
	HRESULT hr = upTerrain->GenerateTerrain(cpDevice, cpCommandList, &allocator, true);
	if (FAILED(hr)) return hr;

	//auto spNormalmap = std::make_shared<CNormalMapImage>(L"../Resources/Cheongsando.raw", TerrainSamples, TerrainSamples);

	upTerrain->SetScale(TerrainHorizontalScale, TerrainVerticalScale, TerrainHorizontalScale);

	spTank = std::make_shared<CModel>();
	auto spMaterial = std::make_shared<CMaterial>(spShader);
	if (!spTank->LoadModelData(cpDevice, cpCommandList, "../Resources/AbramsTank.txt", spMaterial))
		return E_FAIL;

	//-----------------------------------
	upSkybox = std::make_unique<CSkyboxObject>();
	hr = upSkybox->GenerateSkyBox(cpDevice, cpCommandList, allocator);
	if (FAILED(hr)) return hr;

	CTankPlayer::PrepareModel(spTank);
	return BuildVisuals(cpDevice, cpCommandList);
}

HRESULT CGameScene::BuildObjects(ComPtr<ID3D12Device>& cpDevice,
	ComPtr<ID3D12GraphicsCommandList>& cpCommandList, CDescHeapAllocator*)
{
	if (!m_spStageResources) return E_UNEXPECTED;
	m_vObjects.clear();
	m_vEnemies.clear();
	m_vProjectiles.clear();
	m_vHud.clear();
	for (int i = 0; i < HealthHudBegin + m_rules.maxHealth + 1; ++i)
	{
		auto upUi = std::make_unique<CUI>(XMFLOAT4{ 0, 0, 1, 1 }, XMFLOAT4{ 1, 1, 1, 1 });
		const HRESULT hr = upUi->BuildMesh(cpDevice, cpCommandList);
		if (FAILED(hr)) return hr;
		m_vHud.push_back(std::move(upUi));
	}
	return S_OK;
}


//-----------------------------------

void CGameScene::SetStageResources(const std::shared_ptr<CStageResources>& resources)
{
	m_spStageResources = resources;
	m_spShader = resources->spShader;
}

void CGameScene::ReleaseUploadBuffers()
{
	for (auto& ui : m_vHud) ui->ReleaseUploadBuffer();
}

void CGameScene::ReleaseObjects()
{
	m_vEnemies.clear();
	m_vProjectiles.clear();
	m_vHud.clear();
	CScene::ReleaseObjects();
	m_spStageResources.reset();
	m_spShader.reset();
}

void CGameScene::DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	//-----------------------------------
	//[직접 작성한 코드]
	m_spShader->SetGraphicsRootSignature(cpCommandList);
	m_spShader->SetPipelineState(cpCommandList, ERenderState::Default);

	CB_SceneInfo sceneInfo;
	sceneInfo.view = Matrix4x4::Transpose(m_spCamera->GetView());
	sceneInfo.proj = Matrix4x4::Transpose(m_spCamera->GetProj());
	sceneInfo.viewSky = m_spCamera->GetView();
	sceneInfo.viewSky._41 = sceneInfo.viewSky._42 = sceneInfo.viewSky._43 = 0.0f;
	sceneInfo.viewSky = Matrix4x4::Transpose(sceneInfo.viewSky);
	m_spShader->SetSceneMatrix(cpCommandList, sceneInfo);

	//Skybox
	m_spShader->SetPipelineState(cpCommandList, ERenderState::Skybox);
	m_spStageResources->upSkybox->DrawObject(cpCommandList);

	//Terrain
	m_spShader->SetPipelineState(cpCommandList, ERenderState::NormalMap);
	XMFLOAT4X4 temp = Matrix4x4::Transpose(Matrix4x4::Identity());
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &temp, 0);
	XMFLOAT4 color{ 1, 1, 1, 1 };
	cpCommandList->SetGraphicsRoot32BitConstants(1, 4, &color, 0);
	m_spStageResources->upTerrain->DrawObject(cpCommandList);

	//-----------------------------------
	DrawStage(cpCommandList);
}

HRESULT CStageResources::BuildVisuals(const ComPtr<ID3D12Device>& cpDevice, const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	const auto loadModel = [&](std::shared_ptr<CModel>& model, const char* path, const XMFLOAT3& offset)
	{
		model = std::make_shared<CModel>();
		auto material = std::make_shared<CMaterial>(spShader);
		std::string error;
		if (!model->LoadModelData(cpDevice, cpCommandList, path, material, &error))
		{
			const std::string message = std::string(path) + ": " + error + "\n";
			OutputDebugStringA(message.c_str());
			return false;
		}
		const auto& root = model->getData().front().name;
		model->SetFrameLocal(root, Matrix4x4::Multiply(model->GetFrameMatrix(root), Matrix4x4::Translation(offset)));
		return true;
	};
	if (!loadModel(spApache, "../Resources/Apache.txt", { -2.1725f, -3.0716f, 21.0388f }) ||
		!loadModel(spRocks[0], "../Resources/Rock.txt", { -2.9746f, -0.098f, -2.0176f }) ||
		!loadModel(spRocks[1], "../Resources/Rock2.txt", { -2.9407f, -0.098f, -3.3343f })) return E_FAIL;

	MeshData<VertexTexcoord> cube;
	const XMFLOAT3 corners[] =
	{
		{ -.5f, -.5f, -.5f }, { -.5f, .5f, -.5f }, { .5f, .5f, -.5f }, { .5f, -.5f, -.5f },
		{ -.5f, -.5f, .5f }, { -.5f, .5f, .5f }, { .5f, .5f, .5f }, { .5f, -.5f, .5f }
	};
	for (const auto& corner : corners) cube.vertices.push_back({ corner, { 1, 1, 1, 1 }, { 0, 0 } });
	cube.indices =
	{
		0, 1, 2, 0, 2, 3, 4, 7, 6, 4, 6, 5, 0, 4, 5, 0, 5, 1,
		3, 2, 6, 3, 6, 7, 1, 5, 6, 1, 6, 2, 0, 3, 7, 0, 7, 4
	};
	upBox = std::make_unique<CMesh>();
	return upBox->CreateBuffer(cpDevice, cpCommandList, cube);
}

void CGameScene::DrawStageBox(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList, const XMFLOAT3& position,
	const XMFLOAT3& size, const XMFLOAT4& color, float yaw)
{
	Transform pose;
	pose.position = position;
	pose.rotation.y = yaw;
	pose.scale = size;
	const XMFLOAT4X4 matrix = Matrix4x4::Transpose(pose.getMatrix());
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &matrix, 0);
	const XMFLOAT4X4 worldInv = Matrix4x4::Inverse(matrix);
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &worldInv, 16);
	cpCommandList->SetGraphicsRoot32BitConstants(1, 4, &color, 0);
	m_spStageResources->upBox->DrawMesh(cpCommandList);
}

void CGameScene::UpdateStageView()
{
	XMFLOAT3 aimOrigin, aimDirection;
	GetAimRay(aimOrigin, aimDirection);
	const XMFLOAT3 aimEnd = Vector3::Add(aimOrigin, aimDirection, ReticleRayDistance);
	const float hit = TerrainHitFraction(aimOrigin, aimEnd, 0.0f);
	m_reticleAimPoint = Vector3::Lerp(aimOrigin, aimEnd, std::clamp(hit, ReticleMinRayFraction, 1.0f));
	UpdateStageCamera(m_reticleAimPoint);
}

void CGameScene::DrawStageHud(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	m_spShader->SetPipelineState(cpCommandList, ERenderState::UI);
	const XMFLOAT4X4 view = m_spCamera->GetView();
	const XMFLOAT4X4 projection = m_spCamera->GetProj();
	const XMFLOAT3 aimNdc = Vector3::TransformCoord(m_reticleAimPoint, Matrix4x4::Multiply(view, projection));

	const XMFLOAT4X4 identity = Matrix4x4::Identity();
	const CB_SceneInfo sceneInfo{ identity, identity, identity };
	m_spShader->SetSceneMatrix(cpCommandList, sceneInfo, true);

	const float aspectScale = projection._11 / projection._22;
	const XMFLOAT4 white{ .9f, .95f, 1, 1 };
	const XMFLOAT4 lockColor = m_bLockPinned ? XMFLOAT4{ .18f, .96f, 1, 1 } : XMFLOAT4{ 1, .86f, .08f, 1 };
	for (auto& ui : m_vHud) ui->SetVisible(false);
	const auto hudPlane = [&](size_t slot, float x, float y, float width, float height, const XMFLOAT4& color)
	{
		SetHudPlane(slot, x, y, width * aspectScale, height, color);
	};

	const XMFLOAT4 reticle = m_nLockedEnemy < 0 ? white : lockColor;
	if (aimNdc.z >= 0.0f && aimNdc.z <= 1.0f)
	{
		hudPlane(ReticleHorizontalHud, aimNdc.x, aimNdc.y, .04f, .005f, reticle);
		hudPlane(ReticleVerticalHud, aimNdc.x, aimNdc.y, .005f, .04f, reticle);
	}
	for (int i = 0; i < m_rules.maxHealth; ++i)
	{
		const XMFLOAT4 color = i < PlayerObject().GetHealth() ? XMFLOAT4{ .18f, .85f, .35f, 1 } : XMFLOAT4{ .35f, .15f, .15f, 1 };
		hudPlane(HealthHudBegin + i, -.87f + i * .065f * aspectScale, .86f, .055f, .045f, color);
	}
	UpdateHudExtras(aspectScale, lockColor);
	if (m_state == EStageState::Victory || m_state == EStageState::Defeat)
	{
		const XMFLOAT4 color = m_state == EStageState::Victory
			? XMFLOAT4{ .18f, .72f, .32f, 1 } : XMFLOAT4{ .68f, .17f, .19f, 1 };
		hudPlane(HealthHudBegin + m_rules.maxHealth, 0.0f, .45f, .65f, .14f, color);
	}
	for (auto& ui : m_vHud) ui->DrawObject(cpCommandList);
}

void CGameScene::DrawStage(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	m_spShader->SetPipelineState(cpCommandList, ERenderState::Default);
	DrawStageObjects(cpCommandList);
	for (const auto& object : m_vObjects) object->DrawObject(cpCommandList);
	if (m_nLockedEnemy >= 0)
	{
		const auto& enemy = *m_vEnemies[m_nLockedEnemy];
		if (enemy.IsAlive())
		{
			XMFLOAT3 marker = enemy.GetPosition();
			marker.y += enemy.GetMarkerHeight();
			const XMFLOAT4 color = m_bLockPinned ? XMFLOAT4{ .18f, .96f, 1, 1 } : XMFLOAT4{ 1, .86f, .08f, 1 };
			DrawStageBox(cpCommandList, marker, { .6f, .28f, .6f }, color);
		}
	}
	for (const auto& shot : m_vProjectiles) shot->DrawObject(cpCommandList);
	DrawStageHud(cpCommandList);
}


XMFLOAT3 CGameScene::OnTerrain(float x, float z, float offset) const
{
	return { x, m_spStageResources->upTerrain->GetHeightAt(x, z) + offset, z };
}

void CGameScene::BeginStage(EGameStage stage)
{
	m_stage = stage;
	m_state = EStageState::Playing;
	m_bFireRequested = false;
	m_bLockPinned = false;
	m_nLockedEnemy = -1;
	PlayerObject().SetCombatInfo(m_rules.maxHealth, m_rules.aimHeight, m_rules.hitRadius,
		m_rules.hitRadius, 0.0f, m_rules.aimHeight);
	std::fill(std::begin(m_keys), std::end(m_keys), UCHAR{});
	m_fReload = m_fResultTimer = 0.0f;
	m_vEnemies.clear();
	m_vProjectiles.clear();
	// 플레이어 인스턴스는 보존하고 스테이지에 배치한 나머지 객체를 제거한다.
	const auto* player = &PlayerObject();
	std::erase_if(m_vObjects, [player](const auto& object) { return object.get() != player; });

	ResetStage();
	const XMFLOAT4X4 projection = m_spCamera->GetProj();
	m_spCamera->SetPerspective(CameraFovDegrees, projection._22 / projection._11, CameraNear, CameraFar);
	UpdateStageView();
}

void CGameScene::StageKeyPressed(WPARAM key)
{
	if (m_state != EStageState::Playing) return;
	if (key == 'P')
	{
		Win();
		return;
	}
	if (key == VK_SPACE) m_bFireRequested = true;
	if (key == 'Q' && m_nLockedEnemy >= 0) m_bLockPinned = !m_bLockPinned;
	OnStageKeyPressed(key);
}

XMFLOAT3 CGameScene::PlayerAimPoint() const
{
	return PlayerObject().GetAimPoint();
}


void CGameScene::UpdateLock(const XMFLOAT3& origin, const XMFLOAT3& direction)
{
	if (m_bLockPinned && m_nLockedEnemy >= 0 && m_vEnemies[m_nLockedEnemy]->IsAlive()) return;
	m_bLockPinned = false;
	m_nLockedEnemy = -1;
	int directlyHit = -1;
	float bestScore = -1.0f;
	const float maxDistance = m_rules.shotSpeed * m_rules.shotLifetime;
	const XMFLOAT3 rayEnd = Vector3::Add(origin, direction, maxDistance);
	float nearest = (std::min)(1.0f, TerrainHitFraction(origin, rayEnd, LockTerrainClearance));
	for (size_t i = 0; i < m_vEnemies.size(); ++i)
	{
		const auto& enemy = *m_vEnemies[i];
		if (!enemy.IsAlive()) continue;
		const XMFLOAT3 aim = enemy.GetAimPoint();
		const float hit = Collision::SegmentSphereFraction(origin, rayEnd, aim, m_fLockRadius);
		if (hit < nearest)
		{
			directlyHit = static_cast<int>(i);
			nearest = hit;
		}
		const XMFLOAT3 delta = Vector3::Subtract(aim, origin);
		const float distance = Vector3::Length(delta);
		const float dot = Vector3::DotProduct(direction, Vector3::Normalize(delta));
		const float score = dot - distance * LockDistancePenalty;
		if (distance > maxDistance || dot < m_fLockMinDot || score <= bestScore ||
			TerrainHitFraction(origin, aim, LockTerrainClearance) < 1.0f) continue;
		m_nLockedEnemy = static_cast<int>(i);
		bestScore = score;
	}
	if (directlyHit >= 0) m_nLockedEnemy = directlyHit;
}

void CGameScene::UpdateEnemies(float dt)
{
	const XMFLOAT3 target = PlayerAimPoint();
	for (auto* enemy : m_vEnemies)
	{
		if (!enemy->IsAlive()) continue;
		XMFLOAT3 muzzle, direction;
		if (!enemy->TryFire(target, dt, muzzle, direction)) continue;
		if (TerrainHitFraction(muzzle, target, TankShellClearance) < 1.0f) continue;
		SpawnProjectile(muzzle, {
			.direction = direction,
			.speed = TankShellSpeed,
			.lifetime = TankShellLifetime,
			.clearance = TankShellClearance,
			.enemyOwned = true
		});
		enemy->Reload();
	}
}

void CGameScene::FirePlayer(const XMFLOAT3& origin, const XMFLOAT3& direction)
{
	if (m_fReload > 0.0f) return;
	SpawnProjectile(origin, {
		.direction = direction,
		.speed = m_rules.shotSpeed,
		.lifetime = m_rules.shotLifetime,
		.clearance = m_rules.shotClearance,
		.homingDelay = m_rules.homingDelay,
		.homingTurnSpeed = m_rules.homingTurnSpeed,
		.target = m_rules.homing ? m_nLockedEnemy : -1
	});
	m_fReload = m_rules.reload;
}

float CGameScene::TerrainHitFraction(const XMFLOAT3& start, const XMFLOAT3& end, float clearance) const
{
	const float distance = Vector3::Length(Vector3::Subtract(end, start));
	const int steps = (std::max)(1, static_cast<int>(std::ceil(distance / TerrainRaySampleSpacing)));
	for (int i = 0; i <= steps; ++i)
	{
		const float t = static_cast<float>(i) / steps;
		const XMFLOAT3 point = Vector3::Lerp(start, end, t);
		if (std::abs(point.x) > TerrainEdge || std::abs(point.z) > TerrainEdge ||
			point.y <= m_spStageResources->upTerrain->GetHeightAt(point) + clearance) return t;
	}
	return 2.0f;
}

void CGameScene::UpdateProjectiles(float dt)
{
	for (auto& upShot : m_vProjectiles)
	{
		auto& shot = *upShot;
		std::optional<XMFLOAT3> target;
		if (shot.flight.target >= 0 && m_vEnemies[shot.flight.target]->IsAlive())
			target = m_vEnemies[shot.flight.target]->GetAimPoint();
		const float stepTime = shot.AdvanceTime(dt, target);
		const XMFLOAT3 end = shot.NextPosition(stepTime);
		float closest = TerrainHitFraction(shot.Position(), end, shot.flight.clearance);
		int hitEnemy = -1;
		if (shot.flight.enemyOwned)
		{
			const float hit = Collision::SegmentSphereFraction(shot.Position(), end, PlayerAimPoint(),
				PlayerObject().GetHitRadius(false));
			if (hit <= 1.0f && hit < closest)
			{
				closest = hit;
				const bool wasAlive = IsPlayerAlive();
				if (!IsPlayerProtected()) PlayerObject().ApplyDamage();
				if (wasAlive && !IsPlayerAlive())
				{
					m_state = EStageState::Defeat;
					m_fResultTimer = DefeatDelay;
					m_bLockPinned = false;
					m_nLockedEnemy = -1;
				}
			}
		}
		else
		{
			for (size_t i = 0; i < m_vEnemies.size(); ++i)
			{
				auto& enemy = *m_vEnemies[i];
				if (!enemy.IsAlive()) continue;
				const float radius = enemy.GetHitRadius(shot.flight.target >= 0);
				const XMFLOAT3 center = enemy.GetHitPoint();
				const float hit = Collision::SegmentSphereFraction(shot.Position(), end, center, radius);
				if (hit <= 1.0f && hit < closest)
				{
					closest = hit;
					hitEnemy = static_cast<int>(i);
				}
			}
		}
		if (hitEnemy >= 0) m_vEnemies[hitEnemy]->ApplyDamage();
		if (closest <= 1.0f) shot.flight.lifetime = 0.0f;
		shot.Position() = end;
	}
	std::erase_if(m_vProjectiles, [](const std::unique_ptr<CProjectileObject>& shot) { return shot->flight.lifetime <= 0.0f; });
}

void CGameScene::Win()
{
	m_fResultTimer = VictoryHoldTime();
	m_state = m_fResultTimer > 0.0f ? EStageState::Victory : EStageState::Finished;
	m_vProjectiles.clear();
}

void CGameScene::ProcessInput(UCHAR* keys, float)
{
	std::copy_n(keys, 256, m_keys);
}

void CGameScene::Update(float dt)
{
	const UCHAR* keys = m_keys;
	dt = std::clamp(dt, 0.0f, MaxTimeStep);
	if (m_state == EStageState::Finished) return;
	if (m_state == EStageState::Victory || m_state == EStageState::Defeat)
	{
		m_fResultTimer -= dt;
		if (m_fResultTimer <= 0.0f)
		{
			if (m_state == EStageState::Victory) m_state = EStageState::Finished;
			else BeginStage(m_stage);
		}
		return;
	}

	UpdatePlayer(keys, dt);
	m_fReload = (std::max)(0.0f, m_fReload - dt);
	XMFLOAT3 origin, direction;
	GetAimRay(origin, direction);
	origin = Vector3::Add(origin, direction, m_rules.muzzleOffset);
	UpdateLock(origin, direction);

	const bool automaticShot = TryAutoAim(keys, dt, origin, direction);
	if (m_bFireRequested || automaticShot) FirePlayer(origin, direction);
	m_bFireRequested = false;
	UpdateEnemies(dt);
	UpdateProjectiles(dt);

	const bool allDestroyed = std::none_of(m_vEnemies.begin(), m_vEnemies.end(),
		[](const CDamageableObject* enemy) { return enemy->IsAlive(); });
	if (IsPlayerAlive() && (allDestroyed || HasReachedExit())) Win();
	UpdateStageView();
}

void CGameScene::AddTarget(const XMFLOAT3& position)
{
	auto object = std::make_unique<CTargetObject>(*m_spStageResources->upBox);
	object->SetPosition(position);
	m_vEnemies.push_back(object.get());
	m_vObjects.push_back(std::move(object));
}

void CGameScene::AddEnemyTank(float x, float z, float playerStartZ)
{
	auto enemyTank = std::make_unique<CEnemyTank>();
	enemyTank->UsePreparedTankModel(m_spStageResources->spTank);
	enemyTank->ResetStagePose(OnTerrain(x, z, TankGroundOffset), std::atan2(-x, playerStartZ - z));
	enemyTank->FollowStageSlope(*m_spStageResources->upTerrain);
	m_vEnemies.push_back(enemyTank.get());
	m_vObjects.push_back(std::move(enemyTank));
}

void CGameScene::SetLockSettings(float minDot, float radius)
{
	m_fLockMinDot = minDot;
	m_fLockRadius = radius;
}

std::optional<XMFLOAT3> CGameScene::LockedAimPoint() const
{
	if (m_nLockedEnemy < 0) return std::nullopt;
	return m_vEnemies[m_nLockedEnemy]->GetAimPoint();
}

CTankGameScene::CTankGameScene()
	: CGameScene(PlayerRules{
		.maxHealth = PlayerTankHealth,
		.aimHeight = TankAimHeight,
		.hitRadius = TankHitRadius,
		.reload = TankReload,
		.shotSpeed = TankShellSpeed,
		.shotLifetime = TankShellLifetime,
		.shotClearance = TankShellClearance
	})
{
}

CTankGameScene::~CTankGameScene() = default;

HRESULT CTankGameScene::BuildObjects(
	ComPtr<ID3D12Device>& cpDevice,
	ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
	CDescHeapAllocator* DescHeapAllocator)
{
	const HRESULT hr = CGameScene::BuildObjects(cpDevice, cpCommandList, DescHeapAllocator);
	if (FAILED(hr)) return hr;

	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	auto upPlayer = std::make_unique<CTankPlayer>();
	m_pPlayer = upPlayer.get();
	m_pPlayer->UsePreparedTankModel(m_spStageResources->spTank);
	m_vObjects.push_back(std::move(upPlayer));
	//-----------------------------------

	const auto tankBounds = m_spStageResources->spTank->GetModelBounds();
	if (!tankBounds) return E_FAIL;
	m_tankBounds = *tankBounds;

	for (size_t i = 0; i < std::size(m_rockBounds); ++i)
	{
		const auto bounds = m_spStageResources->spRocks[i]->GetModelBounds();
		if (!bounds) return E_FAIL;
		m_rockBounds[i] = *bounds;
	}
	return m_pPlayer->CreateCannonTestMesh(cpDevice, cpCommandList);
}

void CTankGameScene::ReleaseUploadBuffers()
{
	CGameScene::ReleaseUploadBuffers();
	if (m_pPlayer) m_pPlayer->ReleaseCannonTestUploadBuffer();
}

void CTankGameScene::ReleaseObjects()
{
	m_pPlayer = nullptr;
	m_vObstacles.clear();
	CGameScene::ReleaseObjects();
}

void CTankGameScene::ResetStage()
{
	m_bAutoAttack = TankAutoAttackAtStart;
	m_bShield = false;
	m_vObstacles.clear();
	SetLockSettings(TankLockMinDot, TankHitRadius);

	const auto& spawn = Level2Spawn;
	const float startZ = -UsableHalf * spawn.startDepth;
	m_pPlayer->ResetStagePose(OnTerrain(0.0f, startZ, TankGroundOffset), 0.0f);

	for (int i = 0; i < Level2TankCount; ++i)
	{
		const float t = static_cast<float>(i) / (std::max)(1, Level2TankCount - 1);
		const float angle = i * SpiralAngle + spawn.angleOffset;
		const float z = UsableHalf * (spawn.depthStart + t * spawn.depthSpan + std::cos(angle) * spawn.depthScatter);
		const float x = UsableHalf * ((i % Level2TankLanes - (Level2TankLanes - 1) * .5f) * spawn.width +
			std::sin(angle) * Level2TankSideScatter);
		AddEnemyTank(x, z, startZ);
	}

	for (int i = 0; i < Level2ObstacleCount; ++i)
	{
		const float t = static_cast<float>(i) / (std::max)(1, Level2ObstacleCount - 1);
		const float angle = i * SpiralAngle + ObstacleAngleOffset;
		const float ring = ObstacleRingStart + ObstacleRingSpan * std::fmod(i * ObstacleRingStep, 1.0f);
		float x = std::sin(angle) * UsableHalf * ring;
		float z = UsableHalf * (ObstacleDepthStart + t * ObstacleDepthSpan + std::cos(angle) * ObstacleDepthScatter);
		if (std::abs(x) < ObstacleSpawnLaneWidth && z < UsableHalf * ObstacleSpawnLaneDepth)
			x += x < 0.0f ? -ObstacleSpawnLanePush : ObstacleSpawnLanePush;
		z = std::clamp(z, UsableHalf * ObstacleMinDepth, UsableHalf * ObstacleMaxDepth);
		const float scale = ObstacleBaseScale + (i % ObstacleScaleVariants) * ObstacleScaleStep;
		Transform pose;
		pose.position = OnTerrain(x, z, 0.0f);
		pose.rotation.y = angle + (i % ObstacleYawVariants) * ObstacleYawStep;
		pose.scale = { scale, scale, scale };
		//-----------------------------------
		//[아래 코드는 AI로 구현됨]
		const size_t variant = i % std::size(m_rockBounds);
		m_vObstacles.push_back({ Collision::MakeFootprint(m_rockBounds[variant], pose) });
		auto object = std::make_unique<CModelObject>();
		object->SetModel(m_spStageResources->spRocks[variant]);
		object->SetPosition(pose.position);
		object->SetRotation(pose.rotation);
		object->SetScale(pose.scale);
		m_vObjects.push_back(std::move(object));
		//-----------------------------------
	}
}

void CTankGameScene::OnStageKeyPressed(WPARAM key)
{
	if (key == 'T') m_bAutoAttack = !m_bAutoAttack;
	if (key == 'H') m_bShield = !m_bShield;
}

CDamageableObject& CTankGameScene::PlayerObject() const
{
	return *m_pPlayer;
}

void CTankGameScene::GetAimRay(XMFLOAT3& origin, XMFLOAT3& direction) const
{
	m_pPlayer->GetStageCannonRay(origin, direction);
}

bool CTankGameScene::TryAutoAim(const UCHAR* keys, float dt, XMFLOAT3& origin, XMFLOAT3& direction)
{
	constexpr DWORD manualAimMask = CANNON_UP | CANNON_DOWN | TURRET_LEFT | TURRET_RIGHT;
	if (!m_bAutoAttack || (ReadTankControls(keys) & manualAimMask) != 0) return false;
	const auto aim = LockedAimPoint();
	if (!aim) return false;

	const bool aimed = m_pPlayer->AimStageCannon(*aim, dt);
	GetAimRay(origin, direction);
	return aimed;
}


void CTankGameScene::UpdateHudExtras(float aspectScale, const XMFLOAT4& lockColor)
{
	const XMFLOAT4 inactive{ .25f, .25f, .25f, 1 };
	const XMFLOAT4 active{ .9f, .95f, 1, 1 };
	SetHudPlane(AutoAttackHud, -.87f, .77f, .055f * aspectScale, .025f, m_bAutoAttack ? active : inactive);
	SetHudPlane(ShieldHud, -.87f + .065f * aspectScale, .77f, .055f * aspectScale, .025f, m_bShield ? lockColor : inactive);
}

void CTankGameScene::UpdatePlayer(const UCHAR* keys, float dt)
{
	const DWORD input = ReadTankControls(keys);
	if (input)
	{
		const Transform previous = m_pPlayer->GetTransform();
		m_pPlayer->ApplyControls(input, dt, m_spStageResources->upTerrain.get());
		const Transform& candidate = m_pPlayer->GetTransform();

		const BoundingOrientedBox previousBounds = Collision::MakeFootprint(m_tankBounds, previous);
		const BoundingOrientedBox candidateBounds = Collision::MakeFootprint(m_tankBounds, candidate);
		bool collision = std::abs(candidate.position.x) > MovementLimit || std::abs(candidate.position.z) > MovementLimit;
		for (const Obstacle& obstacle : m_vObstacles)
		{
			if (collision) break;
			if (!candidateBounds.Intersects(obstacle.footprint)) continue;

			const XMFLOAT3& center = obstacle.footprint.Center;
			const float oldDistance = Vector3::LengthSquared(Vector3::Subtract(previousBounds.Center, center));
			const float newDistance = Vector3::LengthSquared(Vector3::Subtract(candidateBounds.Center, center));
			collision = !previousBounds.Intersects(obstacle.footprint) || newDistance < oldDistance;
		}
		if (collision)
		{
			m_pPlayer->SetPosition(previous.position);
			m_pPlayer->SetRotation(previous.rotation);
		}
	}
	m_pPlayer->FollowStageSlope(*m_spStageResources->upTerrain);
}

void CTankGameScene::UpdateStageCamera(const XMFLOAT3& aimPoint)
{
	const Transform& tank = m_pPlayer->GetTransform();
	const XMFLOAT3 forward = m_pPlayer->GetTurretHeading();
	XMFLOAT3 eye = Vector3::Add(tank.position, forward, -TankCameraDistance);
	eye.y += TankCameraHeight;
	eye.y = (std::max)(eye.y, m_spStageResources->upTerrain->GetHeightAt(eye) + CameraGroundClearance);
	m_spCamera->SetLookAt(eye, aimPoint, { 0, 1, 0 });
}

CHelicopterGameScene::CHelicopterGameScene()
	: CGameScene(PlayerRules{
		.maxHealth = HelicopterHealth,
		.aimHeight = 0.0f,
		.hitRadius = HelicopterHitRadius,
		.reload = HelicopterReload,
		.shotSpeed = HelicopterMissileSpeed,
		.shotLifetime = HelicopterMissileLifetime,
		.shotClearance = HelicopterMissileClearance,
		.muzzleOffset = HelicopterMissileOffset,
		.homingDelay = HelicopterHomingDelay,
		.homingTurnSpeed = HelicopterHomingTurnSpeed,
		.homing = true
	})
{
}

CHelicopterGameScene::~CHelicopterGameScene() = default;

HRESULT CHelicopterGameScene::BuildObjects(ComPtr<ID3D12Device>& cpDevice,
	ComPtr<ID3D12GraphicsCommandList>& cpCommandList, CDescHeapAllocator* allocator)
{
	const HRESULT hr = CGameScene::BuildObjects(cpDevice, cpCommandList, allocator);
	if (FAILED(hr)) return hr;
	auto upPlayer = std::make_unique<CDamageableObject>();
	m_pStageHelicopter = upPlayer.get();
	m_vObjects.push_back(std::move(upPlayer));
	auto& model = m_spStageResources->spApache;
	m_pStageHelicopter->SetModel(model);
	m_pStageHelicopter->SetScale(ApacheScale);

	const XMFLOAT4X4 mainRotor = model->GetFrameMatrix("rotor");
	const XMFLOAT4X4 mainHub = model->GetFrameMatrix("black_m_6");
	m_mainRotorAxis = Vector3::TransformNormal({ 0.0f, 1.0f, 0.0f }, mainRotor);
	// 날개 프레임 좌표로 환산한 허브 회전축의 중심.
	m_mainRotorOffset = Vector3::TransformNormal({ 0.115010f, 0.0f, -0.696087f }, mainRotor);
	const XMFLOAT3 hubToBladeOrigin{ mainRotor._41 - mainHub._41, mainRotor._42 - mainHub._42, mainRotor._43 - mainHub._43 };
	m_mainHubOffset = Vector3::Add(hubToBladeOrigin, m_mainRotorOffset);

	const XMFLOAT4X4 tailRotor = model->GetFrameMatrix("black_m_7");
	m_tailRotorAxis = Vector3::TransformNormal({ 0.0f, -1.0f, 0.0f }, tailRotor);
	// black_m_7 허브 단면의 중심. 로컬 Y 방향이 회전축이다.
	m_tailRotorOffset = Vector3::TransformNormal({ -0.758458f, 0.0f, -0.058565f }, tailRotor);
	return S_OK;
}

void CHelicopterGameScene::ReleaseObjects()
{
	m_pStageHelicopter = nullptr;
	CGameScene::ReleaseObjects();
}

void CHelicopterGameScene::ResetStage()
{
	m_fRotorTime = 0.0f;
	m_bStageFirstPerson = false;
	const bool targetStage = IsTargetStage();
	SetLockSettings(targetStage ? Level1LockMinDot : Level3LockMinDot, targetStage ? TargetLockRadius : Level3LockHitRadius);

	const auto& spawn = targetStage ? Level1Spawn : Level3Spawn;
	const float startZ = -UsableHalf * spawn.startDepth;
	m_helicopterPose.position = OnTerrain(0.0f, startZ, HelicopterMinClearance);
	m_helicopterPose.rotation = { 0.0f, 0.0f, 0.0f };
	if (targetStage) m_exitPosition = OnTerrain(0.0f, UsableHalf * ExitDepth, ExitHeight);

	UpdateHelicopterPose();
	const int count = targetStage ? Level1TargetCount : Level3TankCount;
	for (int i = 0; i < count; ++i)
	{
		const float t = static_cast<float>(i) / (std::max)(1, count - 1);
		const float angle = i * SpiralAngle + spawn.angleOffset;
		const float z = UsableHalf * (spawn.depthStart + t * spawn.depthSpan + std::cos(angle) * spawn.depthScatter);
		const float x = std::sin(angle) * UsableHalf * spawn.width * (spawn.spreadStart + t * spawn.spreadSpan);
		if (targetStage) AddTarget(OnTerrain(x, z, TargetHeight));
		else AddEnemyTank(x, z, startZ);
	}
}

void CHelicopterGameScene::OnStageKeyPressed(WPARAM key)
{
	if (GetStage() != EGameStage::HelicopterBattle) return;

	const bool wasFirstPerson = m_bStageFirstPerson;
	if (key == 'V') m_bStageFirstPerson = !m_bStageFirstPerson;
	else if (key == '1') m_bStageFirstPerson = true;
	else if (key == '3') m_bStageFirstPerson = false;
	if (wasFirstPerson != m_bStageFirstPerson)
	{
		UpdateHelicopterPose();
		UpdateStageView();
	}
}

XMFLOAT3 CHelicopterGameScene::HelicopterForward() const
{
	return Vector3::Forward(m_helicopterPose.rotation.y, m_helicopterPose.rotation.x);
}

CDamageableObject& CHelicopterGameScene::PlayerObject() const
{
	return *m_pStageHelicopter;
}

void CHelicopterGameScene::GetAimRay(XMFLOAT3& origin, XMFLOAT3& direction) const
{
	origin = m_helicopterPose.position;
	direction = HelicopterForward();
}

bool CHelicopterGameScene::HasReachedExit() const
{
	if (!IsTargetStage()) return false;
	const float x = m_helicopterPose.position.x - m_exitPosition.x;
	const float z = m_helicopterPose.position.z - m_exitPosition.z;
	return x * x + z * z <= ExitRadius * ExitRadius;
}

float CHelicopterGameScene::VictoryHoldTime() const
{
	return IsTargetStage() ? 0.0f : CGameScene::VictoryHoldTime();
}

void CHelicopterGameScene::DrawStageObjects(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	if (!IsTargetStage()) return;

	const XMFLOAT3& exitPosition = m_exitPosition;
	const XMFLOAT4 green{ .20f, 1, .35f, 1 };
	for (const float side : { -1.0f, 1.0f })
	{
		DrawStageBox(cpCommandList, { exitPosition.x, exitPosition.y + .2f, exitPosition.z + side * ExitRadius },
			{ 2.0f * ExitRadius, .35f, .35f }, green);
		DrawStageBox(cpCommandList, { exitPosition.x + side * ExitRadius, exitPosition.y + .2f, exitPosition.z },
			{ .35f, .35f, 2.0f * ExitRadius }, green);
	}
	DrawStageBox(cpCommandList, { exitPosition.x, exitPosition.y + 4.0f, exitPosition.z }, { .4f, 8.0f, .4f }, green);
}

void CHelicopterGameScene::UpdatePlayer(const UCHAR* keys, float dt)
{
	m_fRotorTime += dt;
	const int turn = Down(keys, 'D') - Down(keys, 'A');
	const int lift = Down(keys, 'W') - Down(keys, 'S');
	const int drive = Down(keys, VK_UP) - Down(keys, VK_DOWN);
	const float yaw = XMScalarModAngle(m_helicopterPose.rotation.y + turn * HelicopterYawSpeed * dt);
	const float wantedPitch = -drive * HelicopterPitchLimit;
	const float pitchStep = HelicopterPitchSpeed * dt;
	m_helicopterPose.rotation.x += std::clamp(wantedPitch - m_helicopterPose.rotation.x, -pitchStep, pitchStep);
	XMFLOAT3 position = Vector3::Add(m_helicopterPose.position, Vector3::Forward(yaw), drive * HelicopterDriveSpeed * dt);
	position.x = std::clamp(position.x, -MovementLimit, MovementLimit);
	position.z = std::clamp(position.z, -MovementLimit, MovementLimit);
	const float minimum = m_spStageResources->upTerrain->GetHeightAt(position) + HelicopterMinClearance;
	position.y = std::clamp(position.y + lift * HelicopterLiftSpeed * dt, minimum, minimum + HelicopterAltitudeRange);
	m_helicopterPose.position = position;
	m_helicopterPose.rotation.y = yaw;
	UpdateHelicopterPose();
}

void CHelicopterGameScene::UpdateHelicopterPose()
{
	m_pStageHelicopter->SetVisible(!m_bStageFirstPerson);
	m_pStageHelicopter->SetPosition(m_helicopterPose.position);
	m_pStageHelicopter->SetRotation(m_helicopterPose.rotation.x * HelicopterBodyPitchFactor, m_helicopterPose.rotation.y, 0.0f);

	const float rotorAngle = m_fRotorTime * HelicopterRotorSpeed;
	const float mainAngle = XMScalarModAngle(rotorAngle * MainRotorSpeedFactor);
	m_pStageHelicopter->SetFrameTransform("rotor", Matrix4x4::RotationAroundAxis(m_mainRotorAxis, mainAngle, m_mainRotorOffset));
	m_pStageHelicopter->SetFrameTransform("black_m_6", Matrix4x4::RotationAroundAxis(m_mainRotorAxis, mainAngle, m_mainHubOffset));

	const float tailAngle = XMScalarModAngle(rotorAngle * TailRotorSpeedFactor);
	m_pStageHelicopter->SetFrameTransform("black_m_7", Matrix4x4::RotationAroundAxis(m_tailRotorAxis, tailAngle, m_tailRotorOffset));

}

void CHelicopterGameScene::UpdateStageCamera(const XMFLOAT3&)
{
	const XMFLOAT3 position = m_helicopterPose.position;
	XMFLOAT3 eye;
	XMFLOAT3 target;
	if (m_bStageFirstPerson)
	{
		const XMFLOAT3 apacheCockpit{ 0.0f, 1.7f, 34.0f };
		eye = Vector3::TransformCoord(apacheCockpit, m_pStageHelicopter->GetTransform().getMatrix());
		target = Vector3::Add(eye, HelicopterForward());
	}
	else
	{
		const XMFLOAT3 flatForward = Vector3::Forward(m_pStageHelicopter->GetTransform().rotation.y);
		eye = Vector3::Add(position, flatForward, -HelicopterCameraDistance);
		eye.y += HelicopterCameraHeight;
		target = Vector3::Add(position, HelicopterForward(), HelicopterCameraLookAhead);
		target.y += HelicopterCameraTargetHeight;
	}
	eye.y = (std::max)(eye.y, m_spStageResources->upTerrain->GetHeightAt(eye) + CameraGroundClearance);
	m_spCamera->SetLookAt(eye, target, { 0, 1, 0 });
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CStageResources::ReleaseUploadBuffers()
{
	upTerrain->ReleaseUploadBuffer();
	spTank->ReleaseUploadBuffer();
	spApache->ReleaseUploadBuffer();
	for (auto& model : spRocks) model->ReleaseUploadBuffer();
	upBox->ReleaseUploadBuffer();
	upSkybox->ReleaseUploadBuffer();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameScene::SetHudPlane(size_t index, float x, float y, float width, float height, const XMFLOAT4& color)
{
	CUI& ui = *m_vHud[index];
	ui.SetPosition(x, y, .01f);
	ui.SetScale(width, height, 1.0f);
	ui.SetColor(color);
	ui.SetVisible(true);
}

bool CGameScene::IsPlayerAlive() const { return PlayerObject().IsAlive(); }
XMFLOAT3 CGameScene::PlayerPosition() const { return PlayerObject().GetPosition(); }

void CGameScene::KeyPressed(WPARAM key, CGameFramework& framework)
{
	if (key == VK_ESCAPE) framework.OpenStageSelect();
	else StageKeyPressed(key);
}

void CGameScene::CheckStageTransition(CGameFramework& framework)
{
	if (!StageFinished()) return;
	if (m_stage == EGameStage::HelicopterTargets) framework.EnterStage(EGameStage::TankBattle);
	else framework.OpenStageSelect();
}

void CGameScene::SpawnProjectile(const XMFLOAT3& position, const ProjectileData& data)
{
	m_vProjectiles.push_back(std::make_unique<CProjectileObject>(*m_spStageResources->upBox, position, data));
}
//-----------------------------------
