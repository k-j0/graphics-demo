#pragma once
#include "LitShader.h"

#define NUM_BONES 64 //must match NUM_BONES defined in vertex shader

class SkinnedShader : public LitShader{
private:
	struct BoneBufferType {
		XMMATRIX worldBoneTransform[NUM_BONES];
	};

public:
	SkinnedShader(bool load = true);
	~SkinnedShader();

	void setBones(XMMATRIX** boneMatrices, int numBones);

protected:
	void initBuffers() override;

private:
	ID3D11Buffer* boneBuffer;

};

