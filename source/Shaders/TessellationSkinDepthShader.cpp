#include "TessellationSkinDepthShader.h"



TessellationSkinDepthShader::TessellationSkinDepthShader() : SkinnedShader(false){
	SETUP_SHADER_SKIN(skinned_tessellated_vs, depth_fs);
	SETUP_TESSELATION(default_hs, depth_ds);
}


TessellationSkinDepthShader::~TessellationSkinDepthShader(){
}
