#pragma once

#include "DXF.h"
#include <fbxsdk.h>
#include <string>
#include "LitShader.h"
#include "FBXImportArgs.h"

class FBXMesh : public BaseMesh {
protected:
	///Vertex struct for geometry with position, texture, normals and tangents
	struct VertexType_Tangent {
		XMFLOAT3 position;
		XMFLOAT2 texture;
		XMFLOAT3 normal;
		XMFLOAT3 tangent;
	};

public:
	FBXMesh();
	virtual ~FBXMesh();

	///imports a mesh from an FbxMesh object
	virtual void importMesh(FbxMesh* fbxMesh, FbxSurfaceMaterial* material, std::string folderPath, FBXImportArgs& args, ID3D11Device* device, ID3D11DeviceContext* deviceContext);

	///renders the FBXMesh using the correct material setup
	void render(ID3D11DeviceContext* deviceContext, LitShader* shader, D3D_PRIMITIVE_TOPOLOGY top = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	virtual void sendData(ID3D11DeviceContext *deviceContext, D3D_PRIMITIVE_TOPOLOGY top = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST) override;

	inline void setNormalMap(ID3D11ShaderResourceView* map) { normalMap = map; }
	inline void setDisplacementMap(ID3D11ShaderResourceView* map) { displacementMap = map; }
	inline void setMaterialSpecular(float r, float g, float b) { material->specularColour = XMFLOAT3(r, g, b); }

protected:
	virtual void initBuffers(ID3D11Device* device) override;

	//processes the FbxMesh data to get normal for specific index; returns whether it was successful
	bool getNormalForIndex(FbxMesh* fbxMesh, int index, XMFLOAT3& out_normal);

	//processes the FbxMesh data to get tangent for specific index; returns whether it was successful
	bool getTangentForIndex(FbxMesh* fbxMesh, int vertexCounter, XMFLOAT3& out_tangent);

	//processes the FbxMesh data to get uv for specific index; returns whether it was successful
	bool getUVForIndex(FbxMesh* fbxMesh, int index, int uvIndex, XMFLOAT2& out_uv);

	///Imports material and texture data
	void importMaterials(FbxSurfaceMaterial* material, std::string folderPath);

	///the two following are initialized in importMesh and become nullptr almost immediately in initBuffer.
	VertexType_Tangent* vertices;
	unsigned long* indices;

private:
	///diffuse texture to apply to this mesh when rendering
	ID3D11ShaderResourceView* texture = nullptr;
	ID3D11ShaderResourceView* normalMap = nullptr;
	ID3D11ShaderResourceView* displacementMap = nullptr;
	///Material to apply to this mesh when rendering
	Material* material;
};
