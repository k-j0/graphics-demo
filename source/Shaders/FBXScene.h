
/** Used to load a model from an FBX file using Autodesk's FBX SDK */

#pragma once

#include <fbxsdk.h>
#include "DXF.h"
#include <string>
#include <vector>
#include "FBXSkinnedMesh.h"
#include "LitShader.h"
#include "FBXSkeleton.h"
#include "Animator.h"
#include "SkinnedShader.h"

class FBXScene {

public:
	FBXScene(ID3D11Device* device, ID3D11DeviceContext* deviceContext, std::string filename, FBXImportArgs& args);
	~FBXScene();

	///render the scene
	void render(LitShader* shader, LineShader* lineShader, bool renderSkeleton, D3D_PRIMITIVE_TOPOLOGY top = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	///update the scene (play any animations)
	void update(float dt);

	///Get the current animator
	inline Animator* getAnimator() { return animator; }

	///call Init() before creating any fbx model, and Release() once they've all been loaded in
	static void Init();
	static void Release();

	///Accessors for individual meshes
	inline int meshCount() { return meshes.size(); }
	inline FBXMesh* getMesh(int id) { return meshes[id]; }

protected:
	ID3D11DeviceContext* deviceContext;
	
	///the meshes in this scene
	std::vector<FBXMesh*> meshes;

	///the skeleton for this scene; null if the scene has no skeleton
	FBXSkeleton* skeleton = nullptr;
	BaseMesh* skeletonViewMesh = nullptr;//to view joints in debug view
	Material* skeletonViewMaterial = nullptr;//to view joints in debug view

	///the pose for this scene; null if the scene has no pose
	FbxPose* pose = nullptr;
	Animator* animator = nullptr;

	///the path to the folder where this .fbx is located
	std::string folderPath;

	///the source fbx scene, or nullptr if we don't need animations
	FbxScene* scene = nullptr;

	

	///debug: print an fbx node to sdtout
	void printNode(FbxNode* node);

	///recursively import nodes into this fbx scene
	void importNode(FbxNode* node, FBXImportArgs& args, ID3D11Device* device, ID3D11DeviceContext* deviceContext, FBXJoint* currentJoint = nullptr);

	///import the animations stored within the fbx if any; returns false otherwise
	bool importAnimations(FbxScene* fbxScene);


	//this is only needed for loading stuff; kept around as a single static instance, created via Init() and released via Release().
	static FbxManager* fbxManager;
};

