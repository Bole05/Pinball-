#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModuleGame.h"
#include "ModuleAudio.h"
#include "ModulePhysics.h"

constexpr float PALA_SCALE = 0.25f;

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
		: PhysicEntity(physics->CreateCircle(_x, _y, 9), _listener)//radio de colision de la pelota
		, texture(_texture)
	{

	}

	void Update() override
	{
		int x, y;
		body->GetPhysicPosition(x, y);
		Vector2 position{ (float)x, (float)y };
		float desired_radius = 9.0f;

		// Calcula la escala para que el ancho de la textura coincida con el di�metro f�sico
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


ModuleGame::ModuleGame(Application* app, bool start_enabled) : Module(app, start_enabled)
{	
	sensed = false;
	resetPending = false;
	gameStarted = false;
	score = 0;
	lives = 3;
	ignoreCollisionsFrames = 0;
	
	// Initialize goalkeeper animation
	goalkeeperX = SCREEN_WIDTH / 2.0f;
	baseGoalkeeperSpeed = 1.5f;
	goalkeeperSpeed = baseGoalkeeperSpeed;
	goalkeeperMovingRight = true;
	
	// Initialize base ball velocity
	baseballVelocity = 3.0f;
}

ModuleGame::~ModuleGame()
{}

//-----------------------------------------------------------START--------------------------------------------------//
bool ModuleGame::Start()
{
	LOG("Loading Intro assets");
	bool ret = true;

	App->renderer->camera.x = App->renderer->camera.y = 0;

	//creacion de la textura de fondo
	fondo = LoadTexture("Assets/game_back2.png");
	
	//creacion de la textura de la pelota
	circle = LoadTexture("Assets/ball0001.png"); 

	box = LoadTexture("Assets/crate.png");
	
	// Load goalkeeper decoration texture
	goalkeeper = LoadTexture("Assets/goalkeeper.png");
	
	menuTexture = LoadTexture("Assets/menu_back.png"); // asegúrate de tener esta imagen
	//Load music and sound
	bonus_fx = App->audio->LoadFx("Assets/bonus.wav");
	App->audio->PlayMusic("Assets/Music_font.wav");
	// Create the ball automatically
	circleBody = App->physics->CreateCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 9);
	circleBody->listener = this;
	circleBody->body->SetBullet(true);
	
	//------------------------------Sistema Puntuación---------------------------------------------//

	// Portería rival (arriba) - suma puntos
	goalSensor = App->physics->CreateRectangleSensor(SCREEN_WIDTH / 2, 20, 200, 20);
	goalSensor->listener = this;

	// Portería propia (entre las palancas) - resta vida
	// Posición: justo debajo y entre las dos palancas
	ownGoalSensor = App->physics->CreateRectangleSensor(400, 440, 70, 10);
	ownGoalSensor->listener = this;

	// Sensor de caída (fondo) - resta puntos
	// Ajuste: bajado 25 px respecto al valor anterior
	// Centro en X, Y=450, ancho toda la mesa, altura 10 px
	sensor = App->physics->CreateRectangleSensor(SCREEN_WIDTH / 2, 450, SCREEN_WIDTH, 10);
	sensor->listener = this;

	// Create goalkeeper physics body with proper collision settings
	goalkeeperBody = App->physics->CreateRectangle((int)goalkeeperX, 35, 50, 70);
	goalkeeperBody->body->SetType(b2_kinematicBody);
	goalkeeperBody->listener = this;
	
	// Set up the goalkeeper fixture for better collision response
	b2Fixture* goalkeeperFixture = goalkeeperBody->body->GetFixtureList();
	if (goalkeeperFixture != nullptr)
	{
		goalkeeperFixture->SetRestitution(0.8f); // Make it bouncy
		goalkeeperFixture->SetFriction(0.1f);
		goalkeeperFixture->SetDensity(1.0f);
	}

	//-------------------------------CREACION DE COLISIONES DE LAS PALAS---------------------------//
	pala_right = LoadTexture("Assets/boardR2.png");
	pala_left = LoadTexture("Assets/boardL2.png");
	// Tamaños físicos - reducidos 3 píxeles en cada dimensión
	int ancho_pala = (int)(pala_left.width * PALA_SCALE) - 3;
	int alto_pala = (int)(pala_left.height * PALA_SCALE) - 3;

	// Pivote izquierdo
	PhysBody* pivote_L = App->physics->CreateRectangle(340, 395, 5, 5);
	pivote_L->body->SetType(b2_staticBody);

	// Pala izquierda desplazada
	int offsetX = (int)(pala_left.width * PALA_SCALE / 2.0f);
	pala_l = App->physics->CreateRectangle(360, 395, ancho_pala, alto_pala);
	pala_l->body->SetType(b2_dynamicBody);

	// Junta izquierda
	b2RevoluteJointDef jointDefL;
	jointDefL.Initialize(pivote_L->body, pala_l->body, pivote_L->body->GetWorldCenter());
	jointDefL.enableMotor = true;
	jointDefL.maxMotorTorque = 1000.0f;
	jointDefL.motorSpeed = 0.0f;
	jointDefL.enableLimit = true;
	jointDefL.lowerAngle = -0.25f * b2_pi;
	jointDefL.upperAngle = 0.20f * b2_pi;
	pala_l_joint = App->physics->CreateJoint(&jointDefL);


	// Pivote derecho
	PhysBody* pivote_R = App->physics->CreateRectangle(460, 395, 5, 5);
	pivote_R->body->SetType(b2_staticBody);

	// Pala derecha
	pala_r = App->physics->CreateRectangle(438, 395, ancho_pala, alto_pala);
	pala_r->body->SetType(b2_dynamicBody);

	// Junta derecha
	b2RevoluteJointDef jointDefR;
	jointDefR.Initialize(pivote_R->body, pala_r->body, pivote_R->body->GetWorldCenter());
	jointDefR.enableMotor = true;
	jointDefR.maxMotorTorque = 1000.0f;
	jointDefR.motorSpeed = 0.0f;
	jointDefR.enableLimit = true;
	jointDefR.lowerAngle = -0.20f * b2_pi;
	jointDefR.upperAngle = 0.25f * b2_pi;
	pala_r_joint = App->physics->CreateJoint(&jointDefR);
	//---------------------------------FIN COLISIONES PALAS-----------------------------------------//
	
	//---------------------------------CREACI�N FISICAS MAPA----------------------------------------//

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
	387, 448,
	411, 448,
	410, 456,
	387, 456,
	387, 449
	};

	for (int i = 0; i < 10; i++)
	{
		PIXEL_TO_METERS(game_back5[i]);
	}
	App->physics->CreateChain(0, 0, game_back5, 10);

	//------------------------------------------------FIN FISICA MAPA--------------------------------------------//
	return ret;
}

// Load assets
bool ModuleGame::CleanUp()
{
	LOG("Unloading Intro scene");
	UnloadTexture(menuTexture);
	UnloadTexture(goalkeeper);
	return true;
}

// Update: draw background
update_status ModuleGame::Update()
{
	if (!gameStarted)
	{
		// --- ESTADO DE MENÚ ---
		DrawTexture(menuTexture, 0, 0, WHITE);
		DrawText("Presiona ESPACIO para jugar", 200, 400, 30, WHITE);

		if (IsKeyPressed(KEY_SPACE))
		{
			gameStarted = true;
			lives = 3;
			score = 0;
			ResetBall();
		}

		return UPDATE_CONTINUE;
	}

	// --- ESTADO DE JUEGO (Solo se ejecuta si gameStarted == true) ---

	// 1. Dibujar el fondo del juego
	App->renderer->Draw(fondo, 0, 0);
	
	// Draw the ball
	if (circleBody != nullptr)
	{
		int x, y;
		circleBody->GetPhysicPosition(x, y);
		Vector2 position{ (float)x, (float)y };
		float desired_radius = 9.0f;

		// Calculate scale so texture width matches physical diameter
		float scale = (desired_radius * 2.0f) / (float)circle.width;

		Rectangle source = { 0.0f, 0.0f, (float)circle.width, (float)circle.height };
		Rectangle dest = { position.x, position.y, (float)circle.width * scale, (float)circle.height * scale };
		Vector2 origin = { (float)circle.width * scale / 2.0f, (float)circle.height * scale / 2.0f };
		float rotation = circleBody->GetRotation() * RAD2DEG;

		DrawTexturePro(circle, source, dest, origin, rotation, WHITE);
	}
	
	// Draw and animate goalkeeper decoration
	if (goalkeeper.id != 0) // Check if texture is loaded
	{
		float scale = 0.15f; // Scale down the goalkeeper
		float scaledWidth = goalkeeper.width * scale;
		float scaledHeight = goalkeeper.height * scale;
		
		int goalkeeperY = 25; // Near the top goal area
		
		// Define goal boundaries (adjust these to match your goal area)
		float goalLeft = 300.0f;
		float goalRight = 500.0f;
		float goalCenter = (goalLeft + goalRight) / 2.0f;
		
		// Calculate speed multiplier: 50% increase per point
		float speedMultiplier = 1.0f + (score * 0.50f);
		float currentGoalkeeperSpeed = baseGoalkeeperSpeed * speedMultiplier;
		
		// Animate goalkeeper movement
		if (goalkeeperMovingRight)
		{
			goalkeeperX += currentGoalkeeperSpeed;
			if (goalkeeperX + scaledWidth / 2.0f >= goalRight)
			{
				goalkeeperMovingRight = false;
			}
		}
		else
		{
			goalkeeperX -= currentGoalkeeperSpeed;
			if (goalkeeperX - scaledWidth / 2.0f <= goalLeft)
			{
				goalkeeperMovingRight = true;
			}
		}
		
		// Update goalkeeper physics body position
		if (goalkeeperBody != nullptr)
		{
			// Position the physics body to match the visual goalkeeper (center of the collision box)
			b2Vec2 newPos(PIXEL_TO_METERS(goalkeeperX), PIXEL_TO_METERS(35.0f));
			goalkeeperBody->body->SetTransform(newPos, 0.0f);
		}
		
		// Draw goalkeeper centered on its position
		DrawTextureEx(goalkeeper, 
			Vector2{ goalkeeperX - scaledWidth / 2.0f, (float)goalkeeperY }, 
			0.0f, scale, WHITE);
	}
	

	//-------------------------------CONTROL DE LAS PALAS-------------------------//
	// Control pala izquierda
	if (IsKeyDown(KEY_LEFT)) {
		pala_l_joint->SetMotorSpeed(-20.0f);
	}
	else {
		pala_l_joint->SetMotorSpeed(10.0f);
	}

	// Control pala derecha
	if (IsKeyDown(KEY_RIGHT)) {
		pala_r_joint->SetMotorSpeed(20.0f);
	}
	else {
		pala_r_joint->SetMotorSpeed(-10.0f);
	}
	//------------------------------FIN DE LOS CONTROLES PALAS------------------//

	//------------------------------TEXTURA DE LAS PALAS------------------------//
	int xL, yL;
	pala_l->GetPhysicPosition(xL, yL);

	float w = pala_left.width * PALA_SCALE;
	float h = pala_left.height * PALA_SCALE;

	// El cuerpo f�sico rota alrededor de su centro, no del borde
	Vector2 origin = { w / 2.0f, h / 2.0f };

	DrawTexturePro(
		pala_left,
		Rectangle{ 0, 0, (float)pala_left.width, (float)pala_left.height },
		Rectangle{ (float)xL, (float)yL, w, h },
		origin,
		pala_l->GetRotation() * RAD2DEG,
		WHITE);

	int xR, yR;
	pala_r->GetPhysicPosition(xR, yR);
	DrawTexturePro(pala_right,
		Rectangle{ 0, 0, (float)pala_right.width, (float)pala_right.height },
		Rectangle{ (float)xR, (float)yR, (float)pala_right.width * PALA_SCALE, (float)pala_right.height * PALA_SCALE },
		//Vector2{0.0f, (float)pala_right.width / 2.0f, (float)pala_right.height / 2.0f },
		Vector2{ (float)pala_left.width * PALA_SCALE / 2.0f, (float)pala_left.height * PALA_SCALE / 2.0f },
		pala_r->GetRotation() * RAD2DEG,
		WHITE);
	ray_on = false;
	//----------------------------------FIN TEXTURA PALAS------------------------//


	//----------------------------------Puntuación y Vidas-------------------------------//
	DrawText(TextFormat("Score: %i", score), 682, 99, 23, WHITE);
	DrawText(TextFormat("Lives: %i", lives), 682, 130, 23, WHITE);
	
	// Decrementar frames de ignorar colisiones
	if (ignoreCollisionsFrames > 0)
		ignoreCollisionsFrames--;
	
	// Verificar Game Over
	if (lives <= 0)
	{
		gameStarted = false;
		return UPDATE_CONTINUE;
	}
	//--------------------------------------------------------------------------//

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


	// Prepare for raycast ------------------------------------------------------
	
	vec2i mouse;
	mouse.x = GetMouseX();
	mouse.y = GetMouseY();
	int ray_hit = ray.DistanceTo(mouse);

	vec2f normal(0.0f, 0.0f);

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

	if (resetPending)
	{
		ResetBall();
		resetPending = false;
	}


	return UPDATE_CONTINUE;
}

void ModuleGame::ResetBall()
{
	if (circleBody != nullptr)
	{
		// Generar número aleatorio para decidir el lado (0 = izquierda, 1 = derecha)
		int spawnSide = GetRandomValue(0, 1);
		
		int spawnX, spawnY;
		
		if (spawnSide == 0)
		{
			// Spawn en el borde superior izquierdo
			spawnX = 290; // Posición en el borde izquierdo superior
			spawnY = 50;  // Altura cerca del borde superior
		}
		else
		{
			// Spawn en el borde superior derecho  
			spawnX = 510; // Posición en el borde derecho superior
			spawnY = 50;  // Altura cerca del borde superior
		}
		
		circleBody->body->SetTransform(b2Vec2(PIXEL_TO_METERS(spawnX), PIXEL_TO_METERS(spawnY)), 0);
		circleBody->body->SetLinearVelocity(b2Vec2_zero);
		circleBody->body->SetAngularVelocity(0);

		// Calculate speed multiplier: 50% increase per point
		float speedMultiplier = 1.0f + (score * 0.50f);
		float currentBallVelocity = baseballVelocity * speedMultiplier;
		
		// Empuje inicial hacia abajo con ligera variación lateral
		float horizontalPush = (spawnSide == 0) ? 0.5f : -0.5f; // Pequeño impulso hacia el centro
		circleBody->body->SetLinearVelocity(b2Vec2(horizontalPush, currentBallVelocity));
		
		// Ignorar colisiones por 30 frames (medio segundo a 60fps)
		ignoreCollisionsFrames = 30;
	}
}



void ModuleGame::OnCollision(PhysBody* bodyA, PhysBody* bodyB)
{
	// No procesar colisiones si el juego no ha empezado
	if (!gameStarted)
		return;
	
	// Ignorar colisiones durante los primeros frames después de resetear
	if (ignoreCollisionsFrames > 0)
		return;
    
	// Evitar procesar múltiples veces en el mismo frame si ya hay un reset pendiente
	if (resetPending)
		return;
	
	// Solo nos interesan colisiones de sensores con la pelota
	const bool ballHit = (circleBody != nullptr && bodyB == circleBody);

	// Gol en portería rival (arriba) - suma punto (solo si bodyA es el sensor y bodyB es la pelota)
	if (bodyA == goalSensor && ballHit)
	{
		score++;
		App->audio->PlayFx(bonus_fx);
		resetPending = true;
	}
	// Pelota entra en portería propia (entre palancas) - resta vida (solo si bodyA es el sensor y bodyB es la pelota)
	else if (bodyA == ownGoalSensor && ballHit)
	{
		lives--;
		resetPending = true;
	}
	// Pelota cae abajo (detrás de palancas) - resta punto (solo si bodyA es el sensor y bodyB es la pelota)
	else if (bodyA == sensor && ballHit)
	{
		score--;
		resetPending = true;
	}
}
