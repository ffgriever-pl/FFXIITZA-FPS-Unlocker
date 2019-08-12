#ifndef INTERP_H 
#define INTERP_H

#include <math.h>
#include "Globals.h"

#pragma once

#define deltaVector (targetPosition - initialPosition)

enum EasingTypes 
{
	Linear,
	OnePointFive,
	Squared,
	Cubic,
	Quad,
	Quint,
	Sine,
	SinSq,
	SinInvSq,
	PerlinFast,
	Perlin
};

//TODO: Optimize with something like maps
constexpr int NUM_EASING_TYPES = 11;
const char* EASING_TYPE_NAMES[] = { "Linear", "OnePointFive", "Square", "Cubic", "Quad", "Quint", "Sine", "SinSq", "SinInvSq", "PerlinFast", "Perlin" };


class Interpolator 
{
public:
	float initialTime = 0;
	float endTime = 0;
	float smoothingTime = 0;
	float initialPosition = 0;
	float currentPosition = 0;
	float targetPosition = 0;
	int easingType = EasingTypes::Linear;

public:
	void setType(int& type) 
	{
		easingType = type;
	}
	void setType(const char* type) 
	{
		for (int i = 0; i < NUM_EASING_TYPES; i++) 
		{
			if (EASING_TYPE_NAMES[i] == type) easingType = i;
		}
	}

	float interpolate(float& newTime)
	{
		//TODO: Optimize more
		endTime = initialTime + smoothingTime;
		float deltaTime = ((newTime - initialTime) / (endTime - initialTime));
		float dtSquared = deltaTime * deltaTime;
		float dtCubed = dtSquared * deltaTime;
		float dtQuart = dtCubed * deltaTime;		
		float dtOnePointFive, dtQuint, dtSin, dtSinSquared, dtSinRoot, dtFastPerlin, dtPerlin;

		switch (easingType) 
		{
		case EasingTypes::Linear:
			currentPosition = initialPosition + (deltaVector * deltaTime);
			return currentPosition;
			break;
		case EasingTypes::OnePointFive:
			dtOnePointFive = powf(deltaTime, 1.5f);
			currentPosition = initialPosition + (deltaVector * dtOnePointFive);
			return currentPosition;
			break;
		case EasingTypes::Squared:
			currentPosition = initialPosition + (deltaVector * dtSquared);
			return currentPosition;
			break;
		case EasingTypes::Cubic:
			currentPosition = initialPosition + (deltaVector * dtCubed);
			return currentPosition;
			break;
		case EasingTypes::Quad:
			currentPosition = initialPosition + (deltaVector * dtQuart);
			return currentPosition;
			break;
		case EasingTypes::Quint:
			dtQuint = dtQuart * deltaTime;
			currentPosition = initialPosition + (deltaVector * dtQuint);
			return currentPosition;
			break;
		case EasingTypes::Sine:
			dtSin = sinf(deltaTime * HPI);
			currentPosition = initialPosition + (deltaVector * dtSin);
			return currentPosition;
			break;
		case EasingTypes::SinSq:
			dtSinSquared = sinf(dtSquared * HPI);
			currentPosition = initialPosition + (deltaVector * dtSinSquared);
			return currentPosition;
			break;
		case EasingTypes::SinInvSq:
			dtSinRoot = sinf(powf(deltaTime, 0.5f) * HPI);
			currentPosition = initialPosition + (deltaVector * dtSinRoot);
			return currentPosition;
			break;
		case EasingTypes::PerlinFast:
			dtFastPerlin = 3.f * dtSquared - 2.f * dtCubed;
			currentPosition = initialPosition + (deltaVector * dtFastPerlin);
			return currentPosition;
			break;
		case EasingTypes::Perlin:
			dtPerlin =
				70.f * dtQuart * dtQuart * deltaTime	// 70x^9
				- 315.f * dtQuart * dtQuart			//315x^8
				+ 540.f * dtQuart * dtCubed			//540x^7
				- 420.f * dtQuart * dtSquared			//420x^6
				+ 126.f * dtQuart * deltaTime;		//126x^5
			currentPosition = initialPosition + (deltaVector * dtPerlin);
			return currentPosition;
			break;
		}
		return 1;
	}

	Interpolator() {}
};

#endif



