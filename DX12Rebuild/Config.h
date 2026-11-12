#pragma once

//-----------------------------------
//[직접 작성한 코드]
#define PI 3.141592

namespace config
{
	constexpr unsigned int FRAME_BUFFER_W	= 640;
	constexpr unsigned int FRAME_BUFFER_H	= 640;

	//Frame Rate Sample Count
	constexpr unsigned long MAX_SAMPLE_COUNT = 50;

	constexpr unsigned long MAX_TEXTURE_SLOTS = 64;

	constexpr float	ABRAMS_CANNON_ELEVATION_RADIAN = 20.0f * (PI / 180.0f);
	constexpr float	ABRAMS_CANNON_DEPRESSION_RADIAN = -10.0f * (PI / 180.0f);
}
//-----------------------------------

//-----------------------------------
//[아래 코드는 AI로 구현됨]
namespace config::stage
{
	// Terrain and stage progression
	//-----------------------------------
	//[직접 작성한 코드]
	constexpr int TerrainSamples = 505;
	constexpr float TerrainHorizontalScale = 1.57837f;
	constexpr float TerrainVerticalScale = 0.07539f;
	//-----------------------------------
	constexpr float TerrainEdge = (TerrainSamples - 1) * 0.5f * TerrainHorizontalScale;
	constexpr float UsableHalf = TerrainEdge - 24.0f;
	constexpr float MovementLimit = TerrainEdge - 8.0f;
	constexpr float MaxTimeStep = 0.05f;
	constexpr float DefeatDelay = 1.0f;
	constexpr float VictoryDelay = 2.0f;
	constexpr float TerrainRaySampleSpacing = 1.0f;

	//스카이박스
	//-----------------------------------
	//[직접 작성한 코드]
	constexpr float SkyboxExtent = 100.0f;
	//-----------------------------------

	// Stage population and placement
	constexpr int Level1TargetCount = 12;
	constexpr int Level2TankCount = 6;
	constexpr int Level3TankCount = 5;
	constexpr int Level2ObstacleCount = 56;
	constexpr float SpiralAngle = 2.399963f;

	struct SpawnPattern
	{
		float startDepth;
		float angleOffset;
		float depthStart;
		float depthSpan;
		float depthScatter;
		float width;
		float spreadStart;
		float spreadSpan;
	};

	constexpr SpawnPattern Level1Spawn{ .45f, 0.0f, -.04f, .78f, .07f, .72f, .28f, .60f };
	constexpr SpawnPattern Level2Spawn{ .55f, 0.0f, -.08f, .82f, .08f, .28f, 0.0f, 0.0f };
	constexpr SpawnPattern Level3Spawn{ .48f, .35f, -.02f, .80f, .08f, .70f, .32f, .56f };
	constexpr int Level2TankLanes = 4;
	constexpr float Level2TankSideScatter = .08f;
	constexpr float ExitDepth = .82f;
	constexpr float ExitHeight = .25f;
	constexpr float ExitRadius = 13.5f;
	constexpr float TargetHeight = .8f;
	constexpr float ObstacleAngleOffset = .6f;
	constexpr float ObstacleRingStart = .22f;
	constexpr float ObstacleRingSpan = .70f;
	constexpr float ObstacleRingStep = .618034f;
	constexpr float ObstacleDepthStart = -.34f;
	constexpr float ObstacleDepthSpan = 1.12f;
	constexpr float ObstacleDepthScatter = .16f;
	constexpr float ObstacleSpawnLaneWidth = 12.0f;
	constexpr float ObstacleSpawnLaneDepth = -.35f;
	constexpr float ObstacleSpawnLanePush = 18.0f;
	constexpr float ObstacleMinDepth = -.42f;
	constexpr float ObstacleMaxDepth = .92f;
	constexpr float ObstacleBaseScale = 5.0f * .45f;
	constexpr float ObstacleScaleStep = 5.0f * .10f;
	constexpr int ObstacleScaleVariants = 5;
	constexpr int ObstacleYawVariants = 7;
	constexpr float ObstacleYawStep = .31f;

	// Vehicles and collisions
	constexpr int PlayerTankHealth = 5;
	constexpr int HelicopterHealth = 3;
	constexpr int EnemyTankHealth = 2;
	constexpr int TargetHealth = 1;
	constexpr bool TankAutoAttackAtStart = true;
	constexpr float TankDriveSpeed = 10.0f;
	constexpr float TankTurnSpeedDegrees = 60.0f;
	constexpr float TurretManualSpeedDegrees = 70.0f;
	constexpr float CannonManualSpeedDegrees = 40.0f;
	constexpr float TankGroundOffset = 1.1f;
	constexpr float TankFootprintHalfWidth = 3.4f;
	constexpr float TankFootprintHalfLength = 5.4f;
	constexpr float ApacheScale = .09f;
	constexpr float HelicopterYawSpeed = 1.25f;
	constexpr float HelicopterPitchLimit = .42f;
	constexpr float HelicopterPitchSpeed = 1.7f;
	constexpr float HelicopterDriveSpeed = 18.0f;
	constexpr float HelicopterLiftSpeed = 9.9f;
	constexpr float HelicopterMinClearance = 4.0f;
	constexpr float HelicopterAltitudeRange = 85.0f;
	constexpr float HelicopterBodyPitchFactor = -.45f;
	constexpr float HelicopterRotorSpeed = 22.0f;
	constexpr float MainRotorSpeedFactor = 1.8f;
	constexpr float TailRotorSpeedFactor = 3.2f;

	// Camera
	constexpr float CameraFovDegrees = 60.0f;
	constexpr float CameraNear = .1f;
	constexpr float CameraFar = 1200.0f;
	constexpr float CameraGroundClearance = .5f;
	//-----------------------------------
	//[직접 작성한 코드]
	constexpr float TankCameraDistance = 3.5f;
	constexpr float TankCameraHeight = 1.0f;
	//-----------------------------------
	constexpr float HelicopterCameraDistance = 7.0f;
	constexpr float HelicopterCameraHeight = 4.0f;
	constexpr float HelicopterCameraLookAhead = 3.0f;
	constexpr float HelicopterCameraTargetHeight = .65f;

	// Combat and targeting
	constexpr float TankShellSpeed = 96.0f;
	constexpr float TankShellLifetime = 8.0f;
	constexpr float TankReload = 1.10f;
	constexpr float EnemyTankReload = 2.20f;
	constexpr float TankShellClearance = .28f;
	constexpr float TankHitRadius = 4.25f;
	constexpr float TankAimHeight = 1.0f;
	constexpr float EnemyTankAimHeight = -0.5f;
	constexpr float TankLockMinDot = .925f;
	constexpr float TurretAimSpeed = 2.65f;
	constexpr float CannonAimSpeed = 1.85f;
	constexpr float CannonFireMinDot = .992f;
	constexpr float HelicopterMissileSpeed = 55.0f;
	constexpr float HelicopterMissileLifetime = 10.0f;
	constexpr float HelicopterReload = .36f;
	constexpr float HelicopterMissileOffset = 5.2f;
	constexpr float HelicopterHomingDelay = .55f;
	constexpr float HelicopterHomingTurnSpeed = .70f;
	constexpr float HelicopterMissileClearance = .32f;
	constexpr float HelicopterHitRadius = 3.2f;
	constexpr float Level1LockMinDot = .975f;
	constexpr float Level3LockMinDot = .965f;
	constexpr float Level3LockHitRadius = TankHitRadius * .85f;
	constexpr float TargetLockRadius = 1.9f;
	constexpr float TargetHitRadius = 1.8f;
	constexpr float TargetHomingHitRadius = 4.0f;
	constexpr float LockDistancePenalty = .0008f;
	constexpr float LockTerrainClearance = .05f;
	//-----------------------------------
	//[아래 코드는 AI로 구현됨]
	constexpr float ReticleSize = .08f;
	//-----------------------------------

	static_assert(TerrainRaySampleSpacing > 0.0f);
	static_assert(Level2TankLanes > 0 && ObstacleScaleVariants > 0 && ObstacleYawVariants > 0);
	static_assert(PlayerTankHealth > 0 && HelicopterHealth > 0 && EnemyTankHealth > 0 && TargetHealth > 0);
}
//-----------------------------------