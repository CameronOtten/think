#include "glm/glm.hpp"
#include <glm/gtc/type_ptr.hpp>
#include "glew/GL/glew.h"

#include "types.h"
#include "level.h"
#include "resources.h"
#include "rendering.h"
#include "math.h"




//TODO: Research difference in usage param in glBufferData
//TODO: Add option for mesh buffers to be shorts or ints for the index buffer
MeshBufferHandle UploadMeshToGPU(void* vertexData, void* triangleData, uint32 vertexCount, uint32 indexCount)
{
	MeshBufferHandle buffer;
	buffer.indexCount = indexCount;

	glGenBuffers(1, &(buffer.vboID));
	glBindBuffer(GL_ARRAY_BUFFER, buffer.vboID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertexCount, vertexData, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &(buffer.iboID));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.iboID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16) * indexCount, triangleData, GL_DYNAMIC_DRAW);

	return buffer;
}

MeshBufferHandle CreateEmptyMeshBuffer()
{
	MeshBufferHandle buffer;
	buffer.indexCount = 0;

	glGenBuffers(1, &(buffer.vboID));
	glBindBuffer(GL_ARRAY_BUFFER, buffer.vboID);
	glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &(buffer.iboID));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.iboID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);

	return buffer;
}

uint32 UploadMeshToGPU(const Mesh& mesh, GameMemory& memory)
{
	ASSERT(memory.meshBufferHandles.CanFit(1), __FILE__, __LINE__);
	auto request = memory.meshBufferHandles.RequestFromBlock(1);
	MeshBufferHandle* buffer = request.memory;
	buffer->indexCount = mesh.indexCount;

	glGenBuffers(1, &(buffer->vboID));
	glBindBuffer(GL_ARRAY_BUFFER, buffer->vboID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * mesh.vertexCount, reinterpret_cast<const void*>(mesh.vertices), GL_DYNAMIC_DRAW);

	glGenBuffers(1, &(buffer->iboID));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->iboID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16) * mesh.indexCount, reinterpret_cast<const void*>(mesh.indices), GL_DYNAMIC_DRAW);

	return request.index;
}

QuadBuffer CreateQuadBufferInHeap(uint32 quadCapacity)
{
	QuadBuffer buffer;
	buffer.count = 0;
	buffer.capacity = quadCapacity;
	buffer.mesh.vertexCount = quadCapacity*4;
	buffer.mesh.indexCount = quadCapacity*6;

	buffer.mesh.vertices = (Vertex*)SDL_malloc(quadCapacity*4*sizeof(Vertex));
	buffer.mesh.indices = (uint16*)SDL_malloc(quadCapacity*6*sizeof(uint16));

	buffer.handle.indexCount = 0;
	glGenBuffers(1, &(buffer.handle.vboID));
	glGenBuffers(1, &(buffer.handle.iboID));

	return buffer;
}

QuadBuffer CreateQuadBufferInGameMemory(uint32 quadCapacity, GameMemory& memory)
{
	QuadBuffer buffer;
	buffer.count = 0;
	buffer.capacity = quadCapacity;
	buffer.mesh.vertexCount = quadCapacity*4;
	buffer.mesh.indexCount = quadCapacity*6;

	ASSERT((memory.meshVertexBlock.CanFit(quadCapacity*4)), __FILE__, __LINE__);
	ASSERT((memory.meshIndexBlock.CanFit(quadCapacity*6)) , __FILE__, __LINE__);
	auto vertexRequest = memory.meshVertexBlock.RequestFromBlock(quadCapacity*4);
	auto indexRequest = memory.meshIndexBlock.RequestFromBlock(quadCapacity*6);
	buffer.mesh.vertices = vertexRequest.memory;
	buffer.mesh.indices = indexRequest.memory;

	buffer.handle.indexCount = 0;
	glGenBuffers(1, &(buffer.handle.vboID));
	glGenBuffers(1, &(buffer.handle.iboID));

	return buffer;
}

void DrawCall(const MeshBufferHandle& buffer)
{
	glBindVertexBuffer(0, buffer.vboID, 0, sizeof(Vertex));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.iboID);
	glDrawElements(GL_TRIANGLES, buffer.indexCount, GL_UNSIGNED_SHORT, NULL);
}

void BindTexture(const Texture2D &texture, uint32 slot = 0)
{
	glActiveTexture(GL_TEXTURE0+slot);
	glBindTexture(GL_TEXTURE_2D, texture.glID);
}

inline void SetShaderModelMatrix(uint32 objectBufferID, const mat4f& data)
{
	glBindBuffer(GL_UNIFORM_BUFFER, objectBufferID);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4f), glm::value_ptr(data));
	mat4f inverse = glm::transpose(glm::inverse(data));
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4f), sizeof(mat4f), glm::value_ptr(inverse));
}

inline void SetUniformBufferData(uint32 uniformBufferID, uint32 size, const void* data)
{
	glBindBuffer(GL_UNIFORM_BUFFER, uniformBufferID);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
}

uint32 CreateUniformBuffer(uint32 size, uint32 bindingIndex)
{
	GLuint id;
	glGenBuffers(1, &id);
	glBindBuffer(GL_UNIFORM_BUFFER, id);
	glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_STATIC_DRAW);

	glBindBufferBase(GL_UNIFORM_BUFFER, bindingIndex, id);
	return id;
}

inline void ApplyVertexAttribute(uint32 index, uint32 offset, uint32 numFloats)
{
	glEnableVertexAttribArray(index);
	glVertexAttribFormat(index, numFloats, GL_FLOAT, GL_FALSE, offset);
	glVertexAttribBinding(index, 0);
}

void SetShader(const Material& material, const ShaderAssetTable& shaderTable)
{
	uint32 shader = shaderTable.compiledShaders[material.shaderType];
	glUseProgram(shader);

	//TODO(Cameron): figure out a better way of keeping these locations, will probably have an
	//explicit table mapping them since I'll have a set amount of them
	//NOTE(Cameron): It is possible for uniforms to get compiled out if theyre not used for
	//some reason so maybe just log a warning instead of asserting
	int uniformLocation = glGetUniformLocation(shader, "u_Color");
	//ASSERT(uniformLocation != -1, __FILE__, __LINE__);
	if (uniformLocation != -1)
		glUniform4f(uniformLocation, material.color.x, material.color.y, material.color.z, material.color.w);
}



template <uint32 N>
void UpdateGeometryBuffer(const Level& level, NodeSetPalette<N>& nodeSets, GeometryBuffer& geometryBuffer)
{
	int levelWidth = level.width;
	int levelHeight = level.height;

	NodeCell* cell = &level.cells[0];
	unsigned char type = 0;
	unsigned char shape = 0;
	unsigned char layer = 0;
	vec3f up          = vec3f(0.0f, 1.0f, 0.0f);
	vec3f cellPos     = {0.0,0.0,0.0};
	mat4f identity    = glm::identity<mat4f>();
	mat4f translation = glm::identity<mat4f>();
	Vertex sourceVertex;
	//Method 2 where we map directly to GPU buffers
	/*glBindBuffer(GL_ARRAY_BUFFER, geometryBuffers.vboID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometryBuffers.iboID);
	Vertex* vertices = (Vertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	uint16* indices = (uint16*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);*/
	/*uint32 vertexCount = 0;
	uint32 indexCount = 0;
	uint32 meshIndexStart = 0;*/

	for (int i = 0; i < MAX_NODE_TYPES; i++)
	{
		geometryBuffer.meshes[i].vertexCount = 0;
		geometryBuffer.meshes[i].indexCount = 0;
	}


	for (int y = 0; y < levelHeight; y++)
	for (int x = 0; x < levelWidth; x++)
	for (int z = 0; z < levelWidth; z++)
	{
		cellPos = vec3f(x*CELL_SIZE,y*CELL_SIZE,z*CELL_SIZE);
		for (int i = 0; i < 4; i++)
		{
			type  = cell->types[i];
			shape = cell->shapes[i];
			layer = cell->layers[i];

			if (type == EMPTY || shape == 0) continue;

			if (type > nodeSets.count)
			{
				LOG_WARN("Unsupported node type found in Level! (type: %d)\n",type);
				type = 1;
			}

			Mesh* srcMesh = nodeSets.GetNodePiece(type,shape,layer);
			Mesh* dstMesh = &(geometryBuffer.meshes[type-1]);

			ASSERT((srcMesh != NULL), __FILE__, __LINE__);
			if (srcMesh == NULL) continue;
			ASSERT((dstMesh->vertices != NULL), __FILE__, __LINE__);

			for (uint32 v = 0; v < srcMesh->indexCount; v++)
			{
				dstMesh->indices[dstMesh->indexCount] = dstMesh->vertexCount + (srcMesh->indices[v]);
				dstMesh->indexCount++;
			}

			for (uint32 v = 0; v < srcMesh->vertexCount; v++)
			{
				sourceVertex = srcMesh->vertices[v];

				sourceVertex.position = ROTATION_TABLE[i] * sourceVertex.position;
				sourceVertex.position += cellPos;

				sourceVertex.normal   = ROTATION_TABLE[i] * sourceVertex.normal;

				dstMesh->vertices[dstMesh->vertexCount] = sourceVertex;

				dstMesh->vertexCount++;
			}
		}

		cell++;
	}

	for (int i = 0; i < MAX_NODE_TYPES; i++)
	{
		if (level.nodeTypeGeometyBufferMap[i] < 0) continue;

		MeshBufferHandle* handle = &(geometryBuffer.handles[i]);
		Mesh* mesh = &(geometryBuffer.meshes[i]);
		handle->indexCount = mesh->indexCount;

		glBindBuffer(GL_ARRAY_BUFFER, handle->vboID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * mesh->vertexCount, (const void*)(mesh->vertices), GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle->iboID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16) * mesh->indexCount, (const void*)(mesh->indices), GL_DYNAMIC_DRAW);
	}

	/*geometryBuffers.indexCount = indexCount;

	glUnmapBuffer(GL_ARRAY_BUFFER);
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);*/
}

template<uint32 N>
void RenderPass(uint32 objectUniformBuffer, const Level& level, const NodeSetPalette<N>& nodePalette, const ShaderAssetTable& shaderTable, const GeometryBuffer& geometryBuffer)
{
	SetShaderModelMatrix(objectUniformBuffer, glm::identity<mat4f>());
	for (int i = 0; i < MAX_NODE_TYPES; i++)
	{
		if (level.nodeTypeGeometyBufferMap[i] < 0) continue;

		SetShader(nodePalette.materials[i], shaderTable);

		DrawCall(geometryBuffer.handles[i]);
	}
}

void AddTextToQuadBuffer(std::string text, vec2f startPosition, float scale,
						const UnicodeGlyphAtlas& fontAtlas,
 						float screenAspectRatio, QuadBuffer* buffer)
{
	uint32 charIndex = 0;
	const GlyphData* curGlyph;
	Rect curGlyphRect;
	Rect textRect;
	vec2f position = startPosition;
	float scaleX = scale/screenAspectRatio;
	for (unsigned char c : text)
	{
		if (c == ' ')
		{
			position.x += fontAtlas.spaceWidth*scale;
			continue;
		}
		if (c == '\n')
		{
			position.y -= fontAtlas.lineHeight*scale;
			position.x = startPosition.x;
			continue;
		}

		curGlyph = &fontAtlas.glyphs[c-UNICODE_BASE_CHAR];
		curGlyphRect = curGlyph->positionOffset;
		curGlyphRect.bottom *= scale;
		curGlyphRect.top *= scale;
		curGlyphRect.left *= scaleX;
		curGlyphRect.right *= scaleX;
		textRect.bottom = position.y+curGlyphRect.bottom;
		textRect.left   = position.x+curGlyphRect.left;
		textRect.top    = position.y+curGlyphRect.top;
		textRect.right  = position.x+curGlyphRect.right;

		buffer->AddQuad(textRect, curGlyph->atlasLocation);

		position.x = textRect.right;//+= curGlyph->spacing*scale;
	}
}

void UploadQuadBuffer(const QuadBuffer& quadBufffer)
{
	glBindBuffer(GL_ARRAY_BUFFER, quadBufffer.handle.vboID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * quadBufffer.mesh.vertexCount,
	 					(const void*)(quadBufffer.mesh.vertices), GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadBufffer.handle.iboID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16) * quadBufffer.mesh.indexCount,
	 					(const void*)(quadBufffer.mesh.indices), GL_DYNAMIC_DRAW);
}
