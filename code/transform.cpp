
#include "types.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>


void TRS(mat4f& transform, vec3f translation, quaternion rotation = quaternion(1.0f, 0.0f, 0.0f, 0.0f), vec3f scale = vec3f(1.0f, 1.0f, 1.0f))
{
	transform = glm::translate(transform, translation);
	transform *= glm::toMat4(rotation);
	transform = glm::scale(transform, scale);
}

void TRS(mat4f& matrix, Transform& transform)
{
	matrix = glm::translate(matrix, transform.position);
	matrix *= glm::toMat4(transform.rotation);
	matrix = glm::scale(matrix, transform.scale);
}
