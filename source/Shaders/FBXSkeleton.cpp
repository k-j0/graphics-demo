#include "FBXSkeleton.h"
#include "AppGlobals.h"
#include "Utils.h"

#define VERBOSE false //Set to true to send debug info to cout

#define BONE_SIZE 0.05f //Default: 0.05f
#define SKELETON_OFFSET XMMatrixScaling(-1, 1, 1) * XMMatrixRotationRollPitchYaw(0, PI, 0) * XMMatrixTranslation(5.0f, 0, 0) //How much the skeletons should be offset
#define RENDER_BONES_AS_SPHERES false //if true, will also render spheres at the bone locations
#define PITCH(f) f.x
#define YAW(f) f.y
#define ROLL(f) f.z

#if VERBOSE
#define echo(s, ...) printf("\t\t" s "\n", __VA_ARGS__)
#else
#define echo(s, ...) 
#endif

FBXJoint::FBXJoint(XMFLOAT3 translation, XMFLOAT3 eulerAngles, FBXJoint* parentJoint, FbxNode* fbxNode) {
	parent = parentJoint;
	node = fbxNode;

	//Get inverse matrix of bone's bindpos
	FbxAMatrix bindPose = node->EvaluateGlobalTransform(FBXSDK_TIME_INFINITE);
	position = Utils::toFloat3(bindPose.GetT());
	rotation = Utils::degToRad(Utils::toFloat3(bindPose.GetR()));

	XMMATRIX bindPosT = XMMatrixTranslation(position.x, position.y, position.z);
	XMMATRIX bindPosR = getRotationMatrix(rotation);
	inverseBindPoseMatrix = XMMatrixInverse(nullptr, bindPosR * bindPosT);

	if(parent != nullptr)
		line = new Line(GLOBALS.Device, parent->position, position);

}

FBXJoint::~FBXJoint() {
	for (auto it = children.begin(); it != children.end();) {
		delete *it;
		it = children.erase(it);
	}
	if(line)
		delete line;
}

///Angles are expected in radians.
XMMATRIX FBXJoint::getRotationMatrix(XMFLOAT3& angles) {
	//apply the rotations in the correct order depending on fbx setup
	FbxEuler::EOrder order = node->RotationOrder.Get();
	if (order == FbxEuler::eOrderXYZ) {
		XMMATRIX result = XMMatrixRotationX(PITCH(angles));
		result *= XMMatrixRotationY(YAW(angles));
		result *= XMMatrixRotationZ(ROLL(angles));
		return result;
	}
	else if (order == FbxEuler::eOrderZXY) {
		return XMMatrixRotationRollPitchYaw(PITCH(angles), YAW(angles), ROLL(angles));
	}
	else if (order == FbxEuler::eOrderXZY) {
		XMMATRIX result = XMMatrixRotationX(PITCH(angles));
		result *= XMMatrixRotationZ(ROLL(angles));
		result *= XMMatrixRotationY(YAW(angles));
		return result;
	}
	else if (order == FbxEuler::eOrderYXZ) {
		XMMATRIX result = XMMatrixRotationY(YAW(angles));
		result *= XMMatrixRotationX(PITCH(angles));
		result *= XMMatrixRotationZ(ROLL(angles));
		return result;
	}
	else if (order == FbxEuler::eOrderYZX) {
		XMMATRIX result = XMMatrixRotationY(YAW(angles));
		result *= XMMatrixRotationZ(ROLL(angles));
		result *= XMMatrixRotationX(PITCH(angles));
		return result;
	}
	else if (order == FbxEuler::eOrderZYX) {
		XMMATRIX result = XMMatrixRotationZ(ROLL(angles));
		result *= XMMatrixRotationY(YAW(angles));
		result *= XMMatrixRotationX(PITCH(angles));
		return result;
	}
	else {//what else is there? :o
		echo("Error- Unimplemented rotation order for joints: %d", order);
		return XMMatrixIdentity();
	}
}

int FBXJoint::Count() {
	int localCount = 1;//1 for this bone
	for (FBXJoint* child : children)
		localCount += child->Count();
	return localCount;
}

void FBXJoint::update(FbxTime& time0, FbxTime& time1, float weight, XMMATRIX** worldBoneTransform){

	FbxAMatrix globalPos0;
	FbxAMatrix globalPos1;
	globalPos0 = node->EvaluateGlobalTransform(time0);
	if (weight < 1) {
		//find a position between the two times according to weight
		globalPos1 = node->EvaluateGlobalTransform(time1);
		position = Utils::lerp(Utils::toFloat3(globalPos0.GetT()), Utils::toFloat3(globalPos1.GetT()), weight);//lerp positions
		rotation = Utils::degToRad(Utils::lerpAngleDegrees(Utils::toFloat3(globalPos0.GetR()), Utils::toFloat3(globalPos1.GetR()), weight));//lerp angles (using shortest path)
	}
	else {
		//weight is 1, simply use the global transform at time0
		position = Utils::toFloat3(globalPos0.GetT());
		rotation = Utils::degToRad(Utils::toFloat3(globalPos0.GetR()));
	}

	//update transform in matrix
	// Row Major matrices
	XMMATRIX posMatrix = XMMatrixTranslation(position.x, position.y, position.z);
	XMMATRIX rotMatrix = getRotationMatrix(rotation);
	(*worldBoneTransform)[index] = inverseBindPoseMatrix * rotMatrix * posMatrix;

	//update line position to match
	if(line)
		line->setLine(parent->position, position);

	//update children positions
	for (FBXJoint* child : children)
		child->update(time0, time1, weight, worldBoneTransform);
}



///Renders this bone and its children as debug view
void FBXJoint::render(Shader* shader, LineShader* lineShader, BaseMesh* mesh, XMMATRIX global, int depth) {
	XMMATRIX globalPos = XMMatrixTranslation(position.x, position.y, position.z);
	XMMATRIX globalRot = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	XMMATRIX localScale = XMMatrixScaling(BONE_SIZE, BONE_SIZE, BONE_SIZE);
	XMMATRIX world = globalRot * globalPos * global;
	
#if RENDER_BONES_AS_SPHERES
	mesh->sendData(GLOBALS.DeviceContext);
	shader->setShaderParameters(GLOBALS.DeviceContext, localScale * world, GLOBALS.ViewMatrix, GLOBALS.Renderer->getProjectionMatrix());
	shader->render(GLOBALS.DeviceContext, mesh->getIndexCount());
#endif

	if (parent != nullptr && line != nullptr) {
		line->sendData(GLOBALS.DeviceContext);			// Note: transform is only applied to sphere, not line, cos the line's vertices already match
		lineShader->setShaderParameters(GLOBALS.DeviceContext, global, GLOBALS.ViewMatrix, GLOBALS.Renderer->getProjectionMatrix(), XMFLOAT3(0,0,0));//note: passing the default proj matrix rather than the modified one causes bones to not be rendered correctly when changing fov but whatever cos its just for debugging anyway
		lineShader->render(GLOBALS.DeviceContext, line->getIndexCount());
	}

	--depth;

	if (depth != 0) {
		for (FBXJoint* joint : children)
			joint->render(shader, lineShader, mesh, global, depth);
	}
}

bool FBXJoint::assignClusterID(int id, FbxString & boneName){
	if (!boneName.CompareNoCase(node->GetName())) {
		//yes! assign it.
		index = id;
		echo("\t\tAssigned index %d to bone %s.", index, node->GetName());
		return true;
	}

	//not this one-maybe one of our children?
	for (FBXJoint* joint : children) {
		if (joint->assignClusterID(id, boneName))
			return true;
	}
	return false;//neither of our children has that name soz
}

bool FBXJoint::checkClusterIndices() {
	bool result = true;
	
	if (index < 0) {
		echo("Warning! Bone %s does not have a valid index (%d)", node->GetName(), index);
		result = false;
	}

	for (FBXJoint* joint : children) {
		result &= joint->checkClusterIndices();
	}

	return result;
}





FBXSkeleton::FBXSkeleton(){
}

FBXSkeleton::~FBXSkeleton(){
	if(rootJoint)
		delete rootJoint;//this in turn deletes all of the child joints in the skeleton
	if (worldBoneTransform)
		delete[] worldBoneTransform;
}

void FBXSkeleton::AddJoint(FBXJoint* joint, FBXJoint* parent) {
	echo("Adding joint at %f %f %f; %f %f %f", joint->position.x, joint->position.y, joint->position.z, joint->rotation.x, joint->rotation.y, joint->rotation.z);
	if (parent == nullptr) {
		rootJoint = joint;
	}
	else {
		parent->children.push_back(joint);
	}
}

///Updates positions of the joints in the bone matrices
void FBXSkeleton::update(FbxTime& time0, FbxTime& time1, float weight){

	if (worldBoneTransform == nullptr) {//create the matrices on the first update()
		worldBoneTransform = new XMMATRIX[rootJoint->Count()];//one matrix per bone in there
		echo("Created %d individual bone matrices.", rootJoint->Count());
	}

	rootJoint->update(time0, time1, weight, &worldBoneTransform);

}

void FBXSkeleton::render(Shader* shader, LineShader* lineShader, BaseMesh* mesh) {

	if (mesh == nullptr) {
		echo("Cannot display skeleton: mesh is null.");
		return;
	}

	if (rootJoint == nullptr) {
		echo("Cannot display skeleton: root joint is null.");
		return;
	}

	rootJoint->render(shader, lineShader, mesh, SKELETON_OFFSET);
}

bool FBXSkeleton::assignClusterID(int id, FbxString & boneName){
	if (rootJoint && rootJoint->assignClusterID(id, boneName)) {
		return true;
	}
	echo("Cannot assign cluster id to bone %s...", boneName);
	return false;
}

bool FBXSkeleton::checkClusterIndices() {
	return rootJoint->checkClusterIndices();
}

#undef echo
#undef VERBOSE