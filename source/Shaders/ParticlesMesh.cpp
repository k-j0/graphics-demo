#include "ParticlesMesh.h"

#include "AppGlobals.h"
#include "Utils.h"

#define PARTICLE_VERTEX_TYPE VertexType_Colour //what struct the vertices are made from, to be able to change it easily and throw in more or less data

ParticlesMesh::ParticlesMesh(int particlesCount, float minX, float maxX, float minY, float maxY, float minZ, float maxZ) : minX(minX), maxX(maxX), minY(minY), maxY(maxY), minZ(minZ), maxZ(maxZ) {
	vertexCount = indexCount = particlesCount;
	initBuffers(GLOBALS.Device);
}


ParticlesMesh::~ParticlesMesh(){
}

void ParticlesMesh::sendData(ID3D11DeviceContext * deviceContext, D3D_PRIMITIVE_TOPOLOGY top){
	unsigned int stride;
	unsigned int offset;

	// Set vertex buffer stride and offset.
	stride = sizeof(PARTICLE_VERTEX_TYPE);
	offset = 0;

	deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	deviceContext->IASetPrimitiveTopology(top);
}

void ParticlesMesh::initBuffers(ID3D11Device* device) {

	D3D11_SUBRESOURCE_DATA vertexData, indexData;

	PARTICLE_VERTEX_TYPE* vertices = new PARTICLE_VERTEX_TYPE[vertexCount];//vertexcount == indexcount is always true here
	unsigned long* indices = new unsigned long[indexCount];

	//Load them up with data
	for (int i = 0; i < vertexCount; ++i) {
		//randomize positions:
		vertices[i].position = XMFLOAT3(Utils::random(minX, maxX),Utils::random(minY, maxY),Utils::random(minZ, maxZ));//starting point for the particle
		vertices[i].colour = XMFLOAT4(
			0, Utils::random(-1,-0.3f), Utils::random(0.4f, 0.5f),//xyz is velocity of particle
			Utils::random(0, 1)//w is time diff between 0 and 1 so that all particles don't animate the same
		);

		indices[i] = i;//as simple an indexing scheme as ever <3
	}

	//Create the d3d buffers
	D3D11_BUFFER_DESC vertexBufferDesc = { sizeof(PARTICLE_VERTEX_TYPE) * vertexCount, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
	vertexData = { vertices, 0 , 0 };
	device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);

	D3D11_BUFFER_DESC indexBufferDesc = { sizeof(unsigned long) * indexCount, D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0, 0, 0 };
	indexData = { indices, 0, 0 };
	device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);

	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;
	delete[] indices;
	indices = 0;

}
