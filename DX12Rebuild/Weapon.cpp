#include "pch.h"
#include "Weapon.h"
#include "GameObject.h"

//-----------------------------------
//[아래 코드는 AI로 구현됨]
void CWeapon::Update(float dt)
{
	m_fReload = (std::max)(0.0f, m_fReload - dt);
}

bool CWeapon::TryFire(const XMFLOAT3& direction, CDamageableObject* target, ProjectileData& projectile)
{
	if (m_fReload > 0.0f) return false;
	projectile = {
		.direction = direction,
		.speed = m_settings.speed,
		.lifetime = m_settings.lifetime,
		.clearance = m_settings.clearance,
		.homingDelay = m_settings.homingDelay,
		.homingTurnSpeed = m_settings.homingTurnSpeed,
		.target = m_settings.homing ? target : nullptr,
		.enemyOwned = m_settings.enemyOwned
	};
	m_fReload = m_settings.reload;
	return true;
}
//-----------------------------------
