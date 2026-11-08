#pragma once

#define DIR_FORWARD		0x01
#define DIR_BACKWARD	0x02
#define DIR_LEFT		0x04
#define DIR_RIGHT		0x08
#define DIR_UP			0x10
#define DIR_DOWN		0x20

class CCamera
{
public:
	CCamera();
	~CCamera();

	void Move(const XMFLOAT3& shift);
	void SetLookAt(const XMFLOAT3& pos, const XMFLOAT3& lookAt, const XMFLOAT3& up);
	void SetPerspective(float fovAngle, float aspect, float Near, float Far);
	void SetAspectRatio(float aspect);

	XMFLOAT4X4 GetView() const { return m_xmf4x4View; };
	XMFLOAT4X4 GetProj() const { return m_xmf4x4Proj; };

private:
	//View
	XMFLOAT3	m_xmf3Pos{ 0.0f, 0.0f, -5.0f };
	XMFLOAT3	m_xmf3LookAt{ 0.0f, 0.0f, 0.0f };
	XMFLOAT3	m_xmf3Look;						//πÊ«‚∫§≈Õ
	XMFLOAT3	m_xmf3Up{ 0.0f, 1.0f, 0.0f };	//πÊ«‚∫§≈Õ
	XMFLOAT3	m_xmf3Right;					//πÊ«‚∫§≈Õ
	XMFLOAT4X4	m_xmf4x4View;
	void		SetView();

	//Projection
	float		m_fFovRadian = XMConvertToRadians(90.0f);
	float		m_fAspectRatio = (float)config::FRAME_BUFFER_W / (float)config::FRAME_BUFFER_H;
	float		m_fNear = 0.01f;
	float		m_fFar = 100.0f;
	XMFLOAT4X4	m_xmf4x4Proj;
	void		SetProj();
};

