#pragma once

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