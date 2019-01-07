#include "ParticlesShader.h"

#include "AppGlobals.h"

ParticlesShader::ParticlesShader(){
	SETUP_SHADER_COLOUR(particles_vs, postprocessing_fs);//using default post processing frag here cos it does what we need; just texture.
	SETUP_GEOMETRY(particles_gs);//geom shader for generating billboard quads at each of the verts
}


ParticlesShader::~ParticlesShader(){
	delete particleBuffer;
}

void ParticlesShader::step(float dt) {
	time += dt;
}

void ParticlesShader::initBuffers(){
	SETUP_SHADER_BUFFER(ParticleBufferType, particleBuffer);
}

void ParticlesShader::sendParticlesData() {

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ParticleBufferType* pbPtr;
	GLOBALS.DeviceContext->Map(particleBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	pbPtr = (ParticleBufferType*)mappedResource.pData;
	pbPtr->quadSize = size;
	pbPtr->time = time;
	pbPtr->maxSpeed = maxSpeed;
	GLOBALS.DeviceContext->Unmap(particleBuffer, 0);
	GLOBALS.DeviceContext->VSSetConstantBuffers(1, 1, &particleBuffer);

	GLOBALS.DeviceContext->PSSetShaderResources(0, 1, &texture);

}