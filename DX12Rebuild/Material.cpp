#include "pch.h"
#include "Material.h"
#include "Shader.h"

CMaterial::CMaterial(const std::shared_ptr<CShader>& spShader, std::optional<MaterialData> mData)
	: m_spShader(spShader)
{
	if (mData.has_value()) m_matData = mData.value();
	else ::ZeroMemory(&m_matData, sizeof(MaterialData));
}

CMaterial::~CMaterial()
{
}

void CMaterial::BindMaterial(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	m_spShader->SetPipelineState(cpCommandList);
	cpCommandList->SetGraphicsRoot32BitConstants(2, 4, &m_matData.albedoColor, 0);
}
