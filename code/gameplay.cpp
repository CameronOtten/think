
#include "types.h"

const float32 FPS_CAMERA_SPEED = 15.0f;
const float32 FPS_CAMERA_SENSITIVITY = 600.0f;

const float32 PLAYER_SPEED = 5.0f;
const float32 PLAYER_RADIUS = 0.4f;
const float32 PLAYER_RADIUS_OFFSET = 0.0f;
const float32 GAMEPLAY_CAMERA_HEIGHT = 5.0f;
const float32 GAMEPLAY_CAMERA_Z = -5.0f;

enum GameStateType
{
	GameStateMenu,
	GameStateGameplay,
	GameStateDevFly,
	GameStateDevEdit
};

enum EntityActionState
{
	EntityActionMove,
	EntityActionFall,
	EntityActionJump,
	EntityActionDodge
};

struct EntityState
{
	uint32 transformIndex;
	float32 currentSpeed;
	float32 gravity;
	EntityActionState actionState;
};

struct GameState
{
	Application application;
	GameStateType state;
	float32 deltaTime;
	float32 timeScale;
	EntityManager entityManager;
    EntityState cameraState;
	EntityState playerState;
	Level level;
};

vec3f ConstructMoveVectorFromInput(const GameInput& gameInput)
{
	vec3f dir = vec3f();
	float32 up    = gameInput.actions[GameActionTypeMoveUp].state;
	float32 down  = gameInput.actions[GameActionTypeMoveDown].state;
	float32 left  = gameInput.actions[GameActionTypeMoveLeft].state;
	float32 right = gameInput.actions[GameActionTypeMoveRight].state;
	dir.z  = up > 0 ? up : down > 0 ? -down : 0;
	dir.x  = right > 0 ? right : left > 0 ? -left : 0;
	if (dir.x > 0 || dir.x < 0 || dir.z > 0 || dir.z < 0 )
		dir = glm::normalize(dir);

	return dir;
}

vec2f ConstructLookVectorFromInput(const GameInput& gameInput)
{
	vec2f look;
	look.y  = gameInput.actions[GameActionTypeLookUp].state;
	look.y += gameInput.actions[GameActionTypeLookDown].state;
	look.x  = gameInput.actions[GameActionTypeLookLeft].state;
	look.x += gameInput.actions[GameActionTypeLookRight].state;
	return look;
}


const float EPSILON = 0.00001f;
void UpdateGame(GameState& gameState, const GameInput& gameInput, GameMemory& memory, DebugMemory& Debug)
{
	Transform* transforms = memory.transforms.block;
	float32 dt = gameState.deltaTime*gameState.timeScale;
	switch(gameState.state)
	{
		case GameStateMenu:
		{
			break;
		}
		case GameStateGameplay:
		{

			vec3f& position = transforms[gameState.entityManager.playerEntity].position;
			float32 playerGravity = gameState.playerState.gravity;
			vec3f dir = ConstructMoveVectorFromInput(gameInput);
			switch (gameState.playerState.actionState)
			{
				case EntityActionMove:
				{
					//if (dir.x < -EPSILON || dir.x > EPSILON ||
					//	dir.z < -EPSILON || dir.z > EPSILON)
					{
						//dir = glm::normalize(dir);
						float32 magnitude = PLAYER_SPEED * dt;
						vec3f velocity = dir * magnitude;
						playerGravity -= dt * 5;
						playerGravity = glm::max(playerGravity,-10.0f);

						// RayIntersection hit = SphereCollisionCheck(position,
						// 	 					vec3f(0.0f,0.0f,-0.25f), gameState.level,
						// 	 					PLAYER_RADIUS, Debug, Debug.DebugPlayerCollision);
						// if (hit.hit)
						// {
						// 	playerGravity = 0.0f;
						// 	if (gameInput.actions[GameActionTypeJump].beingPressed)
						// 		playerGravity = 10.0f;
						// }

						//velocity.z = playerGravity*dt;
						gameState.playerState.gravity = playerGravity;
						RayIntersection hit = SphereCollisionCheck(position,
							 					velocity, gameState.level,
							 					PLAYER_RADIUS, Debug, Debug.DebugPlayerCollision);

						Debug.LineBuffer.AddLine(position, position + (dir*0.5f), vec3f(1,1,0));

						if (hit.hit)
						{
							// if (hit.fraction * magnitude >= PLAYER_RADIUS_OFFSET)
							// 	hit.fraction = 0;
							Debug.LineBuffer.AddLine(hit.point, hit.point+hit.normal, vec3f(1,0,0));
							float32 angle = glm::dot(hit.normal,-dir);
							if (angle > 0.33f && angle < 0.77f)
							{
								vec3f tanA = glm::cross(hit.normal,vec3f(0,1,0));
								vec3f tanB = glm::cross(hit.normal,vec3f(0,-1,0));
								Debug.LineBuffer.AddLine(hit.point, hit.point + tanA, vec3f(0,0,1));
								Debug.LineBuffer.AddLine(hit.point, hit.point + tanB, vec3f(0,0,1));
								float32 dotA = glm::dot(tanA,dir);
								float32 dotB = glm::dot(tanB,dir);
								velocity = (dotA > dotB ? tanA * magnitude : tanB * magnitude) *0.5f;
								RayIntersection reflectedHit = SphereCollisionCheck(position,
													 velocity, gameState.level, PLAYER_RADIUS,
													  Debug, Debug.DebugPlayerCollision);
							
								if (reflectedHit.hit)
								{
									// if (hit.fraction * magnitude > PLAYER_RADIUS_OFFSET)
									// 	hit.fraction = 0;
							
									velocity *= hit.fraction;
								}
							}
							else
							{
								velocity *= hit.fraction;
								// const float WALL_THRESHOLD = 0.5f;
								// if (hit.fraction <= WALL_THRESHOLD)
								// 	velocity = vec3f(0);
						 	}

					 	}
							 std::string text = hit.hit ? ("Intersection: " + std::to_string(hit.fraction)) : "none";
							 std::string text2 = "Normal: " + std::to_string(hit.normal.x) +
							  								"," + std::to_string(hit.normal.y) +
															"," + std::to_string(hit.normal.z);
					 		AddTextToQuadBuffer(text, vec2f(-.9f,.6f), 0.05f, Debug.FontAtlas,
					                 gameState.application.aspectRatio, &Debug.TextBuffer);
							 AddTextToQuadBuffer(text2, vec2f(-.9f,.55f), 0.05f, Debug.FontAtlas,
						                 gameState.application.aspectRatio, &Debug.TextBuffer);
						Debug.LineBuffer.AddLine(position, position + velocity * 10.0f, vec3f(0,1,0));
						position += velocity;
					}

					vec3f& cameraPosition = memory.transforms.block[gameState.entityManager.cameraEntity].position;
					cameraPosition = position + vec3f(0.0f,GAMEPLAY_CAMERA_HEIGHT,GAMEPLAY_CAMERA_Z);

					break;
				}
				case EntityActionFall:
				{
					break;
				}
				case EntityActionJump:
				{
					break;
				}
				case EntityActionDodge:
				{
					break;
				}
			}

			break;
		}
		case GameStateDevFly:
		{
			//free roam camera movement
			vec3f& camPos = transforms[gameState.entityManager.cameraEntity].position;
			quaternion& camRot = transforms[gameState.entityManager.cameraEntity].rotation;

			vec3f dir = ConstructMoveVectorFromInput(gameInput);
			//normalize here keeps returning NANs in some cases, i suppose glm doesnt
			//really filter for those cases
			// if (glm::abs(dir.x) > 0 || glm::abs(dir.y) > 0 || glm::abs(dir.z) > 0)
			//     dir = glm::normalize(dir);
			dir *= FPS_CAMERA_SPEED*dt;

			vec2f look = ConstructLookVectorFromInput(gameInput);
			look *= FPS_CAMERA_SENSITIVITY*dt;

			vec3f rightAxis = camRot * vec3f(1.0f, 0.0f, 0.0f);
			vec3f upAxis = camRot * vec3f(0.0f, 1.0f, 0.0f);
			vec3f forwardAxis = camRot * vec3f(0.0f, 0.0f, 1.0f);
			camPos += rightAxis*dir.x;
			camPos += forwardAxis*dir.z;

			//local to world: newRot = oldRot*(inverseOldRot*worldRot)*oldRot
			quaternion worldRot = glm::angleAxis(look.x, vec3f(0.0f, 1.0f, 0.0f));
			quaternion inverse = glm::inverse(camRot);
			camRot = camRot * inverse * worldRot * camRot;
			camRot *= glm::angleAxis(look.y, vec3f(1.0f, 0.0f, 0.0f));
			//end free roam
			break;
		}
		case GameStateDevEdit:
		{
			break;
		}
	}

}
