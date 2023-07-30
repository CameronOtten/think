#include "resources.h"
#include "log.h"
#include "glm/glm.hpp"


#include "level.h"

inline uint32 LinearIndex(uint32 x, uint32 y, uint32 z, uint32 width)
{
	return (x*width) + (y*width*width) + z;
}

inline int32 LevelCellIndexFromThreeDIndex(vec3i threeD, uint32 levelWidth, uint32 levelHeight)
{
	int32 index = -1;
	if ((threeD.x < levelWidth  && threeD.x >= 0) &&
		(threeD.y < levelHeight && threeD.y >= 0) &&
		(threeD.z < levelWidth  && threeD.z >= 0))
	{
		index = LinearIndex((uint32)threeD.x,(uint32)threeD.y,(uint32)threeD.z, levelWidth);
	}

	return index;
}


inline vec3i LevelCellThreeDIndexFromPosition(vec3f worldPosition)
{
	vec3i index;
	index.x = (int32)glm::round(worldPosition.x/CELL_SIZE);
	index.y = (int32)glm::round(worldPosition.y/CELL_SIZE);
	index.z = (int32)glm::round(worldPosition.z/CELL_SIZE);

	return index;
}

inline int32 LevelCellIndexFromPosition(vec3f worldPosition, uint32 levelWidth, uint32 levelHeight)
{
	vec3i threeD = LevelCellThreeDIndexFromPosition(worldPosition);
	return LevelCellIndexFromThreeDIndex(worldPosition,levelWidth, levelHeight);
}

Level GenerateLevelCells(const LevelData& levelData, GameMemory& memory)
{
    uint32 levelWidth = levelData.width;
	uint32 levelCellWidth = levelWidth+1;
	uint32 levelCellHeight = levelData.height;
    uint32 cellCount = levelCellWidth*levelCellWidth*levelCellHeight;

    int typeCount[MAX_NODE_TYPES];
    for (int i = 0; i < MAX_NODE_TYPES; i++)
        typeCount[i] = 0;

	Level level;
	ASSERT((memory.levelCellBlock.CanFit(cellCount)), __FILE__, __LINE__);
	NodeCell* cells = memory.levelCellBlock.block;
	level.cells = cells;
    level.width = levelCellWidth;
    level.height = levelCellHeight;

	GeometryNode* node = &levelData.nodes[0];
    byte  A, B, C, D, Br, Cr, Dr, SA, SB, SC, SD;
    uint32 index;
	for (uint32 y = 0; y < levelCellHeight; y++)
	for (uint32 x = 0; x < levelCellWidth; x++)
	for (uint32 z = 0; z < levelCellWidth; z++)
	{
        index = LinearIndex(x-1, y, z-1, levelWidth);
        bool xZero = x == 0;
        bool zZero = z == 0;
        bool xEdge = x == levelCellWidth-1;
        bool zEdge = z == levelCellWidth-1;
        A = (xZero||zZero) ? 0 : levelData.nodes[index].type;
        B = (xZero||zEdge) ? 0 : levelData.nodes[index+1].type;
        C = (zEdge||xEdge) ? 0 : levelData.nodes[index+levelWidth+1].type;
        D = (xEdge||zZero) ? 0 : levelData.nodes[index+levelWidth].type;
		SA = (xZero||zZero) ? 0 : levelData.nodes[index].slope;
        SB = (xZero||zEdge) ? 0 : levelData.nodes[index+1].slope;
        SC = (zEdge||xEdge) ? 0 : levelData.nodes[index+levelWidth+1].slope;
        SD = (xEdge||zZero) ? 0 : levelData.nodes[index+levelWidth].slope;

        if (A>0) typeCount[A-1]++;

        cells->types[0] = A;
        cells->types[1] = B;
        cells->types[2] = C;
        cells->types[3] = D;
		cells->slopes[0] = SA;
        cells->slopes[1] = SB;
        cells->slopes[2] = SC;
        cells->slopes[3] = SD;

        //Br, Cr, and Dr represent relative difference between the A node (back left corner of quadrant)
        //So 0 is the same as A, 1 is the first unique type from A, 2 the second unique, and 3 the third unique
        if (B == A)
            Br = 0;
        else Br = 1;
        if (C == A)
            Cr = 0;
        else Cr = C == B ? Br : Br+1;
        if (D == A)
            Dr = 0;
        else Dr = D == B ? Br : D == C ? Cr : Cr+1;


        //Br is packed into bytes 1-2, Cr into bytes 3-4, and Dr into bytes 5-6 for all unique numbers
        //This is an extremely wierd way of doing it but is waaaaaaay faster than ridiculous branching alternative
        byte value = 0;
        value |= Br << 0;
        value |= Cr << 2;
        value |= Dr << 4;

        //Shape Key:  0 = Nothing, 1 = Full, 2 = Side, 3 = Inner, 4 = Outer, 5 = Corner
        switch (value)
        {
            case 0: //0000
                cells->shapes[0] = 1;
                cells->shapes[1] = 0;
                cells->shapes[2] = 0;
                cells->shapes[3] = 0;
                break;
            case 1: //0100
                cells->shapes[0] = 0;
                cells->shapes[1] = 4;
                cells->shapes[2] = 0;
                cells->shapes[3] = 3;
                break;
            case 4: //0010
                cells->shapes[0] = 3;
                cells->shapes[1] = 0;
                cells->shapes[2] = 4;
                cells->shapes[3] = 0;
                break;
            case 16://0001
                cells->shapes[0] = 0;
                cells->shapes[1] = 3;
                cells->shapes[2] = 0;
                cells->shapes[3] = 4;
                break;
            case 5://0110
                cells->shapes[0] = 0;
                cells->shapes[1] = 2;
                cells->shapes[2] = 0;
                cells->shapes[3] = 2;
                break;
            case 20://0011
                cells->shapes[0] = 2;
                cells->shapes[1] = 0;
                cells->shapes[2] = 2;
                cells->shapes[3] = 0;
                break;
            case 9://0120
                cells->shapes[0] = 0;
                cells->shapes[1] = 5;
                cells->shapes[2] = 5;
                cells->shapes[3] = 2;
                break;
            case 36://0012
                cells->shapes[0] = 2;
                cells->shapes[1] = 0;
                cells->shapes[2] = 5;
                cells->shapes[3] = 5;
                break;
            case 37://0112
                cells->shapes[0] = 5;
                cells->shapes[1] = 2;
                cells->shapes[2] = 0;
                cells->shapes[3] = 5;
                break;
            case 41://0122
                cells->shapes[0] = 5;
                cells->shapes[1] = 5;
                cells->shapes[2] = 2;
                cells->shapes[3] = 0;
                break;
            case 21://0111
                cells->shapes[0] = 4;
                cells->shapes[1] = 0;
                cells->shapes[2] = 3;
                cells->shapes[3] = 0;
                break;
            case 25://0121
                cells->shapes[0] = 5;
                cells->shapes[1] = 5;
                cells->shapes[2] = 5;
                cells->shapes[3] = 5;
                break;
            case 17://0101
                cells->shapes[0] = 5;
                cells->shapes[1] = 5;
                cells->shapes[2] = 5;
                cells->shapes[3] = 5;
                break;
            case 33://0102
                cells->shapes[0] = 5;
                cells->shapes[1] = 5;
                cells->shapes[2] = 5;
                cells->shapes[3] = 5;
                break;
            case 57://0123
                cells->shapes[0] = 5;
                cells->shapes[1] = 5;
                cells->shapes[2] = 5;
                cells->shapes[3] = 5;
                break;
        }

		cells->position = vec3f(CELL_SIZE*x,CELL_SIZE*y,CELL_SIZE*z);
		cells++;
	}

    uint32 uniqueCount = 0;
    for (int i = 0; i < MAX_NODE_TYPES; i++)
    {
        level.nodeTypeGeometyBufferMap[i] = typeCount[i] > 0 ? uniqueCount : -1;
        uniqueCount += typeCount[i] > 0;
    }

    level.numUniqueNodeTypes = uniqueCount;

    cells = level.cells;
    byte otherShape, otherType;
    uint32  layerSize = levelCellWidth*levelCellWidth;
    byte layer;

    for (uint32 y = 0; y < levelCellHeight; y++)
	for (uint32 x = 0; x < levelCellWidth;  x++)
	for (uint32 z = 0; z < levelCellWidth;  z++)
	{
        for (uint32 i = 0; i < 4; i++)
        {
            layer = 0;

            otherShape = y == 0 ? 0 : (cells-layerSize)->shapes[i];
            otherType  = y == 0 ? 0 : (cells-layerSize)->types [i];
            layer = (cells->shapes[i] == otherShape && cells->types[i] == otherType);

            otherShape = y == levelCellHeight-1 ? 0 : (cells+layerSize)->shapes[i];
            otherType  = y == levelCellHeight-1 ? 0 : (cells+layerSize)->types [i];
            layer = (cells->shapes[i] == otherShape && cells->types[i] == otherType) ? layer : 2;

			if (layer != 1 && cells->shapes[i] > 1 && cells->slopes[i])
				layer = 3;

            cells->layers[i] = layer;
        }

        cells++;
    }

	return level;
}
