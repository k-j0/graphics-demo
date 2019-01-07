//Domain shader for tessellation with displacement map

#define NUM_LIGHTS 8

//displacement modes
#define NO_DISPLACEMENT 0
#define DISPLACEMENT 1

Texture2D displacementMap : register(t0);

cbuffer MatrixBuffer : register(b0){
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

cbuffer ShadowmapMatrixBuffer : register(b3) {//why no b2? because i want to keep VS aligned with DS
	float4 shadowmapMode[NUM_LIGHTS];//0 for no shadowmap, 1 for shadowmap
	matrix lightViewMatrix[NUM_LIGHTS];
	matrix lightProjectionMatrix[NUM_LIGHTS];
}

struct ConstantOutputType{
	float edges[3] : SV_TessFactor;
	float inside : SV_InsideTessFactor;
};

struct DS_IN{
	float3 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
};

struct DS_OUT{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 binormal : BINORMAL;
	float3 tangent : TANGENT;
	float3 worldPosition : TEXCOORD1;
	float3 viewVector : TEXCOORD2;
	float4 lightViewPos[NUM_LIGHTS] : TEXCOORD3;
};

[domain("tri")]
DS_OUT main(ConstantOutputType input, float3 uvwCoord : SV_DomainLocation, const OutputPatch<DS_IN, 3> patch){

	float3 vertexPosition;
	float2 uv;
	float3 norm;
	float3 tangent;
	DS_OUT output;

	// Determine the position of the new vertex.
	vertexPosition = uvwCoord.x * patch[0].position + uvwCoord.y * patch[1].position + uvwCoord.z * patch[2].position;
	uv = uvwCoord.x * patch[0].tex + uvwCoord.y * patch[1].tex + uvwCoord.z * patch[2].tex;
	norm = uvwCoord.x * patch[0].normal + uvwCoord.y * patch[1].normal + uvwCoord.z * patch[2].normal;
	tangent = uvwCoord.x * patch[0].tangent + uvwCoord.y * patch[1].tangent + uvwCoord.z * patch[2].tangent;

	// Matrix multiplication is done later this time in case we need displacement mapping
	float4 pos = float4(vertexPosition, 1.0f);
	float4 worldPosition = mul(pos, worldMatrix);

	// Store the texture coordinates for the fragment shader.
	output.tex = uv;

	// Calculate the normal and tangent vectors against the world matrix only and normalise.
	output.normal = mul(norm, (float3x3)worldMatrix);
	output.normal = normalize(output.normal);
	output.tangent = mul(tangent, (float3x3)worldMatrix);
	output.tangent = normalize(output.tangent);

	// Sample displacement map if needed
	if (displacementMode == DISPLACEMENT) {
		float2 wrappedUvs = output.tex;
		wrappedUvs.x = fmod(100+wrappedUvs.x, 1);
		wrappedUvs.y = fmod(100+wrappedUvs.y, 1);
		float displacement = displacementMap[uint2(wrappedUvs*mapSize)].r;
		//float displacement = 0;
		displacement *= displacementScale;
		displacement += displacementBias;
		//translate position
		worldPosition.xyz += output.normal * displacement;
	}

	// Calculate the position of the vertex against the world, view, and projection matrices.
	output.worldPosition = worldPosition.xyz;
	output.position = mul(worldPosition, viewMatrix);
	output.position = mul(output.position, projectionMatrix);

	// Build binormal from normal and tangent
	output.binormal = cross(output.normal, output.tangent);

	// Calculate view vector for fragment shader
	output.viewVector = normalize(cameraPosition.xyz - worldPosition.xyz);

	// Calculate the position of the vertex as viewed by the light source.
	[unroll]
	for (int i = 0; i < NUM_LIGHTS; ++i) {
		if (shadowmapMode[i].x == 1) {//only multiply if this data will be relevant to fs, otherwise its garbage anyways
			output.lightViewPos[i] = mul(worldPosition, lightViewMatrix[i]);
			output.lightViewPos[i] = mul(output.lightViewPos[i], lightProjectionMatrix[i]);
		}
	}

	return output;
}

