#include "pch.h"
#include "Scene.h"
#include "GameObject.h"
#include "Player.h"
#include "Camera.h"
#include "Shader.h"
#include "Texture.h"
#include "GameFramework.h"

using namespace config::stage;

//-----------------------------------
//[아래 코드는 AI로 구현됨]
namespace
{
	class COpenStageSelectButton : public CMaskedUI
	{
	public:
		using CMaskedUI::CMaskedUI;

		bool OnClick(CGameFramework& framework) override
		{
			framework.OpenStageSelect();
			return true;
		}
	};

	class COpenTutorialButton final : public CUI
	{
	public:
		using CUI::CUI;

		bool OnClick(CGameFramework& framework) override
		{
			framework.OpenTutorial();
			return true;
		}
	};

	class CStageButton final : public CUI
	{
	public:
		CStageButton(const XMFLOAT4& bounds, const XMFLOAT4& color, EGameStage stage, const wchar_t* textureFile)
			: CUI(bounds, color, textureFile), m_stage(stage)
		{
		}

		bool OnClick(CGameFramework& framework) override
		{
			framework.EnterStage(m_stage);
			return true;
		}

	private:
		EGameStage m_stage;
	};

	class CExitButton final : public CUI
	{
	public:
		using CUI::CUI;

		bool OnClick(CGameFramework& framework) override
		{
			framework.RequestExit();
			return true;
		}
	};

	class CTutorialReturnButton final : public CUI
	{
	public:
		using CUI::CUI;

		bool OnClick(CGameFramework& framework) override
		{
			framework.OpenStageSelect();
			return true;
		}

		bool HitTest(float, float) const override { return true; }
	};
}
//-----------------------------------

// -------------------------------------------
//
// Scene
//
// -------------------------------------------

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

CObjectBase* CScene::AddObject(std::unique_ptr<CObjectBase> object)
{
	CObjectBase* result = object.get();
	m_vObjects.push_back(std::move(object));
	return result;
}

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CScene::RemoveObject(const CObjectBase* object)
{
	std::erase_if(m_vObjects, [object](const auto& owned) { return owned.get() == object; });
}
//-----------------------------------

void CScene::AddObjects(std::unique_ptr<CModelObject>* upObjects, size_t size)
{
	for (int i = 0; i < size; ++i)
	{
		AddObject(std::move(upObjects[i]));
	}
}

void CScene::AddObjects(std::vector<std::unique_ptr<CModelObject>>& upObjects)
{
	for (auto& object : upObjects) AddObject(std::move(object));
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
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	for (auto& object : m_vObjects) object->ReleaseUploadBuffer();
	//-----------------------------------
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
	m_spShader->SetSceneData(cpCommandList, sceneInfo);

	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	DrawObjects(cpCommandList, false);
	const XMFLOAT4X4 identity = Matrix4x4::Identity();
	m_spShader->SetSceneData(cpCommandList, CB_SceneInfo{ identity, identity, identity }, true);
	DrawObjects(cpCommandList, true);
	//-----------------------------------
}

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CScene::DrawObjects(const ComPtr<ID3D12GraphicsCommandList>& list, bool screenSpace)
{
	for (const auto& object : m_vObjects)
	{
		if (object->IsScreenSpace() != screenSpace) continue;
		m_spShader->SetPipelineState(list, object->GetRenderState());
		const float tiling = 1.0f;
		list->SetGraphicsRoot32BitConstants(1, 1, &tiling, 4);
		object->DrawObject(list);
	}
}
//-----------------------------------

// -------------------------------------------
//
// GameScene
//
// -------------------------------------------

CGameScene::CGameScene() = default;

CGameScene::~CGameScene()
{
	ReleaseObjects();
}

HRESULT CGameScene::BuildObjects(ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& list, CDescHeapAllocator* allocator)
{
	if (!allocator) return E_INVALIDARG;
	if (!m_spShader) return E_UNEXPECTED;
	auto skybox = std::make_unique<CSkyboxObject>();
	HRESULT hr = skybox->GenerateSkyBox(device, list, *allocator);
	if (FAILED(hr)) return hr;
	AddObject(std::move(skybox));

	//-----------------------------------
	//[직접 작성한 코드]
	auto terrain = std::make_unique<CTerrainObject>();
	auto heightmap = std::make_shared<CHeightMapImage>(L"../Resources/Cheongsando.raw", TerrainSamples, TerrainSamples);
	terrain->SetHeightMap(heightmap);
	hr = terrain->GenerateTerrain(device, list, allocator, true);
	if (FAILED(hr)) return hr;
	terrain->SetScale(TerrainHorizontalScale, TerrainVerticalScale, TerrainHorizontalScale);
	//-----------------------------------
	m_pTerrain = terrain.get();
	AddObject(std::move(terrain));
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
	m_upBoxMesh = std::make_unique<CMesh>();
	hr = m_upBoxMesh->CreateBuffer(device, list, cube);
	if (FAILED(hr)) return hr;
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	auto marker = std::make_unique<CMeshObject>(*m_upBoxMesh, XMFLOAT4{ 1, 1, 1, 1 });
	marker->SetVisible(false);
	m_pLockMarker = marker.get();
	AddObject(std::move(marker));
	//-----------------------------------
	return S_OK;
}

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameScene::SetPlayer(CVehicleObject* player)
{
	m_pPlayer = player;
	m_pPlayer->SetCamera(m_spCamera);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
HRESULT CGameScene::BuildHud(const ComPtr<ID3D12Device>& device,
	const ComPtr<ID3D12GraphicsCommandList>& list, CDescHeapAllocator& allocator)
{
	m_upHud = std::make_unique<CGameHud>();
	return m_upHud->Build(*this, m_pPlayer->GetMaxHealth(), device, list, allocator);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
std::shared_ptr<CModel> CGameScene::FindModel(const char* path) const
{
	const auto found = m_models.find(path);
	return found != m_models.end() ? found->second : nullptr;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
std::shared_ptr<CModel> CGameScene::LoadModel(const ComPtr<ID3D12Device>& device,
	const ComPtr<ID3D12GraphicsCommandList>& list, const char* path, const XMFLOAT3& offset)
{
	if (auto model = FindModel(path)) return model;
	auto model = std::make_shared<CModel>();
	auto material = std::make_shared<CMaterial>(m_spShader);
	std::string error;
	if (!model->LoadModelData(device, list, path, material, &error))
	{
		OutputDebugStringA((std::string(path) + ": " + error + "\n").c_str());
		return nullptr;
	}
	const auto& root = model->getData().front().name;
	model->SetFrameLocal(root, Matrix4x4::Multiply(model->GetFrameMatrix(root), Matrix4x4::Translation(offset)));
	m_models.emplace(path, model);
	return model;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
std::shared_ptr<CModel> CGameScene::LoadTankModel(const ComPtr<ID3D12Device>& device,
	const ComPtr<ID3D12GraphicsCommandList>& list)
{
	constexpr const char* path = "../Resources/AbramsTank.txt";
	if (auto model = FindModel(path)) return model;
	auto model = LoadModel(device, list, path);
	if (model) CTankObject::PrepareModel(model);
	return model;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameScene::ReleaseUploadBuffers()
{
	CScene::ReleaseUploadBuffers();
	for (auto& entry : m_models) entry.second->ReleaseUploadBuffer();
	if (m_upBoxMesh) m_upBoxMesh->ReleaseUploadBuffer();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameScene::ReleaseObjects()
{
	if (m_pPlayer) m_pPlayer->ResetCombat();
	m_vEnemies.clear();
	m_vAttackers.clear();
	m_vProjectiles.clear();
	m_vStageObjects.clear();
	m_upHud.reset();
	m_pPlayer = nullptr;
	m_pTerrain = nullptr;
	m_pLockMarker = nullptr;
	CScene::ReleaseObjects();
	m_models.clear();
	m_upBoxMesh.reset();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameScene::DrawScene(ComPtr<ID3D12GraphicsCommandList>& list)
{
	if (m_upHud)
	{
		const auto projection = m_spCamera->GetProj();
		m_upHud->Update(*m_pPlayer, m_state == EStageState::Victory, m_state == EStageState::Defeat,
			projection._11 / projection._22);
	}
	CScene::DrawScene(list);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
XMFLOAT3 CGameScene::OnTerrain(float x, float z, float offset) const
{
	return { x, Terrain().GetHeightAt(x, z) + offset, z };
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
CObjectBase* CGameScene::AddStageObject(std::unique_ptr<CObjectBase> object)
{
	m_vStageObjects.push_back(object.get());
	return AddObject(std::move(object));
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameScene::ClearProjectiles()
{
	for (auto* shot : m_vProjectiles) RemoveObject(shot);
	m_vProjectiles.clear();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameScene::ClearStageObjects()
{
	ClearProjectiles();
	m_vEnemies.clear();
	m_vAttackers.clear();
	for (auto* object : m_vStageObjects) RemoveObject(object);
	m_vStageObjects.clear();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameScene::BeginStage(EGameStage stage)
{
	m_stage = stage;
	m_state = EStageState::Playing;
	m_fResultTimer = 0.0f;
	m_pPlayer->ResetCombat();
	std::fill(std::begin(m_keys), std::end(m_keys), UCHAR{});
	ClearStageObjects();
	m_pLockMarker->SetVisible(false);
	ResetStage();
	const auto projection = m_spCamera->GetProj();
	m_spCamera->SetPerspective(CameraFovDegrees, projection._22 / projection._11, CameraNear, CameraFar);
	m_pPlayer->UpdateCamera(Terrain());
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameScene::ProcessInput(UCHAR* keys, float)
{
	std::copy_n(keys, 256, m_keys);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameScene::KeyPressed(WPARAM key, CGameFramework& framework)
{
	if (key == VK_ESCAPE) { framework.OpenStageSelect(); return; }
	if (m_state != EStageState::Playing) return;
	if (key == 'P') { Win(); return; }
	m_pPlayer->KeyPressed(key);
	m_pPlayer->UpdateCamera(Terrain());
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameScene::CheckStageTransition(CGameFramework& framework)
{
	if (!StageFinished()) return;
	if (m_stage == EGameStage::HelicopterTargets) framework.EnterStage(EGameStage::TankBattle);
	else framework.OpenStageSelect();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameScene::Update(float dt)
{
	dt = std::clamp(dt, 0.0f, MaxTimeStep);
	if (m_state == EStageState::Finished) return;
	if (m_state != EStageState::Playing)
	{
		m_fResultTimer -= dt;
		if (m_fResultTimer <= 0.0f)
		{
			if (m_state == EStageState::Victory) m_state = EStageState::Finished;
			else BeginStage(m_stage);
		}
		return;
	}

	const Transform previous = m_pPlayer->GetTransform();
	m_pPlayer->ProcessControls(m_keys, dt, Terrain());
	ResolvePlayerMovement(previous);
	m_pPlayer->UpdateTargeting(m_vEnemies, Terrain());
	XMFLOAT3 origin;
	ProjectileData projectile;
	if (m_pPlayer->TryPlayerFire(dt, origin, projectile)) SpawnProjectile(origin, projectile);
	UpdateEnemies(dt);
	UpdateProjectiles(dt);
	UpdateLockMarker();

	const bool allDestroyed = std::none_of(m_vEnemies.begin(), m_vEnemies.end(),
		[](const CDamageableObject* enemy) { return enemy->IsAlive(); });
	if (m_pPlayer->IsAlive() && (allDestroyed || HasReachedExit())) Win();
	m_pPlayer->UpdateCamera(Terrain());
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameScene::UpdateEnemies(float dt)
{
	const XMFLOAT3 target = m_pPlayer->GetAimPoint();
	for (auto* enemy : m_vAttackers)
	{
		if (!enemy->IsAlive() || !enemy->UpdateAttack(target, dt, Terrain())) continue;
		XMFLOAT3 origin;
		ProjectileData projectile;
		if (enemy->FireWeapon(origin, projectile)) SpawnProjectile(origin, projectile);
	}
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameScene::SpawnProjectile(const XMFLOAT3& position, const ProjectileData& data)
{
	auto shot = std::make_unique<CProjectileObject>(*m_upBoxMesh, position, data);
	m_vProjectiles.push_back(shot.get());
	AddObject(std::move(shot));
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameScene::UpdateProjectiles(float dt)
{
	for (auto* shot : m_vProjectiles)
	{
		const XMFLOAT3 start = shot->GetPosition();
		shot->Advance(dt);
		const XMFLOAT3 end = shot->GetPosition();
		float closest = Terrain().GetHitFraction(start, end, shot->GetClearance());
		CDamageableObject* hitObject = nullptr;
		if (shot->IsEnemyOwned())
		{
			const float hit = m_pPlayer->GetHitFraction(start, end, false);
			if (hit <= 1.0f && hit < closest) { closest = hit; hitObject = m_pPlayer; }
		}
		else
		{
			for (auto* enemy : m_vEnemies)
			{
				if (!enemy->IsAlive()) continue;
				const float hit = enemy->GetHitFraction(start, end, shot->IsHoming());
				if (hit <= 1.0f && hit < closest) { closest = hit; hitObject = enemy; }
			}
		}
		if (hitObject) hitObject->ApplyDamage();
		if (closest <= 1.0f) shot->Expire();
	}
	std::erase_if(m_vProjectiles, [this](const CProjectileObject* shot)
	{
		if (!shot->IsExpired()) return false;
		RemoveObject(shot);
		return true;
	});
	if (!m_pPlayer->IsAlive())
	{
		m_state = EStageState::Defeat;
		m_fResultTimer = DefeatDelay;
		m_pLockMarker->SetVisible(false);
		m_pPlayer->ClearTargeting();
	}
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameScene::UpdateLockMarker()
{
	const auto* target = m_pPlayer->GetLockedTarget();
	m_pLockMarker->SetVisible(target && target->IsAlive() && m_state == EStageState::Playing);
	if (!target) return;
	XMFLOAT3 position = target->GetPosition();
	position.y += target->GetMarkerHeight();
	m_pLockMarker->SetPosition(position);
	m_pLockMarker->SetScale(.6f, .28f, .6f);
	m_pLockMarker->SetColor(m_pPlayer->IsLockPinned() ? XMFLOAT4{ .18f, .96f, 1, 1 } : XMFLOAT4{ 1, .86f, .08f, 1 });
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameScene::Win()
{
	m_fResultTimer = VictoryHoldTime();
	m_state = m_fResultTimer > 0.0f ? EStageState::Victory : EStageState::Finished;
	ClearProjectiles();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameScene::AddTarget(const XMFLOAT3& position)
{
	auto target = std::make_unique<CTargetObject>(*m_upBoxMesh);
	target->SetPosition(position);
	m_vEnemies.push_back(target.get());
	AddStageObject(std::move(target));
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameScene::AddEnemyTank(float x, float z, float playerStartZ)
{
	auto enemy = std::make_unique<CEnemyTank>();
	enemy->UsePreparedTankModel(FindModel("../Resources/AbramsTank.txt"));
	enemy->ResetStagePose(OnTerrain(x, z, TankGroundOffset), std::atan2(-x, playerStartZ - z));
	enemy->FollowStageSlope(Terrain());
	m_vEnemies.push_back(enemy.get());
	m_vAttackers.push_back(enemy.get());
	AddStageObject(std::move(enemy));
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
CMeshObject* CGameScene::AddStageBox(const XMFLOAT3& position, const XMFLOAT3& size, const XMFLOAT4& color)
{
	auto box = std::make_unique<CMeshObject>(*m_upBoxMesh, color);
	box->SetPosition(position);
	box->SetScale(size);
	CMeshObject* result = box.get();
	AddStageObject(std::move(box));
	return result;
}
//-----------------------------------

// -------------------------------------------
//
// TankGameScene
//
// -------------------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
CTankGameScene::~CTankGameScene() = default;
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
HRESULT CTankGameScene::BuildObjects(ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& list, CDescHeapAllocator* allocator)
{
	HRESULT hr = CGameScene::BuildObjects(device, list, allocator);
	if (FAILED(hr)) return hr;
	auto tank = LoadTankModel(device, list);
	if (!tank || !tank->GetModelBounds()) return E_FAIL;
	const struct { const char* path; XMFLOAT3 offset; } rocks[] =
	{
		{ "../Resources/Rock.txt", { -2.9746f, -0.098f, -2.0176f } },
		{ "../Resources/Rock2.txt", { -2.9407f, -0.098f, -3.3343f } }
	};
	for (const auto& rock : rocks)
	{
		auto model = LoadModel(device, list, rock.path, rock.offset);
		if (!model || !model->GetModelBounds()) return E_FAIL;
	}
	auto player = std::make_unique<CTankPlayer>();
	player->UsePreparedTankModel(tank);
	m_pPlayer = player.get();
	SetPlayer(m_pPlayer);
	AddObject(std::move(player));
	return BuildHud(device, list, *allocator);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CTankGameScene::ReleaseObjects()
{
	m_vObstacles.clear();
	m_pPlayer = nullptr;
	CGameScene::ReleaseObjects();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CTankGameScene::ResetStage()
{
	m_vObstacles.clear();

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
		const char* path = i % 2 == 0 ? "../Resources/Rock.txt" : "../Resources/Rock2.txt";
		auto model = FindModel(path);
		auto object = std::make_unique<CObstacleObject>();
		object->SetModel(model);
		object->SetPosition(pose.position);
		object->SetRotation(pose.rotation);
		object->SetScale(pose.scale);
		m_vObstacles.push_back(object.get());
		AddStageObject(std::move(object));
		//-----------------------------------
	}
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CTankGameScene::ResolvePlayerMovement(const Transform& previous)
{
	const Transform& candidate = m_pPlayer->GetTransform();
	const auto previousBounds = m_pPlayer->GetFootprint(previous);
	const auto candidateBounds = m_pPlayer->GetFootprint(candidate);
	bool collision = std::abs(candidate.position.x) > MovementLimit || std::abs(candidate.position.z) > MovementLimit;
	for (const auto* obstacle : m_vObstacles)
	{
		if (collision) break;
		const auto footprint = obstacle->GetFootprint();
		if (!candidateBounds.Intersects(footprint)) continue;
		const float oldDistance = Vector3::LengthSquared(Vector3::Subtract(previousBounds.Center, footprint.Center));
		const float newDistance = Vector3::LengthSquared(Vector3::Subtract(candidateBounds.Center, footprint.Center));
		collision = !previousBounds.Intersects(footprint) || newDistance < oldDistance;
	}
	if (collision)
	{
		m_pPlayer->SetPosition(previous.position);
		m_pPlayer->SetRotation(previous.rotation);
	}
}
//-----------------------------------

// -------------------------------------------
//
// HelicopterGameScene
//
// -------------------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
CHelicopterGameScene::~CHelicopterGameScene() = default;
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
HRESULT CHelicopterGameScene::BuildObjects(ComPtr<ID3D12Device>& device,
	ComPtr<ID3D12GraphicsCommandList>& list, CDescHeapAllocator* allocator)
{
	HRESULT hr = CGameScene::BuildObjects(device, list, allocator);
	if (FAILED(hr)) return hr;
	if (!LoadTankModel(device, list)) return E_FAIL;
	auto model = LoadModel(device, list, "../Resources/Apache.txt", { -2.1725f, -3.0716f, 21.0388f });
	if (!model) return E_FAIL;
	auto player = std::make_unique<CHelicopterPlayer>();
	player->SetModel(model);
	m_pPlayer = player.get();
	SetPlayer(m_pPlayer);
	AddObject(std::move(player));
	return BuildHud(device, list, *allocator);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CHelicopterGameScene::ReleaseObjects()
{
	m_pPlayer = nullptr;
	m_pExitMarker = nullptr;
	CGameScene::ReleaseObjects();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CHelicopterGameScene::ResetStage()
{
	m_pExitMarker = nullptr;
	const bool targetStage = IsTargetStage();
	const auto& spawn = targetStage ? Level1Spawn : Level3Spawn;
	const float startZ = -UsableHalf * spawn.startDepth;
	m_pPlayer->ResetStagePose(OnTerrain(0.0f, startZ, HelicopterMinClearance), targetStage);
	if (targetStage)
	{
		const XMFLOAT3 position = OnTerrain(0.0f, UsableHalf * ExitDepth, ExitHeight);
		const XMFLOAT4 green{ .20f, 1, .35f, 1 };
		for (const float side : { -1.0f, 1.0f })
		{
			AddStageBox({ position.x, position.y + .2f, position.z + side * ExitRadius },
				{ 2.0f * ExitRadius, .35f, .35f }, green);
			AddStageBox({ position.x + side * ExitRadius, position.y + .2f, position.z },
				{ .35f, .35f, 2.0f * ExitRadius }, green);
		}
		m_pExitMarker = AddStageBox({ position.x, position.y + 4.0f, position.z }, { .4f, 8.0f, .4f }, green);
	}
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
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
bool CHelicopterGameScene::HasReachedExit() const
{
	if (!m_pExitMarker) return false;
	const auto position = m_pPlayer->GetPosition();
	const auto exit = m_pExitMarker->GetPosition();
	const float x = position.x - exit.x;
	const float z = position.z - exit.z;
	return x * x + z * z <= ExitRadius * ExitRadius;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
float CHelicopterGameScene::VictoryHoldTime() const
{
	return IsTargetStage() ? 0.0f : CGameScene::VictoryHoldTime();
}
//-----------------------------------

// -------------------------------------------
//
// UiScene
//
// -------------------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
CUiScene::CUiScene(const std::shared_ptr<CShader>& spShader)
{
	m_spShader = spShader;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
HRESULT CUiScene::BuildObjects(
	ComPtr<ID3D12Device>& cpDevice,
	ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
	CDescHeapAllocator* DescHeapAllocator)
{
	ReleaseObjects();
	BuildUi();
	for (const auto& upUi : m_vUiObjects)
	{
		const HRESULT hr = upUi->BuildMesh(cpDevice, cpCommandList, DescHeapAllocator);
		if (FAILED(hr)) return hr;
	}
	return S_OK;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CUiScene::ReleaseObjects()
{
	m_vUiObjects.clear();
	CScene::ReleaseObjects();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CUiScene::ResizeUi(int width, int height)
{
	if (width <= 0 || height <= 0) return;

	m_fCanvasPixels = static_cast<float>(width < height ? width : height);
	m_fViewWidth = static_cast<float>(width) / m_fCanvasPixels;
	m_fViewHeight = static_cast<float>(height) / m_fCanvasPixels;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CUiScene::Click(int x, int y, CGameFramework& framework)
{
	const float uiX = static_cast<float>(x) / m_fCanvasPixels - m_fViewWidth * 0.5f;
	const float uiY = m_fViewHeight * 0.5f - static_cast<float>(y) / m_fCanvasPixels;
	for (const auto& upUi : m_vUiObjects)
	{
		if (upUi->HitTest(uiX, uiY) && upUi->OnClick(framework)) return;
	}
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CUiScene::DrawScene(ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	//---------------------------------------
	//[직접 작성한 코드]
	m_spShader->SetGraphicsRootSignature(cpCommandList);
	//---------------------------------------
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	m_spShader->SetPipelineState(cpCommandList, ERenderState::UI);
	//-----------------------------------
	const XMFLOAT4X4 view = Matrix4x4::Identity();
	const XMFLOAT4X4 projection = Matrix4x4::Transpose(Matrix4x4::OrthographicLH(m_fViewWidth, m_fViewHeight, 0.0f, 1.0f));
	const XMFLOAT4 color{ 1.0f, 1.0f, 1.0f, 1.0f };

	//---------------------------------------
	//[직접 작성한 코드]
	//실제 시간값이 아직 아님에 주의
	m_fTime += 0.001f;
	//---------------------------------------
	const CB_SceneInfo sceneInfo{ view, projection, view, m_fTime };
	m_spShader->SetSceneData(cpCommandList, sceneInfo);
	const float tiling = 1.0f;
	cpCommandList->SetGraphicsRoot32BitConstants(1, 1, &tiling, 4);
	cpCommandList->SetGraphicsRoot32BitConstants(1, 4, &color, 0);
	DrawObjects(cpCommandList, true);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CUiScene::AddUi(std::unique_ptr<CUI> upUi)
{
	m_vUiObjects.push_back(upUi.get());
	AddObject(std::move(upUi));
}
//-----------------------------------

// -------------------------------------------
//
// MainMenuScene
//
// -------------------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CMainMenuScene::BuildUi()
{
	const XMFLOAT4 white{ 1, 1, 1, 1 };
	const float titleHeight = .60f * 513.0f / 3300.0f;
	const float playHeight = .24f * 273.0f / 660.0f;
	const XMFLOAT4 title{ .20f, .36f - titleHeight * .5f, .80f, .36f + titleHeight * .5f };
	const XMFLOAT4 play{ .38f, .54f - playHeight * .5f, .62f, .54f + playHeight * .5f };
	AddUi(std::make_unique<CUI>(title, white, L"../Resources/UI/Title.png"));
	AddUi(std::make_unique<COpenStageSelectButton>(play, white,
		L"../Resources/UI/PlayTex.png", L"../Resources/UI/PlayMask2.png"));
}
//-----------------------------------

// -------------------------------------------
//
// StageSelectScene
//
// -------------------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CStageSelectScene::BuildUi()
{
	const XMFLOAT4 white{ 1, 1, 1, 1 };
	const float headingHeight = .30f * 463.0f / 1908.0f;
	AddUi(std::make_unique<CUI>(XMFLOAT4{ .35f, .235f - headingHeight * .5f, .65f, .235f + headingHeight * .5f },
		white, L"../Resources/UI/SelectStage.png"));

	constexpr float firstCenter = .33188919f;
	constexpr float spacing = .076414005f;
	// PNG 원본 크기에 공통 배율을 적용해 글자와 이미지의 비율을 보존한다.
	const XMFLOAT2 imageSizes[]
	{
		{ 1827, 463 }, // Howtoplay
		{ 1827, 276 }, // TargetPractice
		{ 1827, 463 }, // TankBattle
		{ 2194, 463 }, // HelicopterBattle
		{ 1549, 463 }, // StartGame
		{ 690, 463 }   // Exit
	};
	constexpr float imageScale = .07f / 463.0f;
	XMFLOAT4 buttons[6];
	for (int i = 0; i < 6; ++i)
	{
		const float y = firstCenter + i * spacing;
		const float halfWidth = imageSizes[i].x * imageScale * .5f;
		const float halfHeight = imageSizes[i].y * imageScale * .5f;
		buttons[i] = { .5f - halfWidth, y - halfHeight, .5f + halfWidth, y + halfHeight };
	}
	AddUi(std::make_unique<COpenTutorialButton>(buttons[0], white, L"../Resources/UI/Howtoplay.png"));
	AddUi(std::make_unique<CStageButton>(buttons[1], white, EGameStage::HelicopterTargets, L"../Resources/UI/TargetPractice.png"));
	AddUi(std::make_unique<CStageButton>(buttons[2], white, EGameStage::TankBattle, L"../Resources/UI/TankBattle.png"));
	AddUi(std::make_unique<CStageButton>(buttons[3], white, EGameStage::HelicopterBattle, L"../Resources/UI/HelicopterBattle.png"));
	AddUi(std::make_unique<CStageButton>(buttons[4], white, EGameStage::HelicopterTargets, L"../Resources/UI/StartGame.png"));
	AddUi(std::make_unique<CExitButton>(buttons[5], white, L"../Resources/UI/Exit.png"));
}
//-----------------------------------

// -------------------------------------------
//
// PortingTutorialScene
//
// -------------------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CPortingTutorialScene::BuildUi()
{
	const XMFLOAT4 white{ 1, 1, 1, 1 };
	const float cardHeight = .82f;
	const float cardWidth = cardHeight * 939.0f / 1814.0f;
	const float returnWidth = .70f;
	const float returnHeight = returnWidth * 174.0f / 1404.0f;
	const XMFLOAT4 card{ .5f - cardWidth * .5f, .06f, .5f + cardWidth * .5f, .06f + cardHeight };
	const XMFLOAT4 hint{ .5f - returnWidth * .5f, .94f - returnHeight * .5f,
		.5f + returnWidth * .5f, .94f + returnHeight * .5f };
	AddUi(std::make_unique<CUI>(card, white, L"../Resources/UI/Tutorial.png"));
	AddUi(std::make_unique<CTutorialReturnButton>(hint, white, L"../Resources/UI/TutorialReturn.png"));
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CPortingTutorialScene::KeyPressed(WPARAM key, CGameFramework& framework)
{
	if (key == VK_ESCAPE) framework.OpenStageSelect();
}
//-----------------------------------
