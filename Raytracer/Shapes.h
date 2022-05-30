#pragma once
#include "Vec3.h"

struct Sphere
{
	Sphere(){};
	Sphere(Vec3 center, double radius) : center(center), radius(radius){}
	Vec3 center;
	double radius;
};