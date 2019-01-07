#pragma once

///data type for colour, specular, etc for any object

#include "DXF.h"

class Material {

public:
	XMFLOAT3 colour = XMFLOAT3(1, 1, 1);
	XMFLOAT3 specularColour = XMFLOAT3(4, 4, 4);//SHINYYYY *-*
	float specularPower = 20.0f;

};