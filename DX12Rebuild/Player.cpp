#include "pch.h"
#include "Player.h"
#include "Camera.h"

using namespace config::stage;

//-----------------------------------
//[아래 코드는 AI로 구현됨]
namespace
{
	bool Down(const UCHAR* keys, int key) { return (keys[key] & 0x80) != 0; }
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
//-----------------------------------

// -------------------------------------------
//
// TankPlayer
//
// -------------------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
CTankPlayer::CTankPlayer()
{
	SetCombatInfo(PlayerTankHealth, TankAimHeight, TankHitRadius, TankHitRadius, 0.0f, TankAimHeight);
	SetTargeting(TankLockMinDot, TankHitRadius);
}
//-----------------------------------

CTankPlayer::~CTankPlayer() = default;

void CTankPlayer::ProcessInput(DWORD input, float dt, const CTerrainObject* terrain)
{
	ApplyControls(input, dt, terrain);
}

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CTankPlayer::ProcessControls(const UCHAR* keys, float dt, const CTerrainObject& terrain)
{
	const DWORD input = ReadTankControls(keys);
	m_bManualAim = (input & (CANNON_UP | CANNON_DOWN | TURRET_LEFT | TURRET_RIGHT)) != 0;
	if (input) ProcessInput(input, dt, &terrain);
	FollowStageSlope(terrain);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CTankPlayer::ResetControls()
{
	m_bAutoAttack = TankAutoAttackAtStart;
	m_bShield = m_bManualAim = false;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CTankPlayer::OnKeyPressed(WPARAM key)
{
	if (key == 'T') m_bAutoAttack = !m_bAutoAttack;
	if (key == 'H') m_bShield = !m_bShield;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
bool CTankPlayer::AimAutomatically(float dt)
{
	const CDamageableObject* target = GetLockedTarget();
	return m_bAutoAttack && !m_bManualAim && target && target->IsAlive() && AimStageCannon(target->GetAimPoint(), dt);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CTankPlayer::UpdateCamera(const CTerrainObject& terrain)
{
	XMFLOAT3 origin, direction;
	GetAimRay(origin, direction);
	XMFLOAT3 eye = Vector3::Add(GetPosition(), GetTurretHeading(), -TankCameraDistance);
	eye.y += TankCameraHeight;
	eye.y = (std::max)(eye.y, terrain.GetHeightAt(eye) + CameraGroundClearance);
	m_spCamera->SetLookAt(eye, Vector3::Add(eye, direction), { 0, 1, 0 });
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CTankPlayer::ReleaseUploadBuffer()
{
	CTankObject::ReleaseUploadBuffer();
	ReleaseCannonTestUploadBuffer();
}
//-----------------------------------

//-----------------------------------
//[직접 작성한 코드]
HRESULT CTankPlayer::CreateCannonTestMesh(const ComPtr<ID3D12Device>& cpDevice,
	const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	const XMFLOAT3 corners[] = {
		{ -0.5f, -0.5f, -0.5f }, { -0.5f, 0.5f, -0.5f },
		{ 0.5f, 0.5f, -0.5f }, { 0.5f, -0.5f, -0.5f },
		{ -0.5f, -0.5f, 0.5f }, { -0.5f, 0.5f, 0.5f },
		{ 0.5f, 0.5f, 0.5f }, { 0.5f, -0.5f, 0.5f }
	};

	MeshData<VertexTexNormal> cube;
	for (const XMFLOAT3& corner : corners)
	{
		cube.vertices.push_back({ corner, { 1, 1, 1, 1 }, { 0, 0 } });
	}

	cube.indices = {
		0, 1, 2, 0, 2, 3,
		4, 7, 6, 4, 6, 5,
		0, 4, 5, 0, 5, 1,
		3, 2, 6, 3, 6, 7,
		1, 5, 6, 1, 6, 2,
		0, 3, 7, 0, 7, 4
	};

	m_upCannonTestMesh = std::make_unique<CMesh>();
	return m_upCannonTestMesh->CreateBuffer(cpDevice, cpCommandList, cube);
}
//-----------------------------------

//-----------------------------------
//[직접 작성한 코드]
void CTankPlayer::ReleaseCannonTestUploadBuffer()
{
	if (m_upCannonTestMesh) m_upCannonTestMesh->ReleaseUploadBuffer();
}
//-----------------------------------

//-----------------------------------
//[직접 작성한 코드]
void CTankPlayer::CannonTestDraw(CFrameConstants& constants)
{
	if (!m_upCannonTestMesh || m_spModel->GetFrameIndex("maingun") == -1) return;

	XMFLOAT3 muzzle, forward;
	GetStageCannonRay(muzzle, forward);
	const XMFLOAT3 positions[] = { muzzle, Vector3::Add(muzzle, forward, 0.5f) };
	const XMFLOAT4 colors[] = { { 1, 0, 0, 1 }, { 1, 1, 0, 1 } };

	for (int i = 0; i < 2; ++i)
	{
		Transform marker;
		marker.scale = { 0.08f, 0.08f, 0.08f };
		marker.position = positions[i];

		CB_GameObjInfo objInfo;
		objInfo.xmf4x4World = Matrix4x4::Transpose(marker.getMatrix());
		objInfo.xmf4x4WorldInv = Matrix4x4::Inverse(objInfo.xmf4x4World);
		if (!constants.Bind(0, objInfo)) return;

		CB_Material data;
		data.xmf4AlbedoColor = colors[i];
		if (!CMaterial(data).BindMaterialData(constants)) return;

		m_upCannonTestMesh->DrawMesh(constants.GetCommandList());
	}
}
//-----------------------------------

// -------------------------------------------
//
// HelicopterPlayer
//
// -------------------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
CHelicopterPlayer::CHelicopterPlayer()
	: CVehicleObject(
		WeaponSettings{ 
			.reload = HelicopterReload, .speed = HelicopterMissileSpeed,
			.lifetime = HelicopterMissileLifetime, .clearance = HelicopterMissileClearance,
			.muzzleOffset = HelicopterMissileOffset, .homingDelay = HelicopterHomingDelay,
			.homingTurnSpeed = HelicopterHomingTurnSpeed, .homing = true })
{
	SetCombatInfo(HelicopterHealth, 0.0f, HelicopterHitRadius, HelicopterHitRadius, 0.0f);
	SetScale(ApacheScale);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CHelicopterPlayer::SetModel(std::shared_ptr<CModel>& model)
{
	CModelObject::SetModel(model);
	XMFLOAT4X4 mainRotor = m_spModel->GetFrameMatrix("rotor");
	XMFLOAT4X4 mainHub = m_spModel->GetFrameMatrix("black_m_6");
	m_mainRotorAxis = Vector3::TransformNormal({ 0.0f, 1.0f, 0.0f }, mainRotor);
	// 날개 프레임 좌표로 환산한 허브 회전축의 중심.
	m_mainRotorOffset = Vector3::TransformNormal({ 0.115010f, 0.0f, -0.696087f }, mainRotor);

	XMFLOAT3 hubToBladeOrigin{ mainRotor._41 - mainHub._41, mainRotor._42 - mainHub._42, mainRotor._43 - mainHub._43 };
	m_mainHubOffset = Vector3::Add(hubToBladeOrigin, m_mainRotorOffset);

	XMFLOAT4X4 tailRotor = m_spModel->GetFrameMatrix("black_m_7");
	m_tailRotorAxis = Vector3::TransformNormal({ 0.0f, -1.0f, 0.0f }, tailRotor);
	// black_m_7 허브 단면의 중심. 로컬 Y 방향이 회전축이다.
	m_tailRotorOffset = Vector3::TransformNormal({ -0.758458f, 0.0f, -0.058565f }, tailRotor);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CHelicopterPlayer::ResetControls()
{
	m_fFlightPitch = m_fRotorTime = 0.0f;
	m_bFirstPerson = false;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CHelicopterPlayer::ResetStagePose(const XMFLOAT3& position, bool targetStage)
{
	SetPosition(position);
	SetRotation(0.0f, 0.0f, 0.0f);
	m_bAllowFirstPerson = !targetStage;
	SetTargeting(targetStage ? Level1LockMinDot : Level3LockMinDot,
		targetStage ? TargetLockRadius : Level3LockHitRadius);
	UpdatePose();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CHelicopterPlayer::OnKeyPressed(WPARAM key)
{
	if (!m_bAllowFirstPerson) return;
	if (key == 'V') m_bFirstPerson = !m_bFirstPerson;
	else if (key == '1') m_bFirstPerson = true;
	else if (key == '3') m_bFirstPerson = false;
	UpdatePose();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
XMFLOAT3 CHelicopterPlayer::Forward() const
{
	return Vector3::Forward(m_Transform.rotation.y, m_fFlightPitch);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CHelicopterPlayer::GetAimRay(XMFLOAT3& origin, XMFLOAT3& direction) const
{
	origin = GetPosition();
	direction = Forward();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CHelicopterPlayer::ProcessControls(const UCHAR* keys, float dt, const CTerrainObject& terrain)
{
	m_fRotorTime += dt;
	
	int turn = Down(keys, 'D') - Down(keys, 'A');
	int lift = Down(keys, VK_UP) - Down(keys, VK_DOWN);
	int drive = Down(keys, 'W') - Down(keys, 'S');
	float yaw = XMScalarModAngle(m_Transform.rotation.y + turn * HelicopterYawSpeed * dt);
	float wantedPitch = -drive * HelicopterPitchLimit;
	float pitchStep = HelicopterPitchSpeed * dt;
	m_fFlightPitch += std::clamp(wantedPitch - m_fFlightPitch, -pitchStep, pitchStep);

	XMFLOAT3 position = Vector3::Add(m_Transform.position, Vector3::Forward(yaw), drive * HelicopterDriveSpeed * dt);
	position.x = std::clamp(position.x, -MovementLimit, MovementLimit);
	position.z = std::clamp(position.z, -MovementLimit, MovementLimit);
	float minimum = terrain.GetHeightAt(position) + HelicopterMinClearance;
	position.y = std::clamp(position.y + lift * HelicopterLiftSpeed * dt, minimum, minimum + HelicopterAltitudeRange);
	m_Transform.position = position;
	m_Transform.rotation.y = yaw;
	
	UpdatePose();
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CHelicopterPlayer::UpdatePose()
{
	SetVisible(!m_bFirstPerson);
	SetRotation(m_fFlightPitch * HelicopterBodyPitchFactor, m_Transform.rotation.y, 0.0f);

	float rotorAngle = m_fRotorTime * HelicopterRotorSpeed;
	float mainAngle = XMScalarModAngle(rotorAngle * MainRotorSpeedFactor);
	SetFrameTransform("rotor", Matrix4x4::RotationAroundAxis(m_mainRotorAxis, mainAngle, m_mainRotorOffset));
	SetFrameTransform("black_m_6", Matrix4x4::RotationAroundAxis(m_mainRotorAxis, mainAngle, m_mainHubOffset));

	float tailAngle = XMScalarModAngle(rotorAngle * TailRotorSpeedFactor);
	SetFrameTransform("black_m_7", Matrix4x4::RotationAroundAxis(m_tailRotorAxis, tailAngle, m_tailRotorOffset));
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CHelicopterPlayer::UpdateCamera(const CTerrainObject& terrain)
{
	XMFLOAT3 position = m_Transform.position;
	XMFLOAT3 eye;
	XMFLOAT3 target;
	if (m_bFirstPerson)
	{
		XMFLOAT3 apacheCockpit{ 0.0f, 1.7f, 34.0f };
		eye = Vector3::TransformCoord(apacheCockpit, GetTransform().getMatrix());
		target = Vector3::Add(eye, Forward());
	}
	else
	{
		XMFLOAT3 flatForward = Vector3::Forward(GetTransform().rotation.y);
		eye = Vector3::Add(position, flatForward, -HelicopterCameraDistance);
		eye.y += HelicopterCameraHeight;
		target = Vector3::Add(position, Forward(), HelicopterCameraLookAhead);
		target.y += HelicopterCameraTargetHeight;
	}
	eye.y = (std::max)(eye.y, terrain.GetHeightAt(eye) + CameraGroundClearance);
	m_spCamera->SetLookAt(eye, target, { 0, 1, 0 });
}
//-----------------------------------
