#pragma once

///Pass one of these when importing an fbx to give the importer instructions on how to import it

struct FBXImportArgs {
	bool flipUVs = true;//flip the Y uvs upside down
	bool invertZScale = true;//turn to true to invert all the vertices on z axis
	bool invertWindingOrder = false;//turn to true to reverse the winding order when importing meshes from an fbx (Recommended: False)
};