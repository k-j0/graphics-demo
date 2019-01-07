#include "TessellationDepthShader.h"



TessellationDepthShader::TessellationDepthShader(){
	SETUP_SHADER_TANGENT(tessellated_vs, depth_fs);
	SETUP_TESSELATION(default_hs, depth_ds);
}


TessellationDepthShader::~TessellationDepthShader(){
}
