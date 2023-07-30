#pragma once

#include "types.h"
#include "log.h"

const uint32 MAX_TRIS_PER_VERTEX = 10;
const uint32 MESH_BLOCK_MAX_VERTS = 10000000u;
const uint32 MESH_BLOCK_MAX_INDEX = MESH_BLOCK_MAX_VERTS*MAX_TRIS_PER_VERTEX*3;
const uint32 GEO_MESH_MAX_VERTS = 65535;
const uint32 GEO_MESH_MAX_INDEX = GEO_MESH_MAX_VERTS*MAX_TRIS_PER_VERTEX*3;
const uint32 GEO_BLOCK_MAX_VERTS = GEO_MESH_MAX_VERTS*MAX_NODE_TYPES;
const uint32 GEO_BLOCK_MAX_INDEX = GEO_MESH_MAX_INDEX*MAX_NODE_TYPES;
const uint32 MAX_SKINNED_VERTS = 1000000u;
const uint32 MAX_SKINNED_INDEX = MAX_SKINNED_VERTS*MAX_TRIS_PER_VERTEX;

const uint32 MAX_LEVEL_NODES = MAX_LEVEL_WIDTH*MAX_LEVEL_WIDTH*MAX_LEVEL_HEIGHT;
const uint32 MAX_LEVEL_CELLS = MAX_LEVEL_NODES;
const uint32 MAX_TRANSFORMS = 1000;
const uint32 MAX_TRANSFORM_HIERARCHY_NODES = 1000;
const uint32 MAX_SOURCE_MESHES = 1000;
const uint32 MAX_MATERIALS = 100;


//NOTE! this is a really crude way to allocate all necessary memory for the app but nothing will resize and so if the user decides to request
// memory that can't fit into the block, invalid is sent back and the user has to account for that. Also this scheme is not very flexible
// for adding new types that need to be heap allocated, since you have to set maximums for that and add it to the Allocate() function.
// I'm thinking of converting all of these to std::vectors so that they can be contiguous as well as resizable. I also can make the allocate
// function some sort of templated thing with variable types but need to research a good way to do that while keeping the structure pretty similar 



template<typename T>
struct MemoryRequest
{
    bool valid;
    T* memory;
    uint32 index;
};

template<typename T>
struct MemoryBlock
{
    T* block;
    uint32 itemCapacity;
    uint32 itemIndexIntoBlock;

    bool CanFit(uint32 amountOfItems)
    {
        return itemIndexIntoBlock + amountOfItems < itemCapacity;
    }

    MemoryRequest<T> RequestFromBlock(uint32 amountOfItems)
    {
        MemoryRequest<T> request;
        request.valid = CanFit(amountOfItems);
        
        request.memory = block+itemIndexIntoBlock;
        request.index = itemIndexIntoBlock;

        if (request.valid) 
            itemIndexIntoBlock += amountOfItems;
    
        return request;
    }
};

struct GameMemory
{
    uint64 totalAllocation;
    void* memory;

    MemoryBlock<Vertex> meshVertexBlock;
    MemoryBlock<uint16> meshIndexBlock;
    MemoryBlock<Vertex> geometryMeshVertexBlock;
    MemoryBlock<uint16> geometryMeshIndexBlock;
    MemoryBlock<SkinnedVertex> skinnedMeshVertexBlock;
    MemoryBlock<uint16> skinnedMeshIndexBlock;
    MemoryBlock<GeometryNode> levelNodeBlock;
    MemoryBlock<NodeCell> levelCellBlock;
    MemoryBlock<Transform> transforms;
    MemoryBlock<TransformHierarchyNode> transformHierarchyNodes;
    MemoryBlock<Material> materials;
    MemoryBlock<MeshBufferHandle> meshBufferHandles;

    void Allocate()
    {
        meshVertexBlock.itemCapacity = MESH_BLOCK_MAX_VERTS;
        meshIndexBlock.itemCapacity  = MESH_BLOCK_MAX_INDEX;
        geometryMeshVertexBlock.itemCapacity = GEO_BLOCK_MAX_VERTS;
        geometryMeshIndexBlock.itemCapacity  = GEO_BLOCK_MAX_INDEX;
        skinnedMeshVertexBlock.itemCapacity = MAX_SKINNED_VERTS;
        skinnedMeshIndexBlock.itemCapacity = MAX_SKINNED_INDEX;
        levelNodeBlock.itemCapacity  = MAX_LEVEL_NODES;
        levelCellBlock.itemCapacity  = MAX_LEVEL_CELLS;
        transforms.itemCapacity = MAX_TRANSFORMS;
        transformHierarchyNodes.itemCapacity = MAX_TRANSFORM_HIERARCHY_NODES;
        materials.itemCapacity = MAX_MATERIALS;
        meshBufferHandles.itemCapacity = MAX_SOURCE_MESHES;

        uint64 vertSize      = MESH_BLOCK_MAX_VERTS * sizeof(Vertex);
        uint64 indexSize     = MESH_BLOCK_MAX_INDEX * sizeof(uint16);
        uint64 geoVertSize   = GEO_BLOCK_MAX_VERTS * sizeof(Vertex);
        uint64 geoIndexSize  = GEO_BLOCK_MAX_INDEX * sizeof(uint16);
        uint64 skinVertSize  = MAX_SKINNED_VERTS * sizeof(SkinnedVertex);
        uint64 skinIndexSize = MAX_SKINNED_INDEX * sizeof(uint16);
        uint64 levelNodeSize = MAX_LEVEL_NODES * sizeof(GeometryNode);
        uint64 levelCellSize = MAX_LEVEL_CELLS * sizeof(NodeCell);
        uint64 transformSize = MAX_TRANSFORMS * sizeof(Transform);
        uint64 transformHierarchyNodeSize = MAX_TRANSFORM_HIERARCHY_NODES * sizeof(TransformHierarchyNode);
        uint64 materialSize = MAX_MATERIALS * sizeof(Material);
        uint64 meshBufferHandleSize = MAX_SOURCE_MESHES * sizeof(MeshBufferHandle);
        totalAllocation = vertSize + indexSize + geoVertSize +
                geoIndexSize + skinVertSize + skinIndexSize + levelNodeSize +
                levelCellSize + transformSize + transformHierarchyNodeSize +
                materialSize + meshBufferHandleSize;

        memory = SDL_malloc(totalAllocation);
        byte* curByte = (byte*)memory;
        meshVertexBlock.block = (Vertex*)curByte;
        curByte += vertSize;
        meshIndexBlock.block = (uint16*)curByte;
        curByte += indexSize;
        geometryMeshVertexBlock.block = (Vertex*)curByte;
        curByte += geoVertSize;
        geometryMeshIndexBlock.block = (uint16*)curByte;
        curByte += geoIndexSize;
        skinnedMeshVertexBlock.block = (SkinnedVertex*)curByte;
        curByte += skinVertSize;
        skinnedMeshIndexBlock.block = (uint16*)curByte;
        curByte += skinIndexSize;
        levelNodeBlock.block = (GeometryNode*)curByte;
        curByte += levelNodeSize;
        levelCellBlock.block = (NodeCell*)curByte;
        curByte += levelCellSize;
        transforms.block = (Transform*)curByte;
        curByte += transformSize;
        transformHierarchyNodes.block = (TransformHierarchyNode*)curByte;
        curByte += transformHierarchyNodeSize;
        materials.block = (Material*)curByte;
        curByte += materialSize;
        meshBufferHandles.block = (MeshBufferHandle*)curByte;
        curByte += meshBufferHandleSize;
    }
};

const uint32 MAX_RENDER_ENTITIES = 1000;
const uint32 MAX_SKINNED_RENDER_ENTITIES = 100;
struct EntityManager
{
    RenderEntity renderEntities[MAX_RENDER_ENTITIES];
    SkinnedRenderEntity skinnedRenderEntities[MAX_SKINNED_RENDER_ENTITIES];

    uint32 playerEntity;
    uint32 cameraEntity;

    uint32 renderEntityCount;
    uint32 skinndedRenderEntityCount;

    EntityManager()
    {
         renderEntityCount = 0;
         skinndedRenderEntityCount = 0;
    }

    RenderEntity* CreateRenderEntity(uint32 transformIndex, uint32 meshBufferIndex, uint32 materialIndex)
    {
        ASSERT(renderEntityCount+1 < MAX_RENDER_ENTITIES, __FILE__, __LINE__);
        RenderEntity* value = &renderEntities[renderEntityCount];
        value->transformIndex = transformIndex;
        value->meshBufferIndex = meshBufferIndex;
        value->materialIndex = materialIndex;
        renderEntityCount++;
        return value;
    }
};
