#include "FBXSkinnedMesh.h"

#include "Utils.h"
#include "AppGlobals.h"

#define VERBOSE false //turn to true to get verbose import output to console

#if VERBOSE
#define echo(s, ...) printf(s "\n", __VA_ARGS__)
#else
#define echo(s, ...) //compile to nothing at all
#endif

FBXSkinnedMesh::FBXSkinnedMesh(FBXSkeleton* skeleton) : skeleton(skeleton) {

}

FBXSkinnedMesh::~FBXSkinnedMesh(){
}

///Note: it is assumed that skeleton has been assigned before call to this function.
void FBXSkinnedMesh::importMesh(FbxMesh * fbxMesh, FbxSurfaceMaterial * material, std::string folderPath, FBXImportArgs & args, ID3D11Device * device, ID3D11DeviceContext * deviceContext){

	echo("\tImporting skinned mesh %s", fbxMesh->GetName());

	bool importTangents = true;

	if (!skeleton) {
		echo("\t\tError: skeleton is null, cannot import skinned mesh");
		return;
	}

	//count how many vertices, indices, normals, uvs we have
	vertexCount = fbxMesh->GetControlPointsCount();
	indexCount = fbxMesh->GetPolygonVertexCount();
	int normalCount = fbxMesh->GetElementNormalCount();
	int uvCount = fbxMesh->GetElementUVCount();
	int triCount = fbxMesh->GetPolygonCount();
	echo("\t\t%d vertices, %d normals, %d tangents, %d uvs, %d indices, %d triangles, %d deformers", vertexCount, normalCount, fbxMesh->GetElementTangentCount(), uvCount, indexCount, triCount, fbxMesh->GetDeformerCount());

	//if we have more normals than vertices, we don't have a choice but to default to having duplicate vertices since each of our vertices has one normal associated.
	if (normalCount > vertexCount) {
		vertexCount = normalCount;
		echo("\t\tMore normals than vertices; defaulting to %d vertices.", vertexCount);
	}


	//get control points
	FbxVector4* controlPoints = fbxMesh->GetControlPoints();

	// Import skin ----------- (might want to use Ctrl+M+A here, because it gets messy :)) -------------------------------------------------------------------------------------

	//Get skin deformer:
	FbxDeformer* deformer = fbxMesh->GetDeformer(0, FbxDeformer::eSkin);
	if (!deformer) {
		echo("\t\tError: no deformer!");
		return;
	}
	//make sure the deformer type is skin:
	FbxDeformer::EDeformerType defType = deformer->GetDeformerType();
	if (defType != FbxDeformer::eSkin) {
		echo("\t\tError: deformer is not eSkin!");
		return;
	}
	FbxSkin* skin = FbxCast<FbxSkin>(deformer);
	if (!skin) {
		echo("\t\tError: could not cast deformer to skin..");
		return;
	}
	//get the clusters:
	int clusterCount = skin->GetClusterCount();
	echo("\t\tCluster count: %d", clusterCount);
	//get the skinning type:
	FbxSkin::EType skinningType = skin->GetSkinningType();
	if (skinningType != FbxSkin::eLinear) {//i can only afford to import linear skin as of right now :)
		echo("\t\tError: skinning type is not eLinear; unsupported.");
		return;
	}

	numBones = clusterCount;
	//the number of bones is the same as the number of skin clusters, since there's one per bone.

	///We're going to record the weight and cluster id info for each vertex id (corresponding to indices as presented by FbxMesh::GetPolygonVertex())
	/// And use that data later on to fill in the vertices (which don't share the same indexing scheme unfortunately :p)
	struct VertexWeightInfo {
		XMUINT4 boneIds = XMUINT4(NUM_BONES, NUM_BONES, NUM_BONES, NUM_BONES);//NUM_BONES means unassigned (yet)
		XMUINT4 boneIds2 = XMUINT4(NUM_BONES, NUM_BONES, NUM_BONES, NUM_BONES);
		XMFLOAT4 boneWeights = XMFLOAT4(0, 0, 0, 0);
		XMFLOAT4 boneWeights2 = XMFLOAT4(0, 0, 0, 0);
		int numInfluences = 0;

		/// Attempts to add weight data to this vertex weight info
		///			Note to any concerned reader:
		///			I am aware that this is starting to look like shitty Javascript hierarchy (or even Java AWT events
		///			embedded in lambda upon lambda, if anyone has experience with those), and I am truly sorry that I'm
		///			having to resort to declaring types and functions within a function.
		///			Please bear with me :)
		void AssignWeightData(uint32_t clusterId, float influence) {
			numInfluences++;
			if (boneIds.x == NUM_BONES) {//take up slot #0
				boneIds.x = clusterId;
				boneWeights.x = influence;
			}
			else if (boneIds.y == NUM_BONES) {//take up slot #1
				boneIds.y = clusterId;
				boneWeights.y = influence;
			}
			else if (boneIds.z == NUM_BONES) {//take up slot #2
				boneIds.z = clusterId;
				boneWeights.z = influence;
			}
			else if (boneIds.w == NUM_BONES) {//take up slot #3
				boneIds.w = clusterId;
				boneWeights.w = influence;
			}
			else if (boneIds2.x == NUM_BONES) {//take up slot #4
				boneIds2.x = clusterId;
				boneWeights2.x = influence;
			}
			else if(boneIds2.y == NUM_BONES) {//take up slot #5
				boneIds2.y = clusterId;
				boneWeights2.y = influence;
			}
			else if (boneIds2.z == NUM_BONES) {//take up slot #6
				boneIds2.z = clusterId;
				boneWeights2.z = influence;
			}
			else if (boneIds2.w == NUM_BONES) {//take up slot #7
				boneIds2.w = clusterId;
				boneWeights2.z = influence;
			}
			else {//Note: here, we might want to find whichever of our current 8 bones has the least influence
				//			and replace it instead of discarding the data? idk, lets see if we run into problems.
				echo("Attention! This vertex already has 8 bones influencing it... (total so far: %d)", numInfluences);
			}
		}
	};
	VertexWeightInfo* weightData = new VertexWeightInfo[indexCount];//we're deleting this later

	//Go through clusters
	for (int c = 0; c < clusterCount; ++c) {
		FbxCluster* cluster = skin->GetCluster(c);
		echo("\t\t\tCluster #%d %s", c, cluster->GetLink()->GetName());
		echo("\t\t\t\tInfluences: %d", cluster->GetControlPointIndicesCount());
		//Assign cluster id to the relevant bone of our skeleton
		FbxString clusterName = cluster->GetLink()->GetName();
		if (!skeleton->assignClusterID(c, clusterName)) {
			echo("\t\t\t\tError: could not assign cluster id to %s", clusterName);
			return;//nope, we couldn't find the bone in the skeleton :/
		}

		//Assign this id to all vertices influences by the cluster
		// (we're keeping track of it separately, and will assign it to the vertex later down in the function)
		int* indices = cluster->GetControlPointIndices();
		double* weights = cluster->GetControlPointWeights();
		for (int i = 0; i < cluster->GetControlPointIndicesCount(); ++i) {
			VertexWeightInfo* info = &weightData[indices[i]];
			info->AssignWeightData(c, weights[i]);// <-- this is the relevant line; we're adding influence from this cluster to a vertex
		}
	}

	//Check all bones have been assigned an index:
	if (!skeleton->checkClusterIndices()) {
		echo("\t\t\t\tError: some bones did not have valid indices!");
		return;
	}

	// End import skin ----------------------------------------------------------------------------------------------------------------------------------------------------------

	//create temporary vertex & index buffers
	//as of right now, this results in the same vertex being copied instead of indexed
	//it should only create one for vertices which are used for different tris but with the same normal/uv
	vertexCount = indexCount;
	skinVertices = new VertexType_Skin[vertexCount];
	indices = new unsigned long[indexCount];
	
	//extract data from fbx mesh
	int vertex = 0;
	for (int tri = 0; tri < triCount; ++tri) {
#if VERBOSE
		if (fbxMesh->GetPolygonSize(tri) != 3) {//warning because we cannot process quads or anything else right now
			echo("\t\tWarning: This mesh is not triangulated.");//need to triangulate fbx in maya before importing!
		}
#endif
		//go through the vertices of this triangle
		for (int i = 0; i < 3; ++i) {
			int index = fbxMesh->GetPolygonVertex(tri, i);
			FbxVector4 controlPoint = controlPoints[index];

			//position:
			if (args.invertZScale)
				skinVertices[vertex].position = XMFLOAT3(controlPoint[0], controlPoint[1], -controlPoint[2]);
			else
				skinVertices[vertex].position = XMFLOAT3(controlPoint[0], controlPoint[1], controlPoint[2]);

			//normal:
			skinVertices[vertex].normal = XMFLOAT3(0, 0, 0);
			if (!getNormalForIndex(fbxMesh, index, skinVertices[vertex].normal)) {
				echo("\t\tCouldn't read normal.");
			}
			if (args.invertZScale)
				skinVertices[vertex].normal.z *= -1;

			//tangent:
			skinVertices[vertex].tangent = XMFLOAT3(0, 0, 0);
			if (importTangents) {
				if (!getTangentForIndex(fbxMesh, vertex, skinVertices[vertex].tangent)) {
					echo("\t\tCouldn't read tangent.");
					importTangents = false;//stop importing tangent data for this mesh then :|
				}
				if (args.invertZScale)
					skinVertices[vertex].tangent.z *= -1;
			}

			//uv:
			skinVertices[vertex].texture = XMFLOAT2(0, 0);
			if (!getUVForIndex(fbxMesh, index, fbxMesh->GetTextureUVIndex(tri, i), skinVertices[vertex].texture)) {
				echo("\t\tCouldn't read UV.");
			}
			if (args.flipUVs) skinVertices[vertex].texture.y = 1.0f - skinVertices[vertex].texture.y;

			//bone index and weight:
			VertexWeightInfo* info = &weightData[index];//get the weight data for this particular index (index is fbx based, not ours)
			skinVertices[vertex].boneIds = info->boneIds;//NUM_BONES = not yet assigned
			skinVertices[vertex].boneIds2 = info->boneIds2;//NUM_BONES = not yet assigned
			skinVertices[vertex].boneWeights = info->boneWeights;
			skinVertices[vertex].boneWeights2 = info->boneWeights2;
			if (info->boneIds.x == NUM_BONES) {
				echo("\t\tWarning: this vertex (%d) receives no influence from any bone.", index);
			}

			//index:
			if (args.invertWindingOrder) {
				//quickly flip over two vertices for each triangle to get an inverted winding order
				int corrected_v = vertex;
				if (vertex % 3 == 1) ++corrected_v;
				else if (vertex % 3 == 2) --corrected_v;
				indices[corrected_v] = vertex;
			}
			else {
				indices[vertex] = vertex;
			}
			++vertex;
		}
	}

	delete[] weightData;

	importMaterials(material, folderPath);

	//initialize directx buffers
	initBuffers(device);
}

void FBXSkinnedMesh::initBuffers(ID3D11Device* device) {

	//setup vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc = { sizeof(VertexType_Skin) * vertexCount, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
	D3D11_SUBRESOURCE_DATA vertexData = { skinVertices, 0 , 0 };
	device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);

	//setup index buffer
	D3D11_BUFFER_DESC indexBufferDesc = { sizeof(unsigned long) * indexCount, D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0, 0, 0 };
	D3D11_SUBRESOURCE_DATA indexData = { indices, 0, 0 };
	device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);

	//get rid of temporary buffers
	delete[] skinVertices;
	skinVertices = nullptr;
	delete[] indices;
	indices = nullptr;
}

void FBXSkinnedMesh::sendData(ID3D11DeviceContext * deviceContext, D3D_PRIMITIVE_TOPOLOGY top) {

	unsigned int stride;
	unsigned int offset;

	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType_Skin);
	offset = 0;

	deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	deviceContext->IASetPrimitiveTopology(top);
}

void FBXSkinnedMesh::sendAnimationData(SkinnedShader * shader){
	shader->setBones(skeleton->getWorldBoneTransforms(), numBones);
}
