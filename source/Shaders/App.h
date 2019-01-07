#pragma once

#include "DXF.h"
#include "DefaultShader.h"
#include "FBXScene.h"
#include "ExtendedLight.h"
#include "Material.h"
#include "PostProcessingPass.h"
#include "ColourGradingShader.h"
#include "LineShader.h"
#include "Line.h"
#include "SkinnedShader.h"
#include "DepthShader.h"
#include "SkinDepthShader.h"
#include "GaussianBlurShader.h"
#include "BloomShader.h"
#include "CombinationShader.h"
#include "TessellationShader.h"
#include "SquareMesh.h"
#include "TessellatedSkinnedShader.h"
#include "TessellationSkinDepthShader.h"
#include "TessellationDepthShader.h"
#include "ParticlesMesh.h"
#include "ParticlesShader.h"

class App : public BaseApplication {

public:
	App();
	~App();
	void init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN) override;

	bool frame() override;

	void handleInput(float dt) override;

protected:
	bool render() override;
	void geometry(LitShader* shader, SkinnedShader* skinnedShader, ParticlesShader* particlesShader, XMMATRIX& world, XMMATRIX& view, XMMATRIX& projection, XMFLOAT3 cameraPosition, bool sendShadowmaps, D3D_PRIMITIVE_TOPOLOGY top = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	void gui();

	void updateFov();

private:
	///Shaders for geometry
	LitShader* shader = nullptr;
	LineShader* lineShader = nullptr;
	DepthShader* depthShader = nullptr;
	SkinnedShader* skinnedShader = nullptr;
	SkinDepthShader* skinnedDepthShader = nullptr;
	TessellationShader* tessellationShader = nullptr;
	TessellationDepthShader* tessellationDepthShader = nullptr;
	TessellatedSkinnedShader* tessellatedSkinnedShader = nullptr;
	TessellationSkinDepthShader* tessellatedSkinnedDepthShader = nullptr;//these names are getting ridiculous
	ParticlesShader* particlesShader = nullptr;//better ^-^

	///Lighting
	ExtendedLight* lights = nullptr;
	ID3D11ShaderResourceView** shadowMaps = nullptr;
	int numLights;
	bool showShadowmaps = false;//Debug: show shadow maps

	///Meshes, materials, textures
	Material* material = nullptr;
	FBXScene* scene = nullptr;
	FBXScene* robot = nullptr;
	ParticlesMesh* particles = nullptr;
	bool renderScene = true;
	bool renderRobot = true;
	bool tessellate = true;
	bool lighting = true;
	bool showDepth = false;//when true, overlays the depth map on top of everything

	///Post processing shaders and passes
	PPTextureShader* textureShader;
	PostProcessingPass depthPass;
	ColourGradingShader* colourGrading;
	PostProcessingPass colourGradingPass;
	BloomShader* bloom;
	GaussianBlurShader* gaussian;
	PostProcessingPass bloomPass;
	PostProcessingPass vGaussianPass;
	CombinationShader* combine;
	PostProcessingPass combinationPass;
	bool showLut = false;//when true, displays the tonemapped LUT used by the colour grading effect
	bool disablePostProcessing = false;
	float fov = 60.0f;
	XMMATRIX projectionMatrix;

	///Animations for the robot
	Animation* leftRightAnimation;
	Animation* fallingAnimation;
	Animation* walkAnimation;
	Animation* runningAnimation;
	Animation* stealthAnimation;
	Animation* stealthLeftRightAnimation;
	bool renderSkeleton = false;//when true, displays the joints next to their skinned meshes
	float timeScale = 1;

	//Other animated stuff
	float lightBlink = 0;//makes street lamp blink somewhat randomly
	bool lightBlinkState = true;//false->off, true->on
	float robotYaw = 0;//makes robot turn slowly
	float cameraSpeed = 5;

	///Whether to show ui
	bool showUi = false;

};
