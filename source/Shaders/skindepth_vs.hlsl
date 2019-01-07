//skinned vertex shader for depth pass

#define NUM_BONES 64 //we can have a maximum of 64 bones (which should be arguably more than enough)

cbuffer MatrixBuffer : register(b0) {
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projectionMatrix;
}

cbuffer CameraBuffer : register(b1) {
	float3 cameraPosition;
	float far;//the far plane's distance from camera
};

cbuffer BoneBuffer : register(b2) {
	matrix worldBoneTransform[NUM_BONES];
	//i'd use a float3x4 here, but for some reason i dont have access to XMFLOAT3X4 in c++ so whatever
}

struct VS_IN {
	float3 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	uint4 boneIds : BLENDINDICES0;//8 bones acting on each vertex
	uint4 boneIds2 : BLENDINDICES1;
	float4 boneWeights : BLENDWEIGHT0;//the bone weights for each bone
	float4 boneWeights2 : BLENDWEIGHT1;
};

struct VS_OUT {
	float4 position : SV_POSITION;
	float3 worldPosition : TEXCOORD1;
	float4 cameraPosition : TEXCOORD2;
};


///Compute skin position for one bone
float3 skinOne(float weight, float index, float4 position) {
	float3 skinned = weight * mul(worldBoneTransform[index], position).xyz;
	return skinned;
}


///Compute skin position for 4 bones
float3 skin(float4 boneWeights, float4 boneIndices, float4 position) {
	float3 blendPos = skinOne(boneWeights[0], boneIndices[0], position);
	blendPos += skinOne(boneWeights[1], boneIndices[1], position);
	blendPos += skinOne(boneWeights[2], boneIndices[2], position);
	blendPos += skinOne(boneWeights[3], boneIndices[3], position);
	return blendPos;
}


VS_OUT main(VS_IN input) {
	VS_OUT output;

	float4 bindPos = float4(input.position.xyz, 1.0f);

	//Linear skinning of position
	float4 skinPos = float4(skin(input.boneWeights, input.boneIds, bindPos) + skin(input.boneWeights2, input.boneIds2, bindPos), 1.0f); // influences 0 thru 7
	float4 blendPos = skinPos * float4(1, 1, -1, 1);//invert Z scale

	// Calculate the position of the vertex against the world, view, and projection matrices.
	float4 worldPosition = mul(blendPos, worldMatrix);
	output.worldPosition = worldPosition.xyz;
	output.position = mul(worldPosition, viewMatrix);
	output.position = mul(output.position, projectionMatrix);

	//Pass camera position and 1/far to frag
	output.cameraPosition = float4(cameraPosition.xyz, 1.0f / far);

	return output;
}