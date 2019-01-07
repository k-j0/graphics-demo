#pragma once
#include "FBXMesh.h"

///Used to import and render skinned meshes with up to 8 influences per vertex

#include "FBXSkeleton.h"
#include "SkinnedShader.h"

class FBXSkinnedMesh : public FBXMesh{
protected:
	///Vertex struct for geometry with position, texture, normals and bound skin
	struct VertexType_Skin {
		XMFLOAT3 position;
		XMFLOAT2 texture;
		XMFLOAT3 normal;
		XMFLOAT3 tangent;
		XMUINT4 boneIds;//ids 0 1 2 3
		XMUINT4 boneIds2;//ids 4 5 6 7
		XMFLOAT4 boneWeights;//weights 0 1 2 3
		XMFLOAT4 boneWeights2;//weights 4 5 6 7
	};

public:
	FBXSkinnedMesh(FBXSkeleton*);
	~FBXSkinnedMesh();

	///Overriden to import bone influences as well
	void importMesh(FbxMesh* fbxMesh, FbxSurfaceMaterial* material, std::string folderPath, FBXImportArgs& args, ID3D11Device* device, ID3D11DeviceContext* deviceContext) override;

	void sendData(ID3D11DeviceContext *deviceContext, D3D_PRIMITIVE_TOPOLOGY top = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST) override;

	///Call this before rendering to send animation data to skinning shader
	void sendAnimationData(SkinnedShader* shader);

protected:
	//similar to vertices in FBXMesh, this will be destroyed in initBuffers, almost immediately when importing.
	VertexType_Skin* skinVertices;

	int numBones = 0;//the number of bones / skin clusters

	FBXSkeleton* skeleton = nullptr;

	void initBuffers(ID3D11Device* device) override;

};

