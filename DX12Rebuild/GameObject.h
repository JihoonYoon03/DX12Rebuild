#pragma once

class CModel;
class CMesh;
class CHeightMapImage;

struct Transform
{
	XMFLOAT3 scale		{ 1, 1, 1 };
	XMFLOAT3 rotation	{ 0, 0, 0 };
	XMFLOAT3 position	{ 0, 0, 0 };

	inline XMFLOAT4X4 getMatrix() const {
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

// -------------------------------------------
// 
// ObjectBase
// 
// -------------------------------------------
class CObjectBase
{
public:
	CObjectBase();
	virtual ~CObjectBase();

	void SetScale(const float x, const float y, const float z);
	void SetScale(const XMFLOAT3& scale);
	void SetScale(const float scale);
	void SetRotation(const float x, const float y, const float z);
	void SetRotation(const XMFLOAT3& rotation);
	void SetPosition(const float x, const float y, const float z);
	void SetPosition(const XMFLOAT3& position);

	virtual void DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList) = 0;

protected:
	Transform	m_Transform;
};

// -------------------------------------------
// 
// ModelObject
// 
// -------------------------------------------
class CModelObject : public CObjectBase
{
public:
	CModelObject();
	virtual ~CModelObject();

	void SetModel(std::shared_ptr<CModel>& model) { m_spTankModel = model; }

	XMFLOAT3 GetPosition() { return m_Transform.position; }

	void DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;

	void ReleaseModelUploadBuffer();

protected:
	Transform					m_Transform;
	std::shared_ptr<CModel>		m_spTankModel;
};

// -------------------------------------------
// 
// TerrainObject
// 
// -------------------------------------------
class CTerrainObject : public CObjectBase
{
public:
	CTerrainObject();
	~CTerrainObject();

	void SetHeightMap(std::shared_ptr<CHeightMapImage>& pHeightMapImage);
	HRESULT GenerateTerrain(ComPtr<ID3D12Device>& cpDevice, ComPtr<ID3D12GraphicsCommandList>& cpCommandList);

	float GetHeightAt(float x, float z) const;
	float GetHeightAt(const XMFLOAT3& pos) const;

	void DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;

	void ReleaseUploadBuffer();
private:
	int									m_nColSlice;
	int									m_nRowSlice;

	std::shared_ptr<CHeightMapImage>	m_spHeightMapImage;
	std::unique_ptr<CMesh>				m_upTerrain;
};