
enum GameActionType
{
	GameActionTypeMoveUp,
	GameActionTypeMoveDown,
	GameActionTypeMoveLeft,
	GameActionTypeMoveRight,
	GameActionTypeLookUp,
	GameActionTypeLookDown,
	GameActionTypeLookLeft,
	GameActionTypeLookRight,
	GameActionTypeDodge,
	GameActionTypeLightSwing,
	GameActionTypeHeavySwing,
	GameActionTypeJump,

	GameActionTypeCount
};

struct GameAction
{
	float32 state;
	bool beingPressed;
	bool beingReleased;
};

struct GameInput
{
	GameAction actions[GameActionTypeCount];
};

struct InputMap
{
	SDL_Scancode keys[GameActionTypeCount];
};
