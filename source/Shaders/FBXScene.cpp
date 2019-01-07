#include "FBXScene.h"

#include "Utils.h"
#include "AppGlobals.h"

#define VERBOSE false //set to false to bypass printing additional info when importing fbx files


#if VERBOSE
#define echo(s, ...) printf(s "\n", __VA_ARGS__)
#else
#define echo(s, ...) 
#endif


FbxManager* FBXScene::fbxManager = nullptr;

FBXScene::FBXScene(ID3D11Device* device, ID3D11DeviceContext* context, std::string filename, FBXImportArgs& args){
	if (fbxManager == nullptr) {
		//need to call Init() before reaching here
		abort();
	}

	//extract the folder path to be able to get relative textures and such
	std::vector<string> splitPath;
	Utils::split(filename, '/', &splitPath);
	folderPath = "";
	if (splitPath.size() <= 1) {
		folderPath = "/";
	} else {
		for (int i = 0; i < splitPath.size() - 1; ++i) {
			folderPath += splitPath[i] + "/";
		}
	}
#if VERBOSE
	printf("Loading FBX in folder %s\n", folderPath.c_str());
#endif

	deviceContext = context;

	//load the fbx into an fbx scene
	FbxImporter* importer = FbxImporter::Create(fbxManager, "");
	if (!importer->Initialize(filename.c_str(), -1, fbxManager->GetIOSettings())) {
		abort();
	}
	FbxScene* scene = FbxScene::Create(fbxManager, "mScene");
	importer->Import(scene);
	importer->Destroy();

	//walk the fbx scene to import what we need (ie meshes)
	importNode(scene->GetRootNode(), args, device, deviceContext);

	//import any animations
	if (skeleton && importAnimations(scene)) {
		this->scene = scene;
	}
	else {//if there are no animations in the scene, we don't need to keep it.
		scene->Destroy();
	}

	skeletonViewMesh = new SphereMesh(GLOBALS.Device, GLOBALS.DeviceContext, 2);
	skeletonViewMaterial = new Material;
	skeletonViewMaterial->colour = XMFLOAT3(1, 1, 1);

}

FBXScene::~FBXScene(){
	for (FBXMesh* mesh : meshes) {
		delete mesh;
	}
	if (skeleton != nullptr)
		delete skeleton;
	if (skeletonViewMesh != nullptr)
		delete skeletonViewMesh;
	if (skeletonViewMaterial != nullptr)
		delete skeletonViewMaterial;
	if (scene)
		scene->Destroy();//destroy that scene now since we've kept it around for animation info
	if (animator)
		delete animator;
}

///Imports an fbx node and its children into the scene; only imports meshes and joints.
void FBXScene::importNode(FbxNode* node, FBXImportArgs& args, ID3D11Device* device, ID3D11DeviceContext* deviceContext, FBXJoint* currentJoint) {

#if VERBOSE
	printf("Importing %s\n", node->GetName());
	//display the geometric offset if it is not zero (because we can't import it)
	const FbxVector4 lT = node->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 lR = node->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 lS = node->GetGeometricScaling(FbxNode::eSourcePivot);
	if (lT[0] != 0 || lT[1] != 0 || lT[2] != 0 || lT[3] != 1) {
		printf("\tWarning! Offset translation is non-zero: %f, %f, %f, %f\n", lT[0], lT[1], lT[2], lT[3]);
	}
	if (lR[0] != 0 || lR[1] != 0 || lR[2] != 0 || lR[3] != 1) {
		printf("\tWarning! Offset rotation is non-zero: %f, %f, %f, %f\n", lR[0], lR[1], lR[2], lR[3]);
	}
	if (lS[0] != 1 || lS[1] != 1 || lS[2] != 1 || lS[3] != 1) {
		printf("\tWarning! Offset scaling is non-unit: %f, %f, %f, %f\n", lS[0], lS[1], lS[2], lS[3]);
	}
#endif

	FbxAMatrix globalPosition = node->EvaluateGlobalTransform(FBXSDK_TIME_INFINITE);//get default global position
	const FbxVector4 globalTranslation = globalPosition.GetT();
	const FbxVector4 globalRotation = globalPosition.GetR();
	const FbxVector4 globalScale = globalPosition.GetS();
	echo("\tPosition: [%f, %f, %f]; Rotation: [%f, %f, %f]", globalTranslation[0], globalTranslation[1], globalTranslation[2],
															globalRotation[0], globalRotation[1], globalRotation[2]);
	if (globalScale[0] < 0.9999f || globalScale[1] < 0.9999f || globalScale[2] < 0.9999f || globalScale[0] > 1.0001f || globalScale[1] > 1.0001f || globalScale[2] > 1.0001f) {
		echo("\tWarning! Global scale of this node is non-unit: %f, %f, %f", globalScale[0], globalScale[1], globalScale[2]);
	}

	//get this node's attributes
	for (int i = 0; i < node->GetNodeAttributeCount(); ++i) {
		FbxNodeAttribute* attribute = node->GetNodeAttributeByIndex(i);
		//depending on attribute, let's see what we can import
		if (attribute != nullptr) {
			switch (attribute->GetAttributeType()) {
			case FbxNodeAttribute::eSkeleton:
				echo("\t-Joint");
				//Add this bone to the Scene's skeleton
				{
					FbxSkeleton* fbxSkeleton = (FbxSkeleton*)node->GetNodeAttribute();
					if (fbxSkeleton && fbxSkeleton->GetSkeletonType() == FbxSkeleton::eLimbNode) {
						if (skeleton == nullptr) {
							skeleton = new FBXSkeleton;
						}
						FBXJoint* joint = new FBXJoint(Utils::toFloat3(globalTranslation), Utils::degToRad(Utils::toFloat3(globalRotation)), currentJoint, node);
						skeleton->AddJoint(joint, currentJoint);
						currentJoint = joint;//pass this to children nodes
					}
					else {
						echo("Bone is not eLimbNode! Cannot import.");
					}
				}
				break;
			case FbxNodeAttribute::eMesh:
				echo("\t-Mesh");
				//add this mesh to the scene
				{
					FbxMesh* fbxMesh = node->GetMesh();
					if (fbxMesh != nullptr) {
						int materialCount = node->GetSrcObjectCount<FbxSurfaceMaterial>();
						//find only the first valid material and use it for the whole mesh
						//no support for multiple materials on the same mesh
						FbxSurfaceMaterial* material = nullptr;
						for (int mat = 0; mat < materialCount; ++mat) {
							material = node->GetSrcObject<FbxSurfaceMaterial>(mat);
							if (material != nullptr)
								break;
						}
						if (!skeleton) {
							meshes.push_back(new FBXMesh);
						}
						else {//if there is a skeleton already loaded from that fbx file, push a skinned mesh instead of a mesh
							meshes.push_back(new FBXSkinnedMesh(skeleton));
						}
						meshes.back()->importMesh(fbxMesh, material, folderPath, args, device, deviceContext);
					}
				}
				break;
			default:
				echo("\t-Unimported type");
				break;
			}
		}
	}

	//recursively import children:
	for (int i = 0; i < node->GetChildCount(); ++i)
		importNode(node->GetChild(i), args, device, deviceContext, currentJoint);
}

///Imports animation data in the fbx scene
bool FBXScene::importAnimations(FbxScene* fbxScene) {
	
	int animationStacks = fbxScene->GetSrcObjectCount<FbxAnimStack>();
	echo("Number of animation stacks: %d", animationStacks);
	if (animationStacks == 0) return false;
	else if (animationStacks > 1) {
		echo("Warning: only the first animation stack will be imported.");
	}

	FbxAnimStack* animStack = fbxScene->GetSrcObject<FbxAnimStack>(0);
	if (animStack) {
		echo("Importing animation stack %s", animStack->GetName());

		int animationLayers = animStack->GetMemberCount<FbxAnimLayer>();
		echo("Number of animation layers: %d", animationLayers);

		if (animationLayers == 0) return false;
		else if (animationLayers > 1) {
			echo("Warning: only the first animation layer will be imported.");
		}

		FbxAnimLayer* animLayer = animStack->GetMember<FbxAnimLayer>(0);
		if (animLayer) {
			echo("Importing animation layer %s", animLayer->GetName());

			FbxTakeInfo* takeInfo = fbxScene->GetTakeInfo(animStack->GetName());
			if (takeInfo != nullptr) {
				echo("\tStart: %f", takeInfo->mLocalTimeSpan.GetStart().GetSecondDouble());
				echo("\tEnd: %f", takeInfo->mLocalTimeSpan.GetStop().GetSecondDouble());
				echo("\tDuration: %f", takeInfo->mLocalTimeSpan.GetDuration().GetSecondDouble());

				///Create the animator object that will be updated each frame.
				animator = new Animator(takeInfo->mLocalTimeSpan.GetStart(), takeInfo->mLocalTimeSpan.GetStop());

				return true;
			}
			else {
				echo("No take info.");
				return false;
			}
			
		}
		else {
			echo("No animation layer.");
			return false;
		}

	}
	else {
		echo("No animation stack.");
		return false;
	}
}

///Updates animations
void FBXScene::update(float dt) {
	if (skeleton && animator) {
		
		animator->update(dt);

		skeleton->update(animator->getCurrent0(), animator->getCurrent1(), animator->getWeight0());

	}
}

///Renders the meshes in this scene; also renders the skeleton if renderSkeleton evaluates to true
void FBXScene::render(LitShader* shader, LineShader* lineShader, bool renderSkeleton, D3D_PRIMITIVE_TOPOLOGY top) {
	
	//render all meshes:
	for (FBXMesh* mesh : meshes) {
		//shader should be skinned shader if the fbx contains animated models
		if (skeleton) {
			FBXSkinnedMesh* skinnedMesh = dynamic_cast<FBXSkinnedMesh*>(mesh);
			SkinnedShader* skinnedShader = dynamic_cast<SkinnedShader*>(shader);//you better have given me the right type of shader here :)
			skinnedMesh->sendAnimationData(skinnedShader);
		}
		mesh->render(deviceContext, shader, top);
	}

	if (renderSkeleton) {
		//render skeleton as debug view
		if (skeleton != nullptr) {
			shader->setMaterialParameters(GLOBALS.DeviceContext, nullptr, nullptr, nullptr, skeletonViewMaterial);
			skeleton->render(shader, lineShader, skeletonViewMesh);
		}
	}

}

///initialize what we'll need to load fbx's
void FBXScene::Init() {
	fbxManager = FbxManager::Create();
	FbxIOSettings* ioSettings = FbxIOSettings::Create(fbxManager, IOSROOT);
	fbxManager->SetIOSettings(ioSettings);
}

///get rid of everything we need to load fbx's when we're done loading all we need
void FBXScene::Release() {
	fbxManager->GetIOSettings()->Destroy();
	fbxManager->Destroy();
}

///Debug print info about this node and its children
void FBXScene::printNode(FbxNode* node) {
	printf("\n%s, %d, %d, %d:", node->GetName(), node->LclTranslation.Get()[0], node->LclTranslation.Get()[1], node->LclTranslation.Get()[2]);
	for (int i = 0; i < node->GetNodeAttributeCount(); ++i) {
		FbxNodeAttribute* attr = node->GetNodeAttributeByIndex(i);
		if (!attr) continue;
		FbxNodeAttribute::EType type = attr->GetAttributeType();
		printf("\n%s: ", attr->GetName());
		switch (type) {
		case FbxNodeAttribute::eUnknown: printf("unidentified"); break;
		case FbxNodeAttribute::eNull: printf("null"); break;
		case FbxNodeAttribute::eMarker: printf("marker"); break;
		case FbxNodeAttribute::eSkeleton: printf("skeleton"); break;
		case FbxNodeAttribute::eMesh: printf("mesh"); break;
		case FbxNodeAttribute::eNurbs: printf("nurbs"); break;
		case FbxNodeAttribute::ePatch: printf("patch"); break;
		case FbxNodeAttribute::eCamera: printf("camera"); break;
		case FbxNodeAttribute::eCameraStereo: printf("stereo"); break;
		case FbxNodeAttribute::eCameraSwitcher: printf("camera switcher"); break;
		case FbxNodeAttribute::eLight: printf("light"); break;
		case FbxNodeAttribute::eOpticalReference: printf("optical reference"); break;
		case FbxNodeAttribute::eOpticalMarker: printf("marker"); break;
		case FbxNodeAttribute::eNurbsCurve: printf("nurbs curve"); break;
		case FbxNodeAttribute::eTrimNurbsSurface: printf("trim nurbs surface"); break;
		case FbxNodeAttribute::eBoundary: printf("boundary"); break;
		case FbxNodeAttribute::eNurbsSurface: printf("nurbs surface"); break;
		case FbxNodeAttribute::eShape: printf("shape"); break;
		case FbxNodeAttribute::eLODGroup: printf("lodgroup"); break;
		case FbxNodeAttribute::eSubDiv: printf("subdiv"); break;
		default: printf("unknown"); break;
		}
	}
	for (int i = 0; i < node->GetChildCount(); ++i) {
		printNode(node->GetChild(i));
	}
}

#undef VERBOSE
#undef echo