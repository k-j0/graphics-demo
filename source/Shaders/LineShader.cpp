#include "LineShader.h"



LineShader::LineShader() {
	SETUP_SHADER(line_vs, line_fs);
}

LineShader::~LineShader(){
}
