///This shader simply converts a position; no normals or uvs.

cbuffer MatrixBuffer : register(b0) {
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projectionMatrix;
}

struct VS_IN {
	float3 position : POSITION;
};

struct VS_OUT {
	float4 position : SV_POSITION;
};

VS_OUT main(VS_IN input) : SV_POSITION {
	VS_OUT output;

	// Calculate the position of the vertex against the world, view, and projection matrices.
	float4 worldPosition = mul(float4(input.position, 1), worldMatrix);
	output.position = mul(worldPosition, viewMatrix);
	output.position = mul(output.position, projectionMatrix);

	return output;
}