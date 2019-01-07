#include "SkinnedShader.h"

#include "Utils.h"
#include "AppGlobals.h"


SkinnedShader::SkinnedShader(bool load){
	if (load) {//might not want to load the shaders in if we're deriving from this class
		SETUP_SHADER_SKIN(skinned_vs, default_fs);//set it up as a skinned shader!
	}
}

SkinnedShader::~SkinnedShader(){
}

void SkinnedShader::setBones(XMMATRIX** boneMatrices, int numBones){

	if (numBones > NUM_BONES || numBones < 0) {
		printf("Error! Cannot use %d bones, maximum is %d.\n", numBones, NUM_BONES);
		return;
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource;

	BoneBufferType* bonePtr;
	GLOBALS.DeviceContext->Map(boneBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	bonePtr = (BoneBufferType*)mappedResource.pData;
	for (int i = 0; i < numBones; ++i) {
		bonePtr->worldBoneTransform[i] = (*boneMatrices)[i];//note: no transposing here, it's assumed any transposition will have happened beforehand if needed (in FBXSkeleton.cpp)
	}//Note that, for efficiency, we're not writing to whatever bones are between numBones and NUM_BONES; we're assuming the vertices will never attempt to read from that portion
	GLOBALS.DeviceContext->Unmap(boneBuffer, 0);
	GLOBALS.DeviceContext->VSSetConstantBuffers(2, 1, &boneBuffer);

}

void SkinnedShader::initBuffers(){
	LitShader::initBuffers();

	SETUP_SHADER_BUFFER(BoneBufferType, boneBuffer);

}
