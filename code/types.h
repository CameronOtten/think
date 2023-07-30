#pragma once

#include "glm/glm.hpp"
#include <string>

typedef unsigned char byte;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;
typedef __int16 int16;
typedef __int32 int32;
typedef __int64 int64;
typedef float float32;
typedef double float64;
typedef glm::mat<4,4,glm::f32,glm::packed_highp> mat4f;
typedef glm::vec<2,glm::f32,glm::packed_highp> vec2f;
typedef glm::vec<3,glm::f32,glm::packed_highp> vec3f;
typedef glm::vec<4,glm::f32,glm::packed_highp> vec4f;
typedef glm::vec<2,glm::int32,glm::packed_highp> vec2i;
typedef glm::vec<3,glm::int32,glm::packed_highp> vec3i;
typedef glm::vec<4,glm::int32,glm::packed_highp> vec4i;
typedef glm::vec<2,glm::uint32,glm::packed_highp> uvec2i;
typedef glm::vec<3,glm::uint32,glm::packed_highp> uvec3i;
typedef glm::vec<4,glm::uint32,glm::packed_highp> uvec4i;
typedef glm::quat quaternion;

const uint32 MAX_NODE_TYPES = 4;
const float CELL_SIZE = 1.0f;
const float CELL_HALF = CELL_SIZE/2.0f;
const float CELL_QUAR = CELL_HALF/2.0f;
const float CELL_DIAG = glm::sqrt((CELL_SIZE*CELL_SIZE)+(CELL_SIZE*CELL_SIZE));
const float CELL_X = glm::sqrt(CELL_HALF*CELL_HALF/2.0f);
const float CELL_Y = glm::sqrt(CELL_SIZE*CELL_SIZE/2.0f);
const float CELL_DHA = (CELL_DIAG/2.0f)-((CELL_DIAG-CELL_X)/2.0f);
const float CELL_DHB = (CELL_DIAG/4.0f)+(CELL_DIAG/8.0f);
const float CELL_OFFA = glm::sqrt(CELL_DHA*CELL_DHA/2.0f);
const float CELL_OFFB = glm::sqrt(CELL_DHB*CELL_DHB/2.0f);
const byte EMPTY = 0;
const uint32 MAX_LEVEL_WIDTH = 1000;
const uint32 MAX_LEVEL_HEIGHT = 8;


const float RAD_90 = glm::radians(90.0f);
const float RAD_45 = glm::radians(45.0f);

const quaternion ROTATION_TABLE[4] =
{
	quaternion(vec3f(0.0f,0.0f,    0.0f)),
	quaternion(vec3f(0.0f,RAD_90,  0.0f)),
	quaternion(vec3f(0.0f,RAD_90*2,0.0f)),
	quaternion(vec3f(0.0f,RAD_90*3,0.0f))
};

struct Rect
{
	float32 bottom;
	float32 top;
	float32 left;
	float32 right;

	Rect& operator*=(const float& value)
	{
		bottom *= value;
		top *= value;
		left *= value;
		right *= value;
		return *this;
	}

};

struct Application
{
	SDL_Window* window;
	int windowWidth;
	int windowHeight;
	float32 aspectRatio;
};


struct Vertex //48
{
	vec3f position;
	vec3f normal;
	vec2f uv0;
	vec4f color;

	Vertex() {}
	Vertex(vec3f position, vec3f normal, vec2f uv0, vec4f color)
		: position(position), normal(normal), uv0(uv0), color(color) {}
};

struct SkinnedVertex
{
	vec3f position;
	vec3f normal;
	vec2f uv0;
	vec4f color;
	vec4f joints;
	vec4f weights;

	SkinnedVertex() {}
	SkinnedVertex(vec3f position, vec3f normal, vec2f uv0, vec4f color, vec4f joints, vec4f weights)
		: position(position), normal(normal), uv0(uv0), color(color),
		 joints(joints), weights(weights) {}
};

struct NodeCell //24 bytes
{
	byte types[4];
	byte slopes[4];
	byte shapes[4];
	byte layers[4];
	vec3f position;
};

struct GeometryNode
{
	byte type;
	byte slope;
};

struct Mesh
{
	uint32 vertexCount;
	uint32 indexCount;
	Vertex* vertices;
	uint16* indices;

	Mesh()
	{
		vertexCount = 0;
		indexCount = 0;
		vertices = NULL;
		indices = NULL;
	}
};
struct SkinnedMesh
{
	uint32 vertexCount;
	uint32 indexCount;
	SkinnedVertex* vertices;
	uint16* indices;
};

struct CameraData
{
	mat4f cameraViewProjection;
	mat4f mainLightViewProjection;
};

struct ObjectData
{
	mat4f model;
	mat4f inverse;
};

struct MeshBufferHandle
{
	uint32 vboID;
	uint32 iboID;
	uint32 indexCount;
};

struct GeometryBuffer
{
	Mesh Block;
	Mesh             meshes[MAX_NODE_TYPES];
	MeshBufferHandle handles[MAX_NODE_TYPES];
};

struct QuadBuffer
{
	Mesh mesh;
	MeshBufferHandle handle;
	uint32 count;
	uint32 capacity;

	bool AddQuad(Rect position, Rect uv)
	{
		if (count + 1 > capacity) return false;

		uint32 vertIndex = count*4;
		Vertex* vert = &(mesh.vertices[vertIndex]);
		vert->position = { position.left,  position.bottom, 0 };
		vert->normal   = {0.0,0.0,1.0};
		vert->uv0      = { uv.left, uv.bottom };
		vert->color    = {1.0,1.0,1.0,1.0};

		vert = &(mesh.vertices[vertIndex+1]);
		vert->position = { position.left,  position.top, 0 };
		vert->normal   = {0.0,0.0,1.0};
		vert->uv0      = { uv.left, uv.top };
		vert->color    = {1.0,1.0,1.0,1.0};

		vert = &(mesh.vertices[vertIndex+2]);
		vert->position = { position.right,  position.top, 0 };
		vert->normal   = {0.0,0.0,1.0};
		vert->uv0      = { uv.right, uv.top };
		vert->color    = {1.0,1.0,1.0,1.0};

		vert = &(mesh.vertices[vertIndex+3]);
		vert->position = { position.right,  position.bottom, 0 };
		vert->normal   = {0.0,0.0,1.0};
		vert->uv0      = { uv.right, uv.bottom };
		vert->color    = {1.0,1.0,1.0,1.0};

		uint32 triIndex = count*6;
		mesh.indices[triIndex  ] = vertIndex;
		mesh.indices[triIndex+1] = vertIndex+1;
		mesh.indices[triIndex+2] = vertIndex+2;
		mesh.indices[triIndex+3] = vertIndex;
		mesh.indices[triIndex+4] = vertIndex+2;
		mesh.indices[triIndex+5] = vertIndex+3;

		count++;
		mesh.vertexCount+=4;
		mesh.indexCount+=6;
		handle.indexCount+= 6;
		return true;
	}

	void Clear()
	{
		count = 0;
		mesh.vertexCount = 0;
		mesh.indexCount = 0;
		handle.indexCount = 0;
	}
};

struct EnvironmentData
{
	vec4f ambientLightTop;
	vec4f ambientLightBottom;
	vec4f viewPosition;
};

enum LightType
{
	Directional,
	Point
};

const int MAX_POINT_LIGHTS = 8;
struct Light
{
	vec4f color;
	vec4f position;
	LightType lightType;
	float padding1;
	float padding2;
	float padding3;
};

struct Box
{
	vec3f offset;
	vec3f extents;

	Box(vec3f offset, vec3f extents)
		: offset(offset), extents(extents) {}
};

struct ShaderAsset
{
	std::string vertexShaderFilePath;
	std::string fragmentShaderFilePath;
};

enum ShaderType
{
	VertexColorUnlit,
	VertexColorLit,
	TriplanarLit,

	Count
};

struct Material
{
	ShaderType shaderType;

	vec4f color;
};

struct ShaderAssetTable
{
	ShaderAsset shaderAssetFilePaths[ShaderType::Count];
	unsigned int compiledShaders[ShaderType::Count];
};

struct GeometryNodeAssetTable
{
	std::string nodeSetFilePaths[MAX_NODE_TYPES];
	std::string materialFilePaths[MAX_NODE_TYPES];
};

struct Texture2D
{
	int width;
	int height;
	int channels;
	unsigned int glID;
};


struct NodeShape
{
	//unsigned int variationCount;
	Mesh mesh;

	NodeShape() { mesh = Mesh(); }
};

struct NodeStack
{
	NodeShape layers[4];

	NodeStack()
	{
		layers[0] = NodeShape();
		layers[1] = NodeShape();
		layers[2] = NodeShape();
		layers[3] = NodeShape();
	}
};

struct NodeSet
{
	NodeStack shapeStacks[5];

	unsigned int vertexCount;
	unsigned int indexCount;

	NodeSet()
	{
		vertexCount = 0;
		indexCount = 0;
		shapeStacks[0] = NodeStack();
		shapeStacks[1] = NodeStack();
		shapeStacks[2] = NodeStack();
		shapeStacks[3] = NodeStack();
		shapeStacks[4] = NodeStack();
	}
};

template<unsigned int N>
struct NodeSetPalette
{
	NodeSet sets[N];
	uint32 materials[N];
	unsigned int count = N;

	Mesh* GetNodePiece(unsigned char type, unsigned char shape, unsigned char layer)
	{
		//int variationCount = slices[pieceType].pieces[layer].variationCount;
		return &(sets[type-1].shapeStacks[shape-1].layers[layer].mesh); // + Random range between 0 and variation count
	}
};

struct LevelData
{
	unsigned int width;
	unsigned int height;
	unsigned int totalSize;
	GeometryNode* nodes;
};

struct GlyphData
{
	bool valid;
	Rect atlasLocation;
	Rect positionOffset;
	float spacing;
	float aspectRatio;
};

const int UNICODE_BASE_CHAR = 33;
const int UNICODE_MAX_CHAR = 126;
const int UNICODE_CHAR_SET_LENGTH = UNICODE_MAX_CHAR-UNICODE_BASE_CHAR+1;
struct UnicodeGlyphAtlas
{
	GlyphData glyphs[UNICODE_CHAR_SET_LENGTH];
	Texture2D atlasTexture;
	float spaceWidth;
	float lineHeight;
};

struct Transform
{
	vec3f position = vec3f(0.0f);
	quaternion rotation = quaternion(1.0f, 0.0f, 0.0f, 0.0f);
	vec3f scale = vec3f(1.0f, 1.0f, 1.0f);
};

const int MAX_CHILDREN_PER_NODE = 16;
struct TransformHierarchyNode
{
	byte childrenIndices[MAX_CHILDREN_PER_NODE];
	uint32 childCount;
};

struct TransformHierarchy
{
	Transform* transforms;
	TransformHierarchyNode* nodes;
	uint32 transformCount;
};

struct Entity
{
	uint32 transformIndex;
};

struct RenderEntity
{
	uint32 transformIndex;
	uint32 meshBufferIndex;
	uint32 materialIndex;
};

struct SkinnedRenderEntity
{
	TransformHierarchy transformHierachy;
	uint32 meshHandleIndex;
	uint32 materialIndex;
};

struct ModelHierarchyNode
{
	Mesh mesh;
	uint32 transformIndex;
	byte childrenIndices[MAX_CHILDREN_PER_NODE];
	uint32 childCount;
};

struct ModelHierarchy
{
	TransformHierarchy hierarchy;
	std::vector<ModelHierarchyNode> meshes;
	SkinnedMesh skinnedMesh;
	uint32 skinnedMeshTransformIndex;
};
