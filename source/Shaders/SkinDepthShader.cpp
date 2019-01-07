#include "SkinDepthShader.h"



SkinDepthShader::SkinDepthShader() : SkinnedShader(false) {
	SETUP_SHADER_SKIN(skindepth_vs, depth_fs);
}


SkinDepthShader::~SkinDepthShader(){
}
