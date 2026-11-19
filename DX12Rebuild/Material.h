#pragma once

#include "pch.h"

struct CB_Material
{
	//순수한 기본 색상
	XMFLOAT4	xmf4AlbedoColor		{ 1.0f, 1.0f, 1.0f, 1.0f };
	//자체 발광 색상
	XMFLOAT4	xmf4EmissiveColor	{ 0.0f, 0.0f, 0.0f, 0.0f };

	//Roughness의 반대
	float		fGlossiness			{ 0.0f };
	float		fMetallic			{ 0.0f };
	float		fSpecularHighlight	{ 0.0f };
	//광택 반사 정도
	float		fGlossyReflection	{ 0.0f };

	//텍스쳐 타일링 빈도(1.0f 기본)
	XMFLOAT2	xmf2Tiling				{ 1.0f, 1.0f };
};

//-----------------------------------
//[아래 코드는 AI로 구현됨]
class CMaterial
{
public:
	CMaterial() = default;
	explicit CMaterial(const CB_Material& data) : m_matData(data) {}

	void SetMaterialData(const CB_Material& data) { m_matData = data; }
	bool BindMaterialData(CFrameConstants& constants) const;

private:
	CB_Material m_matData;
};
//-----------------------------------

