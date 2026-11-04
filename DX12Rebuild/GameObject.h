#pragma once

class CModel;

struct Transform
{
	XMFLOAT3 scale{ 1, 1, 1 };
	XMFLOAT3 rotation;
	XMFLOAT3 position;

	inline XMFLOAT4X4 getMatrix() {
		XMMATRIX objWorldXM =
			XMMatrixMultiply(
				XMMatrixMultiply(
					XMMatrixScaling(scale.x, scale.y, scale.z),
					XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z)),
				XMMatrixTranslation(position.x, position.y, position.z));

		XMFLOAT4X4 objWorld;
		XMStoreFloat4x4(&objWorld, objWorldXM);

		return objWorld;
	}
};

class CModelObject
{
public:
	CModelObject();
	virtual ~CModelObject();

	void SetModel(std::shared_ptr<CModel>& model) { m_spModel = model; }

	void SetScale(const float x, const float y, const float z);
	void SetScale(const XMFLOAT3& scale);
	void SetRotation(const float x, const float y, const float z);
	void SetRotation(const XMFLOAT3& rotation);
	void SetPosition(const float x, const float y, const float z);
	void SetPosition(const XMFLOAT3& position);

	void DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList);

	void ReleaseModelUploadBuffer();

private:
	std::shared_ptr<CModel>		m_spModel;
	Transform					m_Transform;
};