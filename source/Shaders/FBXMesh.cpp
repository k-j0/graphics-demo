#include "FBXMesh.h"

#include "Utils.h"
#include "AppGlobals.h"

#define VERBOSE false //turn to true to get verbose import output to console

//output only when in verbose mode - otherwise echo() compiles to nothing
#if VERBOSE
#define echo(s, ...) printf((std::string(s)+"\n").c_str(), __VA_ARGS__)
#else
#define echo(s, ...)
#endif

FBXMesh::FBXMesh(){
}

FBXMesh::~FBXMesh(){
	if(material)
		delete material;
}

void FBXMesh::importMesh(FbxMesh* fbxMesh, FbxSurfaceMaterial* material, std::string folderPath, FBXImportArgs& args, ID3D11Device* device, ID3D11DeviceContext* deviceContext) {

	echo("\tImporting mesh %s", fbxMesh->GetName());

	bool importTangents = true;

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

	//create temporary vertex & index buffers
	//as of right now, this results in the same vertex being copied instead of indexed
	//it should only create one for vertices which are used for different tris but with the same normal/uv
	vertexCount = indexCount;
	vertices = new VertexType_Tangent[vertexCount];
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
				vertices[vertex].position = XMFLOAT3(controlPoint[0], controlPoint[1], -controlPoint[2]);
			else
				vertices[vertex].position = XMFLOAT3(controlPoint[0], controlPoint[1], controlPoint[2]);

			//normal:
			vertices[vertex].normal = XMFLOAT3(0, 0, 0);
			if (!getNormalForIndex(fbxMesh, index, vertices[vertex].normal)) {
				echo("\t\tCouldn't read normal.");
			}
			if (args.invertZScale)
				vertices[vertex].normal.z *= -1;

			//tangent:
			vertices[vertex].tangent = XMFLOAT3(0, 0, 0);
			if (importTangents) {
				if (!getTangentForIndex(fbxMesh, vertex, vertices[vertex].tangent)) {
					echo("\t\tCouldn't read tangent.");
					importTangents = false;//stop importing tangent data for this mesh then :|
				}
				/*if (vertices[vertex].tangent.x == 0 && vertices[vertex].tangent.y == 0 && vertices[vertex].tangent.z == 0) {
					echo("\t\tTangent is (0,0,0)!");
				}*/
				if (args.invertZScale)
					vertices[vertex].tangent.z *= -1;
			}

			//uv:
			vertices[vertex].texture = XMFLOAT2(0, 0);
			if (!getUVForIndex(fbxMesh, index, fbxMesh->GetTextureUVIndex(tri, i), vertices[vertex].texture)) {
				echo("\t\tCouldn't read UV.");
			}
			if (args.flipUVs) vertices[vertex].texture.y = 1.0f - vertices[vertex].texture.y;

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

	importMaterials(material, folderPath);

	//initialize directx buffers
	initBuffers(device);
}

///Imports the diffuse texture for the mesh
void FBXMesh::importMaterials(FbxSurfaceMaterial* material, std::string folderPath) {

	//get material/texture data
	this->material = new Material;
	if (material != nullptr) {
		echo("\t\tAssigning material %s", material->GetName());

		//Diffuse texture:
		FbxProperty diffuse = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
		if (diffuse.GetSrcObjectCount<FbxLayeredTexture>() > 0) {
			echo("\t\t\tLayered textures are not supported.");
		}
		else {//regular texture :)
			int textureCount = diffuse.GetSrcObjectCount<FbxTexture>();
#if VERBOSE
			if (textureCount > 1) {
				echo("\t\t\t%d textures; will only get first one.", textureCount);
			}
#endif
			if (textureCount > 0) {
				//attempt to get the first texture (usually the only one)
				FbxFileTexture* texture = FbxCast<FbxFileTexture>(diffuse.GetSrcObject<FbxTexture>(0));
				if (texture != nullptr) {
					std::string filename = std::string(texture->GetFileName());
					//only keep the file's name, not the rest of the path:
					std::vector<std::string> splitFilename;
					Utils::split(filename, '/', &splitFilename);
					Utils::split(splitFilename.back(), '\\', &splitFilename);
					filename = folderPath + splitFilename.back();//add the folder path beforehand to make it relative
					echo("\t\t\tAssigning texture %s", filename.c_str());
					std::wstring w_filename = std::wstring(filename.begin(), filename.end());//convert it to widestring
					GLOBALS.TextureManager->loadTexture(filename, (WCHAR*)w_filename.c_str());
					this->texture = GLOBALS.TextureManager->getTexture(filename);
				}
				else {
					echo("\t\t\tTexture was null!");
				}
			}
			else {
				echo("\t\t\tNo texture.");
			}
		}

		//Displacement map:
		FbxProperty displacement = material->FindProperty(FbxSurfaceMaterial::sBump);
		if (displacement.GetSrcObjectCount<FbxLayeredTexture>() > 0) {
			echo("\t\t\tLayered displacement maps are not supported.");
		}
		else {
			int textureCount = displacement.GetSrcObjectCount<FbxTexture>();
			if (textureCount > 1) {
				echo("\t\t\t%d displacement maps; will only get first one.", textureCount);
			}

			if (textureCount > 0) {
				//attempt to get the first texture.
				FbxFileTexture* texture = FbxCast<FbxFileTexture>(displacement.GetSrcObject<FbxTexture>(0));
				if (texture) {
					std::string filename = std::string(texture->GetFileName());
					//only keep the file's name, not the rest of the path:
					std::vector<std::string> splitFilename;
					Utils::split(filename, '/', &splitFilename);
					Utils::split(splitFilename.back(), '\\', &splitFilename);
					filename = folderPath + splitFilename.back();//add the folder path beforehand to make it relative
					echo("\t\t\tAssigning displacement map %s", filename.c_str());
					std::wstring w_filename = std::wstring(filename.begin(), filename.end());//convert it to widestring
					GLOBALS.TextureManager->loadTexture(filename, (WCHAR*)w_filename.c_str());
					this->displacementMap = GLOBALS.TextureManager->getTexture(filename);

					//Assign normal map as well, assuming same name with "-normal" added to it (cos i can't figure out how to add one in maya lmao)
					std::vector<std::string> filenameAndExtension;
					Utils::split(filename, '.', &filenameAndExtension);
					std::string normalFilename = filenameAndExtension.front();
					for (int i = 1; i < filenameAndExtension.size() - 1; ++i)
						normalFilename += "." + filenameAndExtension[i];
					normalFilename += "-normal." + filenameAndExtension.back();
					echo("\t\t\tAssigning normal map %s", normalFilename.c_str());
					std::wstring w_normalfilename = std::wstring(normalFilename.begin(), normalFilename.end());//convert it to widestring
					GLOBALS.TextureManager->loadTexture(normalFilename, (WCHAR*)w_normalfilename.c_str());
					this->normalMap = GLOBALS.TextureManager->getTexture(normalFilename);

				}
				else {
					echo("\t\t\tDisplacement map was null!");
				}
			}
			else {
				echo("\t\t\tNo displacement map.");
			}
		}

		//get material colours
		FbxSurfaceLambert* lambert = FbxCast<FbxSurfaceLambert>(material);
		if (lambert) {
			this->material->colour = Utils::toFloat3(lambert->Diffuse);
			FbxSurfacePhong* phong = FbxCast<FbxSurfacePhong>(lambert);
			if (phong) {
				this->material->specularColour = Utils::toFloat3(phong->Specular);
				//For the purpose of this demo i want everything to be extra shiny!
				this->material->specularColour.x *= 5;
				this->material->specularColour.y *= 5;
				this->material->specularColour.z *= 5;
			}
			else {
				echo("\t\tNo phong.");
				this->material->specularColour = XMFLOAT3(0, 0, 0);//no shiny :'(
			}
		}
		else {
			echo("\t\tNo lambert.");
		}
	}
	else {
		echo("\t\tNo material for mesh.");
	}
}

void FBXMesh::initBuffers(ID3D11Device* device) {
	
	//fill vertex and index buffers for gpu
	D3D11_SUBRESOURCE_DATA vertexData, indexData;

	//setup vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc = { sizeof(VertexType_Tangent) * vertexCount, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
	vertexData = { vertices, 0 , 0 };
	device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);

	//setup index buffer
	D3D11_BUFFER_DESC indexBufferDesc = { sizeof(unsigned long) * indexCount, D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0, 0, 0 };
	indexData = { indices, 0, 0 };
	device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);

	//get rid of temporary buffers
	delete[] vertices;
	vertices = nullptr;
	delete[] indices;
	indices = nullptr;
}

///passing in the index of a control point, this ***should*** fill in out_normal with the point's normal - otherwise, returns false.
bool FBXMesh::getNormalForIndex(FbxMesh* fbxMesh, int index, XMFLOAT3& out_normal) {

	if (fbxMesh->GetElementNormalCount() <= 0) {
		echo("\t\t\tNo normals in mesh.");
		//attempt to generate normals by control point
		if (!fbxMesh->GenerateNormals(true, true)) {
			echo("\t\t\tCould not recalculate normals.");
			return false;
		}
	}

	FbxGeometryElementNormal* normalPtr = fbxMesh->GetElementNormal(0);
	if (normalPtr->GetMappingMode() != FbxGeometryElement::eByControlPoint) {//can't import by-face normals for now (or anything other than by-vertex, really)
		echo("\t\t\tNormals are not eByControlPoint; regenerating.");
		//attempt to regenerate normals by control point
		if (!fbxMesh->GenerateNormals(true, true)) {
			echo("\t\t\tCould not recalculate normals.");
			return false;
		}
	}

	//depending on the way the normal is referenced we may need to do this a number of different ways
	switch (normalPtr->GetReferenceMode()) {
	case FbxGeometryElement::eDirect: {//normals are referenced directly
			FbxDouble* n = normalPtr->GetDirectArray().GetAt(index).mData;
			out_normal.x = n[0];
			out_normal.y = n[1];
			out_normal.z = n[2];
		}
		break;
	case FbxGeometryElement::eIndexToDirect: {//one more step
			int id = normalPtr->GetIndexArray().GetAt(index);
			FbxDouble* n = normalPtr->GetDirectArray().GetAt(id).mData;
			out_normal.x = n[0];
			out_normal.y = n[1];
			out_normal.z = n[2];
		}
		break;
	default://then there's nothing i can do here
		echo("\t\t\tCan't read normals, reference mode is neither eDirect nor eIndexToDirect...");
		return false;
	}

	return true;
}

///passing in the index of a control point, this ***should*** fill in out_normal with the point's normal - otherwise, returns false.
bool FBXMesh::getTangentForIndex(FbxMesh* fbxMesh, int vertexCounter, XMFLOAT3& out_tangent) {

	if (fbxMesh->GetElementTangentCount() <= 0) {
		echo("\t\t\tNo tangents in mesh; recalculating.");
		//attempt to generate some
		if (!fbxMesh->GenerateTangentsData(0, true, true)) {
			echo("\t\tCould not recalculate tangents.");
			return false;
		}
		else if(fbxMesh->GetElementTangentCount() <= 0){
			echo("\t\tRecalculating tangents failed unexpectedly.");
			return false;
		}
	}

	FbxGeometryElementTangent* tangentPtr = fbxMesh->GetElementTangent(0);
	if (tangentPtr->GetMappingMode() != FbxGeometryElement::eByPolygonVertex) {//can't import by-face tangents for now (or anything other than by-vertex, really)
		echo("\t\t\tTangents are not eByPolygonVertex. Cannot import.");
		FbxLayerElement::EMappingMode mode = tangentPtr->GetMappingMode();
		switch (mode)
		{
		case fbxsdk::FbxLayerElement::eNone:
			echo("\t\t\teNone");
			break;
		case fbxsdk::FbxLayerElement::eByControlPoint:
			echo("\t\t\teByControlPoint");
			break;
		case fbxsdk::FbxLayerElement::eByPolygonVertex:
			echo("\t\t\teByPolygonVertex");
			break;
		case fbxsdk::FbxLayerElement::eByPolygon:
			echo("\t\t\teByPolygon");
			break;
		case fbxsdk::FbxLayerElement::eByEdge:
			echo("\t\t\teByEdge");
			break;
		case fbxsdk::FbxLayerElement::eAllSame:
			echo("\t\t\teAllSame");
			break;
		default:
			echo("\t\t\tUndefined");
			break;
		}
		return false;
	}

	switch (tangentPtr->GetReferenceMode()) {
	case FbxGeometryElement::eDirect: {//tangents are only referenced directly as far as i can tell
			FbxVector4 tngt = tangentPtr->GetDirectArray().GetAt(vertexCounter);
			out_tangent.x = static_cast<float>(tngt.mData[0]);
			out_tangent.y = static_cast<float>(tngt.mData[1]);
			out_tangent.z = static_cast<float>(tngt.mData[2]);
		}
		break;
	default://then there's nothing i can do here
		echo("\t\t\tCan't read tangents, reference mode is not eDirect...");
		return false;
	}

	return true;
}

///passing in the index of a control point, this ***should*** fill in out_uv with the point's uv - otherwise, returns false
bool FBXMesh::getUVForIndex(FbxMesh* fbxMesh, int index, int uvIndex, XMFLOAT2& out_uv) {

	if (fbxMesh->GetElementUVCount() <= 0) {
		echo("\t\t\tNo uvs in mesh.");
		return false;
	}

	FbxGeometryElementUV* uvPtr = fbxMesh->GetElementUV(0);
	if (uvPtr->GetMappingMode() != FbxGeometryElement::eByPolygonVertex) {//honestly uvs should always be by control point anyways, right? - wrong. turns out they're by polygon vertex.
		switch (uvPtr->GetMappingMode()) {
		case FbxGeometryElement::EMappingMode::eByControlPoint:
			echo("\t\t\tUVs are eByControlPoint; cannot import.");
			break;
		default:
			echo("\t\t\tUVs are not eByPolygonVertex; cannot import.");
			break;
		}
		return false;
	}

	switch (uvPtr->GetReferenceMode()) {
	case FbxGeometryElement::eDirect:
	case FbxGeometryElement::eIndexToDirect: {
			FbxDouble* uv = uvPtr->GetDirectArray().GetAt(uvIndex).mData;
			out_uv.x = uv[0];
			out_uv.y = uv[1];
		}
		break;
	default:
		echo("\t\t\tCan't read uvs, reference mode is neither eDirect nor eIndexToDirect...");
		return false;
	}

	return true;
}

void FBXMesh::render(ID3D11DeviceContext* deviceContext, LitShader* shader, D3D_PRIMITIVE_TOPOLOGY top) {
	//first setup the texture if we have one
	shader->setMaterialParameters(deviceContext, texture, normalMap, displacementMap, material);
	sendData(deviceContext, top);
	shader->render(deviceContext, getIndexCount());
}

void FBXMesh::sendData(ID3D11DeviceContext* deviceContext, D3D_PRIMITIVE_TOPOLOGY top) {
	unsigned int stride;
	unsigned int offset;

	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType_Tangent);
	offset = 0;

	deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	deviceContext->IASetPrimitiveTopology(top);
}

#undef VERBOSE
#undef echo