//Domain shader for tessellation with displacement map for depth pass

//displacement modes
#define NO_DISPLACEMENT 0
#define DISPLACEMENT 1

Texture2D displacementMap : register(t0);

cbuffer MatrixBuffer : register(b0) {
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projectionMatrix;
};

cbuffer CameraBuffer : register(b1) {
	float3 cameraPosition;
	float far;//the far plane's distance from camera
};

cbuffer DisplacementBuffer : register(b2) {
	float displacementMode;
	float displacementScale;
	float displacementBias;
	float mapSize;
};

struct ConstantOutputType {
	float edges[3] : SV_TessFactor;
	float inside : SV_InsideTessFactor;
};

struct DS_IN {
	float3 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;//unused
};

struct DS_OUT {
	float4 position : SV_POSITION;
	float3 worldPosition : TEXCOORD1;
	float4 cameraPosition : TEXCOORD2;
};

[domain("tri")]
DS_OUT main(ConstantOutputType input, float3 uvwCoord : SV_DomainLocation, const OutputPatch<DS_IN, 3> patch) {

	float3 vertexPosition;
	float2 uv;
	float3 norm;
	float3 tangent;
	DS_OUT output;

	// Determine the position of the new vertex.
	vertexPosition = uvwCoord.x * patch[0].position + uvwCoord.y * patch[1].position + uvwCoord.z * patch[2].position;
	uv = uvwCoord.x * patch[0].tex + uvwCoord.y * patch[1].tex + uvwCoord.z * patch[2].tex;
	norm = uvwCoord.x * patch[0].normal + uvwCoord.y * patch[1].normal + uvwCoord.z * patch[2].normal;

	// Matrix multiplication is done later this time in case we need displacement mapping
	float4 pos = float4(vertexPosition, 1.0f);
	float4 worldPosition = mul(pos, worldMatrix);

	// Calculate the normal and tangent vectors against the world matrix only and normalise.
	float3 outputNormal = mul(norm, (float3x3)worldMatrix);
	outputNormal = normalize(outputNormal);

	// Sample displacement map if needed
	if (displacementMode == DISPLACEMENT) {
		float displacement = displacementMap[uint2(uv*mapSize)].r;
		//float displacement = 0;
		displacement *= displacementScale;
		displacement += displacementBias;
		//translate position
		worldPosition.xyz += outputNormal * displacement;
	}

	// Calculate the position of the vertex against the world, view, and projection matrices.
	output.worldPosition = worldPosition.xyz;
	output.position = mul(worldPosition, viewMatrix);
	output.position = mul(output.position, projectionMatrix);

	// Pass camera position to depth fs
	output.cameraPosition = float4(cameraPosition.xyz, 1.0f/far);

	return output;
}

