#pragma once

#include "Globals.h"
#include "Module.h"

#include "p2Point.h"

#include "raylib.h"
#include <vector>
#ifdef _MSC_VER
#pragma warning(push)
// Suppress: Variable is uninitialized (C26495) for third-party Box2D types included here
#pragma warning(disable : 26495)
#endif
#include "Box2D/Box2D.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
class PhysBody;
class PhysicEntity;


class ModuleGame : public Module
{
public:
	ModuleGame(Application* app, bool start_enabled = true);
	~ModuleGame();

	bool Start();
	update_status Update();
	bool CleanUp();
	void OnCollision(PhysBody* bodyA, PhysBody* bodyB);
	void ResetBall();

	std::vector<PhysicEntity*> entities{};
    
	PhysBody* sensor = nullptr;
	PhysBody* pala_r = nullptr;
	PhysBody* pala_l = nullptr;
	PhysBody* goalSensor = nullptr;
	PhysBody* ownGoalSensor = nullptr;
	PhysBody* goalkeeperBody = nullptr;
	PhysBody* circleBody = nullptr;
	bool sensed = false;
	bool resetPending = false;
	bool gameStarted = false;
	int ignoreCollisionsFrames = 0;
    
	// Goalkeeper animation variables
	float goalkeeperX = 0.0f;
	float goalkeeperSpeed = 0.0f;
	bool goalkeeperMovingRight = false;
	
	// Speed scaling: 5% increase per point
	float baseGoalkeeperSpeed = 1.5f;
	float baseballVelocity = 3.0f;

	Texture2D circle{};
	Texture2D box{};
	Texture2D rick{};
	Texture2D fondo{};
	Texture2D pala_right{};
	Texture2D pala_left{};
	Texture2D goalkeeper{};
	Texture2D menuTexture{};

	b2RevoluteJoint* pala_l_joint = nullptr; 
	b2RevoluteJoint* pala_r_joint = nullptr;

	uint32 bonus_fx = 0u;

	vec2<int> ray{0, 0};
	bool ray_on = false;
	int score = 0;
	int lives = 3;
};
