#include "App.h"

#include "AppGlobals.h"
#include "Utils.h"

#define LOWCOST_STARTUP false //set to true to disable post-processing by default
#define RES_PATH "res/"

App::App(){
}

App::~App(){
	if (leftRightAnimation)
		delete leftRightAnimation;
	if (fallingAnimation)
		delete fallingAnimation;
	if (walkAnimation)
		delete walkAnimation;
	if (runningAnimation)
		delete runningAnimation;
	if (stealthAnimation)
		delete stealthAnimation;
	if (stealthLeftRightAnimation)
		delete stealthLeftRightAnimation;

	if (scene)
		delete scene;
	if (robot)
		delete robot;
	if (lights)
		delete[] lights;
	if (shadowMaps)
		delete[] shadowMaps;
	if (material)
		delete material;
	if (particles)
		delete particles;

	if (shader)
		delete shader;
	if (lineShader)
		delete lineShader;
	if (skinnedShader)
		delete skinnedShader;
	if (depthShader)
		delete depthShader;
	if (textureShader)
		delete textureShader;
	if (skinnedDepthShader)
		delete skinnedDepthShader;
	if (tessellationShader)
		delete tessellationShader;
	if (tessellationDepthShader)
		delete tessellationDepthShader;
	if (tessellatedSkinnedShader)
		delete tessellatedSkinnedShader;
	if (tessellatedSkinnedDepthShader)
		delete tessellatedSkinnedDepthShader;
	if (particlesShader)
		delete particlesShader;

	if (colourGrading)
		delete colourGrading;
	if (gaussian)
		delete gaussian;
	if (bloom)
		delete bloom;
	if (combine)
		delete combine;

	//release fbx sdk objects (we've been keeping some around for animation purposes)
	FBXScene::Release();
}

void App::init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input *in, bool VSYNC, bool FULL_SCREEN){
	// Call super/parent init function (required!)
	BaseApplication::init(hinstance, hwnd, screenWidth, screenHeight, in, VSYNC, FULL_SCREEN);

	printf("Starting up.\n");

	//setup globals access to certain pointers & values
	GLOBALS.TextureManager = textureMgr;
	GLOBALS.Renderer = renderer;
	GLOBALS.Device = renderer->getDevice();
	GLOBALS.DeviceContext = renderer->getDeviceContext();
	GLOBALS.ScreenWidth = screenWidth;
	GLOBALS.ScreenHeight = screenHeight;
	GLOBALS.Hwnd = hwnd;

	//load textures
	textureMgr->loadTexture("lut", (WCHAR*)L"" RES_PATH "LUTs/Lut_blue.png");
	textureMgr->loadTexture("glow", (WCHAR*)L"" RES_PATH "glow.png");
	textureMgr->loadTexture("normalmap", (WCHAR*)L"" RES_PATH "Robot_01_Normal.png");
	textureMgr->loadTexture("displacementmap", (WCHAR*)L"" RES_PATH "Robot_01_Displacement.png");

	//initialize materials
	material = new Material;
	material->specularPower = 100.0f;

	//initialize shaders
	shader = new DefaultShader;
	lineShader = new LineShader;
	depthShader = new DepthShader;
	skinnedShader = new SkinnedShader;
	skinnedDepthShader = new SkinDepthShader;
	tessellationShader = new TessellationShader;
	tessellationDepthShader = new TessellationDepthShader;
	tessellatedSkinnedShader = new TessellatedSkinnedShader;
	tessellatedSkinnedDepthShader = new TessellationSkinDepthShader;
	particlesShader = new ParticlesShader;
	particlesShader->texture = textureMgr->getTexture("glow");
	particlesShader->size = 0.4f;
	textureShader = new PPTextureShader;
	colourGrading = new ColourGradingShader;
	colourGrading->fogColour = XMFLOAT3(9.f/255.f, 72.f/255.f, 72.f/255.f);//cool dark-ish blue (light blue looks awesome but then its broken by bloom so whatever)
	colourGrading->setLut(textureMgr->getTexture("lut"));
	colourGrading->setTonemapping(1);
	colourGrading->setExposure(1.687f);
	colourGrading->setBrightness(-0.072f);
	colourGrading->setContrast(0.434f);
	colourGrading->setHue(0);
	colourGrading->setSaturation(0.855f);
	colourGrading->setValue(1);
	colourGrading->vignette = 0.3f;
	bloom = new BloomShader;
	bloom->horizontal = true;
	bloom->distance = 200;
	bloom->threshold = 0.6f;
	bloom->intensity = 1.0f;
	gaussian = new GaussianBlurShader;
	gaussian->horizontal = false;
	gaussian->distance = 200;
	combine = new CombinationShader;
	colourGradingPass.Setup(GLOBALS.Device, GLOBALS.DeviceContext, GLOBALS.ScreenWidth, GLOBALS.ScreenHeight, colourGrading);
	bloomPass.Setup(GLOBALS.Device, GLOBALS.DeviceContext, GLOBALS.ScreenWidth * 0.7f, GLOBALS.ScreenHeight * 0.7f, bloom);
	vGaussianPass.Setup(GLOBALS.Device, GLOBALS.DeviceContext, GLOBALS.ScreenWidth * 0.7f, GLOBALS.ScreenHeight * 0.7f, gaussian);
	depthPass.Setup(GLOBALS.Device, GLOBALS.DeviceContext, GLOBALS.ScreenWidth, GLOBALS.ScreenHeight, textureShader);
	combinationPass.Setup(GLOBALS.Device, GLOBALS.DeviceContext, GLOBALS.ScreenWidth, GLOBALS.ScreenHeight, combine);
#if LOWCOST_STARTUP
	disablePostProcessing = true;//bloom & pp is expensive, so i don't necessarily want it by default (especially on my own laptop hehe)
#endif

	//initialize meshes
	FBXScene::Init();
	FBXImportArgs fbxArgs;
	scene = new FBXScene(renderer->getDevice(), renderer->getDeviceContext(), RES_PATH "scene/scene.fbx", fbxArgs);
	fbxArgs.invertZScale = false;//we need to keep z consistent with skeleton space for skinned meshes (instead we flip z in shader)
	robot = new FBXScene(renderer->getDevice(), renderer->getDeviceContext(), RES_PATH "Robo_01.fbx", fbxArgs);
	particles = new ParticlesMesh(1024, -72, 1, 27, 29, -14, 42);

	//extract animations
#define FRAME(f) f / 350.0f * 14.583333f //since the full take is 350 frames and 14.6 seconds long
#define ANIM(frameBegin, frameEnd) new Animation(FRAME(frameBegin), FRAME(frameEnd))
	leftRightAnimation = ANIM(1.f, 50.f);
	fallingAnimation = ANIM(51.f, 80.f);
	walkAnimation = ANIM(81.f, 107.f);
	runningAnimation = ANIM(108.f, 124.f);
	stealthAnimation = ANIM(125.f, 151.f);
	stealthLeftRightAnimation = ANIM(152.f, 201.f);
#undef FRAME
#undef ANIM
	if(robot) robot->getAnimator()->transitionTo(walkAnimation, 0);

	//initialize lighting
	numLights = 6;
	lights = new ExtendedLight[numLights];
	lights[0].setAmbientColour(42.f/255.f, 89.f/255.f, 109.f/255.f, 1);

	// Street lamp 1 (opposite car)
	lights[0].setDiffuseColour(1, 1, 1, 1);
	lights[0].setPosition(-28, 19, 30);
	lights[0].setDirection(0, -1, 1);
	lights[0].setAttenuation(1, 0, 0);
	lights[0].setType(SPOTLIGHT);
	lights[0].setupShadows();
	lights[0].updateFov(165.f);

	// Street lamp 2 (same side as car)
	lights[1].setDiffuseColour(1, 1, 0, 1);
	lights[1].setPosition(-11, 19, -7);
	lights[1].setDirection(0, -0.7f, -1);
	lights[1].setAttenuation(0.6f, 0.04f, 0);
	lights[1].setType(SPOTLIGHT);
	lights[1].setupShadows();
	lights[1].updateFov(165.f);

	// Directional cool light from alleyway
	lights[2].setDiffuseColour(82.f / 255.f, 170.f / 255.f, 167.f / 255.f, 1);
	lights[2].setPosition(-24, 15, -3.8f);
	lights[2].setDirection(-0.05f, -0.3f, 1);
	lights[2].setType(DIRECTIONAL_LIGHT);
	lights[2].setupShadows();

	//Right headlight
	lights[3].setDiffuseColour(1, 1, 0, 1);
	lights[3].setPosition(-59.4f, 3.38f, 19.9f);
	lights[3].setDirection(1.2f, 0, 14.f);
	lights[3].setAttenuation();
	lights[3].setType(SPOTLIGHT);
	lights[3].setupShadows();
	lights[3].updateFov(165.f);

	//Left headlight
	lights[4].setDiffuseColour(1, 1, 0, 1);
	lights[4].setPosition(-51.5f, 3.38f, 19.1f);
	lights[4].setDirection(1.2f, 0, 14.f);
	lights[4].setAttenuation();
	lights[4].setType(SPOTLIGHT);
	lights[4].setupShadows();
	lights[4].updateFov(165.f);

	// Directional warm light from second alleyway
	lights[5].setDiffuseColour(43.f / 255.f, 0.f / 255.f, 69.f / 255.f, 1);
	lights[5].setPosition(-41, 15, 38.f);
	lights[5].setDirection(0.05f, -0.2f, -1);
	lights[5].setType(DIRECTIONAL_LIGHT);
	lights[5].setupShadows();

	shadowMaps = new ID3D11ShaderResourceView*[numLights];

	//place camera
	camera->setPosition(-20, 4, 0);
	camera->setRotation(0, 90, 0);
	updateFov();

	//normal and displacement mapping to spice up the robot
	if(robot){
		robot->getMesh(0)->setNormalMap(textureMgr->getTexture("normalmap"));
		robot->getMesh(0)->setDisplacementMap(textureMgr->getTexture("displacementmap"));
		//i want extra shininess on that robot boy
		robot->getMesh(0)->setMaterialSpecular(5, 5, 5);
	}
}

///Change fov of projection matrix
void App::updateFov() {
	projectionMatrix = Utils::changeFov(renderer->getProjectionMatrix(), fov, (float)GLOBALS.ScreenWidth / GLOBALS.ScreenHeight);
}

bool App::frame(){
	//update base app
	if (!BaseApplication::frame())
		return false;

	//main logic; update animated robot + particles
	if (robot)
		robot->update(timer->getTime() * timeScale);
	particlesShader->step(timer->getTime() * timeScale);

	//Update other animated stuff
	//street lamp blinks
	lightBlink += timer->getTime();
	if (lightBlink > 5.0f) {
		lightBlinkState = !lightBlinkState;
		if (lightBlinkState) {
			lights[0].setType(SPOTLIGHT);//turn on
			lightBlink = Utils::random(-6, 0);//wait a bit
			if (lightBlink > -2) lightBlink = Utils::random(3, 5);//1 third of times, blinks much quicker
		}
		else {
			lights[0].setType(INACTIVE_LIGHT);//turn off
			lightBlink = Utils::random(4.9f, 5);//turn on quicker (in up to .1 seconds)
		}
	}
	//robot walks around
	robotYaw -= timer->getTime();

	//render to screen
	if (!render())
		return false;
	return true;
}

void App::handleInput(float dt) {
	//let the base application control the camera, but 5 times as fast
	BaseApplication::handleInput(dt*cameraSpeed);

	//right click to show debug ui
	if (input->isRightMouseDown()) {
		input->setRightMouse(false);
		showUi = !showUi;
	}
}

///Render geometry with any custom shaders
void App::geometry(LitShader* shader, SkinnedShader* skinnedShader, ParticlesShader* particlesShader, XMMATRIX& worldMatrix, XMMATRIX& viewMatrix, XMMATRIX& projectionMatrix, XMFLOAT3 cameraPosition, bool sendShadowmaps, D3D_PRIMITIVE_TOPOLOGY top) {
	//Scene
	shader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, cameraPosition);
	shader->setLightParameters(renderer->getDeviceContext(), cameraPosition, &lights, shadowMaps, sendShadowmaps, lighting? numLights : 0);
	if (scene != nullptr && renderScene) scene->render(shader, nullptr, false, top);

	//Animated robot
	//Robot walks around (see world matrix)
	skinnedShader->setShaderParameters(renderer->getDeviceContext(), XMMatrixTranslation(-3, 0, 0) * XMMatrixRotationY(robotYaw) * XMMatrixTranslation(-10, 0, -2.5f), viewMatrix, projectionMatrix, cameraPosition);
	//  Note: since shaders are packed the same way with the same registers, no need to re-send that same data here! :)
	//skinnedShader->setLightParameters(renderer->getDeviceContext(), cameraPosition, &lights, shadowMaps, sendShadowmaps, lighting? numLights : 0);
	if (robot != nullptr && renderRobot) robot->render(skinnedShader, lineShader, renderSkeleton, top);//use skinned shader to render robot

	//Particles
	if (particles && particlesShader) {
		particlesShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, cameraPosition);
		particlesShader->sendParticlesData();
		particles->sendData(GLOBALS.DeviceContext, D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		particlesShader->render(GLOBALS.DeviceContext, particles->getIndexCount());
	}
	
}

bool App::render(){
	XMMATRIX worldMatrix;
	
	//update camera position and get matrices
	camera->update();
	worldMatrix = renderer->getWorldMatrix();
	GLOBALS.ViewMatrix = camera->getViewMatrix();


// ** shadow mapping passes ** //

	for (int i = 0; i < numLights; ++i) {
		if (lights[i].StartRecordingShadowmap()) {

			//Render geometry from the point of view of this light
			XMMATRIX lightViewMatrix = lights[i].getView();
			XMMATRIX lightProjectionMatrix = lights[i].getProjection();

			geometry(depthShader, skinnedDepthShader, nullptr /*particles dont need to cast shadows*/, worldMatrix, lightViewMatrix, lightProjectionMatrix, lights[i].getPosition(), false);

		}
		//keep track of the shadowmap (might be null)
		shadowMaps[i] = lights[i].StopRecordingShadowmap();
	}


// ** depth pass ** //

	if (!wireframeToggle && !disablePostProcessing) {//no need to grab depths if we're not going to render fog
		depthPass.Begin(renderer, GLOBALS.DeviceContext, XMFLOAT3(0, 0, 0));

		//Render geometry to depth
		if (tessellate) {
			geometry(tessellationDepthShader, tessellatedSkinnedDepthShader, particlesShader/*depth!*/, worldMatrix, GLOBALS.ViewMatrix, projectionMatrix, camera->getPosition(), false, D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
		}
		else {
			geometry(depthShader, skinnedDepthShader, particlesShader/*depth!*/, worldMatrix, GLOBALS.ViewMatrix, projectionMatrix, camera->getPosition(), false);
		}

		depthPass.End(renderer);
	}


// ** render geometry ** //

	//clear scene
	if (!wireframeToggle && !disablePostProcessing) {
		if(colourGradingPass.enabled)
			colourGradingPass.Begin(renderer, renderer->getDeviceContext(), XMFLOAT3(0.2f, 0.2f, 0.3f));
		else if(bloomPass.enabled)
			bloomPass.Begin(renderer, renderer->getDeviceContext(), XMFLOAT3(0.2f, 0.2f, 0.3f));
		else renderer->beginScene(1, 1, 1, 1);

	} else //in wireframe mode, we don't want any post processing at all.
		renderer->beginScene(1, 1, 1, 1);

	// Geometry
	if (tessellate) {
		geometry(tessellationShader, tessellatedSkinnedShader, particlesShader, worldMatrix, GLOBALS.ViewMatrix, projectionMatrix, camera->getPosition(), true, D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	}
	else {
		geometry(shader, skinnedShader, particlesShader, worldMatrix, GLOBALS.ViewMatrix, projectionMatrix, camera->getPosition(), true);
	}


// ** post processing ** //

	//only do post processing and stuff if not in wireframe mode
	if (!wireframeToggle && !disablePostProcessing) {

		//end whichever is enabled (or none)
		colourGradingPass.End();
		bloomPass.End();

		// Finish up colour grading if it's enabled
		if(colourGradingPass.enabled){

			//Apply tonemapping
			bloomPass.Begin();
			colourGrading->setColourGrading(renderer, renderer->getDeviceContext(), camera->getOrthoViewMatrix(), depthPass.getRenderTexture()->getShaderResourceView());
			colourGradingPass.Render(camera->getOrthoViewMatrix());
			bloomPass.End();
		}

		//Finish up with bloom if it's enabled
		if (bloomPass.enabled) {

			//Blur horizontally and only keep brightest pixels
			vGaussianPass.Begin();
			bloom->setBlur();
			bloom->setBloom();
			bloomPass.Render(camera->getOrthoViewMatrix());
			vGaussianPass.End();

			//Blur vertically...
			combinationPass.Begin();
			gaussian->setBlur();
			vGaussianPass.Render(camera->getOrthoViewMatrix());
			combinationPass.End();

			//And combine results of bloom and what we had before starting bloom (ie colour grading or nothing)
			combine->setTexture1(bloomPass.getRenderTexture()->getShaderResourceView());
			combinationPass.Render(camera->getOrthoViewMatrix());
		}

		//Show depth if we need to
		if (showDepth) {
			depthPass.Render(camera->getOrthoViewMatrix());
		}

	}//!wireframe
	

// ** UI ** //

	//Show colour grading LUT
	if (showLut) colourGrading->showLut(renderer, camera->getOrthoViewMatrix());

	//Show shadowmaps
	if (showShadowmaps) {
		for (int i = 0; i < numLights; ++i) {
			ID3D11ShaderResourceView* map = lights[i].getShadowmap();
			if (map != nullptr) {
				#define MAP_SIZE 200.0f
				OrthoMesh ortho(GLOBALS.Device, GLOBALS.DeviceContext, MAP_SIZE, MAP_SIZE, -GLOBALS.ScreenWidth/2+MAP_SIZE/2+i*MAP_SIZE, -GLOBALS.ScreenHeight/2+MAP_SIZE/2);
				ortho.sendData(GLOBALS.DeviceContext);
				textureShader->setShaderParameters(GLOBALS.DeviceContext, worldMatrix, camera->getOrthoViewMatrix(), renderer->getOrthoMatrix(), XMFLOAT3(0,0,0));
				textureShader->setTextureData(GLOBALS.DeviceContext, map);
				textureShader->render(GLOBALS.DeviceContext, ortho.getIndexCount());
				#undef MAP_SIZE
			}
		}
	}

	//Show ui
	gui();

	//swap buffers
	renderer->endScene();

	return true;
}

void App::gui()
{
	// Force turn off unnecessary shader stages.
	renderer->getDeviceContext()->GSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->HSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->DSSetShader(NULL, NULL, 0);

	// Display FPS
	ImGui::Text("FPS: %.2f", timer->getFPS());

	// Enabling/disabling stuff
	if (ImGui::CollapsingHeader("Scene parameters")) {
		ImGui::Checkbox("Wireframe mode", &wireframeToggle);
		ImGui::Checkbox("Disable post processing", &disablePostProcessing);
		ImGui::Checkbox("View scene", &renderScene);
		ImGui::Checkbox("View robot", &renderRobot);
		ImGui::Checkbox("Show depth", &showDepth);
		float newFov = fov;
		ImGui::SliderFloat("Fov", &newFov, 1, 180);
		if (newFov != fov) {
			fov = newFov;
			updateFov();
		}
		ImGui::SliderFloat("Far plane", &GLOBALS.FarPlane, 1, 100);
		ImGui::Text("Camera pos %f %f %f", camera->getPosition().x, camera->getPosition().y, camera->getPosition().z);
		ImGui::SliderFloat("Camera speed", &cameraSpeed, 0, 10);
	}

	//Tessellation params
	if (ImGui::CollapsingHeader("Dynamic tessellation")) {
		ImGui::Checkbox("Tessellate", &tessellate);
		ImGui::SliderFloat("Minimum tessellation", &GLOBALS.TessellationMin, 1, GLOBALS.TessellationMax);
		ImGui::SliderFloat("Maximum tessellation", &GLOBALS.TessellationMax, GLOBALS.TessellationMin, 64);
		ImGui::SliderFloat("Tessellation range", &GLOBALS.TessellationRange, 0, 1);
		ImGui::SliderFloat("Displacement scale", &GLOBALS.DisplacementScale, 0, 1);
	}

	//Lighting params
	if (ImGui::CollapsingHeader("Lighting")) {
		ImGui::Checkbox("Apply lighting", &lighting);
		ImGui::Checkbox("Normal mapping", &GLOBALS.normalMapping);
		ImGui::Text("Shadowmap resolution:");
		if (ImGui::Button("256")) for (int i = 0; i < numLights; ++i) lights[i].setShadowmapRes(256);
		if (ImGui::Button("512")) for (int i = 0; i < numLights; ++i) lights[i].setShadowmapRes(512);
		if (ImGui::Button("1028")) for (int i = 0; i < numLights; ++i) lights[i].setShadowmapRes(1028);
		if (ImGui::Button("2048")) for (int i = 0; i < numLights; ++i) lights[i].setShadowmapRes(2048);
		if (ImGui::Button("4096")) for (int i = 0; i < numLights; ++i) lights[i].setShadowmapRes(4096);
		ImGui::SliderFloat("Shadowmap bias", &GLOBALS.ShadowmapBias, 0, 0.01f);
		ImGui::Checkbox("Show shadowmaps", &showShadowmaps);
		ImGui::Checkbox("Show out of range shadowmaps", &GLOBALS.ShadowmapSeeErrors);
		//Control for each light:
		float amb[3] = { lights[0].getAmbientColour().x, lights[0].getAmbientColour().y, lights[0].getAmbientColour().z };
		if (ImGui::ColorEdit3("Ambient colour", amb))
			lights[0].setAmbientColour(amb[0], amb[1], amb[2], 1);
		for (int i = 0; i < numLights; ++i) {
			if (ImGui::CollapsingHeader(std::string("Light " + std::to_string(i)).c_str())) {
				//Position
				float pos[3] = { lights[i].getPosition().x, lights[i].getPosition().y, lights[i].getPosition().z };
				if(ImGui::SliderFloat3(std::string("Position (" + std::to_string(i) + ")").c_str(), pos, -30, 30))
					lights[i].setPosition(pos[0], pos[1], pos[2]);
				//Direction
				float dir[3] = { lights[i].getDirection().x, lights[i].getDirection().y, lights[i].getDirection().z };
				if (ImGui::SliderFloat3(std::string("Direction (" + std::to_string(i) + ")").c_str(), dir, -1, 1))
					if (dir[0] != 0 || dir[1] != 0 || dir[2] != 0)//otherwise might crash
						lights[i].setDirection(dir[0], dir[1], dir[2]);
				//Diffuse
				float dif[3] = { lights[i].getDiffuseColour().x, lights[i].getDiffuseColour().y, lights[i].getDiffuseColour().z };
				if (ImGui::ColorEdit3(std::string("Diffuse (" + std::to_string(i) + ")").c_str(), dif))
					lights[i].setDiffuseColour(dif[0], dif[1], dif[2], 1);
				//Attenuation
				float att[3] = { lights[i].getAttenuation().x, lights[i].getAttenuation().y, lights[i].getAttenuation().z };
				ImGui::SliderFloat(std::string("Constant attenuation (" + std::to_string(i) + ")").c_str(), &att[0], 0, 2);
				ImGui::SliderFloat(std::string("Linear attenuation (" + std::to_string(i) + ")").c_str(), &att[1], 0, 1);
				ImGui::SliderFloat(std::string("Quadratic attenuation (" + std::to_string(i) + ")").c_str(), &att[2], 0, 1);
				lights[i].setAttenuation(att[0], att[1], att[2]);
				if (ImGui::Button(std::string("Reset attenuation (" + std::to_string(i) + ")").c_str()))
					lights[i].setAttenuation();
				//Shadowmap size
				float sz = lights[i].getShadowmapSize();
				if (ImGui::SliderFloat(std::string("Shadowmap size (" + std::to_string(i) + ")").c_str(), &sz, 1, 100))
					lights[i].setShadowmapSize(sz);
				//Shadowmap projection fov
				float fv = lights[i].getProjectionFov();
				if (ImGui::SliderFloat(std::string("Shadowmap projection fov (" + std::to_string(i) + ")").c_str(), &fv, 1, 180))
					lights[i].updateFov(fv);
			}
		}
	}

	// Colour grading params
	if (ImGui::CollapsingHeader("Colour Grading")) {
		ImGui::Checkbox("Apply colour grading", &colourGradingPass.enabled);
		colourGrading->gui();
		ImGui::Checkbox("Show tonemapped LUT", &showLut);
	}

	// Bloom params
	if (ImGui::CollapsingHeader("Bloom")) {
		ImGui::Checkbox("Apply bloom", &bloomPass.enabled);
		ImGui::SliderFloat("Threshold", &bloom->threshold, 0, 3);
		ImGui::SliderFloat("Intensity", &bloom->intensity, 0, 5);
		int blurDist = gaussian->distance;
		ImGui::SliderInt("Blur distance", &blurDist, 1, MAX_NEIGHBOURS);
		gaussian->distance = blurDist;
		bloom->distance = blurDist;
	}

	// Particles params
	if (ImGui::CollapsingHeader("Particles")) {
		ImGui::SliderFloat("Size", &particlesShader->size, 0, 5);
		ImGui::SliderFloat("Speed", &particlesShader->maxSpeed, 0, 100);
	}

	// Animations menu
	if (ImGui::CollapsingHeader("Animations")) {
		ImGui::Text("Use these buttons to change which animation the Robot uses.");
		if (ImGui::Button("Left Right"))
			robot->getAnimator()->transitionTo(leftRightAnimation, 1.0f);
		if (ImGui::Button("Fall"))
			robot->getAnimator()->transitionTo(fallingAnimation, 1.0f);
		if (ImGui::Button("Walk"))
			robot->getAnimator()->transitionTo(walkAnimation, 1.0f);
		if (ImGui::Button("Run"))
			robot->getAnimator()->transitionTo(runningAnimation, 1.0f);
		if (ImGui::Button("Stealth"))
			robot->getAnimator()->transitionTo(stealthAnimation, 1.0f);
		if (ImGui::Button("Stealth Left Right"))
			robot->getAnimator()->transitionTo(stealthLeftRightAnimation, 1.0f);
		ImGui::Checkbox("Render bones", &renderSkeleton);
		ImGui::SliderFloat("Timescale", &timeScale, 0, 1);
	}

	// Render UI
	ImGui::Render();
	if(showUi) ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}