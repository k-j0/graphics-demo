#pragma once
#include "Shader.h"
class ParticlesShader :	public Shader{
private:
	struct ParticleBufferType {
		float quadSize;
		float time;
		float maxSpeed;
		float padding;
	};

public:
	ParticlesShader();
	~ParticlesShader();

	void sendParticlesData();

	//updates particle simulation
	void step(float dt);

	//params:
	float size = 1;
	float maxSpeed = 35;
	ID3D11ShaderResourceView* texture;

protected:
	void initBuffers() override;

	float time = 0;//increases each frame by dt

private:
	ID3D11Buffer* particleBuffer;
};

