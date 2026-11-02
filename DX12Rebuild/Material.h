#pragma once

class CShader;

struct MaterialData
{
	XMFLOAT4	albedoColor;
	XMFLOAT4	emissiveColor;
	float		glossiness;
	float		metallic;
	float		SpecularHighlight;
	float		GlossyReflection;
};

class CMaterial
{
public:
	CMaterial(const std::shared_ptr<CShader>& spShader = nullptr, std::optional<MaterialData> mData = std::nullopt);
	~CMaterial();

	void SetShader(const std::shared_ptr<CShader>& spShader) { m_spShader = spShader; }
	void SetMaterialData(const MaterialData& mData) { m_matData = mData; }

	void BindMaterial(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList);

private:
	MaterialData				m_matData;
	std::shared_ptr<CShader>	m_spShader;
};

