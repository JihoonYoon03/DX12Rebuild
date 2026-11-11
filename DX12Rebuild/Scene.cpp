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

HRESULT CScene::BuildObjects(
	ComPtr<ID3D12Device>& cpDevice,
	ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
	CSRVDescHeapAllocator* SRVDespHeapAllocator)
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
	m_spShader->SetPipelineState(cpCommandList, ERenderState::Default);
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

struct CGameScene::StageResources
{
	std::unique_ptr<CTerrainObject> upTerrain;
	std::shared_ptr<CModel> spTank;
	std::shared_ptr<CModel> spApache;
	std::shared_ptr<CModel> spRocks[2];
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	std::unique_ptr<CSkyboxObject> upSkybox;
	//-----------------------------------
	CMesh box;
};

CGameScene::CGameScene(const PlayerRules& rules)
	: m_rules(rules)
{
}

//-----------------------------------
//[직접 작성한 코드]
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
//-----------------------------------

HRESULT CGameScene::BuildObjects(
	ComPtr<ID3D12Device>& cpDevice,
	ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
	CSRVDescHeapAllocator* SRVDespHeapAllocator)
{
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	if (!SRVDespHeapAllocator) return E_INVALIDARG;
	//-----------------------------------

	m_spStageResources = std::make_shared<StageResources>();
	auto& resources = *m_spStageResources;

	//-----------------------------------
	//[직접 작성한 코드]
	resources.upTerrain = std::make_unique<CTerrainObject>();
	auto spHeightmap = std::make_shared<CHeightMapImage>(L"../Resources/Cheongsando.raw", TerrainSamples, TerrainSamples);
	resources.upTerrain->SetHeightMap(spHeightmap);
	HRESULT hr = resources.upTerrain->GenerateTerrain(cpDevice, cpCommandList, SRVDespHeapAllocator);
	if (FAILED(hr)) return hr;
	resources.upTerrain->SetScale(TerrainHorizontalScale, TerrainVerticalScale, TerrainHorizontalScale);

	resources.spTank = std::make_shared<CModel>();
	auto spMaterial = std::make_shared<CMaterial>(m_spShader);
	if (!resources.spTank->LoadModelData(cpDevice, cpCommandList, "../Resources/AbramsTank.txt", spMaterial))
		return E_FAIL;

	resources.upSkybox = std::make_unique<CSkyboxObject>();
	hr = resources.upSkybox->GenerateSkyBox(cpDevice, cpCommandList, *SRVDespHeapAllocator);
	if (FAILED(hr)) return hr;
	//-----------------------------------

	return BuildStageVisuals(cpDevice, cpCommandList);
}

void CGameScene::ShareStageResources(const CGameScene& source)
{
	m_spStageResources = source.m_spStageResources;
	m_spShader = source.m_spShader;
	m_spTexShader = source.m_spTexShader;
}

void CGameScene::ReleaseUploadBuffers()
{
	if (!m_spStageResources) return;
	auto& resources = *m_spStageResources;
	resources.upTerrain->ReleaseUploadBuffer();
	resources.spTank->ReleaseUploadBuffer();
	resources.spApache->ReleaseUploadBuffer();
	for (auto& model : resources.spRocks) model->ReleaseUploadBuffer();
	resources.box.ReleaseUploadBuffer();
	resources.upSkybox->ReleaseUploadBuffer();
}

void CGameScene::ReleaseObjects()
{
	m_vEnemies.clear();
	m_spStageResources.reset();
	m_spTexShader.reset();
	m_spShader.reset();
}

void CGameScene::DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	//-----------------------------------
	//[직접 작성한 코드]
	m_spShader->SetPipelineState(cpCommandList, ERenderState::Default);
	XMFLOAT4X4 view = Matrix4x4::Transpose(m_spCamera->GetView());
	XMFLOAT4X4 proj = Matrix4x4::Transpose(m_spCamera->GetProj());
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &proj, 32);
	//-----------------------------------

	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	XMFLOAT4X4 skyView = m_spCamera->GetView();
	skyView._41 = skyView._42 = skyView._43 = 0.0f;
	skyView = Matrix4x4::Transpose(skyView);
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &skyView, 16);
	m_spTexShader->SetPipelineState(cpCommandList, ERenderState::Skybox);
	m_spStageResources->upSkybox->DrawObject(cpCommandList);
	//-----------------------------------

	//-----------------------------------
	//[직접 작성한 코드]
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &view, 16);

	//Terrain
	m_spTexShader->SetPipelineState(cpCommandList, ERenderState::Default);
	XMFLOAT4X4 temp = Matrix4x4::Transpose(Matrix4x4::Identity());
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &temp, 0);
	XMFLOAT4 color{ 1, 1, 1, 1 };
	cpCommandList->SetGraphicsRoot32BitConstants(2, 4, &color, 0);
	m_spStageResources->upTerrain->DrawObject(cpCommandList);

	//-----------------------------------
	DrawStage(cpCommandList);
}

HRESULT CGameScene::BuildStageVisuals(const ComPtr<ID3D12Device>& cpDevice, const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	const auto loadModel = [&](std::shared_ptr<CModel>& model, const char* path, const XMFLOAT3& offset)
	{
		model = std::make_shared<CModel>();
		auto material = std::make_shared<CMaterial>(m_spShader);
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
	auto& spApache = m_spStageResources->spApache;
	if (!loadModel(spApache, "../Resources/Apache.txt", { -2.1725f, -3.0716f, 21.0388f }) ||
		!loadModel(m_spStageResources->spRocks[0], "../Resources/Rock.txt", { -2.9746f, -0.098f, -2.0176f }) ||
		!loadModel(m_spStageResources->spRocks[1], "../Resources/Rock2.txt", { -2.9407f, -0.098f, -3.3343f })) return E_FAIL;

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
	return m_spStageResources->box.CreateBuffer(cpDevice, cpCommandList, cube);
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
	cpCommandList->SetGraphicsRoot32BitConstants(2, 4, &color, 0);
	m_spStageResources->box.DrawMesh(cpCommandList);
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
	const XMFLOAT4X4 view = m_spCamera->GetView();
	const XMFLOAT4X4 projection = m_spCamera->GetProj();
	const XMFLOAT3 aimNdc = Vector3::TransformCoord(m_reticleAimPoint, Matrix4x4::Multiply(view, projection));

	const XMFLOAT4X4 identity = Matrix4x4::Identity();
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &identity, 16);
	cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &identity, 32);

	const float aspectScale = projection._11 / projection._22;
	const XMFLOAT4 white{ .9f, .95f, 1, 1 };
	const XMFLOAT4 lockColor = m_bLockPinned ? XMFLOAT4{ .18f, .96f, 1, 1 } : XMFLOAT4{ 1, .86f, .08f, 1 };
	const auto hudBox = [&](float x, float y, float width, float height, const XMFLOAT4& color)
	{
		DrawStageBox(cpCommandList, { x, y, .01f }, { width * aspectScale, height, .002f }, color);
	};

	const XMFLOAT4 reticle = m_nLockedEnemy < 0 ? white : lockColor;
	if (aimNdc.z >= 0.0f && aimNdc.z <= 1.0f)
	{
		hudBox(aimNdc.x, aimNdc.y, .04f, .005f, reticle);
		hudBox(aimNdc.x, aimNdc.y, .005f, .04f, reticle);
	}
	for (int i = 0; i < m_rules.maxHealth; ++i)
	{
		const XMFLOAT4 color = i < m_nHealth ? XMFLOAT4{ .18f, .85f, .35f, 1 } : XMFLOAT4{ .35f, .15f, .15f, 1 };
		hudBox(-.87f + i * .065f * aspectScale, .86f, .055f, .045f, color);
	}
	DrawStageHudExtras(cpCommandList, aspectScale, lockColor);
	if (m_state == EStageState::Victory || m_state == EStageState::Defeat)
	{
		const XMFLOAT4 color = m_state == EStageState::Victory
			? XMFLOAT4{ .18f, .72f, .32f, 1 } : XMFLOAT4{ .68f, .17f, .19f, 1 };
		hudBox(0.0f, .45f, .65f, .14f, color);
	}
}

void CGameScene::DrawStage(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	using namespace config::stage;
	m_spShader->SetPipelineState(cpCommandList, ERenderState::Default);
	DrawStageObjects(cpCommandList);
	m_spShader->SetPipelineState(cpCommandList, ERenderState::Default);
	for (size_t i = 0; i < m_vEnemies.size(); ++i)
	{
		const auto& enemy = m_vEnemies[i];
		if (enemy.health <= 0) continue;
		if (enemy.upTank) enemy.upTank->DrawObject(cpCommandList);
		else DrawStageBox(cpCommandList, enemy.Position(), { 1.0f, 1.6f, 1.0f }, { .86f, .18f, .16f, 1 });

		if (static_cast<int>(i) != m_nLockedEnemy) continue;
		XMFLOAT3 marker = enemy.Position();
		marker.y += enemy.upTank ? EnemyTankAimHeight + 3.0f : 1.3f;
		const XMFLOAT4 color = m_bLockPinned ? XMFLOAT4{ .18f, .96f, 1, 1 } : XMFLOAT4{ 1, .86f, .08f, 1 };
		DrawStageBox(cpCommandList, marker, { .6f, .28f, .6f }, color);
	}
	for (const auto& shot : m_vProjectiles)
	{
		const XMFLOAT4 color = shot.enemyOwned ? XMFLOAT4{ 1, .35f, .10f, 1 } : XMFLOAT4{ 1, .9f, .36f, 1 };
		DrawStageBox(cpCommandList, shot.position, { .38f, .38f, .38f }, color);
	}
	DrawStageHud(cpCommandList);
}

XMFLOAT3 CGameScene::Enemy::Position() const
{
	return upTank ? upTank->GetTransform().position : targetPosition;
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
	m_nHealth = m_rules.maxHealth;
	m_fReload = m_fResultTimer = 0.0f;
	m_vEnemies.clear();
	m_vProjectiles.clear();

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
	XMFLOAT3 position = PlayerPosition();
	position.y += m_rules.aimHeight;
	return position;
}

XMFLOAT3 CGameScene::EnemyAimPoint(const Enemy& enemy) const
{
	XMFLOAT3 point = enemy.Position();
	if (enemy.upTank) point.y += EnemyTankAimHeight;
	return point;
}

void CGameScene::UpdateLock(const XMFLOAT3& origin, const XMFLOAT3& direction)
{
	if (m_bLockPinned && m_nLockedEnemy >= 0 && m_vEnemies[m_nLockedEnemy].health > 0) return;
	m_bLockPinned = false;
	m_nLockedEnemy = -1;
	int directlyHit = -1;
	float bestScore = -1.0f;
	const float maxDistance = m_rules.shotSpeed * m_rules.shotLifetime;
	const XMFLOAT3 rayEnd = Vector3::Add(origin, direction, maxDistance);
	float nearest = (std::min)(1.0f, TerrainHitFraction(origin, rayEnd, LockTerrainClearance));
	for (size_t i = 0; i < m_vEnemies.size(); ++i)
	{
		const auto& enemy = m_vEnemies[i];
		if (enemy.health <= 0) continue;
		const XMFLOAT3 aim = EnemyAimPoint(enemy);
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
	for (Enemy& enemy : m_vEnemies)
	{
		if (!enemy.upTank || enemy.health <= 0) continue;
		enemy.reload = (std::max)(0.0f, enemy.reload - dt);
		if (!enemy.upTank->AimStageCannon(target, dt) || enemy.reload > 0.0f) continue;

		XMFLOAT3 muzzle, direction;
		enemy.upTank->GetStageCannonRay(muzzle, direction);
		if (TerrainHitFraction(muzzle, target, TankShellClearance) < 1.0f) continue;
		m_vProjectiles.push_back({
			.position = muzzle,
			.direction = direction,
			.speed = TankShellSpeed,
			.lifetime = TankShellLifetime,
			.clearance = TankShellClearance,
			.enemyOwned = true
		});
		enemy.reload = EnemyTankReload;
	}
}

void CGameScene::FirePlayer(const XMFLOAT3& origin, const XMFLOAT3& direction)
{
	if (m_fReload > 0.0f) return;
	m_vProjectiles.push_back({
		.position = origin,
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
	for (Projectile& shot : m_vProjectiles)
	{
		const float stepTime = (std::min)(dt, shot.lifetime);
		shot.lifetime -= stepTime;
		if (shot.target >= 0 && m_vEnemies[shot.target].health > 0)
		{
			shot.homingDelay -= stepTime;
			if (shot.homingDelay <= 0.0f)
			{
				const XMFLOAT3 aim = EnemyAimPoint(m_vEnemies[shot.target]);
				const XMFLOAT3 delta = Vector3::Subtract(aim, shot.position);
				shot.direction = Vector3::RotateTowards(shot.direction, delta, shot.homingTurnSpeed * stepTime);
			}
		}
		const XMFLOAT3 end = Vector3::Add(shot.position, shot.direction, shot.speed * stepTime);
		float closest = TerrainHitFraction(shot.position, end, shot.clearance);
		int hitEnemy = -1;
		if (shot.enemyOwned)
		{
			const float hit = Collision::SegmentSphereFraction(shot.position, end, PlayerAimPoint(),
				m_rules.hitRadius);
			if (hit <= 1.0f && hit < closest)
			{
				closest = hit;
				if (!IsPlayerProtected() && m_nHealth > 0 && --m_nHealth == 0)
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
				auto& enemy = m_vEnemies[i];
				if (enemy.health <= 0) continue;
				const float radius = enemy.upTank ? TankHitRadius : (shot.target >= 0 ? TargetHomingHitRadius : TargetHitRadius);
				XMFLOAT3 center = enemy.Position();
				if (enemy.upTank) center.y += TankAimHeight;
				const float hit = Collision::SegmentSphereFraction(shot.position, end, center, radius);
				if (hit <= 1.0f && hit < closest)
				{
					closest = hit;
					hitEnemy = static_cast<int>(i);
				}
			}
		}
		if (hitEnemy >= 0) --m_vEnemies[hitEnemy].health;
		if (closest <= 1.0f) shot.lifetime = 0.0f;
		shot.position = end;
	}
	std::erase_if(m_vProjectiles, [](const Projectile& shot) { return shot.lifetime <= 0.0f; });
}

void CGameScene::Win()
{
	m_fResultTimer = VictoryHoldTime();
	m_state = m_fResultTimer > 0.0f ? EStageState::Victory : EStageState::Finished;
	m_vProjectiles.clear();
}

void CGameScene::ProcessInput(UCHAR* keys, float dt)
{
	if (!::GetKeyboardState(keys)) return;
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
		[](const Enemy& enemy) { return enemy.health > 0; });
	if (m_nHealth > 0 && (allDestroyed || HasReachedExit())) Win();
	UpdateStageView();
}

void CGameScene::AddTarget(const XMFLOAT3& position)
{
	m_vEnemies.push_back({ nullptr, position, TargetHealth });
}

void CGameScene::AddEnemyTank(float x, float z, float playerStartZ)
{
	auto enemyTank = std::make_unique<CTankPlayer>();
	enemyTank->UsePreparedTankModel(m_spStageResources->spTank);
	enemyTank->ResetStagePose(OnTerrain(x, z, TankGroundOffset), std::atan2(-x, playerStartZ - z));
	enemyTank->FollowStageSlope(*m_spStageResources->upTerrain);
	m_vEnemies.push_back({ std::move(enemyTank), {}, EnemyTankHealth });
}

void CGameScene::SetLockSettings(float minDot, float radius)
{
	m_fLockMinDot = minDot;
	m_fLockRadius = radius;
}

std::optional<XMFLOAT3> CGameScene::LockedAimPoint() const
{
	if (m_nLockedEnemy < 0) return std::nullopt;
	return EnemyAimPoint(m_vEnemies[m_nLockedEnemy]);
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
	CSRVDescHeapAllocator* SRVDespHeapAllocator)
{
	const HRESULT hr = CGameScene::BuildObjects(cpDevice, cpCommandList, SRVDespHeapAllocator);
	if (FAILED(hr)) return hr;

	//-----------------------------------
	//[직접 작성한 코드]
	m_upPlayer = std::make_unique<CTankPlayer>();
	m_upPlayer->SetModel(m_spStageResources->spTank);
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
	return m_upPlayer->CreateCannonTestMesh(cpDevice, cpCommandList);
}

void CTankGameScene::ReleaseUploadBuffers()
{
	CGameScene::ReleaseUploadBuffers();
	if (m_upPlayer) m_upPlayer->ReleaseCannonTestUploadBuffer();
}

void CTankGameScene::ReleaseObjects()
{
	m_upPlayer.reset();
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
	m_upPlayer->ResetStagePose(OnTerrain(0.0f, startZ, TankGroundOffset), 0.0f);

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
		m_vObstacles.push_back({ pose, variant, Collision::MakeFootprint(m_rockBounds[variant], pose) });
		//-----------------------------------
	}
}

void CTankGameScene::OnStageKeyPressed(WPARAM key)
{
	if (key == 'T') m_bAutoAttack = !m_bAutoAttack;
	if (key == 'H') m_bShield = !m_bShield;
}

XMFLOAT3 CTankGameScene::PlayerPosition() const
{
	return m_upPlayer->GetTransform().position;
}

void CTankGameScene::GetAimRay(XMFLOAT3& origin, XMFLOAT3& direction) const
{
	m_upPlayer->GetStageCannonRay(origin, direction);
}

bool CTankGameScene::TryAutoAim(const UCHAR* keys, float dt, XMFLOAT3& origin, XMFLOAT3& direction)
{
	constexpr DWORD manualAimMask = CANNON_UP | CANNON_DOWN | TURRET_LEFT | TURRET_RIGHT;
	if (!m_bAutoAttack || (ReadTankControls(keys) & manualAimMask) != 0) return false;
	const auto aim = LockedAimPoint();
	if (!aim) return false;

	const bool aimed = m_upPlayer->AimStageCannon(*aim, dt);
	GetAimRay(origin, direction);
	return aimed;
}

void CTankGameScene::DrawStageObjects(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	if (IsPlayerAlive()) m_upPlayer->DrawObject(cpCommandList);
	for (const auto& obstacle : m_vObstacles)
	{
		m_spStageResources->spRocks[obstacle.variant]->DrawModel(cpCommandList, obstacle.pose.getMatrix());
	}
}

void CTankGameScene::DrawStageHudExtras(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
	float aspectScale, const XMFLOAT4& lockColor)
{
	const XMFLOAT4 inactive{ .25f, .25f, .25f, 1 };
	const XMFLOAT4 active{ .9f, .95f, 1, 1 };
	const XMFLOAT3 size{ .055f * aspectScale, .025f, .002f };
	DrawStageBox(cpCommandList, { -.87f, .77f, .01f }, size, m_bAutoAttack ? active : inactive);
	DrawStageBox(cpCommandList, { -.87f + .065f * aspectScale, .77f, .01f }, size, m_bShield ? lockColor : inactive);
}

void CTankGameScene::UpdatePlayer(const UCHAR* keys, float dt)
{
	const DWORD input = ReadTankControls(keys);
	if (input)
	{
		const Transform previous = m_upPlayer->GetTransform();
		m_upPlayer->ApplyControls(input, dt, m_spStageResources->upTerrain.get());
		const Transform& candidate = m_upPlayer->GetTransform();

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
			m_upPlayer->SetPosition(previous.position);
			m_upPlayer->SetRotation(previous.rotation);
		}
	}
	m_upPlayer->FollowStageSlope(*m_spStageResources->upTerrain);
}

void CTankGameScene::UpdateStageCamera(const XMFLOAT3& aimPoint)
{
	const Transform& tank = m_upPlayer->GetTransform();
	const XMFLOAT3 forward = m_upPlayer->GetTurretHeading();
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

void CHelicopterGameScene::ShareStageResources(const CGameScene& source)
{
	CGameScene::ShareStageResources(source);
	m_upStageHelicopter = std::make_unique<CModelObject>();
	auto& model = m_spStageResources->spApache;
	m_upStageHelicopter->SetModel(model);
	m_upStageHelicopter->SetScale(ApacheScale);

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
}

void CHelicopterGameScene::ReleaseObjects()
{
	m_upStageHelicopter.reset();
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
	if (wasFirstPerson != m_bStageFirstPerson) UpdateStageView();
}

XMFLOAT3 CHelicopterGameScene::HelicopterForward() const
{
	return Vector3::Forward(m_helicopterPose.rotation.y, m_helicopterPose.rotation.x);
}

XMFLOAT3 CHelicopterGameScene::PlayerPosition() const
{
	return m_helicopterPose.position;
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
	if (IsPlayerAlive() && !m_bStageFirstPerson) m_upStageHelicopter->DrawObject(cpCommandList);
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
}

void CHelicopterGameScene::UpdateStageCamera(const XMFLOAT3&)
{
	m_upStageHelicopter->SetPosition(m_helicopterPose.position);
	m_upStageHelicopter->SetRotation(m_helicopterPose.rotation.x * HelicopterBodyPitchFactor, m_helicopterPose.rotation.y, 0.0f);

	const float rotorAngle = m_fRotorTime * HelicopterRotorSpeed;
	const float mainAngle = XMScalarModAngle(rotorAngle * MainRotorSpeedFactor);
	m_upStageHelicopter->SetFrameTransform("rotor", Matrix4x4::RotationAroundAxis(m_mainRotorAxis, mainAngle, m_mainRotorOffset));
	m_upStageHelicopter->SetFrameTransform("black_m_6", Matrix4x4::RotationAroundAxis(m_mainRotorAxis, mainAngle, m_mainHubOffset));

	const float tailAngle = XMScalarModAngle(rotorAngle * TailRotorSpeedFactor);
	m_upStageHelicopter->SetFrameTransform("black_m_7", Matrix4x4::RotationAroundAxis(m_tailRotorAxis, tailAngle, m_tailRotorOffset));

	const XMFLOAT3 position = m_helicopterPose.position;
	XMFLOAT3 eye;
	XMFLOAT3 target;
	if (m_bStageFirstPerson)
	{
		const XMFLOAT3 apacheCockpit{ 0.0f, 1.7f, 34.0f };
		eye = Vector3::TransformCoord(apacheCockpit, m_upStageHelicopter->GetTransform().getMatrix());
		target = Vector3::Add(eye, HelicopterForward());
	}
	else
	{
		const XMFLOAT3 flatForward = Vector3::Forward(m_upStageHelicopter->GetTransform().rotation.y);
		eye = Vector3::Add(position, flatForward, -HelicopterCameraDistance);
		eye.y += HelicopterCameraHeight;
		target = Vector3::Add(position, HelicopterForward(), HelicopterCameraLookAhead);
		target.y += HelicopterCameraTargetHeight;
	}
	eye.y = (std::max)(eye.y, m_spStageResources->upTerrain->GetHeightAt(eye) + CameraGroundClearance);
	m_spCamera->SetLookAt(eye, target, { 0, 1, 0 });
}
//-----------------------------------
