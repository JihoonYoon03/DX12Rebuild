#include "pch.h"
#include "Player.h"
#include "Camera.h"
#include "GameObject.h"
#include "Mesh.h"
//-----------------------------------
//[아래 코드는 AI로 구현됨]
#include <algorithm>
#include <cmath>
//-----------------------------------

CPlayer::CPlayer()
{}

CPlayer::~CPlayer()
{}

void CPlayer::SetCamera(std::shared_ptr<CCamera> spCamera)
{
	m_spCamera = spCamera;
}

void CPlayer::Update()
{
}

void CPlayer::ProcessInput(DWORD dwInput, float deltaTime)
{
	XMFLOAT3 Right	{ 1.0f, 0.0f, 0.0f };
	XMFLOAT3 Up		{ 0.0f, 1.0f, 0.0f };
	XMFLOAT3 Look	{ 0.0f, 0.0f, 1.0f };
	XMFLOAT3 xmf3Shift = XMFLOAT3(0.0f, 0.0f, 0.0f);
	if (dwInput & DIR_FORWARD) xmf3Shift = Vector3::Add(xmf3Shift, Look, 1.0f);
	if (dwInput & DIR_BACKWARD) xmf3Shift = Vector3::Add(xmf3Shift, Look, -1.0f);

	if (dwInput & DIR_RIGHT) xmf3Shift = Vector3::Add(xmf3Shift, Right, 1.0f);
	if (dwInput & DIR_LEFT) xmf3Shift = Vector3::Add(xmf3Shift, Right, -1.0f);

	if (dwInput & DIR_UP) xmf3Shift = Vector3::Add(xmf3Shift, Up, 1.0f);
	if (dwInput & DIR_DOWN) xmf3Shift = Vector3::Add(xmf3Shift, Up, -1.0f);

	//상수값은 나중에 카메라 속도로 분리
	float moveDistance = 10.0f * deltaTime;
	xmf3Shift = Vector3::Normalize(xmf3Shift);
	xmf3Shift = Vector3::Multiply(moveDistance, xmf3Shift);

	//Custom
}


//--------------------------------
// 
// CTankPlayer
// 
//--------------------------------
CTankPlayer::CTankPlayer()
{
}

CTankPlayer::~CTankPlayer()
{
}

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CTankPlayer::SetModel(std::shared_ptr<CModel>& model)
{
	PrepareModel(model);
	m_spModel = model;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CTankPlayer::PrepareModel(std::shared_ptr<CModel>& model)
{
	//-----------------------------------
	//[직접 작성한 코드]
	//하드코딩 된 부분은 나중에 string vector 넘겨받는 형태로 처리하게 변경 예정


	XMFLOAT4X4 mtxTurret = model->GetFrameMatrix("Turret");

	//포신의 기존 로컬 행렬 저장
	XMFLOAT4X4 localOrigin = model->GetFrameMatrix("maingun");

	//터렛에 파츠 연결 => 파츠의 부모 변환 포함한 기본 모델 행렬 적용하면서,
	//변환은 새 parent를 따르도록
	model->SetFrameParent("maingun", "Turret");
	//터렛 기본 설정으로 회전이 적용되어 있음. 그걸 빼줘야 함
	//파츠들의 초기 변환 유지. 터렛의 변환 행렬이 부모행렬로 곱해져 상쇄됨.
	model->SetFrameLocal("maingun", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = model->GetFrameMatrix("m2");
	model->SetFrameParent("m2", "Turret");
	model->SetFrameLocal("m2", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = model->GetFrameMatrix("comhatch");
	model->SetFrameParent("comhatch", "Turret");
	model->SetFrameLocal("comhatch", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = model->GetFrameMatrix("ChamferCyl");
	model->SetFrameParent("ChamferCyl", "Turret");
	model->SetFrameLocal("ChamferCyl", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = model->GetFrameMatrix("Tube01");
	model->SetFrameParent("Tube01", "Turret");
	model->SetFrameLocal("Tube01", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = model->GetFrameMatrix("Cylinder01");
	model->SetFrameParent("Cylinder01", "Turret");
	model->SetFrameLocal("Cylinder01", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = model->GetFrameMatrix("Cylinder00");
	model->SetFrameParent("Cylinder00", "Turret");
	model->SetFrameLocal("Cylinder00", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = model->GetFrameMatrix("Rectangle6");
	model->SetFrameParent("Rectangle6", "Turret");
	model->SetFrameLocal("Rectangle6", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = model->GetFrameMatrix("ldoor");
	model->SetFrameParent("ldoor", "Turret");
	model->SetFrameLocal("ldoor", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = model->GetFrameMatrix("rdoor");
	model->SetFrameParent("rdoor", "Turret");
	model->SetFrameLocal("rdoor", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = model->GetFrameMatrix("Rectangle2");
	model->SetFrameParent("Rectangle2", "Turret");
	model->SetFrameLocal("Rectangle2", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = model->GetFrameMatrix("Rectangle3");
	model->SetFrameParent("Rectangle3", "Turret");
	model->SetFrameLocal("Rectangle3", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = model->GetFrameMatrix("Rectangle4");
	model->SetFrameParent("Rectangle4", "Turret");
	model->SetFrameLocal("Rectangle4", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = model->GetFrameMatrix("Rectangle5");
	model->SetFrameParent("Rectangle5", "Turret");
	model->SetFrameLocal("Rectangle5", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = model->GetFrameMatrix("grille");
	model->SetFrameParent("grille", "Turret");
	model->SetFrameLocal("grille", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = model->GetFrameMatrix("tray");
	model->SetFrameParent("tray", "Turret");
	model->SetFrameLocal("tray", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));
	//-----------------------------------
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CTankPlayer::ProcessInput(DWORD dwInput, float deltaTime, const CTerrainObject* terrain)
{
	m_spCamera->Move(ApplyControls(dwInput, deltaTime, terrain));
}
//-----------------------------------


//-----------------------------------
//[직접 작성한 코드]
XMFLOAT3 CTankPlayer::ApplyControls(DWORD dwInput, float deltaTime, const CTerrainObject* terrain)
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
	xmf3Shift = Vector3::Normalize(xmf3Shift);
	xmf3Shift = Vector3::Add(Vector3::Multiply(moveDistance, forward), xmf3Shift);
	if (terrain)
	{
		//-----------------------------------
		//[아래 코드는 AI로 구현됨]
		float terrainHeight = terrain->GetHeightAt(Vector3::Add(m_Transform.position, xmf3Shift)) + config::stage::TankGroundOffset;
		//-----------------------------------
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
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	return xmf3Shift;
	//-----------------------------------
}
//-----------------------------------

void CTankPlayer::Move(XMFLOAT3 shift)
{
	m_Transform.position = Vector3::Add(m_Transform.position, shift);
}

HRESULT CTankPlayer::CreateCannonTestMesh(const ComPtr<ID3D12Device>& cpDevice,
	const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	const XMFLOAT3 corners[] = {
		{ -0.5f, -0.5f, -0.5f }, { -0.5f, 0.5f, -0.5f },
		{ 0.5f, 0.5f, -0.5f }, { 0.5f, -0.5f, -0.5f },
		{ -0.5f, -0.5f, 0.5f }, { -0.5f, 0.5f, 0.5f },
		{ 0.5f, 0.5f, 0.5f }, { 0.5f, -0.5f, 0.5f }
	};

	MeshData<VertexTexcoord> cube;
	for (const auto& corner : corners)
		cube.vertices.push_back({ corner, { 1, 1, 1, 1 }, { 0, 0 } });
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

void CTankPlayer::ReleaseCannonTestUploadBuffer()
{
	if (m_upCannonTestMesh) m_upCannonTestMesh->ReleaseUploadBuffer();
}

void CTankPlayer::CannonTestDraw(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	if (!m_upCannonTestMesh || m_spModel->GetFrameIndex("maingun") == -1) return;

	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	XMFLOAT3 muzzle, forward;
	GetStageCannonRay(muzzle, forward);
	const XMFLOAT3 positions[] = { muzzle, Vector3::Add(muzzle, forward, 0.5f) };
	//-----------------------------------
	const XMFLOAT4 colors[] = { { 1, 0, 0, 1 }, { 1, 1, 0, 1 } };

	for (int i = 0; i < 2; ++i)
	{
		Transform marker;
		marker.scale = { 0.08f, 0.08f, 0.08f };
		//-----------------------------------
		//[아래 코드는 AI로 구현됨]
		marker.position = positions[i];
		//-----------------------------------
		const XMFLOAT4X4 markerWorld = Matrix4x4::Transpose(marker.getMatrix());
		cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &markerWorld, 0);
		const XMFLOAT4X4 worldInv = Matrix4x4::Inverse(markerWorld);
		cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &worldInv, 16);
		cpCommandList->SetGraphicsRoot32BitConstants(1, 4, &colors[i], 0);
		m_upCannonTestMesh->DrawMesh(cpCommandList);
	}
}

void CTankPlayer::DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	if (!m_spModel || !IsVisible()) return;
	//-----------------------------------

	m_spModel->DrawModel(cpCommandList, m_umFrameTransform, m_Transform.getMatrix());
	//CannonTestDraw(cpCommandList);
}

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CTankPlayer::UsePreparedTankModel(const std::shared_ptr<CModel>& spModel)
{
	m_spModel = spModel;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CTankPlayer::ResetStagePose(const XMFLOAT3& position, float yaw)
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
void CTankPlayer::FollowStageSlope(const CTerrainObject& terrain)
{
	constexpr float halfWidth = config::stage::TankFootprintHalfWidth;
	constexpr float halfLength = config::stage::TankFootprintHalfLength;
	const auto& position = m_Transform.position;
	const float yaw = m_Transform.rotation.y;
	const float sine = std::sin(yaw);
	const float cosine = std::cos(yaw);
	const auto corner = [&](float right, float forward)
	{
		return terrain.GetHeightAt(position.x + cosine * right + sine * forward,
			position.z - sine * right + cosine * forward);
	};
	const float frontLeft = corner(-halfWidth, halfLength);
	const float frontRight = corner(halfWidth, halfLength);
	const float backLeft = corner(-halfWidth, -halfLength);
	const float backRight = corner(halfWidth, -halfLength);
	const float frontMinusBack = (frontLeft + frontRight - backLeft - backRight) * 0.5f;
	const float rightMinusLeft = (frontRight + backRight - frontLeft - backLeft) * 0.5f;
	m_Transform.rotation.x = -std::atan2(frontMinusBack, 2.0f * halfLength);
	m_Transform.rotation.z = std::atan2(rightMinusLeft, 2.0f * halfWidth);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
XMFLOAT3 CTankPlayer::GetTurretHeading()
{
	const XMFLOAT4X4 turretWorld = Matrix4x4::Multiply(
		m_spModel->GetFrameMatrix("Turret", m_umFrameTransform), m_Transform.getMatrix());
	XMFLOAT3 forward = Vector3::TransformNormal({ 0.0f, -1.0f, 0.0f }, turretWorld);
	forward.y = 0.0f;
	return Vector3::Normalize(forward);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CTankPlayer::GetStageCannonRay(XMFLOAT3& origin, XMFLOAT3& direction)
{
	const XMFLOAT4X4 cannonWorld = Matrix4x4::Multiply(
		m_spModel->GetFrameMatrix("maingun", m_umFrameTransform), m_Transform.getMatrix());
	//-----------------------------------
	//[직접 작성한 코드]
	origin = Vector3::TransformCoord({ 0.0f, -1.368214f, 0.0f }, cannonWorld);
	direction = Vector3::Normalize(Vector3::TransformNormal({ 0.0f, -1.0f, 0.0f }, cannonWorld));
	//-----------------------------------
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
bool CTankPlayer::AimStageCannon(const XMFLOAT3& target, float dt)
{
	XMFLOAT3 origin, direction;
	GetStageCannonRay(origin, direction);
	const XMFLOAT3 desired = Vector3::Normalize(Vector3::Subtract(target, origin));
	const float yawDelta = XMScalarModAngle(std::atan2(desired.x, desired.z) - std::atan2(direction.x, direction.z));
	const float yawStep = config::stage::TurretAimSpeed * dt;
	m_fTurretYaw = XMScalarModAngle(m_fTurretYaw + std::clamp(yawDelta, -yawStep, yawStep));
	Transform turret;
	turret.rotation.y = m_fTurretYaw;
	SetFrameTransform("Turret", turret);

	GetStageCannonRay(origin, direction);
	const float pitchDelta = std::asin(std::clamp(desired.y, -1.0f, 1.0f)) -
		std::asin(std::clamp(direction.y, -1.0f, 1.0f));
	const float pitchStep = config::stage::CannonAimSpeed * dt;
	m_fCannonPitch = std::clamp(m_fCannonPitch - std::clamp(pitchDelta, -pitchStep, pitchStep),
		-config::ABRAMS_CANNON_ELEVATION_RADIAN, -config::ABRAMS_CANNON_DEPRESSION_RADIAN);
	Transform cannon;
	cannon.rotation.x = m_fCannonPitch;
	SetFrameTransform("maingun", cannon);

	GetStageCannonRay(origin, direction);
	const XMFLOAT3 finalTarget = Vector3::Normalize(Vector3::Subtract(target, origin));
	return Vector3::DotProduct(finalTarget, direction) >= config::stage::CannonFireMinDot;
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
CEnemyTank::CEnemyTank()
{
	SetCombatInfo(config::stage::EnemyTankHealth, config::stage::EnemyTankAimHeight,
		config::stage::TankHitRadius, config::stage::TankHitRadius, config::stage::EnemyTankAimHeight + 3.0f,
		config::stage::TankAimHeight);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
bool CEnemyTank::TryFire(const XMFLOAT3& target, float dt,
	XMFLOAT3& origin, XMFLOAT3& direction)
{
	m_fReload = (std::max)(0.0f, m_fReload - dt);
	if (!AimStageCannon(target, dt) || m_fReload > 0.0f) return false;
	GetStageCannonRay(origin, direction);
	return true;
}
//-----------------------------------
