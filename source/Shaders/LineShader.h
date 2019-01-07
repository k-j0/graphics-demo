#pragma once
#include "Shader.h"
class LineShader : public Shader{
public:
	LineShader();
	~LineShader();

protected:
	inline void initBuffers() override {}
};
