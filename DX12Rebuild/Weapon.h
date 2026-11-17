#pragma once

class CDamageableObject;
struct ProjectileData;

//-----------------------------------
//[아래 코드는 AI로 구현됨]
struct WeaponSettings
{
	float reload;

	float speed;
	float lifetime;
	float clearance;
	float muzzleOffset = 0.0f;

	float homingDelay = 0.0f;
	float homingTurnSpeed = 0.0f;

	bool homing = false;
	bool enemyOwned = false;
};

// -------------------------------------------
//
// Weapon
//
// -------------------------------------------
class CWeapon
{
public:
	explicit CWeapon(const WeaponSettings& settings) : m_settings(settings) {}

	void Update(float dt);
	void Reset() { m_fReload = 0.0f; }

	bool TryFire(const XMFLOAT3& direction, CDamageableObject* target, ProjectileData& projectile);

	float GetRange() const { return m_settings.speed * m_settings.lifetime; }
	float GetClearance() const { return m_settings.clearance; }
	float GetMuzzleOffset() const { return m_settings.muzzleOffset; }

private:
	WeaponSettings m_settings;

	float m_fReload = 0.0f;
};
//-----------------------------------
