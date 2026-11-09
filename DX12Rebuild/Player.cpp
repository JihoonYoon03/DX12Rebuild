#include "pch.h"
#include "Player.h"
#include "Camera.h"
#include "GameObject.h"
#include "Mesh.h"

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

void CTankPlayer::SetModel(std::shared_ptr<CModel>& model)
{
	//하드코딩 된 부분은 나중에 string vector 넘겨받는 형태로 처리하게 변경 예정

	m_spModel = model;
	XMFLOAT4X4 mtxTurret = m_spModel->GetFrameMatrix("Turret");

	//포신의 기존 로컬 행렬 저장
	XMFLOAT4X4 localOrigin = m_spModel->GetFrameMatrix("maingun");

	//터렛에 파츠 연결 => 파츠의 부모 변환 포함한 기본 모델 행렬 적용하면서,
	//변환은 새 parent를 따르도록
	m_spModel->SetFrameParent("maingun", "Turret");
	//터렛 기본 설정으로 회전이 적용되어 있음. 그걸 빼줘야 함
	//파츠들의 초기 변환 유지. 터렛의 변환 행렬이 부모행렬로 곱해져 상쇄됨.
	m_spModel->SetFrameLocal("maingun", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = m_spModel->GetFrameMatrix("m2");
	m_spModel->SetFrameParent("m2", "Turret");
	m_spModel->SetFrameLocal("m2", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = m_spModel->GetFrameMatrix("comhatch");
	m_spModel->SetFrameParent("comhatch", "Turret");
	m_spModel->SetFrameLocal("comhatch", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = m_spModel->GetFrameMatrix("ChamferCyl");
	m_spModel->SetFrameParent("ChamferCyl", "Turret");
	m_spModel->SetFrameLocal("ChamferCyl", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = m_spModel->GetFrameMatrix("Tube01");
	m_spModel->SetFrameParent("Tube01", "Turret");
	m_spModel->SetFrameLocal("Tube01", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = m_spModel->GetFrameMatrix("Cylinder01");
	m_spModel->SetFrameParent("Cylinder01", "Turret");
	m_spModel->SetFrameLocal("Cylinder01", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = m_spModel->GetFrameMatrix("Cylinder00");
	m_spModel->SetFrameParent("Cylinder00", "Turret");
	m_spModel->SetFrameLocal("Cylinder00", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = m_spModel->GetFrameMatrix("Rectangle6");
	m_spModel->SetFrameParent("Rectangle6", "Turret");
	m_spModel->SetFrameLocal("Rectangle6", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = m_spModel->GetFrameMatrix("ldoor");
	m_spModel->SetFrameParent("ldoor", "Turret");
	m_spModel->SetFrameLocal("ldoor", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = m_spModel->GetFrameMatrix("rdoor");
	m_spModel->SetFrameParent("rdoor", "Turret");
	m_spModel->SetFrameLocal("rdoor", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = m_spModel->GetFrameMatrix("Rectangle2");
	m_spModel->SetFrameParent("Rectangle2", "Turret");
	m_spModel->SetFrameLocal("Rectangle2", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = m_spModel->GetFrameMatrix("Rectangle3");
	m_spModel->SetFrameParent("Rectangle3", "Turret");
	m_spModel->SetFrameLocal("Rectangle3", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = m_spModel->GetFrameMatrix("Rectangle4");
	m_spModel->SetFrameParent("Rectangle4", "Turret");
	m_spModel->SetFrameLocal("Rectangle4", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = m_spModel->GetFrameMatrix("Rectangle5");
	m_spModel->SetFrameParent("Rectangle5", "Turret");
	m_spModel->SetFrameLocal("Rectangle5", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = m_spModel->GetFrameMatrix("grille");
	m_spModel->SetFrameParent("grille", "Turret");
	m_spModel->SetFrameLocal("grille", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));

	localOrigin = m_spModel->GetFrameMatrix("tray");
	m_spModel->SetFrameParent("tray", "Turret");
	m_spModel->SetFrameLocal("tray", Matrix4x4::Multiply(localOrigin, Matrix4x4::Inverse(mtxTurret)));
}

void CTankPlayer::ProcessInput(DWORD dwInput, float deltaTime, const CTerrainObject* terrain)
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
		float terrainHeight = terrain->GetHeightAt(Vector3::Add(m_Transform.position, xmf3Shift)) + 1.0f;
		//플레이어 위치를 빼줘야 이동량만 나옴
		xmf3Shift.y = terrainHeight - m_Transform.position.y;
	}

	Move(xmf3Shift);
	m_spCamera->Move(xmf3Shift);

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
}

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

	const XMFLOAT4X4 cannonWorld = Matrix4x4::Multiply(
		m_spModel->GetFrameMatrix("maingun", m_umFrameTransform), m_Transform.getMatrix());
	const XMMATRIX world = XMLoadFloat4x4(&cannonWorld);

	// AbramsTank.txt places the muzzle ring at local Y = -1.368214.
	const XMVECTOR muzzle = XMVector3TransformCoord(XMVectorSet(0, -1.368214f, 0, 1), world);
	const XMVECTOR forward = XMVector3Normalize(
		XMVector3TransformNormal(XMVectorSet(0, -1, 0, 0), world));

	const XMVECTOR positions[] = { muzzle, XMVectorMultiplyAdd(forward, XMVectorReplicate(0.5f), muzzle) };
	const XMFLOAT4 colors[] = { { 1, 0, 0, 1 }, { 1, 1, 0, 1 } };

	for (int i = 0; i < 2; ++i)
	{
		Transform marker;
		marker.scale = { 0.08f, 0.08f, 0.08f };
		XMStoreFloat3(&marker.position, positions[i]);
		const XMFLOAT4X4 markerWorld = Matrix4x4::Transpose(marker.getMatrix());
		cpCommandList->SetGraphicsRoot32BitConstants(0, 16, &markerWorld, 0);
		cpCommandList->SetGraphicsRoot32BitConstants(2, 4, &colors[i], 0);
		m_upCannonTestMesh->DrawMesh(cpCommandList);
	}
}

void CTankPlayer::DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	if (!m_spModel) return;

	m_spModel->DrawModel(cpCommandList, m_umFrameTransform, m_Transform.getMatrix());
	CannonTestDraw(cpCommandList);
}
