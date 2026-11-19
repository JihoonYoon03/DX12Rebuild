#include "pch.h"
#include "GameObject.h"
#include "Texture.h"
#include "Shader.h"
#include "Scene.h"
#include "Material.h"

using namespace config::stage;

// -------------------------------------------
//
// ObjectBase
//
// -------------------------------------------

CObjectBase::CObjectBase()
{}

CObjectBase::~CObjectBase()
{}

//-----------------------------------
//[아래 코드는 AI로 구현됨]
bool CObjectBase::BindWorld(CFrameConstants& constants) const
{
	CB_GameObjInfo objInfo;
	objInfo.xmf4x4World = Matrix4x4::Transpose(m_Transform.getMatrix());
	objInfo.xmf4x4WorldInv = Matrix4x4::Inverse(objInfo.xmf4x4World);
	return constants.Bind(0, objInfo);
}
//-----------------------------------

void CObjectBase::SetScale(const float x, const float y, const float z)
{
	m_Transform.scale.x = x;
	m_Transform.scale.y = y;
	m_Transform.scale.z = z;
}

void CObjectBase::SetScale(const XMFLOAT3& scale)
{
	SetScale(scale.x, scale.y, scale.z);
}

void CObjectBase::SetScale(const float scale)
{
	SetScale(scale, scale, scale);
}

void CObjectBase::SetRotation(const float x, const float y, const float z)
{
	m_Transform.rotation.x = x;
	m_Transform.rotation.y = y;
	m_Transform.rotation.z = z;
}

void CObjectBase::SetRotation(const XMFLOAT3& rotation)
{
	SetRotation(rotation.x, rotation.y, rotation.z);
}

void CObjectBase::SetPosition(const float x, const float y, const float z)
{
	m_Transform.position.x = x;
	m_Transform.position.y = y;
	m_Transform.position.z = z;
}

void CObjectBase::SetPosition(const XMFLOAT3& position)
{
	SetPosition(position.x, position.y, position.z);
}

//-----------------------------------
//[아래 코드는 AI로 구현됨]
ERenderState CObjectBase::GetRenderState() const { return ERenderState::Default; }
//-----------------------------------

// -------------------------------------------
//
// ModelObject
//
// -------------------------------------------

CModelObject::CModelObject()
	: CObjectBase()
{
	m_spModel = std::make_shared<CModel>();
}

CModelObject::~CModelObject()
{}

void CModelObject::SetFrameTransform(const std::string& name, const Transform& transform)
{
	SetFrameTransform(name, transform.getMatrix());
}

void CModelObject::SetFrameTransform(const std::string& name, const XMFLOAT4X4& transform)
{
	int index = m_spModel->GetFrameIndex(name);
	if (index == -1) return;

	m_umFrameTransform[index] = transform;
}

void CModelObject::DrawObject(CFrameConstants& constants)
{
	if (m_spModel) m_spModel->DrawModel(constants, m_umFrameTransform, m_Transform.getMatrix());
}

void CModelObject::ReleaseModelUploadBuffer()
{
	m_spModel->ReleaseUploadBuffer();
}

// -------------------------------------------
//
// TerrainObject
//
// -------------------------------------------

CTerrainObject::CTerrainObject()
{
}

CTerrainObject::~CTerrainObject()
{}

void CTerrainObject::SetHeightMap(std::shared_ptr<CHeightMapImage>& pHeightMapImage)
{
	m_spHeightMapImage = std::move(pHeightMapImage);
	m_nColSlice = m_spHeightMapImage->GetHeightMapWidth();
	m_nRowSlice = m_spHeightMapImage->GetHeightMapLength();
	//나누는 수치는 나중에 머터리얼 도입 시 텍스쳐 사이즈와 연동
	m_fTiling = m_nColSlice / 10;
}

HRESULT CTerrainObject::GenerateTerrain(const GraphicsResources& graphics, bool normalMapping)
{
	if (m_spHeightMapImage->GetHeightMapPixels().empty()) return E_FAIL;

	m_upTerrainTex = std::make_unique<CTexture>();
	HRESULT hr = m_upTerrainTex->CreateTexture(
		graphics.descriptors, graphics.device, graphics.commandList, L"../Resources/grass.dds");
	if (FAILED(hr))
	{
		m_upTerrainTex.reset();
		return hr;
	}

	if (normalMapping)
	{
		m_upNormalMap = std::make_unique<CTexture>();
		hr = m_upNormalMap->CreateTexture(
			graphics.descriptors, graphics.device, graphics.commandList, L"../Resources/TerrainNormal.dds");
		if (FAILED(hr))
		{
			m_upNormalMap.reset();
			return hr;
		}
	}

	MeshData<VertexTexNormal> terrain;
	float interval = 1.0f;
	float halfInterval = interval / 2;

	//정점 정보 설정 구간
	for (int z = 0; z < m_nRowSlice; ++z)
	{
		for (int x = 0; x < m_nColSlice; ++x)
		{
			float y = 0.0f;

			if (m_spHeightMapImage)
				y = m_spHeightMapImage->GetHeight(x, z);

			terrain.vertices.push_back(
				{
					//halfInterval은 지형 중심이 원점이 되도록 하기 위함
					XMFLOAT3
					((x - ((m_nColSlice - 1) * halfInterval)) * interval,
					y,
					(z - ((m_nRowSlice - 1) * halfInterval)) * interval),

					XMFLOAT4
					(1.0f, 1.0f, 1.0f, 1.0f),

					XMFLOAT2
					(float(x) / (m_nColSlice - 1), float(z) / (m_nRowSlice - 1)),

					XMFLOAT3
					(0.0f, 1.0f, 0.0f)
				}
			);
		}
	}

	//각 칸의 인덱스 설정 구간
	//인덱스는 정점 사이 칸을 순회하므로 -1 해줘야 함
	for (int j = 0, z = 0; z < m_nRowSlice - 1; ++z)
	{
		//홀수 번째 줄
		if (z % 2 == 0)
		{
			//정점 인덱스 번호는 기존대로 col만큼 처리해줘야 함
			for (int x = 0; x < m_nColSlice; ++x)
			{
				//첫 정점 제외, 각 칸 줄의 왼쪽 아래 시작점 한 번씩 추가
				if ((x == 0) && (z > 0))
					terrain.indices.push_back(x + (z * m_nColSlice));
				//아랫줄의 정점 인덱스
				terrain.indices.push_back(x + (z * m_nColSlice));
				//한 칸 위 정점 인덱스
				terrain.indices.push_back((x + (z * m_nColSlice)) + m_nColSlice);
			}
		}
		//짝수 번째 줄
		else
		{
			for (int x = m_nColSlice - 1; x >= 0; --x)
			{
				//맨 오른쪽 정점일 때, 각 칸 줄 오른쪽 아래 정점 1번씩 추가
				if (x == m_nColSlice - 1)
					terrain.indices.push_back(x + (z * m_nColSlice));
				//나머진 위와 동일
				terrain.indices.push_back(x + (z * m_nColSlice));
				terrain.indices.push_back((x + (z * m_nColSlice)) + m_nColSlice);
			}
		}
	}

	m_upTerrain = std::make_unique<CMesh>();
	m_upTerrain->SetTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	hr = m_upTerrain->CreateBuffer<VertexTexNormal>(graphics.device, graphics.commandList, terrain);
	if (FAILED(hr))
	{
		OutputDebugString(L"CTerrainObject::GenerateTerrain() Failed to create Buffer\n");
		return hr;
	}

	return S_OK;
}

void CTerrainObject::ReleaseUploadBuffer()
{
	m_upTerrain->ReleaseUploadBuffer();
	if (m_upTerrainTex) m_upTerrainTex->ReleaseUploadBuffer();
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	if (m_upNormalMap) m_upNormalMap->ReleaseUploadBuffer();
	//-----------------------------------
}

float CTerrainObject::GetHeightAt(float x, float z) const
{
	//지형 좌표계로 변환
	XMFLOAT3 local(x, 0.0f, z);
	local = Vector3::TransformCoord(local, Matrix4x4::Inverse(m_Transform.getMatrix()));

	//지형의 인덱스로 변환
	float pixelX = local.x + (m_nColSlice - 1) * 0.5f;
	float pixelZ = local.z + (m_nRowSlice - 1) * 0.5f;

	return m_spHeightMapImage->GetHeight(pixelX, pixelZ)
		* m_Transform.scale.y + m_Transform.position.y;
}

float CTerrainObject::GetHeightAt(const XMFLOAT3& pos) const
{
	return GetHeightAt(pos.x, pos.z);
}

void CTerrainObject::DrawObject(CFrameConstants& constants)
{
	if (!BindWorld(constants)) return;

	CB_Material data;
	data.xmf2Tiling = { m_fTiling, m_fTiling };
	if (!CMaterial(data).BindMaterialData(constants)) return;

	const ComPtr<ID3D12GraphicsCommandList>& cpCommandList = constants.GetCommandList();
	if (m_upTerrainTex)
		cpCommandList->SetGraphicsRootDescriptorTable(3, m_upTerrainTex->getGPUDescHandle());

	if (m_upNormalMap)
		cpCommandList->SetGraphicsRootDescriptorTable(4, m_upNormalMap->getGPUDescHandle());

	m_upTerrain->DrawMesh(cpCommandList);
}

//-----------------------------------
//[아래 코드는 AI로 구현됨]
ERenderState CTerrainObject::GetRenderState() const
{
	return m_upNormalMap ? ERenderState::NormalMap : ERenderState::Texture;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
float CTerrainObject::GetHitFraction(const XMFLOAT3& start, const XMFLOAT3& end, float clearance) const
{
	const float distance = Vector3::Length(Vector3::Subtract(end, start));
	const int steps = (std::max)(1, static_cast<int>(std::ceil(distance / config::stage::TerrainRaySampleSpacing)));
	for (int i = 0; i <= steps; ++i)
	{
		const float t = static_cast<float>(i) / steps;
		const XMFLOAT3 point = Vector3::Lerp(start, end, t);
		if (std::abs(point.x) > config::stage::TerrainEdge || std::abs(point.z) > config::stage::TerrainEdge ||
			point.y <= GetHeightAt(point) + clearance) return t;
	}
	return 2.0f;
}
//-----------------------------------

// -------------------------------------------
//
// SkyboxObject
//
// -------------------------------------------

CSkyboxObject::CSkyboxObject() = default;

CSkyboxObject::~CSkyboxObject() = default;

HRESULT CSkyboxObject::GenerateSkyBox(const GraphicsResources& graphics)
{
	const wchar_t* filenames[] =
	{
		L"../Resources/SkyBox_Front_0.dds",
		L"../Resources/SkyBox_Back_0.dds",
		L"../Resources/SkyBox_Left_0.dds",
		L"../Resources/SkyBox_Right_0.dds",
		L"../Resources/SkyBox_Top_0.dds",
		L"../Resources/SkyBox_Bottom_0.dds"
	};
	const XMFLOAT2 uv[] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
	const XMFLOAT4 white{ 1, 1, 1, 1 };
	const float extent = config::stage::SkyboxExtent;

	m_vSkyTextures.resize(std::size(filenames));
	m_vPlane.resize(std::size(filenames));
	for (size_t i = 0; i < std::size(filenames); ++i)
	{
		m_vSkyTextures[i] = std::make_unique<CTexture>();
		HRESULT hr = m_vSkyTextures[i]->CreateTexture(graphics.descriptors, graphics.device, graphics.commandList, filenames[i]);
		if (FAILED(hr))
		{
			OutputDebugStringW(L"Skybox texture load failed: ");
			OutputDebugStringW(filenames[i]);
			OutputDebugStringW(L"\n");
			return hr;
		}

		//-----------------------------------
		//[아래 코드는 AI로 구현됨]
		//평면 데이터 생성
		MeshData<VertexTexNormal> plane;
		const float sign = i % 2 == 0 ? 1.0f : -1.0f;
		for (const XMFLOAT2& texcoord : uv)
		{
			const float u = texcoord.x == 0.0f ? -extent : extent;
			const float v = texcoord.y == 0.0f ? extent : -extent;
			const XMFLOAT3 pos[] =
			{
				{ sign * u, v, sign * extent },
				{ -sign * extent, v, sign * u },
				{ u, sign * extent, -sign * v }
			};
			plane.vertices.push_back({ pos[i / 2], white, texcoord });
		}
		plane.indices = { 0, 1, 2, 0, 2, 3 };
		//-----------------------------------

		m_vPlane[i] = std::make_unique<CMesh>();
		hr = m_vPlane[i]->CreateBuffer(graphics.device, graphics.commandList, plane);
		if (FAILED(hr)) return hr;
	}

	return S_OK;
}

void CSkyboxObject::DrawObject(CFrameConstants& constants)
{
	if (!BindWorld(constants)) return;
	if (!CMaterial().BindMaterialData(constants)) return;

	const ComPtr<ID3D12GraphicsCommandList>& cpCommandList = constants.GetCommandList();
	for (size_t i = 0; i < m_vPlane.size(); ++i)
	{
		cpCommandList->SetGraphicsRootDescriptorTable(3, m_vSkyTextures[i]->getGPUDescHandle());
		m_vPlane[i]->DrawMesh(cpCommandList);
	}
}

void CSkyboxObject::ReleaseUploadBuffer()
{
	for (const std::unique_ptr<CMesh>& plane : m_vPlane)
	{
		plane->ReleaseUploadBuffer();
	}
	for (const std::unique_ptr<CTexture>& texture : m_vSkyTextures)
	{
		texture->ReleaseUploadBuffer();
	}
}

//-----------------------------------
//[아래 코드는 AI로 구현됨]
ERenderState CSkyboxObject::GetRenderState() const { return ERenderState::Skybox; }
//-----------------------------------

// -------------------------------------------
//
// MeshObject
//
// -------------------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
CMeshObject::CMeshObject(CMesh& mesh, const XMFLOAT4& color)
	: m_pMesh(&mesh), m_color(color)
{
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CMeshObject::DrawObject(CFrameConstants& constants)
{
	if (!m_bVisible || !BindWorld(constants)) return;

	CB_Material data;
	data.xmf4AlbedoColor = m_color;
	if (!CMaterial(data).BindMaterialData(constants)) return;

	m_pMesh->DrawMesh(constants.GetCommandList());
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
ERenderState CMeshObject::GetRenderState() const { return ERenderState::Unlit; }
//-----------------------------------

// -------------------------------------------
//
// DamageableObject
//
// -------------------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CDamageableObject::SetCombatInfo(int health, float aimHeight, float hitRadius, float homingHitRadius, float markerHeight, float hitHeight)
{
	m_nHealth = m_nMaxHealth = health;
	m_fAimHeight = aimHeight;
	m_fHitHeight = hitHeight;
	m_fHitRadius = hitRadius;
	m_fHomingHitRadius = homingHitRadius;
	m_fMarkerHeight = markerHeight;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
XMFLOAT3 CDamageableObject::GetAimPoint() const
{
	XMFLOAT3 point = GetPosition();
	point.y += m_fAimHeight;
	return point;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CDamageableObject::DrawObject(CFrameConstants& constants)
{
	if (IsVisible()) CModelObject::DrawObject(constants);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
XMFLOAT3 CDamageableObject::GetHitPoint() const
{
	XMFLOAT3 point = GetPosition();
	point.y += m_fHitHeight;
	return point;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
float CDamageableObject::GetHitFraction(const XMFLOAT3& start, const XMFLOAT3& end, bool homing) const
{
	return Collision::SegmentSphereFraction(start, end, GetHitPoint(), GetHitRadius(homing));
}
//-----------------------------------

// -------------------------------------------
//
// TargetObject
//
// -------------------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
CTargetObject::CTargetObject(CMesh& mesh)
	: m_visual(mesh, { .86f, .18f, .16f, 1 })
{
	m_visual.SetScale(1.0f, 1.6f, 1.0f);
	SetCombatInfo(config::stage::TargetHealth, 0.0f, config::stage::TargetHitRadius,
		config::stage::TargetHomingHitRadius, 1.3f);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CTargetObject::DrawObject(CFrameConstants& constants)
{
	if (!IsVisible()) return;
	m_visual.SetPosition(GetPosition());
	m_visual.DrawObject(constants);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
ERenderState CTargetObject::GetRenderState() const { return ERenderState::Unlit; }
//-----------------------------------

// -------------------------------------------
//
// ProjectileObject
//
// -------------------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
CProjectileObject::CProjectileObject(CMesh& mesh, const XMFLOAT3& position, const ProjectileData& data)
	: CMeshObject(mesh, data.enemyOwned ? XMFLOAT4{ 1, .35f, .10f, 1 } : XMFLOAT4{ 1, .9f, .36f, 1 }),
	  m_flight(data)
{
	SetPosition(position);
	SetScale(.38f);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CProjectileObject::Advance(float dt)
{
	const float step = (std::min)(dt, m_flight.lifetime);
	m_flight.lifetime -= step;
	if (m_flight.target && m_flight.target->IsAlive())
	{
		m_flight.homingDelay -= step;
		if (m_flight.homingDelay <= 0.0f)
			m_flight.direction = Vector3::RotateTowards(m_flight.direction,
				Vector3::Subtract(m_flight.target->GetAimPoint(), GetPosition()), m_flight.homingTurnSpeed * step);
	}
	SetPosition(Vector3::Add(GetPosition(), m_flight.direction, m_flight.speed * step));
}
//-----------------------------------

// -------------------------------------------
//
// ObstacleObject
//
// -------------------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CObstacleObject::SetModel(std::shared_ptr<CModel>& model)
{
	CModelObject::SetModel(model);
	m_bounds = model->GetModelBounds().value_or(BoundingBox{});
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
BoundingOrientedBox CObstacleObject::GetFootprint() const
{
	return Collision::MakeFootprint(m_bounds, m_Transform);
}
//-----------------------------------

// -------------------------------------------
//
// VehicleObject
//
// -------------------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
CVehicleObject::CVehicleObject(const WeaponSettings& weapon)
	:	m_weapon(weapon) 
{
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CVehicleObject::ResetCombat()
{
	ResetHealth();
	m_weapon.Reset();
	ClearTargeting();
	m_bFireRequested = false;
	ResetControls();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CVehicleObject::KeyPressed(WPARAM key)
{
	if (key == VK_SPACE) m_bFireRequested = true;
	if (key == 'Q' && m_pLockedTarget) m_bLockPinned = !m_bLockPinned;
	OnKeyPressed(key);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CVehicleObject::SetTargeting(float minDot, float radius)
{
	m_fLockMinDot = minDot;
	m_fLockRadius = radius;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CVehicleObject::ApplyDamage()
{
	if (!IsProtected()) CDamageableObject::ApplyDamage();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
bool CVehicleObject::TryPlayerFire(float dt, XMFLOAT3& origin, ProjectileData& projectile)
{
	m_weapon.Update(dt);
	const bool automatic = AimAutomatically(dt);
	const bool requested = m_bFireRequested || automatic;
	m_bFireRequested = false;
	return requested && FireWeapon(origin, projectile);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
bool CVehicleObject::FireWeapon(XMFLOAT3& origin, ProjectileData& projectile)
{
	XMFLOAT3 direction;
	GetAimRay(origin, direction);
	origin = Vector3::Add(origin, direction, m_weapon.GetMuzzleOffset());
	return m_weapon.TryFire(direction, m_pLockedTarget, projectile);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CVehicleObject::UpdateTargeting(const std::vector<CDamageableObject*>& targets, const CTerrainObject& terrain)
{
	if (m_bLockPinned && m_pLockedTarget && m_pLockedTarget->IsAlive()) return;
	
	XMFLOAT3 origin, direction;
	GetAimRay(origin, direction);
	origin = Vector3::Add(origin, direction, m_weapon.GetMuzzleOffset());
	
	m_bLockPinned = false;
	m_pLockedTarget = nullptr;
	
	CDamageableObject* directlyHit = nullptr;
	float bestScore = -1.0f;
	
	float maxDistance = m_weapon.GetRange();
	XMFLOAT3 rayEnd = Vector3::Add(origin, direction, maxDistance);
	float nearest = (std::min)(1.0f, terrain.GetHitFraction(origin, rayEnd, LockTerrainClearance));
	
	for (size_t i = 0; i < targets.size(); ++i)
	{
		CDamageableObject& enemy = *targets[i];
		
		if (!enemy.IsAlive()) continue;
		
		XMFLOAT3 aim = enemy.GetAimPoint();
		float hit = Collision::SegmentSphereFraction(origin, rayEnd, aim, m_fLockRadius);
		
		if (hit < nearest)
		{
			directlyHit = targets[i];
			nearest = hit;
		}
		
		XMFLOAT3 delta = Vector3::Subtract(aim, origin);
		float distance = Vector3::Length(delta);
		float dot = Vector3::DotProduct(direction, Vector3::Normalize(delta));
		float score = dot - distance * LockDistancePenalty;
		
		if (distance > maxDistance || dot < m_fLockMinDot || score <= bestScore ||
			terrain.GetHitFraction(origin, aim, LockTerrainClearance) < 1.0f) continue;
		m_pLockedTarget = targets[i];
		bestScore = score;
	}
	if (directlyHit) m_pLockedTarget = directlyHit;
}
//-----------------------------------

// -------------------------------------------
//
// TankObject
//
// -------------------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
CTankObject::CTankObject() : CTankObject(WeaponSettings{ TankReload, TankShellSpeed, TankShellLifetime, TankShellClearance }) {}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
CTankObject::CTankObject(const WeaponSettings& weapon) : CVehicleObject(weapon) {}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CTankObject::GetAimRay(XMFLOAT3& origin, XMFLOAT3& direction) const
{
	GetStageCannonRay(origin, direction);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
BoundingOrientedBox CTankObject::GetFootprint(const Transform& pose) const
{
	return Collision::MakeFootprint(m_hitBounds.value_or(BoundingBox{}), pose);
}
//-----------------------------------

void CTankObject::SetModel(std::shared_ptr<CModel>& model)
{
	UsePreparedTankModel(model);
}

//-----------------------------------
//[직접 작성한 코드]
XMFLOAT3 CTankObject::ApplyControls(DWORD dwInput, float deltaTime, const CTerrainObject* terrain)
{
	float turn = 0.0f;
	if (dwInput & DIR_LEFT) turn -= 1.0f;
	if (dwInput & DIR_RIGHT) turn += 1.0f;

	float yaw = m_Transform.rotation.y += turn * m_fTurnSpeed * deltaTime;
	XMFLOAT3 forward{ sinf(yaw), 0.0f, cosf(yaw) };

	float drive = 0.0f;
	if (dwInput & DIR_FORWARD) drive += 1.0f;
	if (dwInput & DIR_BACKWARD) drive -= 1.0f;

	XMFLOAT3 xmf3Shift = XMFLOAT3(0.0f, 0.0f, 0.0f);

	float moveDistance = drive * m_fVelocity * deltaTime;
	xmf3Shift = Vector3::Add(Vector3::Multiply(moveDistance, forward), xmf3Shift);
	if (terrain)
	{
		float terrainHeight = terrain->GetHeightAt(Vector3::Add(m_Transform.position, xmf3Shift)) + config::stage::TankGroundOffset;
		//플레이어 위치를 빼줘야 이동량만 나옴
		xmf3Shift.y = terrainHeight - m_Transform.position.y;
	}

	Move(xmf3Shift);

	float turretTurn = 0.0f;
	if (dwInput & TURRET_LEFT) turretTurn -= 1.0f;
	if (dwInput & TURRET_RIGHT) turretTurn += 1.0f;

	m_fTurretYaw += turretTurn * m_fTurretTurnSpeed * deltaTime;
	Transform turret;
	turret.rotation.y = m_fTurretYaw;
	SetFrameTransform("Turret", turret);

	float cannonTurn = 0.0f;
	if (dwInput & CANNON_UP) cannonTurn -= 1.0f;
	if (dwInput & CANNON_DOWN) cannonTurn += 1.0f;

	m_fCannonPitch += cannonTurn * m_fCannonLiftSpeed * deltaTime;
	//실제 포신 사각 반영. 부호는 위가 -각이므로 반대로 해야 함
	if (m_fCannonPitch < -config::ABRAMS_CANNON_ELEVATION_RADIAN) m_fCannonPitch = -config::ABRAMS_CANNON_ELEVATION_RADIAN;
	if (m_fCannonPitch > -config::ABRAMS_CANNON_DEPRESSION_RADIAN) m_fCannonPitch = -config::ABRAMS_CANNON_DEPRESSION_RADIAN;

	turret.rotation.x = m_fCannonPitch;
	turret.rotation.y = 0.0f;
	SetFrameTransform("maingun", turret);
	return xmf3Shift;
}
//-----------------------------------

//-----------------------------------
//[직접 작성한 코드]
void CTankObject::Move(XMFLOAT3 shift)
{
	m_Transform.position = Vector3::Add(m_Transform.position, shift);
}
//-----------------------------------

//-----------------------------------
//[직접 작성한 코드]
void CTankObject::DrawObject(CFrameConstants& constants)
{
	if (IsVisible()) CModelObject::DrawObject(constants);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CTankObject::UsePreparedTankModel(const std::shared_ptr<CModel>& spModel)
{
	m_spModel = spModel;
	m_hitBounds = spModel ? spModel->GetModelBounds() : std::nullopt;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
XMFLOAT3 CTankObject::GetAimPoint() const
{
	return GetHitPoint();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
XMFLOAT3 CTankObject::GetHitPoint() const
{
	if (!m_hitBounds) return CDamageableObject::GetHitPoint();
	return Vector3::TransformCoord(m_hitBounds->Center, m_Transform.getMatrix());
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
float CTankObject::GetHitFraction(const XMFLOAT3& start, const XMFLOAT3& end, bool homing) const
{
	if (!m_hitBounds) return CDamageableObject::GetHitFraction(start, end, homing);
	const BoundingOrientedBox bounds = Collision::TransformOrientedBox(*m_hitBounds, m_Transform.getMatrix());
	return Collision::SegmentBoxFraction(start, end, bounds);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CTankObject::ResetStagePose(const XMFLOAT3& position, float yaw)
{
	SetPosition(position);
	SetRotation(0.0f, yaw, 0.0f);
	m_fTurretYaw = 0.0f;
	m_fCannonPitch = 0.0f;
	m_umFrameTransform.clear();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CTankObject::FollowStageSlope(const CTerrainObject& terrain)
{
	float halfWidth = config::stage::TankFootprintHalfWidth;
	float halfLength = config::stage::TankFootprintHalfLength;
	
	XMFLOAT3& position = m_Transform.position;
	
	float yaw = m_Transform.rotation.y;
	float sine = std::sin(yaw);
	float cosine = std::cos(yaw);
	
	XMFLOAT2 corners[] = {
		{ -halfWidth, halfLength }, { halfWidth, halfLength },
		{ -halfWidth, -halfLength }, { halfWidth, -halfLength }
	};
	
	float heights[4]{};
	
	for (size_t i = 0; i < std::size(corners); ++i)
	{
		float right = corners[i].x;
		float forward = corners[i].y;
		heights[i] = terrain.GetHeightAt(position.x + cosine * right + sine * forward,
			position.z - sine * right + cosine * forward);
	}

	float frontMinusBack = (heights[0] + heights[1] - heights[2] - heights[3]) * 0.5f;
	float rightMinusLeft = (heights[1] + heights[3] - heights[0] - heights[2]) * 0.5f;
	m_Transform.rotation.x = -std::atan2(frontMinusBack, 2.0f * halfLength);
	m_Transform.rotation.z = std::atan2(rightMinusLeft, 2.0f * halfWidth);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
XMFLOAT3 CTankObject::GetTurretHeading() const
{
	XMFLOAT4X4 turretWorld = Matrix4x4::Multiply(
		m_spModel->GetFrameMatrix("Turret", m_umFrameTransform), m_Transform.getMatrix());
	XMFLOAT3 forward = Vector3::TransformNormal({ 0.0f, -1.0f, 0.0f }, turretWorld);
	forward.y = 0.0f;
	return Vector3::Normalize(forward);
}
//-----------------------------------

void CTankObject::GetStageCannonRay(XMFLOAT3& origin, XMFLOAT3& direction) const
{
	XMFLOAT4X4 cannonWorld = Matrix4x4::Multiply(
		m_spModel->GetFrameMatrix("maingun", m_umFrameTransform), m_Transform.getMatrix());
	//-----------------------------------
	//[직접 작성한 코드]
	origin = Vector3::TransformCoord({ 0.0f, -1.368214f, 0.0f }, cannonWorld);
	direction = Vector3::Normalize(Vector3::TransformNormal({ 0.0f, -1.0f, 0.0f }, cannonWorld));
	//-----------------------------------
}

//-----------------------------------
//[아래 코드는 AI로 구현됨]
bool CTankObject::AimStageCannon(const XMFLOAT3& target, float dt)
{
	XMFLOAT3 origin, direction;
	GetStageCannonRay(origin, direction);
	
	XMFLOAT3 desired = Vector3::Normalize(Vector3::Subtract(target, origin));
	float yawDelta = XMScalarModAngle(std::atan2(desired.x, desired.z) - std::atan2(direction.x, direction.z));
	float yawStep = config::stage::TurretAimSpeed * dt;
	m_fTurretYaw = XMScalarModAngle(m_fTurretYaw + std::clamp(yawDelta, -yawStep, yawStep));
	
	Transform turret;
	turret.rotation.y = m_fTurretYaw;
	SetFrameTransform("Turret", turret);

	GetStageCannonRay(origin, direction);
	float pitchDelta = std::asin(std::clamp(desired.y, -1.0f, 1.0f)) -
		std::asin(std::clamp(direction.y, -1.0f, 1.0f));
	float pitchStep = config::stage::CannonAimSpeed * dt;

	m_fCannonPitch = std::clamp(m_fCannonPitch - std::clamp(pitchDelta, -pitchStep, pitchStep),
		-config::ABRAMS_CANNON_ELEVATION_RADIAN, -config::ABRAMS_CANNON_DEPRESSION_RADIAN);
	
	Transform cannon;
	cannon.rotation.x = m_fCannonPitch;
	SetFrameTransform("maingun", cannon);

	GetStageCannonRay(origin, direction);
	XMFLOAT3 finalTarget = Vector3::Normalize(Vector3::Subtract(target, origin));
	return Vector3::DotProduct(finalTarget, direction) >= config::stage::CannonFireMinDot;
}
//-----------------------------------

// -------------------------------------------
//
// EnemyTank
//
// -------------------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
CEnemyTank::CEnemyTank()
	: CTankObject(WeaponSettings{ .reload = EnemyTankReload, .speed = TankShellSpeed,
		.lifetime = TankShellLifetime, .clearance = TankShellClearance, .enemyOwned = true })
{
	SetCombatInfo(EnemyTankHealth, EnemyTankAimHeight, TankHitRadius, TankHitRadius,
		EnemyTankAimHeight + 3.0f, TankAimHeight);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
bool CEnemyTank::UpdateAttack(const XMFLOAT3& target, float dt, const CTerrainObject& terrain)
{
	m_weapon.Update(dt);
	if (!AimStageCannon(target, dt)) return false;
	XMFLOAT3 origin, direction;
	GetAimRay(origin, direction);
	return terrain.GetHitFraction(origin, target, m_weapon.GetClearance()) >= 1.0f;
}
//-----------------------------------

// -------------------------------------------
//
// UI
//
// -------------------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
CUI::CUI(const XMFLOAT4& bounds, const XMFLOAT4& color, const wchar_t* textureFile)
	: m_color(color), m_textureFile(textureFile ? textureFile : L"")
{
	SetPosition((bounds.x + bounds.z) * 0.5f - 0.5f, 0.5f - (bounds.y + bounds.w) * 0.5f, 0.5f);
	SetScale(bounds.z - bounds.x, bounds.w - bounds.y, 1.0f);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
HRESULT CUI::BuildMesh(const GraphicsResources& graphics)
{
	MeshData<VertexTexNormal> meshData;
	meshData.vertices =
	{
		{ { -0.5f,  0.5f, 0.0f }, { 1, 1, 1, 1 }, { 0.0f, 0.0f } },
		{ {  0.5f,  0.5f, 0.0f }, { 1, 1, 1, 1 }, { 1.0f, 0.0f } },
		{ {  0.5f, -0.5f, 0.0f }, { 1, 1, 1, 1 }, { 1.0f, 1.0f } },
		{ { -0.5f, -0.5f, 0.0f }, { 1, 1, 1, 1 }, { 0.0f, 1.0f } }
	};
	meshData.indices = { 0, 1, 2, 0, 2, 3 };
	
	HRESULT hr = m_mesh.CreateBuffer(graphics.device, graphics.commandList, meshData);
	if (FAILED(hr)) return hr;
	
	if (m_textureFile.empty()) return S_OK;

	std::shared_ptr<CTexture> texture = std::make_shared<CTexture>();
	hr = texture->CreateTexture(graphics.descriptors, graphics.device, graphics.commandList, m_textureFile.c_str(), true);
	if (FAILED(hr))
	{
		OutputDebugStringW(m_textureFile.c_str());
		OutputDebugStringW(L": UI texture load failed\n");
		return hr;
	}
	
	return BindTexture(texture);
}
//-----------------------------------

HRESULT CUI::BindTexture(std::shared_ptr<CTexture>& texture)
{
	if (!texture) return E_INVALIDARG;
	m_spUITexture = texture;
	return S_OK;
}

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CUI::ReleaseUploadBuffer()
{
	m_mesh.ReleaseUploadBuffer();
	if (m_spUITexture) m_spUITexture->ReleaseUploadBuffer();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CUI::DrawObject(CFrameConstants& constants)
{
	if (!m_bVisible || !BindWorld(constants)) return;

	CB_Material data;
	data.xmf4AlbedoColor = m_color;
	if (!CMaterial(data).BindMaterialData(constants)) return;

	const ComPtr<ID3D12GraphicsCommandList>& cpCommandList = constants.GetCommandList();
	if (m_spUITexture)
		cpCommandList->SetGraphicsRootDescriptorTable(3, m_spUITexture->getGPUDescHandle());

	m_mesh.DrawMesh(cpCommandList);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
bool CUI::HitTest(float x, float y) const
{
	XMFLOAT4X4 inverse = Matrix4x4::Inverse(m_Transform.getMatrix());
	XMFLOAT3 local = Vector3::TransformCoord({ x, y, m_Transform.position.z }, inverse);
	return m_bVisible && std::abs(local.x) <= 0.5f && std::abs(local.y) <= 0.5f;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
ERenderState CUI::GetRenderState() const
{
	return HasTexture() ? ERenderState::UI : ERenderState::Unlit;
}
//-----------------------------------

// -------------------------------------------
//
// MaskedUI
//
// -------------------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
CMaskedUI::CMaskedUI(const XMFLOAT4& bounds, const XMFLOAT4& color,
	const wchar_t* textureFile, const wchar_t* maskTextureFile)
	: CUI(bounds, color, textureFile), m_maskTextureFile(maskTextureFile ? maskTextureFile : L"")
{
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
HRESULT CMaskedUI::BuildMesh(const GraphicsResources& graphics)
{
	HRESULT hr = CUI::BuildMesh(graphics);
	if (FAILED(hr)) return hr;
	if (m_spMaskTexture) return S_OK;
	if (m_maskTextureFile.empty()) return E_INVALIDARG;

	std::shared_ptr<CTexture> mask = std::make_shared<CTexture>();
	hr = mask->CreateTexture(graphics.descriptors, graphics.device, graphics.commandList, m_maskTextureFile.c_str(), true);
	if (FAILED(hr))
	{
		OutputDebugStringW(m_maskTextureFile.c_str());
		OutputDebugStringW(L": UI mask texture load failed\n");
		return hr;
	}
	SetMaskTexture(mask);
	return S_OK;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
ERenderState CMaskedUI::GetRenderState() const
{
	return ERenderState::MaskedUI;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CMaskedUI::DrawObject(CFrameConstants& constants)
{
	const ComPtr<ID3D12GraphicsCommandList>& cpCommandList = constants.GetCommandList();
	if (!HasTexture() || !m_spMaskTexture) return;
	cpCommandList->SetGraphicsRootDescriptorTable(5, m_spMaskTexture->getGPUDescHandle());
	CUI::DrawObject(constants);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CMaskedUI::ReleaseUploadBuffer()
{
	CUI::ReleaseUploadBuffer();
	if (m_spMaskTexture) m_spMaskTexture->ReleaseUploadBuffer();
}
//-----------------------------------

// -------------------------------------------
//
// GameHud
//
// -------------------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
HRESULT CGameHud::Build(CScene& scene, int maxHealth, const GraphicsResources& graphics)
{
	std::vector<const wchar_t*> textures
	{
		L"../Resources/UI/Aim.png",
		L"../Resources/UI/AutoShoot.png",
		L"../Resources/UI/Shield.png"
	};

	for (int i = 0; i < maxHealth; ++i)
	{
		textures.push_back(L"../Resources/UI/HealthBar.png");
	}

	textures.push_back(L"../Resources/UI/Victory!.png");
	textures.push_back(L"../Resources/UI/Defeat.png");

	// 조준선, 체력, 결과 등 HUD 오브젝트 생성 및 등록
	for (const wchar_t* file : textures)
	{
		std::shared_ptr<CUI> ui = std::make_shared<CUI>(
			XMFLOAT4{ 0, 0, 1, 1 }, XMFLOAT4{ 1, 1, 1, 1 }, file);

		HRESULT hr = ui->BuildMesh(graphics);
		if (FAILED(hr)) return hr;
		
		ui->SetVisible(false);
		m_ui.push_back(ui.get());
		scene.AddObject(std::move(ui));
	}
	return S_OK;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameHud::SetPlane(size_t slot, float x, float y, float width, float height, const XMFLOAT4& color)
{
	CUI* ui = m_ui[slot];
	ui->SetPosition(x, y, .01f);
	ui->SetScale(width, height, 1.0f);
	ui->SetColor(color);
	ui->SetVisible(true);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CGameHud::Update(const CVehicleObject& player, bool victory, bool defeat, float aspectScale)
{
	XMFLOAT4 white{ 1, 1, 1, 1 };
	CDamageableObject* locked = player.GetLockedTarget();
	bool hasTarget = locked && locked->IsAlive();
	
	XMFLOAT4 lockColor = player.IsLockPinned() ? XMFLOAT4{ .18f, .96f, 1, 1 } : XMFLOAT4{ 1, .86f, .08f, 1 };
	XMFLOAT4 reticleColor = hasTarget ? lockColor : white;
	
	for (CUI* ui : m_ui)
	{
		ui->SetVisible(false);
	}

	SetPlane(Aim, 0, 0, ReticleSize * aspectScale, ReticleSize, reticleColor);
	
	for (int i = 0; i < player.GetMaxHealth(); ++i)
	{
		const XMFLOAT4 color =
			i < player.GetHealth() ? white : XMFLOAT4{ 0.25f, 0.25f, 0.25f, 1 };

		SetPlane(
			HealthBegin + i,
			-0.87f + i * 0.065f * aspectScale, 0.86f,
			0.055f * (259.0f / 260.0f) * aspectScale,
			0.055f,
			color);
	}

	if (player.IsAutoAttacking())
	{
		SetPlane(AutoAttack, 0, 0, ReticleSize * aspectScale, ReticleSize, reticleColor);
	}

	if (player.ShowsShield())
	{
		SetPlane(
			Shield,
			-0.87f, 0.77f, 0.07f * (266.0f / 268.0f) * aspectScale,
			0.07f,
			player.IsProtected() ? white : XMFLOAT4{ 0.25f, 0.25f, 0.25f, 1 });
	}

	if (victory || defeat)
	{
		const size_t slot = HealthBegin + player.GetMaxHealth() + (defeat ? 1 : 0);
		const float imageWidth = defeat ? 1027.0f : 1151.0f;
		SetPlane(slot, 0, 0.45f, 0.65f * aspectScale, 0.65f * 463.0f / imageWidth, white);
	}
}
//-----------------------------------
