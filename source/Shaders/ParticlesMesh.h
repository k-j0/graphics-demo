#pragma once

#include "DXF.h"

class ParticlesMesh : public BaseMesh{
public:
	ParticlesMesh(int particlesCount = 100, float minX = 0, float maxX = 0, float minY = 0, float maxY = 0, float minZ = 0, float maxZ = 0);
	~ParticlesMesh();

	void sendData(ID3D11DeviceContext* deviceContext, D3D_PRIMITIVE_TOPOLOGY top = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST) override;

protected:
	void initBuffers(ID3D11Device* device) override;

	float minX, maxX, minY, maxY, minZ, maxZ;

};

