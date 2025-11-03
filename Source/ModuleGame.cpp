#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModuleGame.h"
#include "ModuleAudio.h"
#include "ModulePhysics.h"

class PhysicEntity
{
protected:

	PhysicEntity(PhysBody* _body, Module* _listener)
		: body(_body)
		, listener(_listener)
	{
		body->listener = listener;
	}

public:
	virtual ~PhysicEntity() = default;
	virtual void Update() = 0;

	virtual int RayHit(vec2<int> ray, vec2<int> mouse, vec2<float>& normal)
	{
		return 0;
	}

protected:
	PhysBody* body;
	Module* listener;
};

class Circle : public PhysicEntity
{
public:
	Circle(ModulePhysics* physics, int _x, int _y, Module* _listener, Texture2D _texture)
		: PhysicEntity(physics->CreateCircle(_x, _y, 15), _listener)//radio de colision de la pelota
		, texture(_texture)
	{

	}

	void Update() override
	{
		int x, y;
		body->GetPhysicPosition(x, y);
		Vector2 position{ (float)x, (float)y };
		//float scale = 0.5f;
		//Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
		//Rectangle dest = { position.x, position.y, (float)texture.width * scale, (float)texture.height * scale };
		//Vector2 origin = { (float)texture.width / 2.0f, (float)texture.height / 2.0f };
		//float rotation = body->GetRotation() * RAD2DEG;
		//DrawTexturePro(texture, source, dest, origin, rotation, WHITE);
		float desired_radius = 15.0f;

		// Calcula la escala para que el ancho de la textura coincida con el diámetro físico
		float scale = (desired_radius * 2.0f) / (float)texture.width;

		Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
		Rectangle dest = { position.x, position.y, (float)texture.width * scale, (float)texture.height * scale };
		Vector2 origin = { (float)texture.width * scale / 2.0f, (float)texture.height * scale / 2.0f };
		float rotation = body->GetRotation() * RAD2DEG;

		DrawTexturePro(texture, source, dest, origin, rotation, WHITE);
	}

private:
	Texture2D texture;

};

class Box : public PhysicEntity
{
public:
	Box(ModulePhysics* physics, int _x, int _y, Module* _listener, Texture2D _texture)
		: PhysicEntity(physics->CreateRectangle(_x, _y, 100, 50), _listener)
		, texture(_texture)
	{

	}

	void Update() override
	{
		int x, y;
		body->GetPhysicPosition(x, y);
		DrawTexturePro(texture, Rectangle{ 0, 0, (float)texture.width, (float)texture.height },
			Rectangle{ (float)x, (float)y, (float)texture.width, (float)texture.height },
			Vector2{ (float)texture.width / 2.0f, (float)texture.height / 2.0f}, body->GetRotation() * RAD2DEG, WHITE);
	}

	int RayHit(vec2<int> ray, vec2<int> mouse, vec2<float>& normal) override
	{
		return body->RayCast(ray.x, ray.y, mouse.x, mouse.y, normal.x, normal.y);;
	}

private:
	Texture2D texture;

};

class Rick : public PhysicEntity
{
public:
	// Pivot 0, 0
	static constexpr int rick_head[64] = {
			14, 36,
			42, 40,
			40, 0,
			75, 30,
			88, 4,
			94, 39,
			111, 36,
			104, 58,
			107, 62,
			117, 67,
			109, 73,
			110, 85,
			106, 91,
			109, 99,
			103, 104,
			100, 115,
			106, 121,
			103, 125,
			98, 126,
			95, 137,
			83, 147,
			67, 147,
			53, 140,
			46, 132,
			34, 136,
			38, 126,
			23, 123,
			30, 114,
			10, 102,
			29, 90,
			0, 75,
			30, 62
	};

	Rick(ModulePhysics* physics, int _x, int _y, Module* _listener, Texture2D _texture)
		: PhysicEntity(physics->CreateChain(GetMouseX() - 50, GetMouseY() - 100, rick_head, 64), _listener)
		, texture(_texture)
	{

	}

	void Update() override
	{
		int x, y;
		body->GetPhysicPosition(x, y);
		DrawTextureEx(texture, Vector2{ (float)x, (float)y }, body->GetRotation() * RAD2DEG, 1.0f, WHITE);
	}

private:
	Texture2D texture;
};



ModuleGame::ModuleGame(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	ray_on = false;
	sensed = false;
}

ModuleGame::~ModuleGame()
{}

// Load assets
bool ModuleGame::Start()
{
	LOG("Loading Intro assets");
	bool ret = true;

	App->renderer->camera.x = App->renderer->camera.y = 0;

	fondo = LoadTexture("Assets/game_back2.png");
	//pala_left= LoadTexture("Assets/boardL2.png");
	////Image pala_leftt = LoadImageFromTexture(pala_left);
	////ImageResize(&pala_leftt, 30, 30);
	////UnloadTexture(pala_left);
	////Texture2D resizedTexture = LoadTextureFromImage(pala_leftt);
	//pala_right= LoadTexture("Assets/boardR2.png");


	circle = LoadTexture("Assets/ball0001.png"); 
	box = LoadTexture("Assets/crate.png");
	rick = LoadTexture("Assets/rick_head.png");
	
	bonus_fx = App->audio->LoadFx("Assets/bonus.wav");

	
	sensor = App->physics->CreateRectangleSensor(SCREEN_WIDTH / 2, SCREEN_HEIGHT, SCREEN_WIDTH, 50);
	//int ANCHO_FISICO_PALA = pala_left.width; 
	//int ALTO_FISICO_PALA = pala_left.height;  

	//int PIVOTE_X = 340; // El '460' que ya tenías
	//int PIVOTE_Y = 395; // El '295' que ya tenías

	//PhysBody* pivote_L = App->physics->CreateRectangle(PIVOTE_X, PIVOTE_Y, 5, 5);
	//pivote_L->body->SetType(b2_staticBody);


	//int pala_center_x = PIVOTE_X;
	//int pala_center_y = PIVOTE_Y;

	//pala_l = App->physics->CreateRectangle(pala_center_x, pala_center_y, ANCHO_FISICO_PALA/2, ALTO_FISICO_PALA/2);
	//pala_l->body->SetType(b2_dynamicBody);

	//// 6. Crea la junta (esto lo tenías perfecto)
	//b2RevoluteJointDef jointDef;
	//jointDef.Initialize(pivote_L->body, pala_l->body, pivote_L->body->GetWorldCenter());
	//jointDef.enableMotor = true;
	//jointDef.maxMotorTorque = 1000.0f;
	//jointDef.motorSpeed = 0.0f;
	//jointDef.enableLimit = true;
	//jointDef.lowerAngle = -0.25f * b2_pi;
	//jointDef.upperAngle = 0.20f * b2_pi;

	//pala_l_joint = App->physics->CreateJoint(&jointDef);
	

	
	//---------------------------------CREACIÓN FISICAS MAPA----------------------------------------//

	int game_back1[20] = {
	274, 121,
	303, 142,
	303, 340,
	340, 383,
	331, 398,
	291, 354,
	289, 154,
	267, 140,
	266, 127,
	273, 121
	};


	for (int i = 0; i < 20; i++)
	{
		PIXEL_TO_METERS(game_back1[i]);
	}
	App->physics->CreateChain(0, 0, game_back1, 20);



	int game_back2[20] = {
	527, 121,
	529, 140,
	507, 152,
	507, 349,
	468, 397,
	460, 391,
	457, 383,
	493, 337,
	491, 142,
	526, 121
	};
	for (int i = 0; i < 20; i++)
{
	PIXEL_TO_METERS(game_back2[i]);
}
	App->physics->CreateChain(0, 0, game_back2, 20);


	int game_back3[36] = {
	500, 35,
	512, 17,
	511, 10,
	287, 10,
	286, 15,
	296, 34,
	275, 35,
	253, 47,
	246, 55,
	240, 70,
	240, 478,
	558, 478,
	558, 118,
	558, 71,
	548, 53,
	535, 41,
	521, 36,
	501, 35
	};
	for (int i = 0; i < 36; i++)
	{
		PIXEL_TO_METERS(game_back3[i]);
	}
	App->physics->CreateChain(0, 0, game_back3, 36);

	int game_back4[34] = {
		489, 47,
		490, 37,
		498, 19,
		301, 20,
		309, 37,
		311, 47,
		277, 47,
		264, 53,
		255, 59,
		246, 74,
		247, 461,
		551, 463,
		551, 82,
		546, 67,
		537, 55,
		521, 48,
		490, 47
	};

	for (int i = 0; i < 34; i++)
	{
		PIXEL_TO_METERS(game_back4[i]);
	}
	App->physics->CreateChain(0, 0, game_back4, 34);

	int game_back5[10] = {
	388, 441,
	411, 441,
	412, 458,
	387, 458,
	387, 441
	};
	for (int i = 0; i < 10; i++)
	{
		PIXEL_TO_METERS(game_back5[i]);
	}
	App->physics->CreateChain(0, 0, game_back5, 10);

	return ret;
}

// Load assets
bool ModuleGame::CleanUp()
{
	LOG("Unloading Intro scene");

	return true;
}

// Update: draw background
update_status ModuleGame::Update()
{
	App->renderer->Draw(fondo, 0, 0);

	//if (IsKeyPressed(KEY_LEFT)) {
	//	// Aplica velocidad al motor para "subir"
	//	pala_l_joint->SetMotorSpeed(-20.0f); // Velocidad negativa (anti-horario)
	//}
	//else {
	//	// Aplica velocidad para "bajar"
	//	pala_l_joint->SetMotorSpeed(10.0f);
	//}

	if(IsKeyPressed(KEY_SPACE))
	{
		ray_on = !ray_on;
		ray.x = GetMouseX();
		ray.y = GetMouseY();
	}

	if(IsKeyPressed(KEY_ONE))
	{
		entities.emplace_back(new Circle(App->physics, GetMouseX(), GetMouseY(), this, circle));
		
	}

	if(IsKeyPressed(KEY_TWO))
	{
		entities.emplace_back(new Box(App->physics, GetMouseX(), GetMouseY(), this, box));
	}

	if(IsKeyPressed(KEY_THREE))
	{
		entities.emplace_back(new Rick(App->physics, GetMouseX(), GetMouseY(), this, rick));
	}

	// Prepare for raycast ------------------------------------------------------
	
	vec2i mouse;
	mouse.x = GetMouseX();
	mouse.y = GetMouseY();
	int ray_hit = ray.DistanceTo(mouse);

	vec2f normal(0.0f, 0.0f);

	//int x, y;
	//pala_l->GetPhysicPosition(x, y);

	//
	//// -- FIN DE LOS AJUSTES --
	//DrawTexturePro(pala_left,
	//	Rectangle{ 0, 0, (float)pala_left.width, (float)pala_left.height },
	//	Rectangle{ (float)x, (float)y, (float)pala_left.width*0.2f, (float)pala_left.height*0.2f },
	//	Vector2{ (float)pala_left.width / 2.0f, (float)pala_left.height / 2.0f },
	//	pala_l->GetRotation() * RAD2DEG, 
	//	WHITE);



	for (PhysicEntity* entity : entities)
	{
		entity->Update();
		if (ray_on)
		{
			int hit = entity->RayHit(ray, mouse, normal);
			if (hit >= 0)
			{
				ray_hit = hit;
			}
		}
	}
	

	// ray -----------------
	if(ray_on == true)
	{
		vec2f destination((float)(mouse.x-ray.x), (float)(mouse.y-ray.y));
		destination.Normalize();
		destination *= (float)ray_hit;

		DrawLine(ray.x, ray.y, (int)(ray.x + destination.x), (int)(ray.y + destination.y), RED);

		if (normal.x != 0.0f)
		{
			DrawLine((int)(ray.x + destination.x), (int)(ray.y + destination.y), (int)(ray.x + destination.x + normal.x * 25.0f), (int)(ray.y + destination.y + normal.y * 25.0f), Color{ 100, 255, 100, 255 });
		}
	}

	return UPDATE_CONTINUE;
}

void ModuleGame::OnCollision(PhysBody* bodyA, PhysBody* bodyB)
{
	App->audio->PlayFx(bonus_fx);
}
