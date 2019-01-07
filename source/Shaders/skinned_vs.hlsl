//default skinning vertex shader: blends between bone positions and transforms by world-view-projection matrices

#define NUM_LIGHTS 8
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

cbuffer ShadowmapMatrixBuffer : register(b3) {
	float4 shadowmapMode[NUM_LIGHTS];//0 for no shadowmap, 1 for shadowmap
	matrix lightViewMatrix[NUM_LIGHTS];
	matrix lightProjectionMatrix[NUM_LIGHTS];
}

struct VS_IN {
	float3 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	uint4 boneIds : BLENDINDICES0;//8 bones acting on each vertex
	uint4 boneIds2 : BLENDINDICES1;
	float4 boneWeights : BLENDWEIGHT0;//the bone weights for each bone
	float4 boneWeights2 : BLENDWEIGHT1;
};

struct VS_OUT {
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 binormal : BINORMAL;
	float3 tangent : TANGENT;
	float3 worldPosition : TEXCOORD1;
	float3 viewVector : TEXCOORD2;
	float4 lightViewPos[NUM_LIGHTS] : TEXCOORD3;
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
	float4 bindNorm = float4(input.normal.xyz, 1.0f);
	float4 bindTang = float4(input.tangent.xyz, 1.0f);

	//Linear skinning of position
	float4 skinPos = float4(skin(input.boneWeights, input.boneIds, bindPos) + skin(input.boneWeights2, input.boneIds2, bindPos), 1.0f); // influences 0 thru 7
	float4 blendPos = skinPos * float4(1, 1, -1, 1);//invert Z scale

	//Linear skinning of normal
	float3 skinNorm = skin(input.boneWeights, input.boneIds, bindNorm) + skin(input.boneWeights2, input.boneIds2, bindNorm);
	float3 blendNorm = skinNorm * float4(1, 1, -1, 1);//invert Z scale (normal is normalized later on)

	//Linear skinning of tangent
	float3 skinTang = skin(input.boneWeights, input.boneIds, bindTang) + skin(input.boneWeights2, input.boneIds2, bindTang);
	float3 blendTang = skinTang;// *float4(1, 1, -1, 1);//invert Z scale

	// Calculate the position of the vertex against the world, view, and projection matrices.
	float4 worldPosition = mul(blendPos, worldMatrix);
	output.worldPosition = worldPosition.xyz;
	output.position = mul(worldPosition, viewMatrix);
	output.position = mul(output.position, projectionMatrix);

	// Store the texture coordinates for the fragment shader.
	output.tex = input.tex;

	// Calculate the normal and tangent vectors against the world matrix only and normalise.
	output.normal = mul(blendNorm, (float3x3)worldMatrix);
	output.normal = normalize(output.normal);
	output.tangent = mul(blendTang, (float3x3)worldMatrix);
	output.tangent = normalize(output.tangent);

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