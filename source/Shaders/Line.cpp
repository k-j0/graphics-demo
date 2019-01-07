#include "Line.h"



Line::Line(ID3D11Device* device, XMFLOAT3 from, XMFLOAT3 to){
	setLine(from, to);
	initBuffers(device);
}


Line::~Line(){
}

void Line::initBuffers(ID3D11Device* device) {
	vertexCount = 2;
	indexCount = 2;

	D3D11_SUBRESOURCE_DATA vertexData, indexData;

	LineVertex* vertices = new LineVertex[vertexCount];
	unsigned long* indices = new unsigned long[indexCount];

	vertices[0].position = start;
	vertices[1].position = end;
	indices[0] = 0;
	indices[1] = 1;

	D3D11_BUFFER_DESC vertexBufferDesc = { sizeof(LineVertex) * vertexCount, D3D11_USAGE_DYNAMIC, D3D11_BIND_VERTEX_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0 };
	vertexData = { vertices, 0, 0 };
	device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);

	D3D11_BUFFER_DESC indexBufferDesc = { sizeof(unsigned long) * indexCount, D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0, 0, 0 };
	indexData = { indices, 0, 0 };
	device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);

	delete[] vertices;
	delete[] indices;
}

///Send data to GPU
void Line::sendData(ID3D11DeviceContext* deviceContext, D3D_PRIMITIVE_TOPOLOGY top) {
	unsigned int stride = sizeof(LineVertex);
	unsigned int offset = 0;

	//Update line positions on gpu
	LineVertex* dataPtr;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	deviceContext->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	dataPtr = (LineVertex*)mappedResource.pData;
	dataPtr[0].position = start;
	dataPtr[1].position = end;
	deviceContext->Unmap(vertexBuffer, 0);

	deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	deviceContext->IASetPrimitiveTopology(top);
}