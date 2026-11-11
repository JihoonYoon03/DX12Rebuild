#pragma once

#include "Shader.h"

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
	CMaterial(const std::shared_ptr<CShader>& spShader = nullptr);
	~CMaterial();

	void SetShader(const std::shared_ptr<CShader>& spShader) { m_spShader = spShader; }

	void SetDefaultMaterialData(const MaterialData& mData);
	void SetMaterialData(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList, const MaterialData& mData);

	void BindMaterial(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList);

private:
	ERenderState				m_eRenderState = ERenderState::Default;
	MaterialData				m_matData;
	std::shared_ptr<CShader>	m_spShader;
};

