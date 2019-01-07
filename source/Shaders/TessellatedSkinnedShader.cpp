#include "TessellatedSkinnedShader.h"



TessellatedSkinnedShader::TessellatedSkinnedShader() : SkinnedShader(false) {
	SETUP_SHADER_SKIN(skinned_tessellated_vs, default_fs);
	SETUP_TESSELATION(default_hs, default_ds);
}


TessellatedSkinnedShader::~TessellatedSkinnedShader(){
}
