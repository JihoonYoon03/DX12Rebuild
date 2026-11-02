#include "pch.h"
#include "Material.h"
#include "Shader.h"

CMaterial::CMaterial(const std::shared_ptr<CShader>& spShader)
	: m_spShader(spShader)
{
	::ZeroMemory(&m_matData, sizeof(MaterialData));
}

CMaterial::~CMaterial()
{
}

void CMaterial::SetDefaultMaterialData(const MaterialData& mData)
{
	m_matData = mData;
}

void CMaterial::SetMaterialData(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList, const MaterialData& mData)
{
	cpCommandList->SetGraphicsRoot32BitConstants(2, 4, &mData.albedoColor, 0);
}

void CMaterial::BindMaterial(const ComPtr<ID3D12GraphicsCommandList>& cpCommandList)
{
	m_spShader->SetPipelineState(cpCommandList);
}
