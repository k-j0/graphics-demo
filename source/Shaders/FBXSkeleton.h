#pragma once

#include "DXF.h"
#include "LitShader.h"
#include <vector>
#include "LineShader.h"
#include "Line.h"
#include <fbxsdk.h>

class FBXSkeleton;

class FBXJoint {
	friend class FBXSkeleton;

protected:
	std::vector<FBXJoint*> children;//the child bones of this bone
	FBXJoint* parent;//the parent of this bone, or null if root
	XMFLOAT3 position, rotation;//global position and rotation
	XMMATRIX inverseBindPoseMatrix;//the initial bind pose matrix

	int index = -1;//the cluster index corresponding to this bone; -1 until it is assigned.

	FbxNode* node;//the fbx node that corresponds to this joint; to be able to update position based on time.

	///Debug: to draw line between two bones
	Line* line;

	///Updates the joint's position based on animation
	void update(FbxTime& time0, FbxTime& time1, float weight, XMMATRIX** worldBoneTransform);

	///Debug: renders the joint
	void render(Shader* shader, LineShader* lineShader, BaseMesh* mesh, XMMATRIX global, int depth = -1);//using depth > 0 means only that depth of bones will be rendered

	///Walks down the hierachy of bones to assign the specified cluster index to the bone with the name boneName.
	bool assignClusterID(int id, FbxString& boneName);

	///Returns false if one of the indices is not right
	bool checkClusterIndices();

	///Walks down the tree to count how many bones we have in total
	int Count();

	///Get the rotation matrix, given the required angles:
	XMMATRIX getRotationMatrix(XMFLOAT3& rotation);

public:
	///Creates a joint with a global transform (note- no scaling)
	FBXJoint(XMFLOAT3 translation, XMFLOAT3 eulerAngles, FBXJoint* parentJoint, FbxNode* fbxNode);
	///Safely deletes this joint's children
	~FBXJoint();
};

class FBXSkeleton{
public:
	FBXSkeleton();
	~FBXSkeleton();

	///Add a joint to this skeleton; provided the parent joint
	void AddJoint(FBXJoint* joint, FBXJoint* parent);

	///Updates the positions of the joints based on time (see Animator class for explanation on how time0, time1 and weight are used to create transitions)
	void update(FbxTime& time0, FbxTime& time1, float weight);

	///Debug: renders the skeleton
	void render(Shader* shader, LineShader* lineShader, BaseMesh* mesh);

	///Assigns cluster index to particular bone; returns false if unsuccesful
	bool assignClusterID(int id, FbxString& boneName);
	bool checkClusterIndices();

	inline XMMATRIX** getWorldBoneTransforms() { return &worldBoneTransform; }

protected:
	FBXJoint* rootJoint = nullptr;
	XMMATRIX* worldBoneTransform = nullptr;

};

