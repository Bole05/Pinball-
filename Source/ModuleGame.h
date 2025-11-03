#pragma once

#include "Globals.h"
#include "Module.h"

#include "p2Point.h"

#include "raylib.h"
#include <vector>
#include "Box2D/Box2D.h"
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

public:

	std::vector<PhysicEntity*> entities;
	
	PhysBody* sensor;
	PhysBody* pala_r;
	PhysBody* pala_l;
	PhysBody* goalSensor;
	PhysBody* circleBody;
	bool sensed;
	bool resetPending;
	

	Texture2D circle;
	Texture2D box;
	Texture2D rick;
	Texture2D fondo;
	Texture2D pala_right;
	Texture2D pala_left;

	b2RevoluteJoint* pala_l_joint; 
	b2RevoluteJoint* pala_r_joint;

	uint32 bonus_fx;

	vec2<int> ray;
	bool ray_on;
	int score = -4;
};
