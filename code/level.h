#pragma once

#include "glm/glm.hpp"


/*
* struct NodeKey //260 bytes
* {
*	Mesh* meshes[4];
*	unsigned char rotations[4];
* }
*/

struct Level
{
	//glm::vec3 origin;
	NodeCell* cells;
	uint32 width;
	uint32 height;
	uint32 numUniqueNodeTypes;
	int nodeTypeGeometyBufferMap[MAX_NODE_TYPES];

	TransformHierarchy* objects;
	uint32 objectCount;
};
