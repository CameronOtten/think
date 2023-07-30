#include "sdl/SDL.h"
#include "log.h"
#include "string"
#include "json/json.hpp"
#include "glm/glm.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

#include "memory.h"
#include "types.h"
#include "rendering.h"
#include "resources.h"
#include "level.h"

void* LoadFile(const char* filepath)
{
	void* file = SDL_LoadFile(filepath, NULL);
	ASSERT((file != 0), __FILE__, __LINE__);
	return file;
}

//TODO: change the texture functions to act in the same way as our mesh api so that loading a texture and uploading it to the GPU are separate
void CreateTexture(Texture2D& texture, const void* data, bool generateMipMaps, GLint internalFormat, GLenum format,
				   GLenum componentType, GLint minFilter, GLint magFilter, GLint wrapMode)
{
	glGenTextures(1, &texture.glID);
	glBindTexture(GL_TEXTURE_2D, texture.glID);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, texture.width, texture.height, 0, format, componentType, data);

	if (generateMipMaps)
		glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
}

Texture2D LoadTexture(const char* filepath)
{
	Texture2D texture;
	stbi_set_flip_vertically_on_load(true);
	byte* data = stbi_load(filepath, &texture.width, &texture.height, &texture.channels, 0);

	ASSERT((texture.channels>=3), __FILE__, __LINE__);
	GLint internFormat = texture.channels==4?GL_RGBA8:GL_RGB8;
	GLint format = texture.channels==4?GL_RGBA:GL_RGB;
	CreateTexture(texture, data, true, internFormat, format, GL_UNSIGNED_BYTE, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT);

	stbi_image_free(data);

	return texture;
}

uint32 CompileShader(GLenum type, const char* source)
{
	uint32 id = glCreateShader(type);
	glShaderSource(id, 1, &source, NULL);
	glCompileShader(id);

	GLint compiled;
	glGetShaderiv(id, GL_COMPILE_STATUS, &compiled);
	if (compiled != GL_TRUE)
	{
		GLsizei log_length = 0;
		GLchar message[1024];
		glGetShaderInfoLog(id, 1024, &log_length, message);
		LOG_ERROR("Shader Compilation Failed (%s): %s\n", (type == GL_VERTEX_SHADER ? "vertex" : "fragment"), message);
		glDeleteShader(id);
		return 0;
	}

	return id;
}

uint32 CreateShader(const char* vertexSource, const char* fragmentSource)
{
	uint32 vs = CompileShader(GL_VERTEX_SHADER, vertexSource);
	uint32 fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
	uint32 program = glCreateProgram();

	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);

	GLint linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (linked != GL_TRUE)
	{
		GLsizei log_length = 0;
		GLchar message[1024];
		glGetProgramInfoLog(program, 1024, &log_length, message);
		LOG_ERROR("Shader Linking Failed: %s\n", message);
		glDeleteProgram(program);
		return 0;
	}
	glValidateProgram(program);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}

uint32 LoadShader(const char* vertexShaderFilepath, const char* fragmentShaderFilepath)
{
	void* vertexSource = LoadFile(vertexShaderFilepath);
	void* fragmentSource = LoadFile(fragmentShaderFilepath);

	uint32 shader = CreateShader((const char*)vertexSource, (const char*)fragmentSource);

	SDL_free(vertexSource);
	SDL_free(fragmentSource);

	return shader;
}

struct BufferView
{
	int bufferIndex;
	int byteLength;
	int byteOffset;
};

struct MeshImportView
{
	std::string name;
	BufferView positionView;
	BufferView normalView;
	BufferView uv0View;
	BufferView colorView;
	BufferView jointsView;
	BufferView weightsView;
	BufferView triView;
	uint32 vertexCount;
	uint32 indexCount;

	bool ushortIndices;
	bool hasColors;
	bool skinned;
};

using json = nlohmann::json;
BufferView GetBufferView(const json& jsonData, uint32 index)
{
	BufferView bv;
	bv.bufferIndex = jsonData["bufferViews"][index]["buffer"];
	bv.byteLength = jsonData["bufferViews"][index]["byteLength"];
	bv.byteOffset = jsonData["bufferViews"][index]["byteOffset"];

	return bv;
}

//This works by filling up the bytes of one type(bytesPerValue(ie vec3 = 12)), by the amount of bytes packed linearly
void FillBytes(byte* destination, byte* bytesStart, uint32 numBytes, uint32 bytesPerValue, uint32 stride)
{
	for (uint32 i = 0; i < numBytes; i += bytesPerValue)
	{
		for (uint32 b = 0; b < bytesPerValue; b++)
		{
			*(destination+b) = *bytesStart;// (bytesStart + (i * bytesPerValue) + b);
			bytesStart++;
		}
		destination += stride;
	}
}

void FillMeshBytes(const MeshImportView& view, byte* binaryData, byte* vertexBufferStart, byte* triangleIndexStart)
{
	FillBytes(vertexBufferStart+offsetof(Vertex, position), binaryData+view.positionView.byteOffset, view.positionView.byteLength, sizeof(vec3f), sizeof(Vertex));
	FillBytes(vertexBufferStart+offsetof(Vertex, normal)  , binaryData+view.normalView.byteOffset  , view.normalView.byteLength  , sizeof(vec3f), sizeof(Vertex));
	FillBytes(vertexBufferStart+offsetof(Vertex, uv0)     , binaryData+view.uv0View.byteOffset     , view.uv0View.byteLength     , sizeof(vec2f), sizeof(Vertex));
	FillBytes(triangleIndexStart                          , binaryData+view.triView.byteOffset     , view.triView.byteLength     , sizeof(uint16), sizeof(uint16));
	if (view.hasColors)
	{
		float max = (float)USHRT_MAX;
		Vertex* vertex = (Vertex*)vertexBufferStart;
		byte* binaryIt = binaryData+view.colorView.byteOffset;
		for (uint32 i = 0; i < view.vertexCount; i++)
		{
			uint16 r = *(uint16*)binaryIt;
			binaryIt+=2;
			uint16 g = *(uint16*)binaryIt;
			binaryIt+=2;
			uint16 b = *(uint16*)binaryIt;
			binaryIt+=2;
			uint16 a = *(uint16*)binaryIt;
			binaryIt+=2;

			vertex->color.r = r/max;
			vertex->color.g = g/max;
			vertex->color.b = b/max;
			vertex->color.a = a/max;
			vertex++;
		}
	}
	else
	{
		Vertex* vertex = (Vertex*)vertexBufferStart;
		for (uint32 i = 0; i < view.vertexCount; i++)
			vertex[i].color = vec4f(1.0f, 1.0f, 1.0f, 1.0f);
	}

}

void FillSkinnedMeshBytes(const MeshImportView& view, byte* binaryData, byte* vertexBufferStart, byte* triangleIndexStart)
{
	FillBytes(vertexBufferStart+offsetof(SkinnedVertex, position), binaryData+view.positionView.byteOffset, view.positionView.byteLength, sizeof(vec3f), sizeof(SkinnedVertex));
	FillBytes(vertexBufferStart+offsetof(SkinnedVertex, normal)  , binaryData+view.normalView.byteOffset  , view.normalView.byteLength  , sizeof(vec3f), sizeof(SkinnedVertex));
	FillBytes(vertexBufferStart+offsetof(SkinnedVertex, uv0)     , binaryData+view.uv0View.byteOffset     , view.uv0View.byteLength     , sizeof(vec2f), sizeof(SkinnedVertex));
	FillBytes(vertexBufferStart+offsetof(SkinnedVertex, weights) , binaryData+view.weightsView.byteOffset , view.weightsView.byteLength , sizeof(vec4f), sizeof(SkinnedVertex));
	FillBytes(triangleIndexStart, binaryData+view.triView.byteOffset, view.triView.byteLength, sizeof(uint16), sizeof(uint16));

	SkinnedVertex* vertex = (SkinnedVertex*)vertexBufferStart;
	byte* jointIt = binaryData+view.jointsView.byteOffset;
	for (uint32 i = 0; i < view.vertexCount; i++)
	{
		vertex->joints.x = *(float*)jointIt;
		vertex->joints.y = *(float*)jointIt+1;
		vertex->joints.z = *(float*)jointIt+2;
		vertex->joints.w = *(float*)jointIt+3;
		vertex++;
	}

	vertex = (SkinnedVertex*)vertexBufferStart;
	if (view.hasColors)
	{

		float max = (float)USHRT_MAX;
		byte* binaryIt = binaryData+view.colorView.byteOffset;
		for (uint32 i = 0; i < view.vertexCount; i++)
		{
			uint16 r = *(uint16*)binaryIt;
			binaryIt+=2;
			uint16 g = *(uint16*)binaryIt;
			binaryIt+=2;
			uint16 b = *(uint16*)binaryIt;
			binaryIt+=2;
			uint16 a = *(uint16*)binaryIt;
			binaryIt+=2;

			vertex->color.r = r/max;
			vertex->color.g = g/max;
			vertex->color.b = b/max;
			vertex->color.a = a/max;
			vertex++;
		}
	}
	else
	{
		for (uint32 i = 0; i < view.vertexCount; i++)
			vertex[i].color = vec4f(1.0f, 1.0f, 1.0f, 1.0f);
	}

}

MeshImportView ExtractMeshImportView(const json& jsonData, uint32 meshIndex)
{
	//bool hasColors = !(jsonData["meshes"][0]["primitives"][0]["attributes"]["COLOR_0"].is_null());
	json attributes = jsonData["meshes"][meshIndex]["primitives"][0]["attributes"];
	bool hasColors = attributes.find("COLOR_0") != attributes.end();
	bool skinned = attributes.find("JOINTS_0") != attributes.end();

	uint32 accColorIndex    = 0;
	uint32 accJointsIndex    = 0;
	uint32 accWeightsIndex    = 0;
	uint32 accPositionIndex = attributes["POSITION"];
	uint32 accNormalIndex   = attributes["NORMAL"];
	uint32 accUV0Index      = attributes["TEXCOORD_0"];
	if (hasColors) accColorIndex  = attributes["COLOR_0"];
	if (skinned)
	{
		accJointsIndex = attributes["JOINTS_0"];
		accWeightsIndex = attributes["WEIGHTS_0"];
	}
	uint32 accTriIndex      = jsonData["meshes"][meshIndex]["primitives"][0]["indices"];
	uint32 bvPositionIndex  = jsonData["accessors"][accPositionIndex]["bufferView"];
	uint32 bvNormalIndex    = jsonData["accessors"][accNormalIndex]["bufferView"];
	uint32 bvUV0Index       = jsonData["accessors"][accUV0Index]["bufferView"];
	uint32 bvColorIndex     = jsonData["accessors"][accColorIndex]["bufferView"];
	uint32 bvJointsIndex    = jsonData["accessors"][accJointsIndex]["bufferView"];
	uint32 bvWeightsIndex   = jsonData["accessors"][accWeightsIndex]["bufferView"];
	uint32 bvTriIndex       = jsonData["accessors"][accTriIndex]["bufferView"];

	MeshImportView meshView;
	meshView.name          = jsonData["nodes"][meshIndex]["name"];
	meshView.vertexCount   = jsonData["accessors"][accPositionIndex]["count"];
	meshView.indexCount    = jsonData["accessors"][accTriIndex     ]["count"];
	meshView.ushortIndices = jsonData["accessors"][accTriIndex]["componentType"] == 5123;
	meshView.hasColors     = hasColors;
	meshView.skinned       = skinned;

	meshView.positionView = GetBufferView(jsonData, bvPositionIndex);
	meshView.normalView   = GetBufferView(jsonData, bvNormalIndex);
	meshView.uv0View      = GetBufferView(jsonData, bvUV0Index);
	meshView.colorView    = GetBufferView(jsonData, bvColorIndex);
	meshView.jointsView   = GetBufferView(jsonData, bvJointsIndex);
	meshView.weightsView  = GetBufferView(jsonData, bvWeightsIndex);
	meshView.triView      = GetBufferView(jsonData, bvTriIndex);

	if (!meshView.ushortIndices)
	{
		LOG_ERROR("Have not supported mesh index format other than uint16!,"
 					"source mesh: %s", meshView.name.c_str());
	}
	return meshView;
}


std::string ExtractBinaryFilepath(const char* parentFilepath, const json& jsonData)
{
	std::string parse = jsonData["buffers"][0]["uri"];
	std::string fileStr = std::string(parentFilepath);
	std::string binDir = (fileStr.substr(0, fileStr.find_last_of('/') + 1)) + parse;
	return binDir;
}

Mesh ExtractMesh(const MeshImportView& meshView, byte* bin, GameMemory& memory)
{
	Mesh mesh;

	mesh.vertexCount = meshView.vertexCount;
	mesh.indexCount  = meshView.indexCount;
	ASSERT(memory.meshVertexBlock.CanFit(mesh.vertexCount), __FILE__, __LINE__);
	ASSERT(memory.meshIndexBlock.CanFit(mesh.indexCount), __FILE__, __LINE__);

	auto vertexRequest = memory.meshVertexBlock.RequestFromBlock(mesh.vertexCount);
	auto indexRequest = memory.meshIndexBlock.RequestFromBlock(mesh.indexCount);
	byte* vertices  = (byte*)vertexRequest.memory;
	byte* triangles = (byte*)indexRequest.memory;

	mesh.vertices = (Vertex*)vertices;
	mesh.indices = (uint16*)triangles;

	FillMeshBytes(meshView, bin, vertices, triangles);
	return mesh;
}

SkinnedMesh ExtractSkinnedMesh(const MeshImportView& meshView, byte* bin, GameMemory& memory)
{
	SkinnedMesh skinnedMesh;
	skinnedMesh.vertexCount = meshView.vertexCount;
	skinnedMesh.indexCount  = meshView.indexCount;

	ASSERT(memory.skinnedMeshVertexBlock.CanFit(skinnedMesh.vertexCount), __FILE__, __LINE__);
	ASSERT(memory.skinnedMeshIndexBlock.CanFit (skinnedMesh.indexCount), __FILE__, __LINE__);

	auto vertexRequest = memory.skinnedMeshVertexBlock.RequestFromBlock(skinnedMesh.vertexCount);
	auto indexRequest = memory.skinnedMeshIndexBlock.RequestFromBlock(skinnedMesh.indexCount);
	byte* vertices  = (byte*)vertexRequest.memory;
	byte* triangles = (byte*)indexRequest.memory;

	skinnedMesh.vertices = (SkinnedVertex*)vertices;
	skinnedMesh.indices = (uint16*)triangles;

	FillSkinnedMeshBytes(meshView, bin, vertices, triangles);

	return skinnedMesh;
}


//TODO: refactor to allow multiple meshes per file
Mesh LoadModel(const char* filepath, GameMemory& memory)
{
	void* file = LoadFile(filepath);
	json jsonData = json::parse((char*)file);
	byte* bin = (byte*)LoadFile(ExtractBinaryFilepath(filepath, jsonData).c_str());

	MeshImportView meshView = ExtractMeshImportView(jsonData, 0);
	Mesh mesh = ExtractMesh(meshView, bin, memory);

	SDL_free(file);
	SDL_free(bin);

	return mesh;
}

vec2f JsonParseVector2(const json& jsonData)
{
	vec4f value;
	value.x = jsonData["x"];
	value.y = jsonData["y"];
	return value;
}

vec3f JsonParseVector3(const json& jsonData)
{
	vec4f value;
	value.x = jsonData["x"];
	value.y = jsonData["y"];
	value.z = jsonData["z"];
	return value;
}

vec4f JsonParseVector4(const json& jsonData)
{
	vec4f value;
	value.x = jsonData["x"];
	value.y = jsonData["y"];
	value.z = jsonData["z"];
	value.w = jsonData["w"];
	return value;
}

Rect JsonParseRect(const json& jsonData)
{
	Rect r;

	r.bottom = jsonData["bottom"];
	r.top    = jsonData["top"];
	r.left   = jsonData["left"];
	r.right  = jsonData["right"];
	return r;
}

Transform JsonParseTransform(const json& jsonData)
{
	Transform value;
	if(jsonData.find("translation") != jsonData.end())
	{
		value.position.x = jsonData["translation"][0];
		value.position.y = jsonData["translation"][1];
		value.position.z = jsonData["translation"][2];
	}
	if(jsonData.find("rotation") != jsonData.end())
	{
		value.rotation.x = jsonData["rotation"][0];
		value.rotation.y = jsonData["rotation"][1];
		value.rotation.z = jsonData["rotation"][2];
		value.rotation.w = jsonData["rotation"][3];
	}
	if(jsonData.find("scale") != jsonData.end())
	{
		value.scale.x = jsonData["scale"][0];
		value.scale.y = jsonData["scale"][1];
		value.scale.z = jsonData["scale"][2];
	}
	return value;
}

struct Animation
{


};

struct Vec3Keyframe
{
	float32 timestamp;
	vec3f value;
};

struct QuaternionKeyframe
{
	float32 timestamp;
	quaternion value;
};

// void Animate()
// {
// 	float time;
//
// 	foreach(Node in animationNodes)
// 	{
// 		Transform t from node;
// 		iterate to find t0
// 	}
//
// }




ModelHierarchy LoadAnimatedModel(const char* filepath, GameMemory& memory)
{
	void* file = LoadFile(filepath);
	json jsonData = json::parse((char*)file);
	byte* bin = (byte*)LoadFile(ExtractBinaryFilepath(filepath, jsonData).c_str());

	json& nodesData = jsonData["nodes"];
	const uint32 objectCount = (uint32)nodesData.size();
	const uint32 meshCount = (uint32)jsonData["meshes"].size();
	ASSERT(memory.transforms.CanFit(objectCount+1), __FILE__, __LINE__);
	ASSERT(memory.transformHierarchyNodes.CanFit(objectCount+1), __FILE__, __LINE__);
	auto transformRequest = memory.transforms.RequestFromBlock(objectCount+1);
	auto nodeRequest = memory.transformHierarchyNodes.RequestFromBlock(objectCount+1);
	Transform* transforms = transformRequest.memory;
	TransformHierarchyNode* nodes = nodeRequest.memory;
	ModelHierarchy result;
	result.hierarchy.transforms = transforms;
	result.hierarchy.nodes = nodes;
	result.hierarchy.transformCount = objectCount+1;
	result.meshes.reserve(meshCount);
	//these are so we can index them the same as the indices from the file since we're adding
	//an additional parent
	Transform* childTransforms = transforms+1;
	TransformHierarchyNode* childNodes = nodes+1;

	//Assign root nodes as children to parent node
	json& sceneNodes = jsonData["scenes"][0]["nodes"];
	uint32 parentChildCount = (uint32)sceneNodes.size();
	nodes->childCount = parentChildCount;
	byte* parentChildren = (byte*)&nodes->childrenIndices;
	for (uint32 i = 0; i < parentChildCount; i++)
		parentChildren[i] = sceneNodes[i];

	for (uint32 i = 0; i < objectCount; i++)
	{
		json& nodeData = nodesData[i];
		childTransforms[i] = JsonParseTransform(nodeData);
		TransformHierarchyNode& n = childNodes[i];
		if(nodeData.find("children") != nodeData.end())
		{
			json& childrenData = nodeData["children"];
			uint32 childCount = (uint32)childrenData.size();
			n.childCount = childCount;
			byte* children = (byte*)&n.childrenIndices;
			for (uint32 c = 0; c < childCount; c++)
				children[c] = childrenData[c]+1; // add one so now we can be relative to new parent
		}

		if(nodeData.find("mesh") != nodeData.end())
		{
			int32 meshIndex = nodeData["mesh"];
			MeshImportView view = ExtractMeshImportView(jsonData, meshIndex);
			if(nodeData.find("skin") != nodeData.end())
			{
				result.skinnedMesh = ExtractSkinnedMesh(view, bin, memory);
				result.skinnedMeshTransformIndex = i+1;
			}
			else
			{
				ModelHierarchyNode mNode;
				mNode.mesh = ExtractMesh(view, bin, memory);
				mNode.transformIndex = i+1;
				result.meshes.push_back(mNode);
			}
		}
	}

	SDL_free(file);
	SDL_free(bin);


	return result;
}

struct NodeSetImportData
{
	uint32 type;
	uint32 layer;
};

//TODO: Currently no error checking in place, assuming we're passing a valid node set import
NodeSet LoadNodeSet(const char* filepath, GameMemory& memory)
{
	void* file = LoadFile(filepath);
	json jsonData = json::parse((char*)file);
	byte* bin = (byte*)LoadFile(ExtractBinaryFilepath(filepath, jsonData).c_str());

	const uint32 meshCount = (uint32)jsonData["scenes"][0]["nodes"].size();
	std::vector<MeshImportView> meshViews;
	std::vector<NodeSetImportData> nodeSetImportData;
	meshViews.reserve(meshCount);
	nodeSetImportData.reserve(meshCount);

	uint32 totalVerts = 0;
	uint32 totalIndices  = 0;
	for (uint32 i = 0; i < meshCount; i++)
	{
		meshViews.push_back(ExtractMeshImportView(jsonData, i));
		totalVerts += meshViews[i].vertexCount;
		totalIndices  += meshViews[i].indexCount;

		std::string name = meshViews[i].name;
		NodeSetImportData nsData = {0};

		char nameConvBit = name[0];
		switch (nameConvBit)
		{
			case 'F':
				nsData.type = 0;
				break;
			case 'S':
				nsData.type = 1;
				break;
			case 'I':
				nsData.type = 2;
				break;
			case 'O':
				nsData.type = 3;
				break;
			case 'C':
				nsData.type = 4;
				break;
		}

		nameConvBit = name[2];
		switch (nameConvBit)
		{
			case 'B':
				nsData.layer = 0;
				break;
			case 'M':
				nsData.layer = 1;
				break;
			case 'T':
				nsData.layer = 2;
				break;
			case 'R':
				nsData.layer = 3;
				break;
		}

		nodeSetImportData.push_back(nsData);
	}

	ASSERT(memory.meshVertexBlock.CanFit(totalVerts), __FILE__, __LINE__);
	ASSERT(memory.meshIndexBlock.CanFit(totalIndices), __FILE__, __LINE__);

	auto vertexRequest = memory.meshVertexBlock.RequestFromBlock(totalVerts);
	auto indexRequest = memory.meshIndexBlock.RequestFromBlock(totalIndices);
	Vertex* vertices = vertexRequest.memory;
	uint16* indices  = indexRequest.memory;

	NodeSet nodeSet;
	nodeSet.vertexCount = totalVerts;
	nodeSet.indexCount = totalIndices;

	for (uint32 i = 0; i < meshCount; i++)
	{
		NodeSetImportData& ns = nodeSetImportData[i];
		MeshImportView& view = meshViews[i];
		Mesh& mesh = nodeSet.shapeStacks[ns.type].layers[ns.layer].mesh;
		mesh.vertexCount = view.vertexCount;
		mesh.indexCount  = view.indexCount;
		mesh.vertices = vertices;
		mesh.indices  = indices;

		FillMeshBytes(view, bin, (byte*)vertices, (byte*)indices);

		vertices += view.vertexCount;
		indices += view.indexCount;
	}

	SDL_free(file);
	SDL_free(bin);

	return nodeSet;
}

LevelData LoadLevelData(const char* filepath, GameMemory& memory)
{
	void* file = LoadFile(filepath);
	json jsonData = json::parse((char*)file);

	int width = jsonData["width"];
	int height = jsonData["height"];
	std::string name = jsonData["name"];
	LOG_INFO("Level: %s has width of %d\n", name.c_str(), width);

	std::vector<uint32> nodes = jsonData["nodes"];
	std::vector<uint32> slopes = jsonData["slopes"];

	LevelData levelData;
	uint32 totalSize = width * width * height;

	if (totalSize != nodes.size() || totalSize != slopes.size())
	{
		LOG_ERROR("Level file corrupted! Node block size does not match level width and height! %s\n", filepath);
	}

	ASSERT(memory.levelNodeBlock.CanFit(totalSize), __FILE__, __LINE__);
	auto nodeRequest = memory.levelNodeBlock.RequestFromBlock(totalSize);
	GeometryNode* dataBlock = nodeRequest.memory;
	levelData.width = width;
	levelData.height = height;
	levelData.totalSize = totalSize;
	levelData.nodes = dataBlock;

	for (uint32 index = 0; index < totalSize; index++)
	{
		dataBlock[index].type  = (byte)(nodes[index]);
		dataBlock[index].slope = (byte)(slopes[index]);
	}

	SDL_free(file);

	return levelData;
}



uint32 LoadMaterial(const char* filepath, GameMemory& memory)
{
	void* file = LoadFile(filepath);
	json jsonData = json::parse((char*)file);

	ASSERT(memory.materials.CanFit(1), __FILE__, __LINE__);
	auto request = memory.materials.RequestFromBlock(1);
	Material* mat = request.memory;
	mat->shaderType = jsonData["shaderType"];
	mat->color = JsonParseVector4(jsonData["color"]);

	SDL_free(file);

	return request.index;
}




UnicodeGlyphAtlas LoadFont(std::string fontName)
{
	std::string filepath = "resources/fonts/" + fontName;
	void* file = LoadFile((filepath+"_layout.json").c_str());
	json jsonData = json::parse((char*)file);

	auto glyphs = jsonData["glyphs"];

	UnicodeGlyphAtlas glyphAtlas;
	GlyphData* glyph;
	uint32 size = (uint32)glyphs.size();
	uint32 texSize = jsonData["atlas"]["width"];

	auto it = glyphs[0];
	glyphAtlas.spaceWidth = it["advance"];
	glyphAtlas.lineHeight = jsonData["metrics"]["lineHeight"];

	//NOTE: atm Im assuming 0 to be the space character in all cases, so start at 1
	for (uint32 i = 1; i < size; i++)
	{
		it = glyphs[i];
		uint32 unicode = it["unicode"];
		Rect atlasLoc = JsonParseRect(it["atlasBounds"]);
		atlasLoc.bottom /= texSize;
		atlasLoc.top    /= texSize;
		atlasLoc.left   /= texSize;
		atlasLoc.right  /= texSize;
		glyph = &glyphAtlas.glyphs[unicode-UNICODE_BASE_CHAR];
		glyph->atlasLocation = atlasLoc;
		glyph->spacing = it["advance"];
		glyph->positionOffset = JsonParseRect(it["planeBounds"]);
		glyph->aspectRatio = (atlasLoc.right-atlasLoc.left)/(atlasLoc.top-atlasLoc.bottom);
	}

	glyphAtlas.atlasTexture = LoadTexture((filepath+"_atlas.png").c_str());

	SDL_free(file);

	return glyphAtlas;
}
