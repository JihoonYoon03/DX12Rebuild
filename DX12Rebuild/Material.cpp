#include "pch.h"
#include "Material.h"

//-----------------------------------
//[아래 코드는 AI로 구현됨]
bool CMaterial::BindMaterialData(CFrameConstants& constants) const
{
	return constants.Bind(1, m_matData);
}
//-----------------------------------
