#pragma once

class CModel;
class CMesh;
class CHeightMapImage;

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

	XMFLOAT3 GetPosition() { return m_Transform.position; }

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

	virtual void SetModel(std::shared_ptr<CModel>& model) { m_spModel = model; }

	void SetFrameTransform(const std::string& name, const Transform& transform);

	void DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;

	void ReleaseModelUploadBuffer();

protected:
	std::shared_ptr<CModel>				m_spModel;
	//프레임 별로 적용되는 변환
	std::unordered_map<int, Transform>	m_umFrameTransform;
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
	float								m_fTiling;

	std::shared_ptr<CHeightMapImage>	m_spHeightMapImage;
	std::unique_ptr<CMesh>				m_upTerrain;
};