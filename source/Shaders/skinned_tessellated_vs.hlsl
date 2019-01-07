//tessellated skinning shader - transforms by bones and passes output to hull shader for tessellation

#define NUM_BONES 64 //we can have a maximum of 64 bones (which should be arguably more than enough)

cbuffer BoneBuffer : register(b2) {
	matrix worldBoneTransform[NUM_BONES];
	//i'd use a float3x4 here, but for some reason i dont have access to XMFLOAT3X4 in c++ so whatever
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
	float4 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
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

	//Pass results to hull
	output.position = blendPos;
	output.normal = blendNorm;
	output.tex = input.tex;
	output.tangent = blendTang;

	return output;
}