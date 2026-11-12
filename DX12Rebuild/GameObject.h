#pragma once

class CModel;
class CMesh;
class CHeightMapImage;
class CTexture;
class CDescHeapAllocator;

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

	XMFLOAT3 GetPosition() const { return m_Transform.position; }
	const Transform& GetTransform() const { return m_Transform; }

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
	void SetFrameTransform(const std::string& name, const XMFLOAT4X4& transform);

	void DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;

	void ReleaseModelUploadBuffer();

protected:
	std::shared_ptr<CModel>				m_spModel;
	//프레임 별로 적용되는 변환
	std::unordered_map<int, XMFLOAT4X4>	m_umFrameTransform;
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
	HRESULT GenerateTerrain(
		ComPtr<ID3D12Device>& cpDevice,
		ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
		CDescHeapAllocator* DescHeapAllocator = nullptr,
		bool normalMapping = false
	);

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
	std::unique_ptr<CTexture>			m_upTerrainTex;
	std::unique_ptr<CTexture>			m_upNormalMap;
};

// -------------------------------------------
// 
// SkyboxObject
// 
// -------------------------------------------
class CSkyboxObject : public CObjectBase
{
public:
	CSkyboxObject();
	~CSkyboxObject() override;

	HRESULT GenerateSkyBox(
		const ComPtr<ID3D12Device>& cpDevice,
		const ComPtr<ID3D12GraphicsCommandList>& cpCommandList,
		CDescHeapAllocator& allocator);

	void DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;
	void ReleaseUploadBuffer();

private:
	std::vector<std::unique_ptr<CMesh>> m_vPlane;
	std::vector<std::unique_ptr<CTexture>> m_vSkyTextures;
};
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CMeshObject : public CObjectBase
{
public:
	CMeshObject(CMesh& mesh, const XMFLOAT4& color);
	void SetColor(const XMFLOAT4& color) { m_color = color; }
	void DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;

protected:
	CMesh* m_pMesh; // 공용 메시를 참조하며 소유하지 않는다.
	XMFLOAT4 m_color;
};
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CDamageableObject : public CModelObject
{
public:
	void SetCombatInfo(int health, float aimHeight, float hitRadius, float homingHitRadius, float markerHeight, float hitHeight = 0.0f);
	void ApplyDamage() { if (m_nHealth > 0) --m_nHealth; }
	int GetHealth() const { return m_nHealth; }
	bool IsAlive() const { return m_nHealth > 0; }
	void SetVisible(bool visible) { m_bVisible = visible; }
	bool IsVisible() const { return m_bVisible && IsAlive(); }
	void DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;
	XMFLOAT3 GetAimPoint() const;
	XMFLOAT3 GetHitPoint() const;
	float GetHitRadius(bool homing) const { return homing ? m_fHomingHitRadius : m_fHitRadius; }
	float GetMarkerHeight() const { return m_fMarkerHeight; }
	virtual void Reload() {}
	virtual bool TryFire(const XMFLOAT3&, float, XMFLOAT3&, XMFLOAT3&) { return false; }

private:
	bool m_bVisible = true;
	int m_nHealth = 1;
	float m_fAimHeight = 0.0f;
	float m_fHitHeight = 0.0f;
	float m_fHitRadius = 0.0f;
	float m_fHomingHitRadius = 0.0f;
	float m_fMarkerHeight = 0.0f;
};
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CTargetObject final : public CDamageableObject
{
public:
	explicit CTargetObject(CMesh& mesh);
	void DrawObject(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList) override;

private:
	CMeshObject m_visual;
};
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
struct ProjectileData
{
	XMFLOAT3 direction;
	float speed;
	float lifetime;
	float clearance;
	float homingDelay = 0.0f;
	float homingTurnSpeed = 0.0f;
	int target = -1;
	bool enemyOwned = false;
};
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CProjectileObject final : public CMeshObject
{
public:
	CProjectileObject(CMesh& mesh, const XMFLOAT3& position, const ProjectileData& data);
	XMFLOAT3& Position() { return m_Transform.position; }
	float AdvanceTime(float dt, const std::optional<XMFLOAT3>& targetPoint);
	XMFLOAT3 NextPosition(float dt) const;

	ProjectileData flight;
};
//-----------------------------------
