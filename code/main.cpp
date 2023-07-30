
//TODO: Rearrange to put all struct defs in one header file,
//probably in types.h. Also remove inconsitency between
//my objectless style and the lightweight objects ie QuadBuffer

#include "sdl/SDL.h"
#include "cstdio"
#include "windows.h"
#include "glew/GL/glew.h"
#define GLM_FORCE_SWIZZLE
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/quaternion.hpp"
#include <cstddef>
#include <string>
#include <vector>

#include "memory.h"
#include "log.h"
#include "types.h"
#include "transform.cpp"
#include "rendering.cpp"
#include "resources.cpp"
#include "input.cpp"
#include "level.cpp"
#include "debug_tools.cpp"
#include "physics.cpp"
#include "gameplay.cpp"



const int FPS_FRAME_AVG = 4;

//NOTE: Im currently setting both of these to force laptops that have both intel gfx and dedicated gfx.
// Im not entirely sure if setting them on unsupported devices causes any problems, would have to test on
// other hardware to know for sure
extern "C"
{
 	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
 	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

static bool LockCursor;

void ChangeWindowFocus(bool lock)
{
	SDL_SetRelativeMouseMode(lock?SDL_TRUE:SDL_FALSE);
	LockCursor = lock;
}


int main(int argc, char* argv[])
{
	GameState gameState = {0};
    gameState.entityManager = EntityManager();
    gameState.timeScale = 1.0f;
	GameMemory Memory;
	Memory.Allocate();


	Timer timer;
	timer.Mark();

#ifdef DEBUG
	float64 fps = 0;
	float64 frameTimes = 0;
	uint32 frameCount = 1;
#endif

#ifdef WINDOWS
	//This disables the system dpi scaling that occurs on some 4k screens
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
#endif


	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		printf("Failed to initialize SDL: %s\n", SDL_GetError());
		return 1;
	}

	Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
	//Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP;


	//TODO:Fix scaling issue with windows high dpi for 4k screens, want to disable scaling for this app
	SDL_DisplayMode display;
	SDL_GetDesktopDisplayMode(0, &display);

	gameState.application.windowWidth = display.w/2;
	gameState.application.windowHeight = display.h/2;
	gameState.application.aspectRatio = (float32)gameState.application.windowWidth / (float32)gameState.application.windowHeight;

	gameState.application.window = SDL_CreateWindow("Think", SDL_WINDOWPOS_UNDEFINED,
	 	SDL_WINDOWPOS_UNDEFINED, gameState.application.windowWidth, gameState.application.windowHeight, windowFlags);

	if (!gameState.application.window)
	{
		LOG_ERROR("SDL could not open a window: %s\n", SDL_GetError());
		return 1;
	}
	ChangeWindowFocus(true);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef DEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
	SDL_GLContext glContext = SDL_GL_CreateContext(gameState.application.window);
	if (!glContext)
	{
		LOG_ERROR("SDL could not create a GL context: %s\n", SDL_GetError());
		return 1;
	}

	GLenum glewResult = glewInit();
	if (glewResult != GLEW_OK)
	{
		LOG_ERROR("Could not initialize GLEW");
		return 1;
	}

	#ifdef DEBUG
	int32 flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}
	#endif
	SDL_GL_SetSwapInterval(1);

	const GLubyte* vendor = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	LOG_INFO("Current graphics device is %s\n", vendor);
	LOG_INFO("Current OpenGL version is %s\n", version);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearColor(0.1f, 0.105f, 0.13f, 1.0f);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);

	UnicodeGlyphAtlas DefaultFontAtlas = LoadFont("BalooBhaijaan2-Regular");

	//Testing meshes
	Mesh axis = LoadModel("resources/models/axis.gltf", Memory);
	Mesh sphere = LoadModel("resources/models/Sphere.gltf", Memory);
    uint32 axisBuffer = UploadMeshToGPU(axis, Memory);
    uint32 sphereBuffer = UploadMeshToGPU(sphere, Memory);
    uint32 mat = LoadMaterial("resources/materials/player mat.mat", Memory);

    auto playerTransform = Memory.transforms.RequestFromBlock(1);
    playerTransform.memory->position = vec3f(10.0f, 1.0f, 10.0f);
    playerTransform.memory->rotation = quaternion(1.0f, 0.0f, 0.0f, 0.0f);
    playerTransform.memory->scale    = vec3f(1.0f);
    uint32 testPlayer = sphereBuffer;//UploadMeshToGPU(playerModel.meshes[1].mesh, Memory);
    gameState.entityManager.playerEntity = playerTransform.index;

    //RenderEntity* playerRenderMesh = gameState.entityManager.CreateRenderEntity(request.index, testPlayer,mat);
	gameState.entityManager.CreateRenderEntity(playerTransform.index, testPlayer,mat);

    auto cameraRequest = Memory.transforms.RequestFromBlock(1);
    cameraRequest.memory->position = playerTransform.memory->position + vec3f(0.0f, 5.01f, -15.0f);
    cameraRequest.memory->rotation = quaternion(vec3f(glm::radians(45.0f), 0.0f, 0.0f));
    cameraRequest.memory->scale    = vec3f(1.0f);
    gameState.entityManager.cameraEntity = cameraRequest.index;

	//Temporary
	GeometryNodeAssetTable NodeAssets;
	NodeAssets.nodeSetFilePaths[0] = "resources/models/Node Sets/base.gltf";
	NodeAssets.nodeSetFilePaths[1] = "resources/models/Node Sets/base.gltf";
	NodeAssets.nodeSetFilePaths[2] = "resources/models/Node Sets/base.gltf";
	NodeAssets.nodeSetFilePaths[3] = "resources/models/Node Sets/base.gltf";

	NodeAssets.materialFilePaths[0] = "resources/materials/test mat 1.mat";
	NodeAssets.materialFilePaths[1] = "resources/materials/test mat 2.mat";
	NodeAssets.materialFilePaths[2] = "resources/materials/test mat 3.mat";
	NodeAssets.materialFilePaths[3] = "resources/materials/test mat 4.mat";
	//end temporary

	ShaderAssetTable ShaderTable;
	ShaderTable.shaderAssetFilePaths[ShaderType::VertexColorUnlit] = {
		"resources/shaders/Standard_Vertex.glsl",
		"resources/shaders/VertexColorUnlit.glsl" };
	ShaderTable.shaderAssetFilePaths[ShaderType::VertexColorLit] = {
		"resources/shaders/Standard_Vertex.glsl",
		"resources/shaders/VertexColorLit.glsl" };
	ShaderTable.shaderAssetFilePaths[ShaderType::TriplanarLit] = {
		"resources/shaders/Standard_Vertex.glsl",
		"resources/shaders/TriplanarLit.glsl" };

	for (int i = 0; i < ShaderType::Count; i++)
	{
		ShaderAsset& asset = ShaderTable.shaderAssetFilePaths[i];
		ShaderTable.compiledShaders[i] = LoadShader(
			asset.vertexShaderFilePath.c_str(),
			asset.fragmentShaderFilePath.c_str());
	}

	NodeSetPalette<MAX_NODE_TYPES> NodePalette;
	for (int i = 0; i < MAX_NODE_TYPES; i++)
	{
		NodePalette.sets[i] = LoadNodeSet(NodeAssets.nodeSetFilePaths[i].c_str(), Memory);
		NodePalette.materials[i] = LoadMaterial(NodeAssets.materialFilePaths[i].c_str(), Memory);
	}

	LevelData levelData =  LoadLevelData("resources/levels/level_0.level", Memory);

	Texture2D texture = LoadTexture("resources/textures/stone.png");
	

	gameState.state = GameStateGameplay;
	gameState.level = GenerateLevelCells(levelData, Memory);


	GameInput gameInput = {0};
	SDL_Event appEvent = {0};
	mat4f cameraProjection = glm::perspectiveLH(glm::radians(60.0f), gameState.application.aspectRatio, 0.1f, 500.0f);
	float32 cameraSpeed = 15.0f;
	float32 cameraSensitivity = 800.0f;

	DebugMemory Debug;
#ifdef DEBUG
	Debug.Initialize();
    Debug.DebugShadowmap = false;
    Debug.DebugPlayerCollision = false;
    Debug.DebugColliders = false;
    Debug.FontAtlas = DefaultFontAtlas;

	//---Shader Debugging-----------------
	uint32 debugShader = LoadShader(
		"resources/shaders/Screenspace_Vertex.glsl",
		"resources/shaders/debug/Debug_Fragment.glsl");

	Mesh screenQuad;
	Vertex verts[4] =
	{
		Vertex(vec3f( 0.4f, 0.4f,0.0), vec3f(), vec2f(0.0,0.0), vec4f()),
		Vertex(vec3f( 0.4f, 1.0f,0.0), vec3f(), vec2f(0.0,1.0), vec4f()),
		Vertex(vec3f( 1.0f, 1.0f,0.0), vec3f(), vec2f(1.0,1.0), vec4f()),
		Vertex(vec3f( 1.0f, 0.4f,0.0), vec3f(), vec2f(1.0,0.0), vec4f())
	};
	uint16 indices[6] = { 0,1,2,0,2,3 };
	screenQuad.vertexCount = 4;
	screenQuad.indexCount = 6;
	screenQuad.vertices = verts;
	screenQuad.indices = indices;
	uint32 screenQuadBuffer = UploadMeshToGPU(screenQuad, Memory);

	//-------------------------------------
#endif

	GLuint meshVAO;
	glGenVertexArrays(1, &meshVAO);
	glBindVertexArray(meshVAO);

	ApplyVertexAttribute(0, offsetof(Vertex, position), 3);
	ApplyVertexAttribute(1, offsetof(Vertex, normal)  , 3);
	ApplyVertexAttribute(2, offsetof(Vertex, uv0)     , 2);
	ApplyVertexAttribute(3, offsetof(Vertex, color)   , 4);

	uint32 CameraBuffer      = CreateUniformBuffer(sizeof(CameraData), 0);
	uint32 ObjectBuffer      = CreateUniformBuffer(sizeof(ObjectData), 1);
	uint32 EnvironmentBuffer = CreateUniformBuffer(sizeof(EnvironmentData), 2);
	uint32 MainLightBuffer   = CreateUniformBuffer(sizeof(Light), 3);
	uint32 PointLightBuffer  = CreateUniformBuffer(sizeof(Light)*MAX_POINT_LIGHTS, 4);

	GeometryBuffer GeometryRenderBuffer;
	for (int i = 0; i < MAX_NODE_TYPES; i++)
	{
		int blockIndex = gameState.level.nodeTypeGeometyBufferMap[i];
		if (blockIndex < 0)
		{
			GeometryRenderBuffer.meshes[i].vertices = NULL;
			GeometryRenderBuffer.meshes[i].indices  = NULL;
			GeometryRenderBuffer.meshes[i].vertexCount = 0;
			GeometryRenderBuffer.meshes[i].indexCount  = 0;

		}
		else
		{
			GeometryRenderBuffer.meshes[i].vertices = Memory.geometryMeshVertexBlock.block+(blockIndex*GEO_MESH_MAX_VERTS);
			GeometryRenderBuffer.meshes[i].indices  = Memory.geometryMeshIndexBlock.block +(blockIndex*GEO_MESH_MAX_INDEX);
			GeometryRenderBuffer.meshes[i].vertexCount = 0;
			GeometryRenderBuffer.meshes[i].indexCount  = 0;
		}
	}

	for (int i = 0; i < MAX_NODE_TYPES; i++)
	{
		GeometryRenderBuffer.handles[i] = CreateEmptyMeshBuffer();
	}

	UpdateGeometryBuffer(gameState.level, NodePalette, GeometryRenderBuffer);

	CameraData cameraData;
	EnvironmentData environmentData;
	environmentData.ambientLightTop = { 0.95f, 0.92f, 0.91f, 1.0f };
	environmentData.ambientLightBottom = { 0.5f, 0.55f, 0.7f, 1.0f };
	//environmentData.viewPosition = gameState.cameraState.transform[3];


	Light directionalLight;
	directionalLight.position = { -50.0f,30.0f,-50.0f,0.0f };
	directionalLight.color = { 1.0f,0.975f,0.75f, 5.0f };
	float32 shadowArea = 20.0f;
	mat4f directionalLightTransform = glm::lookAtLH(vec3f(directionalLight.position), vec3f(0.0,0.0,0.0), vec3f(0.0f,0.0f,1.0f));
	mat4f directionalLightProjection = glm::orthoLH(-shadowArea,shadowArea,-shadowArea,shadowArea,10.0f,100.0f);
	cameraData.mainLightViewProjection = directionalLightProjection*directionalLightTransform;
	vec4f mainLightToCameraOffset = directionalLightTransform[3]+vec4f(cameraRequest.memory->position,1);

	Light pointLights[MAX_POINT_LIGHTS];
	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		pointLights[i].color     = { 1.0f,0.975f,0.75f, 10.0f };
		pointLights[i].lightType = LightType::Point;
	}
	pointLights[0].position = { 5.0f, 3.0f, 5.0f, 0.0f };
    pointLights[1].position = { 5.0f, 3.0f, 15.0f, 0.0f };
    pointLights[2].position = { 15.0f, 3.0f, 15.0f, 0.0f };
    pointLights[3].position = { 15.0f, 3.0f, 5.0f, 0.0f };

	SetUniformBufferData(MainLightBuffer, sizeof(Light), (const void*)(&directionalLight));
	SetUniformBufferData(PointLightBuffer, sizeof(Light)* MAX_POINT_LIGHTS, (const void*)(&pointLights));

	//---Shadow mapping initialization----
	Texture2D shadowMap;
	shadowMap.width = 2048;
	shadowMap.height = 2048;
	CreateTexture(shadowMap, NULL, false, GL_DEPTH_COMPONENT32,
		 GL_DEPTH_COMPONENT, GL_FLOAT, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_BORDER);
	float32 white[] = {1.0f,1.0f,1.0f,1.0f};

	uint32 shadowSamplerDepthComp;
	glGenSamplers(1, &shadowSamplerDepthComp);
	glSamplerParameteri(shadowSamplerDepthComp, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glSamplerParameteri(shadowSamplerDepthComp, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameterfv(shadowSamplerDepthComp, GL_TEXTURE_BORDER_COLOR, white);
	glSamplerParameteri(shadowSamplerDepthComp, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glSamplerParameteri(shadowSamplerDepthComp, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glSamplerParameteri(shadowSamplerDepthComp, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glSamplerParameteri(shadowSamplerDepthComp, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	uint32 shadowSamplerDepth;
	glGenSamplers(1, &shadowSamplerDepth);
	glSamplerParameteri(shadowSamplerDepth, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glSamplerParameteri(shadowSamplerDepth, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameterfv(shadowSamplerDepth, GL_TEXTURE_BORDER_COLOR, white);
	glSamplerParameteri(shadowSamplerDepth, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glSamplerParameteri(shadowSamplerDepth, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glBindSampler(0, shadowSamplerDepthComp);
	glBindTextureUnit(0, shadowMap.glID);
	glBindSampler(1, shadowSamplerDepth);
	glBindTextureUnit(1, shadowMap.glID);

	uint32 shadowFBO;
	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap.glID, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	uint32 shadowPassShader = LoadShader
		("resources/shaders/ShadowPass_Vertex.glsl",
		"resources/shaders/ShadowPass_Fragment.glsl");

	glUseProgram(shadowPassShader);
	int lightVPShaderLocation = glGetUniformLocation(shadowPassShader, "LightVP");
	ASSERT((lightVPShaderLocation >= 0), __FILE__, __LINE__);
	/*int shadowMapShaderLocation = glGetUniformLocation(shader, "ShadowMap");
	ASSERT(shadowMapShaderLocation >= 0, __FILE__, __LINE__);*/
	//------------------------------------

	LOG_INFO("Load time: %f s\n", timer.ElapsedTime().count());

	InputMap inputMap;
	inputMap.keys[GameActionTypeMoveUp]     = SDL_SCANCODE_W;
	inputMap.keys[GameActionTypeMoveDown]   = SDL_SCANCODE_S;
	inputMap.keys[GameActionTypeMoveLeft]   = SDL_SCANCODE_A;
	inputMap.keys[GameActionTypeMoveRight]  = SDL_SCANCODE_D;
	inputMap.keys[GameActionTypeLookUp]     = (SDL_Scancode)-1;
	inputMap.keys[GameActionTypeLookDown]   = (SDL_Scancode)-1;
	inputMap.keys[GameActionTypeLookLeft]   = (SDL_Scancode)-1;
	inputMap.keys[GameActionTypeLookRight]  = (SDL_Scancode)-1;
	inputMap.keys[GameActionTypeDodge]      = SDL_SCANCODE_LSHIFT;
	inputMap.keys[GameActionTypeLightSwing] = SDL_SCANCODE_LEFT;
	inputMap.keys[GameActionTypeHeavySwing] = SDL_SCANCODE_UP;
    inputMap.keys[GameActionTypeJump]       = SDL_SCANCODE_SPACE;


	bool Running = true;
	while (Running)
	{
#ifdef DEBUG
		Debug.TextBuffer.Clear();
		Debug.LineBuffer.Clear();
		timer.Mark();
#endif

		bool mouseMovement = false;
		//TODO(Cameron): Fix issue with controller not sending
		//input events but is sending device add/remove events
		while (SDL_PollEvent(&appEvent))
		{
			switch (appEvent.type)
			{
				case SDL_QUIT:
				{
					Running = false;
					break;
				}

				case SDL_WINDOWEVENT:
				{
					if (appEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
					{
						int32 x = appEvent.window.data1;
						int32 y = appEvent.window.data2;
						glViewport(0, 0, x, y);
						gameState.application.windowWidth = x;
						gameState.application.windowHeight = y;
					}
					else if (appEvent.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
					{
						ChangeWindowFocus(true);
					}
					else if (appEvent.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
					{
						ChangeWindowFocus(false);
					}
					break;
				}

				case SDL_KEYDOWN:
				case SDL_KEYUP:
				{
					bool press = appEvent.key.state == SDL_PRESSED;
					if (appEvent.key.repeat > 0) break;

					SDL_Scancode key = appEvent.key.keysym.scancode;
					for (uint16 i = 0; i < GameActionTypeCount; i++)
					{
						if (inputMap.keys[i] == key)
						{
							gameInput.actions[i].state = (float32)press;
							gameInput.actions[i].beingPressed = (float32)press;
							gameInput.actions[i].beingReleased = (float32)!press;
						}
					}

#ifdef DEBUG
                    if (press)
                    {
					switch (appEvent.key.keysym.scancode)// physical position of keys (based on qwerty)
					{
						case SDL_SCANCODE_ESCAPE:
							ChangeWindowFocus(false);
							break;
						case SDL_SCANCODE_O:
							gameState.state = gameState.state == GameStateGameplay ? GameStateDevFly : GameStateGameplay;
							break;
						case SDL_SCANCODE_R:
                        {
                            vec3f& playerPos = (Memory.transforms.block[gameState.entityManager.playerEntity].position);
							playerPos = vec3f(5.0f,1.0f,5.0f);
							break;
                        }
                        case SDL_SCANCODE_P:
                            Debug.DebugShadowmap = !Debug.DebugShadowmap;
                            break;
                        case SDL_SCANCODE_J:
                            Debug.DebugColliders = !Debug.DebugColliders;
                            break;
                        case SDL_SCANCODE_K:
                            Debug.DebugPlayerCollision = !Debug.DebugPlayerCollision;
                            break;
                        case SDL_SCANCODE_LEFTBRACKET:
                            gameState.timeScale *= 0.5f;
                            break;
                        case SDL_SCANCODE_RIGHTBRACKET:
                            gameState.timeScale *= 2.0f;
                            break;
					}
                    }
#endif

					break;
				}

				case SDL_MOUSEMOTION:
				{
					float32 x = (float32)appEvent.motion.xrel/gameState.application.windowWidth;
					float32 y = (float32)appEvent.motion.yrel/gameState.application.windowHeight;
					gameInput.actions[GameActionTypeLookUp].state    = glm::max(0.0f, y);
					gameInput.actions[GameActionTypeLookDown].state  = glm::min(0.0f, y);
					gameInput.actions[GameActionTypeLookLeft].state  = glm::min(0.0f, x);
					gameInput.actions[GameActionTypeLookRight].state = glm::max(0.0f, x);
					mouseMovement = true;
					break;
				}

				case SDL_MOUSEBUTTONDOWN:
				{

					break;
				}

				case SDL_CONTROLLERDEVICEADDED:
				{

					break;
				}

				case SDL_CONTROLLERDEVICEREMOVED:
				{

					break;
				}

				case SDL_CONTROLLERBUTTONUP:
				{

					break;
				}
			}
		}
		//TODO: warping mouse creates different movement deltas,
 		//find better way to constrain mouse
		//if (LockCursor)
		//	SDL_WarpMouseInWindow(app.window, app.windowWidth / 2, app.windowHeight / 2);


		UpdateGame(gameState, gameInput, Memory, Debug);

		//Reset game input states
		for (uint16 i = 0; i < GameActionTypeCount; i++)
		{
			gameInput.actions[i].beingPressed = false;
			gameInput.actions[i].beingReleased = false;
		}
		if (!mouseMovement)
		{
			gameInput.actions[GameActionTypeLookUp].state    = 0.0f;
			gameInput.actions[GameActionTypeLookDown].state  = 0.0f;
			gameInput.actions[GameActionTypeLookLeft].state  = 0.0f;
			gameInput.actions[GameActionTypeLookRight].state = 0.0f;
		}

        Transform* transforms = Memory.transforms.block;
        MeshBufferHandle* meshBuffers = Memory.meshBufferHandles.block;
        Material* materials = Memory.materials.block;

        mat4f cameraTransform = glm::identity<mat4f>();
        TRS(cameraTransform, transforms[gameState.entityManager.cameraEntity]);

        //hacky way to move light matrix to fit screen
        //TODO: find better way to fit light matrix to scene
		directionalLightTransform[3] = -cameraTransform[3]+mainLightToCameraOffset;

        //Setup camera matrices
		cameraData.cameraViewProjection = cameraProjection * glm::inverse(cameraTransform);
		cameraData.mainLightViewProjection = directionalLightProjection * directionalLightTransform;
		SetUniformBufferData(CameraBuffer, sizeof(CameraData), (const void*)(&cameraData));

		environmentData.viewPosition = cameraTransform[3];
		SetUniformBufferData(EnvironmentBuffer, sizeof(EnvironmentData), (const void*)(&environmentData));

        RenderEntity* renderEntities = &gameState.entityManager.renderEntities[0];
        mat4f finalTransform;
        uint32 count = gameState.entityManager.renderEntityCount;

        //-------------SHADOW PASS-------------------------
		glBindVertexArray(meshVAO);
		glDisable(GL_BLEND);
		glViewport(0,0,shadowMap.width, shadowMap.height);
		//glCullFace(GL_FRONT);
		glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		glUseProgram(shadowPassShader);

		glUniformMatrix4fv(lightVPShaderLocation, 1, GL_FALSE,
			 glm::value_ptr(cameraData.mainLightViewProjection));

		SetShaderModelMatrix(ObjectBuffer, glm::identity<mat4f>());
		for (int i = 0; i < MAX_NODE_TYPES; i++)
		{
			if (gameState.level.nodeTypeGeometyBufferMap[i] < 0) continue;
			DrawCall(GeometryRenderBuffer.handles[i]);
		}

        for (uint32 i = 0; i < count; i++)
        {
            finalTransform = glm::identity<mat4f>();
            TRS(finalTransform, transforms[renderEntities[i].transformIndex]);
    		SetShaderModelMatrix(ObjectBuffer, finalTransform);
    		DrawCall(meshBuffers[renderEntities[i].meshBufferIndex]);
        }

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		BindTexture(shadowMap, 0);

		glViewport(0,0,gameState.application.windowWidth, gameState.application.windowHeight);
		glCullFace(GL_BACK);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//------------END SHADOW PASS------------------

		//---------------MAIN PASS----------------------
		SetShaderModelMatrix(ObjectBuffer, glm::identity<mat4f>());
		for (int i = 0; i < MAX_NODE_TYPES; i++)
		{
			if (gameState.level.nodeTypeGeometyBufferMap[i] < 0) continue;

			SetShader(materials[NodePalette.materials[i]], ShaderTable);
			DrawCall(GeometryRenderBuffer.handles[i]);
		}


        for (uint32 i = 0; i < count; i++)
        {
            SetShader(materials[renderEntities[i].materialIndex], ShaderTable);
            finalTransform = glm::identity<mat4f>();
            TRS(finalTransform, transforms[renderEntities[i].transformIndex]);
    		SetShaderModelMatrix(ObjectBuffer, finalTransform);
    		DrawCall(meshBuffers[renderEntities[i].meshBufferIndex]);
        }

		// SetShader(materials[playerRenderMesh->materialIndex], ShaderTable);
		// SetShaderModelMatrix(ObjectBuffer, gameState.playerState.transform);
		// DrawCall(meshBuffers[playerRenderMesh->meshBufferIndex]);
		//------------END MAIN PASS---------------

#ifdef DEBUG
		if (Debug.DebugShadowmap)
		{
			glUseProgram(debugShader);
			DrawCall(meshBuffers[screenQuadBuffer]);
		}

		std::string text = "FPS: " + std::to_string(fps);
		AddTextToQuadBuffer(text, vec2f(-.9f,.8f), 0.1f,Debug.FontAtlas,
                gameState.application.aspectRatio, &Debug.TextBuffer);

        text = "Time Scale: " + std::to_string(gameState.timeScale);
		AddTextToQuadBuffer(text, vec2f(-.9f,.7f), 0.1f,Debug.FontAtlas,
                gameState.application.aspectRatio, &Debug.TextBuffer);

		if (Debug.TextBuffer.count > 0)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			UploadQuadBuffer(Debug.TextBuffer);
			glUseProgram(Debug.TextShader);
			BindTexture(Debug.FontAtlas.atlasTexture, 2);
			DrawCall(Debug.TextBuffer.handle);
		}

        if (Debug.DebugColliders)
        {
    		vec3f colliderDebugColor = { 0,1,0 };
    		for (uint32 indexX=0; indexX < gameState.level.width;  indexX++)
    		for (uint32 indexY=1; indexY < gameState.level.height; indexY++)
    		for (uint32 indexZ=0; indexZ < gameState.level.width;  indexZ++)
    		{
    			vec3f pos = vec3f(indexX,indexY,indexZ)*CELL_SIZE;
    			NodeCell cell = gameState.level.cells[LinearIndex(
                            indexX,indexY,indexZ,gameState.level.width)];

    			for (byte q = 0; q < 4; q++)
    			{
    				if (cell.types[q] != 0 && cell.shapes[q] != 0)
                    {
                        uint32 colIndex = GetColliderTableIndex(cell.shapes[q],q);
                        Debug.LineBuffer.AddBox(COLLIDER_BOX_TABLE[colIndex],pos,
                            COLLIDER_ROTATION_TABLE[colIndex],colliderDebugColor);
                    }
    			}

    		}
        }


		//DrawTransformHierarchyRecursive(playerModel, Debug.LineBuffer, vec3f(1,1,1),
        //                playerModel->transform->position,playerModel->transform->rotation);


		if (Debug.LineBuffer.pointCount > 0)
		{
            glClear(GL_DEPTH_BUFFER_BIT);
			glBindVertexArray(Debug.LineVAO);

			UploadLineBuffer(Debug.LineBuffer);

			glUseProgram(Debug.LineShader);
			DrawLines(Debug.LineBuffer);
		}
#endif

		SDL_GL_SwapWindow(gameState.application.window);
		gameState.deltaTime = (float32)timer.ElapsedTime().count();

#ifdef DEBUG
		frameTimes += 1.0/gameState.deltaTime;
		frameCount++;
		if (frameCount>FPS_FRAME_AVG)
		{
			//fps = glm::round(frameTimes/FPS_FRAME_AVG);
			fps = (frameTimes/FPS_FRAME_AVG);
			frameTimes = 0;
			frameCount = 1;
		}
#endif
	}

	// Close and destroy the window
	SDL_DestroyWindow(gameState.application.window);

	// Clean up
	SDL_Quit();

	return 0;
}
