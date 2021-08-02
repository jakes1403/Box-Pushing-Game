/*
Copyright (c) 2019-2021 Jacob Allen
See LICENSE file at root of project for license
*/

// This is the game driving code for Box Pushing Game.
// Here you will find all the gameplay-related code, as well as stuff for the menu

#include "Thruster.hpp"

#include "LuaIntegration.h"

#include <vector>

#include <memory>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#if COMPILE_WITH_IMGUI
#include "imgui\ImGuizmo.h"
#include "imgui\imgui_memory_editor.h"
#endif

#define WORLD_COLTYPE 1
#define BOX_COLTYPE 2


using namespace Thruster;

// Put game info here to make it a bit easier to change that stuff
const int ScreenWidth = 480;
const int ScreenHeight = 272;
const char* GameName = "Box Pushing Game";

TSceneManager GameSceneManager;

TLight2D whiteLight1;
TLight2D redLight1;
TLight2D greenLight1;
TLight2D purpleLight1;
TLight2D purpleLight2;
TLight2D orangeLight1;

TLight2D uiLight1;
TLight2D uiLight2;
TLight2D uiLight3;

TSound tickDownLow;
TSound themeMusic;

TShader frameBuffShader;

shared_ptr<TGameObject> gameSceneRoot;

#ifdef __PSP__
#define printConsole(text) sceIoWrite(1, text, strlen(text))

PSP_MODULE_INFO(GameName, PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_MAX();
#endif

#if THRUSTER_MODE == MODE_EDITOR && COMPILE_WITH_IMGUI
SDL_Texture* target;
#endif

int gameCurrentFrame;

InputQuery inputQuery;

vec2 joyAxis;

TStateManager GameStateManager;

bool showDebugInfo = false;

inline bool isNumNegative(tfloat num)
{
	return num < 0;
}

int Score;

int Timer;

int baseScoreInc;

#define facingUP 0
#define facingDOWN 1
#define facingLEFT 2
#define facingRIGHT 3

TVec2D mousePos;

TTexture guyTex;

class Player {
public:
	void Load()
	{	
		guyTex.loadImage("char.png");
		stepSoundLeftFoot.load("step.wav");
		stepSoundRightFoot.load("step2.wav");
		//guyObject.physShape
	}
	void Create()
	{
		TSprite guySprite;
		guySprite.create(guyTex, { 0, 0 });
		guySprite.isAnimated = true;
		guySprite.animatedFrameSize = { 0.0f, 0.0f, (float)guyTex.width / 12.0f, (float)guyTex.height };
		guySprite.setFrame(0);
		guySprite.worldSize.w = 50;
		guySprite.worldSize.h = 51;

		guyObject.createFromSprite(guySprite);

		guyObject.setHitBoxToPoly(characterHitBoxList);
		guyObject.enablePhysics({ 0, 0 }, false, 2, 0, true);
		cpShapeSetFilter(guyObject.physShape, cpShapeFilterNew(CP_NO_GROUP, 0x00010, 0x00011));
		cpShapeSetCollisionType(guyObject.physShape, WORLD_COLTYPE);
	}
	void Update()
	{
		cpVect velocity = cpBodyGetVelocity(guyObject.physBody);
		tfloat avgVelocity = fabs((velocity.x + velocity.y) / 2);
		if (isRunning && avgVelocity < 15)
		{
			isRunning = false;
		}
		if (isRunning)
		{
			setFrame((int)(((SDL_GetTicks() / 16.6666f) / 5)) % 3);
			
			if (stepTimerLeftFoot.IsExpired())
			{
				//stepSoundLeftFoot.play();
				stepTimerLeftFoot.Start(200);
			}
			if (stepTimerRightFoot.IsExpired())
			{
				//stepSoundRightFoot.play();
				stepTimerRightFoot.Start(400);
			}
		}
		else
		{
			setFrame(3);
		}

		tfloat joyStickCutoff = 0.2f;

		if (fabs(joyAxis.x) > joyStickCutoff || fabs(joyAxis.y) > joyStickCutoff)
		{
			moveInDirection({ joyAxis.x * moveSpeed, joyAxis.y * moveSpeed });
		}
		else if (inputQuery.pointerDown)
		{
			vec2 mouse = ConvertNativeToWorldCords(vec2(inputQuery.pointerX, inputQuery.pointerY));
			vec2 distance = vec2(mouse.x - guyObject.transform.x, mouse.y - guyObject.transform.y);
			if (fabs(glm::distance(vec2(0.0f), distance)) > 20.0f)
			{
				vec2 directionMove = normalize(distance);
				moveInDirection(directionMove * moveSpeed);
			}
		}
		else
		{
			if (inputQuery.buttons[BUTTON_UP])
			{
				moveUp();
			}
			if (inputQuery.buttons[BUTTON_DOWN])
			{
				moveDown();
			}
			if (inputQuery.buttons[BUTTON_RIGHT])
			{
				moveRight();
			}
			if (inputQuery.buttons[BUTTON_LEFT])
			{
				moveLeft();
			}
		}

		guyObject.update();
	}
	void moveLeft()
	{
		moveInDirection({ moveSpeed * -1, 0 });
	}
	void moveRight()
	{
		moveInDirection({ moveSpeed, 0 });
	}
	void moveUp()
	{
		moveInDirection({ 0, moveSpeed });
	}
	void moveDown()
	{
		moveInDirection({ 0, moveSpeed * -1 });
	}
	void moveInDirection(vec2 moveDirection)
	{
		tfloat minMoveSpeed = moveSpeed / 2;
		isRunning = true;
		tfloat absMoveX = fabs(moveDirection.x);
		tfloat absMoveY = fabs(moveDirection.y);

		lookingDirection = moveDirection;

		if (absMoveX > absMoveY)
		{
			if (absMoveX >= minMoveSpeed)
			{
				if (isNumNegative(moveDirection.x))
				{
					direction = facingLEFT;
				}
				else
				{
					direction = facingRIGHT;
				}
			}
		}
		else if (absMoveX < absMoveY)
		{
			if (absMoveY >= minMoveSpeed)
			{
				if (isNumNegative(moveDirection.y))
				{
					direction = facingDOWN;
				}
				else
				{
					direction = facingUP;
				}
			}
		}
		updateLookingDirection();
		cpBodyApplyForceAtLocalPoint(guyObject.physBody, cpv(moveDirection.x, moveDirection.y), cpv(0,0));
	}
	int direction = facingRIGHT;
	
	tfloat moveSpeed = 5000;

	void setFrame(int frameNum)
	{
		if (direction == facingUP)
		{
			guyObject.setFrame(frameNum + 8);
		}
		else if (direction == facingDOWN)
		{
			guyObject.setFrame(frameNum);
		}
		else if (direction == facingRIGHT || direction == facingLEFT)
		{
			guyObject.setFrame(frameNum + 4);
		}
		currentFrame = frameNum;
	}

	void updateLookingDirection()
	{
		if (direction == facingLEFT)
		{
			guyObject.sprite.flipX = true;
		}
		if (direction == facingRIGHT)
		{
			guyObject.sprite.flipX = false;
		}
		setFrame(currentFrame);
	}
	TVec2D getTransform()
	{
		return guyObject.sprite.transform;
	}
	void SetPos(TVec2D place)
	{
		guyObject.setPosition(place);
	}
	vec2 lookingDirection = { 0, 0 };
	bool isRunning;
private:
	TSound stepSoundLeftFoot;
	TSound stepSoundRightFoot;
	TTimer stepTimerLeftFoot;
	TTimer stepTimerRightFoot;
	TVertexList characterHitBoxList = { {-19.524090, -17.250737}, {-15.244838, -25.541790}, {-4.546706, -25.274336}, {1.069813, -19.390364}, {6.151426, -25.006883}, {16.314651, -24.739430}, {20.593904, -22.064897}, {19.791544, -17.250737}, {14.977384, -11.901672}, {12.837758, -7.889872}, {19.256637, -4.680433}, {23.268437, 2.273353}, {23.000983, 14.308751}, {13.640118, 22.599803}, {0.000000, 25.006883}, {-14.442478, 23.134710}, {-22.466077, 15.646018}, {-24.605703, 4.947886}, {-21.396264, -4.145526}, {-15.512291, -8.157325}, {-11.233038, -9.227139}, {-14.709931, -12.169125} };
	TObject guyObject;
	int currentFrame = 1;
};

inline bool isInBounds(TVec2D place, TVec2D bounds)
{
	return place.x > bounds.x && place.y < bounds.y && place.x < bounds.w && place.y > bounds.h;
}

TVec2D checkPos;
bool useCheck = false;

bool wasOnCheck = false;

bool isCheck;

void checkAt(TVec2D pos)
{
	isCheck = true;

	useCheck = true;
	wasOnCheck = false;
	checkPos = pos;
}
void xAt(TVec2D pos)
{
	isCheck = false;

	useCheck = true;
	wasOnCheck = false;
	checkPos = pos;
}

#define Orange 0
#define Green 1
#define Red 2
#define White 3

#define Purple 4
#define Blue 5
#define Yellow 6

#define LightRed 7
#define DarkRed 8

#define LightGreen 9
#define DarkGreen 10

#define DarkOrange 11

#define NumColors 12

vec3 ColorToTint(int color)
{
	switch (color)
	{
	case Red:
		return vec3(0.72156862745f, 0.0f, 0.0f);
	case Orange:
		return vec3(1.0f, 0.41568627451f, 0.0f);
	case Green:
		return vec3(0.0f, 0.50196078431f, 0.0f);
	case White:
		return vec3(1.0f, 1.0f, 1.0f);
	case Purple:
		return vec3(0.8245, 0.085, 0.85);
	case Blue:
		return vec3(0.0696, 0.283, 0.87);
	case Yellow:
		return vec3(1, 0.9833, 0);
	case LightRed:
		return vec3(1, 0.2, 0.2);
	case DarkRed:
		return vec3(0.71, 0, 0);
	case LightGreen:
		return vec3(0.4417, 1, 0.33);
	case DarkGreen:
		return vec3(0.075, 0.45, 0);
	case DarkOrange:
		return vec3(0.95, 0.6333, 0);
	}
}

//class Level1 {
//public:
//	void Load()
//	{
//		TTexture levelTex;
//		levelTex.loadImage("levelbox.png");
//
//		levelSprite.create(levelTex, { 0, 0 });
//
//		levelSprite.worldSize.w = 480;
//		levelSprite.worldSize.h = 272;
//
//		tfloat elast = 0.9f;
//
//		TObject wallLeftPhys;
//		wallLeftPhys.createBlank({ 0, 0 });
//		wallLeftPhys.setHitBoxToPoly(wallLeftVertexList);
//		wallLeftPhys.enablePhysics({ 0, 0 }, true, 100, 0);
//		cpShapeSetElasticity(wallLeftPhys.physShape, elast);
//		cpShapeSetFilter(wallLeftPhys.physShape, cpShapeFilterNew(CP_NO_GROUP, 0x00010, 0x00010));
//
//		TObject wallBotPhys;
//		wallBotPhys.createBlank({ 0, 0 });
//		wallBotPhys.setHitBoxToPoly(wallBotVertexList);
//		wallBotPhys.enablePhysics({ 0, 0 }, true, 100, 0);
//		cpShapeSetElasticity(wallBotPhys.physShape, elast);
//		cpShapeSetFilter(wallBotPhys.physShape, cpShapeFilterNew(CP_NO_GROUP, 0x00010, 0x00010));
//
//		TObject leftBarPhys;
//		leftBarPhys.createBlank({ 0, 0 });
//		leftBarPhys.setHitBoxToPoly(leftBarVertexList);
//		leftBarPhys.enablePhysics({ 0, 0 }, true, 100, 0);
//		cpShapeSetElasticity(leftBarPhys.physShape, elast);
//		cpShapeSetFilter(leftBarPhys.physShape, cpShapeFilterNew(CP_NO_GROUP, 0x00010, 0x00010));
//
//		TObject middleBarPhys;
//		middleBarPhys.createBlank({ 0, 0 });
//		middleBarPhys.setHitBoxToPoly(middleBarVertexList);
//		middleBarPhys.enablePhysics({ 0, 0 }, true, 100, 0);
//		cpShapeSetElasticity(middleBarPhys.physShape, elast);
//		cpShapeSetFilter(middleBarPhys.physShape, cpShapeFilterNew(CP_NO_GROUP, 0x00010, 0x00010));
//
//		TObject rightBarPhys;
//		rightBarPhys.createBlank({ 0, 0 });
//		rightBarPhys.setHitBoxToPoly(rightBarVertexList);
//		rightBarPhys.enablePhysics({ 0, 0 }, true, 100, 0);
//		cpShapeSetElasticity(rightBarPhys.physShape, elast);
//		cpShapeSetFilter(rightBarPhys.physShape, cpShapeFilterNew(CP_NO_GROUP, 0x00010, 0x00010));
//
//		TObject bounds1;
//		bounds1.createBlank({ 0, 0 });
//		bounds1.setHitBoxToPoly(boundsCollider1);
//		bounds1.enablePhysics({ 0, 0 }, true, 100, 0);
//		cpShapeSetFilter(bounds1.physShape, cpShapeFilterNew(CP_NO_GROUP, 0x00001, 0x00011));
//
//		TObject bounds2;
//		bounds2.createBlank({ 0, 0 });
//		bounds2.setHitBoxToPoly(boundsCollider2);
//		bounds2.enablePhysics({ 0, 0 }, true, 100, 0);
//		cpShapeSetFilter(bounds2.physShape, cpShapeFilterNew(CP_NO_GROUP, 0x00001, 0x00011));
//	}
//	void Update()
//	{
//		levelSprite.draw();
//	}
//private:
//	TSprite levelSprite;
//	TVertexList boundsCollider1 = { {-205.671585, 120.487709}, {-139.343170, 119.685349}, {-85.852509, 119.952805}, {8.825959, 120.755165}, {77.294006, 118.348083}, {166.355957, 116.475914}, {213.695190, 125.301872}, {-121.691254, 130.116028}, {-217.439529, 131.185837} };
//	TVertexList boundsCollider2 = { {234.289093, 89.730583}, {237.498535, -118.615540}, {239.103256, -130.383484}, {252.475922, -95.614555}, {246.859390, 103.370697} };
//
//	TVertexList wallLeftVertexList = { {-205.671583, 136.000000}, {-203.531957, 106.580138}, {-204.066863, 33.030482}, {-204.066863, -36.239921}, {-205.404130, -79.567355}, {-203.264503, -116.208456}, {-204.334317, -128.511308}, {-230.812193, -130.116028}, {-232.951819, -29.018682}, {-229.742380, 91.067847}, {-228.405113, 102.835792} };
//	TVertexList wallBotVertexList = { {-220.916421, -124.232055}, {-206.206490, -116.208456}, {-174.379548, -118.615536}, {-123.563422, -115.138643}, {-68.468043, -112.731563}, {-24.070796, -116.208456}, {-6.418879, -116.743363}, {40.652901, -113.801377}, {95.213373, -115.406096}, {171.705015, -117.278269}, {237.498525, -115.406096}, {242.045231, -115.138643}, {240.440511, -129.581121}, {-6.686332, -132.255654}, {-127.842675, -125.569322}, {-211.823009, -129.581121} };
//
//	TVertexList leftBarVertexList = { {-140.412979, 134.930187}, {-138.808260, 94.009833}, {-127.575221, 91.335300}, {-106.446411, 92.940020}, {-84.247788, 95.079646}, {-83.445428, 135.197640} };
//	TVertexList middleBarVertexList = { {6.953786, 136.000000}, {8.023599, 100.428712}, {8.825959, 89.463127}, {20.326450, 87.590954}, {37.978368, 87.590954}, {68.735497, 86.253687}, {77.294002, 86.253687}, {77.828909, 135.732547} };
//	TVertexList rightBarVertexList = { {165.018682, 136.000000}, {163.146509, 89.463127}, {165.821042, 87.056047}, {179.728614, 88.393314}, {198.985251, 89.463127}, {219.579154, 88.393314}, {239.638151, 86.521141} };
//};

class Level2 {
public:
	void Load()
	{
		levelTex.loadImage("level.png");
	}
	void Create()
	{
		levelSprite.create(levelTex, { 0, 0 });

		levelSprite.worldSize.w = 1076;
		levelSprite.worldSize.h = 408;

		tfloat elast = 0.9f;

		for (auto hitbox : hitBoxList)
		{
			TObject wallPhys;
			wallPhys.createBlank({ 0, 0 });
			wallPhys.setHitBoxToPoly(hitbox);
			wallPhys.enablePhysics({ 0, 0 }, true, 100, 0);
			cpShapeSetElasticity(wallPhys.physShape, elast);
			cpShapeSetFilter(wallPhys.physShape, cpShapeFilterNew(CP_NO_GROUP, 0x00010, 0x00010));
			cpShapeSetCollisionType(wallPhys.physShape, WORLD_COLTYPE);

			hitBoxObjects.push_back(wallPhys);
		}

		for (auto hitbox : boundsHitboxList)
		{
			TObject wallPhys;
			wallPhys.createBlank({ 0, 0 });
			wallPhys.setHitBoxToPoly(hitbox);
			wallPhys.enablePhysics({ 0, 0 }, true, 100, 0);
			cpShapeSetElasticity(wallPhys.physShape, elast);
			cpShapeSetFilter(wallPhys.physShape, cpShapeFilterNew(CP_NO_GROUP, 0x00001, 0x00011));

			hitBoxObjects.push_back(wallPhys);
		}
	}
	void Update()
	{
		levelSprite.draw();
	}
private:

	TTexture levelTex;

	TSprite levelSprite;

	vector<TObject> hitBoxObjects;

	vector<TVertexList> hitBoxList = {
		{ {541.577026, 42.578571}, {459.221436, 43.837830}, {378.880707, 123.171158}, {377.191742, 140.288513}, {372.406555, 204.258881}, {450.480621, 204.258881}, {524.273193, 203.251480}, {537.873230, 203.251480}, {538.125061, 144.569992}, {540.391724, 50.881104} },
		{ {15.488148, 203.558594}, {16.290508, 142.311798}, {14.685787, 76.250824}, {14.700546, 55.731247}, {25.933584, 36.474609}, {118.472427, 29.788275}, {231.204056, 115.357101}, {234.680954, 130.334473}, {238.881439, 204.346191}, {84.471138, 203.846710} },
		{{538.945129, -104.613449}, {473.953979, -104.613449}, {416.451508, -108.625244}, {335.413147, -103.543633}, {318.757538, -121.197975}, {292.547119, -141.524445}, {258.313080, -162.118347}, {234.777191, -177.363190}, {235.312103, -194.747650}, {271.418274, -231.388748}, {544.755554, -219.888260}},
		{{242.159210, -185.921692}, {234.135620, -176.560822}, {132.995728, -89.862213}, {111.599464, -124.898590}, {77.900352, -167.156219}, {69.610832, -179.537201}, {74.209450, -211.817230}, {223.448303, -199.246933}},
		{{76.838280, -179.398315}, {69.349579, -179.665878}, {40.464622, -179.933319}, {-15.700569, -179.665878}, {-27.201065, -204.271576}, {90.210938, -216.306976}},
		{{-12.864052, -184.388489}, {-17.410751, -177.969620}, {-40.706009, -128.983658}, {-42.845673, -128.182037}, {-46.590019, -178.998154}, {-44.182938, -204.673676}},
		{{-43.268902, -179.585480}, {-57.443924, -179.318024}, {-88.735962, -180.922760}, {-126.446877, -182.260010}, {-144.366241, -182.794922}, {-162.285614, -189.213806}, {-31.768410, -196.167587}},
		{{-145.168610, -189.213806}, {-145.684311, -154.356995}, {-147.291321, -131.623566}, {-149.698410, -130.018860}, {-160.870163, -147.476257}, {-176.649902, -169.139984}, {-188.150391, -181.977737}, {-191.359833, -192.675873}},
		{{-185.208405, -182.245193}, {-249.308746, -179.228348}, {-274.381592, -203.354401}, {-165.798798, -204.165863}},
		{{-245.767334, -181.699799}, {-325.443970, -115.242371}, {-350.317108, -99.462631}, {-376.794983, -91.706482}, {-400.598328, -90.904129}, {-417.448212, -95.771461}, {-415.037903, -217.997620}, {-228.622940, -214.520721}},
		{{-397.653442, -98.980904}, {-400.862854, -88.817673}, {-417.979889, -63.142159}, {-442.318115, -42.280800}, {-472.540344, -23.291618}, {-474.412537, -79.189362}, {-414.502991, -118.237541}},
		{{-470.769989, -27.566002}, {-472.909607, -17.135323}, {-475.584137, 43.482021}, {-473.979431, 90.018936}, {-475.851593, 165.173309}, {-479.061035, 204.756409}, {-493.236053, 206.896027}, {-502.329468, 114.089729}, {-500.992188, 8.980583}, {-494.840790, -33.811943}, {-491.096436, -42.370453}},
		{{-326.666565, 40.296204}, {-214.068726, 39.761295}, {-198.556427, 55.808495}, {-197.168427, 73.854263}, {-192.354401, 205.441299}, {-337.240845, 205.312744}, {-337.508301, 186.858475}, {-332.694122, 153.426804}, {-329.233582, 106.950508}, {-327.897034, 57.204277}, {-325.770447, 47.991783}},
		{{-56.056099, -129.343002}, {-136.172470, -129.608063}, {-138.430161, -170.312378}, {-58.470863, -172.834335}}
	};

	vector<TVertexList> boundsHitboxList = {
		{{-540.877502, 198.893478}, {-306.588440, 198.091110}, {33.635345, 196.860931}, {399.624023, 198.116501}, {564.871460, 200.915970}, {354.118256, 349.352539}, {-520.434326, 227.175415}},
		{{532.665649, 65.829895}, {536.409973, -123.794495}, {558.876038, -128.876099}, {554.061890, 69.039337}}
	};
};

TTexture boxTexture;

void tint(TTexture& input, Uint8 r, Uint8 g, Uint8 b)
{
#if GRAPHICS_IMPLEMENTATION == G_IMPL_SDL2
	SDL_SetTextureColorMod(input.texture, r, g, b);
#endif
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
	vec4 tint = vec4(255.0f / r, 255.0f / g, 255.0f / b, 1.0f);
	
#endif
}

void correctBox(int color, TVec2D indicatorPos)
{
	checkAt(indicatorPos);
	Score += baseScoreInc;
	LI_RunFunction("RightBox");
}

void wrongBox(int color, TVec2D indicatorPos)
{
	xAt(indicatorPos);
	Score -= baseScoreInc;
	LI_RunFunction("WrongBox");
}

class Box {
public:
	void Create(TVec2D cords, int colorSet)
	{
		color = colorSet;

		TVertexList boxObjVertexList = { {-25.675516, 23.402163}, {-2.941986, 23.402163}, {16.849558, 23.669617}, {23.268437, 24.204523}, {25.675516, 11.901672}, {26.210423, -5.482793}, {26.210423, -21.529990}, {17.651917, -21.797443}, {-5.616519, -23.669617}, {-24.605703, -21.797443}, {-27.012783, -0.936087}, {-27.012783, 12.971485} };

		TSprite boxSprite;

		boxSprite.create(boxTexture, { 0, 0 });

		boxSprite.worldSize.w = 56;
		boxSprite.worldSize.h = 49;

		boxObject.createFromSprite(boxSprite);

		boxObject.setHitBoxToPoly(boxObjVertexList);
		boxObject.enablePhysics({ 0, 0 }, false, 1, 0, false);

		cpShapeSetElasticity(boxObject.physShape, 0.9f);
		cpShapeSetFilter(boxObject.physShape, cpShapeFilterNew(CP_NO_GROUP, 0x00010, 0x00010));


		boxObject.setPosition(cords);

		cpBodyApplyImpulseAtLocalPoint(boxObject.physBody, direction.toCpVect(), cpv(0, 0));

		cpShapeSetCollisionType(boxObject.physShape, BOX_COLTYPE);
		
	}
	void Update(TShader &shader)
	{
		/*Trash:
			{ {-203.040604, 306.080261}, { 14.402946, 204.709747 }}
		Red:
			{ {230.405823, 285.732788}, { 375.098053, 202.554810 }}
		Green:
			{ {538.192078, 60.276749}, { 634.742737, -109.823547 }}
		Orange:
			{ {-492.676544, 288.165283}, { -335.414001, 204.184937 }}*/

		TVec2D myPlace = boxObject.sprite.transform;
		if (isInBounds(myPlace, { -203.040604, 306.080261, 14.402946, 204.709747 })) // Trash bounds
		{
			TVec2D indicatorPos = { -91, 134 };
			if (color == White || color == Purple || color == Blue || color == Yellow)
			{
				correctBox(color, indicatorPos);
				boxObject.destroy();
			}
			else
			{
				wrongBox(color, indicatorPos);
				boxObject.destroy();
			}
		}
		if (isInBounds(myPlace, { 230.405823, 285.732788, 375.098053, 202.554810 })) // Red bounds
		{
			TVec2D indicatorPos = { 306, 146 };
			if (color == Red || color == LightRed || color == DarkRed)
			{
				correctBox(color, indicatorPos);
				boxObject.destroy();
			}
			else
			{
				wrongBox(color, indicatorPos);
				boxObject.destroy();
			}
		}
		if (isInBounds(myPlace, { 538.192078, 60.276749, 634.742737, -109.823547 })) // Green bounds
		{
			TVec2D indicatorPos = { 473, -31 };
			if (color == Green || color == LightGreen || color == DarkGreen)
			{
				correctBox(color, indicatorPos);
				boxObject.destroy();
			}
			else
			{
				wrongBox(color, indicatorPos);
				boxObject.destroy();
			}
		}
		if (isInBounds(myPlace, { -492.676544, 288.165283, -335.414001, 204.184937})) // Orange bounds
		{
			TVec2D indicatorPos = {-404, 129};
			if (color == Orange || color == DarkOrange)
			{
				correctBox(color, indicatorPos);
				boxObject.destroy();
			}
			else
			{
				wrongBox(color, indicatorPos);
				boxObject.destroy();
			}
		}
		vec3 tintColor = ColorToTint(color);
		vec4 tint = vec4(tintColor, 1.0f);
		shader.SetUniformVec4("tint", tint);

		boxObject.update(shader);
	}
	void Destroy()
	{
		boxObject.destroy();
	}
	int color;
	TObject boxObject;
private:
	TVec2D direction = { 0, 1000 };
};

vector<TTexture> boxTextures;
TTexture LoadBox(int color)
{
	TTexture boxTex;

	boxTex.loadImage("box.png");

	vec3 t = ColorToTint(color);

	tint(boxTex, t.r * 255, t.g * 255, t.b * 255);
	
	return boxTex;
}

class Boxes {
public:
	void load()
	{
		boxShader = grabMainShader();
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
		boxShader.AttachShader("boxFrag.frag", GL_FRAGMENT_SHADER);
		boxShader.AttachShader("defaultVert.vert", GL_VERTEX_SHADER);
#endif

		boxShader.Create();

		boxTexture.loadImage("box.png");

		boxSprite.create(boxTexture, { 0, 0 });

		boxSprite.worldSize.w = 56;
		boxSprite.worldSize.h = 49;

#if GRAPHICS_IMPLEMENTATION == G_IMPL_SDL2
		for (int i = 0; i < NumColors; i++)
		{
			boxTextures.push_back(LoadBox(i));
		}
#endif

	}
	void addBox()
	{
		Box toAdd;
		int myColor = randomInt(0, NumColors);
		toAdd.Create({ -100, -102 }, myColor);
		boxList[current++] = toAdd;
	}
	void draw()
	{
		for (int i = 0; i < current; i++)
		{
			// Temp fix for weird issue when building with CMake
			boxList[i].Update(boxShader);
			vec3 tintColor = ColorToTint(boxList[i].color);
			vec4 tint = vec4(tintColor, 1.0f);
			boxShader.SetUniformVec4("tint", tint);
			boxSprite.transform = boxList[i].boxObject.transform;
#if GRAPHICS_IMPLEMENTATION == G_IMPL_SDL2
			boxSprite.spriteTex = boxTextures[boxList[i].color];
#endif
			boxSprite.draw(boxShader);
		}
	}
	void removeAll()
	{
		for (int i = 0; i < current; i++)
		{
			boxList[i].Destroy();
		}
	}
private:
	TShader boxShader;
	int current = 0;
	map<int, Box> boxList;
	TSprite boxSprite;
};

TSprite numSprite;

float numCursorPos = 0.0f;

TVec2D numPosition = { 0.0f };

void LoadNumbers()
{
	TTexture numTex;
	numTex.loadImage("numberset.png");

	numSprite.create(numTex, { 0, 0 });
	numSprite.isAnimated = true;
	numSprite.animatedFrameSize = { 0, 0, (float)numTex.height, (float)numTex.height };
	numSprite.setFrame(0);

	numSprite.attachedToCamera = true;

	numSprite.worldSize.w = 48;
	numSprite.worldSize.h = 48;

}

void WriteNumDigit(int digit)
{
	numSprite.transform = { numPosition.x + numCursorPos, numPosition.y, numPosition.w, numPosition.h };
	numCursorPos += numSprite.worldSize.w / (1.5f / numPosition.w);

	numSprite.setFrame(digit);

	numSprite.draw();
}

void WriteNum(int num, TVec2D position)
{
	string asString = to_string(num);
	numCursorPos = 0.0f;
	numPosition = position;
	for (int i = 0; i < asString.length(); i++)
	{
		WriteNumDigit(asString[i] - '0');
	}
}

TVec2D Wiggle(int from, int to)
{
	return { (tfloat)randomInt(from, to), (tfloat)randomInt(from, to) };
}

TVec2D realCamera;

void wiggleCamera()
{
	int from = -10;
	int to = 10;
	realCamera.x = Camera.transform.x;
	realCamera.y = Camera.transform.y;
	TVec2D wiggle = Wiggle(from, to);
	Camera.transform.x += wiggle.x;
	Camera.transform.y += wiggle.y;
	// Fix
}

vec2 moveOverTimeTo(vec2 myPos, vec2 place, float slack)
{
	vec2 dif = place - myPos;
	if (abs(dif.x) > slack || abs(dif.y) > slack)
	{
		myPos += dif / (float)deltaTime;
	}
	return myPos;
}

Uint32 startCounter;

int countFrom;

void startCountdown(int toCountFrom)
{
	countFrom = toCountFrom;
	startCounter = SDL_GetTicks();
}

int getCountdown()
{
	return countFrom - ((SDL_GetTicks() - startCounter) / 1000);
}

void camShake(Uint32 timer, int intensity = 5)
{
	Camera.transform.x += sin(timer) * intensity;
	Camera.transform.y += cos(timer) * intensity;
}

#define Menu -1
int currentLevel;

bool playingGame;

TSprite beginLevelSprite;

TSprite readySprite;

bool doneReady = false;

int wannaStart;

bool showMenu = true;

Boxes allBoxes;

TSprite logoSprite;
TSprite startSpaceSprite;



bool useTimer = false;



map<int, TTimer> timerList;
int currentTimer = 0;


bool isWin = true;

TSprite winSprite;
TSprite loseSprite;


TTimer waitTimer;
string waitFunc;

Player guy;

void updateWaits()
{
	if (waitTimer.IsStarted() && waitTimer.IsExpired())
	{
		LI_RunFunction(waitFunc.c_str());
	}
}

TTimer checkTimer;

int gameCountdown;
int oldCountdown = 0;
bool toggleFollow = false;
bool isLastTicks = false;

Level2 level;

TSound tickDownSound;
TSprite checkSprite;
TSprite dispenser;
TSprite score;
TSprite timeSprite;
TSprite xSprite;

TTexture beatTex;
TTexture beginLevelTex;
TTexture checkTex;
TTexture dispenserTex;
TTexture logoTex;
TTexture loseTex;
TTexture readyTex;
TTexture scoreTex;
TTexture startSpaceTex;
TTexture timeTex;
TTexture xTex;

bool isClient = false;
bool isServer = false;

class LevelScene : public TScene
{
public:

	bool showScore = false;
	bool isTrans = false;

	void win()
	{
		showGuy = false;
		showScore = false;
		playingGame = false;
		isWin = true;
		SceneStateManager.SetStateTimed(200, this, &LevelScene::startWinLoseDisplay);
	}

	void lose()
	{
		showGuy = false;
		showScore = false;
		playingGame = false;
		isWin = false;
		SceneStateManager.SetStateTimed(200, this, &LevelScene::startWinLoseDisplay);
	}

	void displayWinLose(StateContext& Context)
	{
		isTrans = true;
		if (isWin)
		{
			winSprite.draw();
		}
		else
		{
			loseSprite.draw();
		}
		if (inputQuery.buttons[BUTTON_1] || inputQuery.pointerDown)
		{
			SceneStateManager.SetStateTimed(200, this, &LevelScene::endWinLoseDisplay);
		}
	}

	void startWinLoseDisplay(StateContext& Context)
	{
		isTrans = true;
		float scale = sin(Context.Timer.GetTicks() / 100.0f);

		if (isWin)
		{
			winSprite.transform.w = scale;
			winSprite.transform.h = scale;
			winSprite.draw();
		}
		else
		{
			loseSprite.transform.w = scale;
			loseSprite.transform.h = scale;
			loseSprite.draw();
		}


		if (Context.Timer.IsExpired())
		{
			SceneStateManager.SetStateTimed(1000, this, &LevelScene::displayWinLose);
		}
	}

	void transToLevel(int levelNum)
	{
		currentLevel = levelNum;
		SceneStateManager.SetStateTimed(200, this, &LevelScene::startLevelNumDisplay);
	}

	void endWinLoseDisplay(StateContext& Context)
	{
		isTrans = true;
		float scale = cos(Context.Timer.GetTicks() / 100.0f);

		if (isWin)
		{
			winSprite.transform.w = scale;
			winSprite.transform.h = scale;
			winSprite.draw();
		}
		else
		{
			loseSprite.transform.w = scale;
			loseSprite.transform.h = scale;
			loseSprite.draw();
		}


		if (Context.Timer.IsExpired())
		{
			SceneStateManager.ClearState();
			transToLevel(currentLevel);
		}
	}

	void setupLevel(int levelNum)
	{
		SceneStateManager.SetState(this, &LevelScene::GameState);
		Score = 0;
		baseScoreInc = 1;
		allBoxes.removeAll();
		LI_ExecuteString(TFileToString(TAssetManager.RequestAsset(string("level") + to_string(levelNum) + string(".lua"))).c_str());
		//LI_ExecuteLua((string("assets/level") + to_string(levelNum) + string(".lua")).c_str());
		LI_RunFunction("Start");

		//startCountdown(90);
	}

	void shrinkReady(StateContext& Context)
	{
		float scale = cos(Context.Timer.GetTicks() / 100.0f);
		readySprite.transform.w = scale;
		readySprite.transform.h = scale;

		readySprite.draw();
		if (Context.Timer.IsExpired())
		{
			isTrans = false;
			SceneStateManager.ClearState();
			setupLevel(currentLevel);
		}
	}

	void displayReady(StateContext& Context)
	{
		isTrans = true;
		readySprite.draw();
		if (Context.Timer.IsExpired())
		{
			SceneStateManager.SetStateTimed(200, this, &LevelScene::shrinkReady);
		}
	}

	void shrinkLevelNum(StateContext& Context)
	{
		isTrans = true;
		float scale = cos(Context.Timer.GetTicks() / 100.0f);
		beginLevelSprite.transform.w = scale;
		beginLevelSprite.transform.h = scale;

		float readyScale = sin(Context.Timer.GetTicks() / 100.0f);
		readySprite.transform.w = readyScale;
		readySprite.transform.h = readyScale;

		beginLevelSprite.draw();
		WriteNum(currentLevel, { 0, scale * -20.0f, scale * 0.65f, scale * 0.65f });

		readySprite.draw();

		if (Context.Timer.IsExpired())
		{
			SceneStateManager.SetStateTimed(1500, this, &LevelScene::displayReady);
		}
	}

	void displayLevelNum(StateContext& Context)
	{
		isTrans = true;
		beginLevelSprite.draw();
		WriteNum(currentLevel, { 0, -20.0f, 0.65f, 0.65f });

		if (Context.Timer.IsExpired())
		{
			SceneStateManager.SetStateTimed(200, this, &LevelScene::shrinkLevelNum);
		}
	}

	void startLevelNumDisplay(StateContext& Context)
	{
		isTrans = true;
		float scale = sin(Context.Timer.GetTicks() / 100.0f);
		beginLevelSprite.transform.w = scale;
		beginLevelSprite.transform.h = scale;

		beginLevelSprite.draw();
		WriteNum(currentLevel, { 0, scale * -20.0f, scale * 0.65f, scale * 0.65f });

		if (Context.Timer.IsExpired())
		{
			SceneStateManager.SetStateTimed(1000, this, &LevelScene::displayLevelNum);
		}
	}
	bool showGuy = false;

	void GameState(StateContext& Context)
	{
		showGuy = true;
		showScore = true;
		TVec2D guyPos = guy.getTransform();

		guy.Update();

		allBoxes.draw();

		dispenser.draw();

		if (useCheck)
		{
			if (wasOnCheck == false)
			{
				checkTimer.Start(500);
				wasOnCheck = true;
				if (isCheck)
				{
					xSprite.isHidden = true;
					checkSprite.isHidden = false;
					checkSprite.transform = checkPos;
				}
				else
				{
					checkSprite.isHidden = true;
					xSprite.isHidden = false;
					xSprite.transform = checkPos;
				}
			}

			Uint32 currentCheck = checkTimer.GetTicks();
			if (isCheck)
			{
				xSprite.isHidden = true;
				checkSprite.transform.w = sin(currentCheck / 100.0f);
				checkSprite.transform.h = sin(currentCheck / 100.0f);
			}
			else
			{
				checkSprite.isHidden = true;
				camShake(currentCheck);

				xSprite.transform.w = sin(currentCheck / 100.0f);
				xSprite.transform.h = sin(currentCheck / 100.0f);
			}

			if (checkTimer.IsExpired())
			{
				useCheck = false;
			}

			updateWaits();
		}
		else
		{
			checkSprite.isHidden = true;
			xSprite.isHidden = true;
		}

		checkSprite.draw();
		xSprite.draw();

		score.draw();
		WriteNum(Score, { 156, -95 });

		if (useTimer)
		{
			gameCountdown = getCountdown();

			timeSprite.draw();
			WriteNum(gameCountdown, { -20, 80, 0.65, 0.65 });

			isLastTicks = gameCountdown < 10;

			if (gameCountdown != oldCountdown)
			{
				oldCountdown = gameCountdown;
				if (isLastTicks)
				{
					tickDownLow.play();
				}
				else
				{
					tickDownSound.play();
				}
			}

			if (gameCountdown <= 0)
			{
				lose();
			}

			if (isLastTicks)
			{
				camShake(SDL_GetTicks(), 2);
			}
		}

		if (toggleFollow)
		{
			Camera.transform = guyPos;
		}

		if (Score < 0)
		{
			lose();
		}

		LI_SetIntVarValue("score", Score);

		LI_RunFunction("Update");

		vec2 camTransform = vec2(Camera.transform.x, Camera.transform.y);

		vec2 newCamPos = moveOverTimeTo(camTransform, vec2(guyPos.x, guyPos.y), 1.0f);

		//min(max(newCamPos.x, lowx), highx)

		// Min x: -238 Max X: 265
		// Min y: -60 Max Y: 60

		Camera.transform = { (tfloat)fmin(fmax((float)newCamPos.x, -245), 270), (tfloat)fmin(fmax((float)newCamPos.y, -60), 60) };

		physicsStep(1.0f / TWin_CurrentFPS);

	}

	void MenuState(StateContext& Context)
	{
		isTrans = true;
		logoSprite.transform.r = (sin(SDL_GetTicks() / 100.0f) + 90);
		startSpaceSprite.transform.w = (sin(SDL_GetTicks() / 500.0f) + 3.5f) / 4.5f;
		startSpaceSprite.transform.h = (sin(SDL_GetTicks() / 500.0f) + 3.5f) / 4.5f;
		logoSprite.draw();
		startSpaceSprite.draw();

		Camera.transform.x = sin(SDL_GetTicks() / 2000.0f) * 250;

		if (inputQuery.buttons[BUTTON_1] || inputQuery.pointerDown)
		{
			isTrans = false;
			transToLevel(1);
			//GameSceneManager.SwitchToScene("");
		}
	}
	void Start() override
	{
		windLoop.load("wind.wav");
		//windLoop.playLooped();
		//themeMusic.playLooped();
		bool useLighting = true;
		frameBuffShader.SetUniformBool("useLighting", useLighting);
		SceneStateManager.SetState(this, &LevelScene::MenuState);
	}
	void Update() override
	{
		level.Update(); // Draw last
	}
	void PostUpdate() override
	{
		lightBuff.StartRender(TWin_ScreenWidth, TWin_ScreenHeight);

		//camtest.Start(TWin_ScreenWidth, TWin_ScreenHeight, TWin_ScreenWidth, TWin_ScreenHeight);
		//camtest.StartFrameAsRenderer();

		whiteLight1.sprite.transform.x = guy.getTransform().x;
		whiteLight1.sprite.transform.y = guy.getTransform().y;

		if (showGuy)
		{
			whiteLight1.Draw();
		}
		//whiteLight2.Draw();
		redLight1.Draw();
		greenLight1.Draw();
		purpleLight1.Draw();
		purpleLight2.Draw();
		orangeLight1.Draw();

		if (showScore)
		{
			uiLight1.Draw();
		}
		if (useTimer)
		{
			uiLight2.Draw();
		}
		if (isTrans)
		{
			uiLight3.Draw();
		}
		
		//redLight.Draw();

		//camtest.EndFrameAsRenderer();
		//camtest.End();

		lightBuff.EndRender();
	}

	TSound windLoop;
};

shared_ptr<LevelScene> levelScene;
static int l_makeTickTimer(lua_State* L)
{
	timerList[currentTimer].Start();

	lua_pushinteger(L, currentTimer);
	currentTimer++;
	return 1;
}

static int l_getTickTimer(lua_State* L)
{
	int timer = lua_tointeger(L, 1);

	lua_pushinteger(L, timerList[timer].GetTicks());

	return 1;
}

static int l_transToLevel(lua_State* L)
{
	int levelNum = lua_tointeger(L, 1);

	levelScene->transToLevel(levelNum);

	return 0;
}

static int l_addBox(lua_State* L)
{
	allBoxes.addBox();
	return 0;
}

static int l_enableTimer(lua_State* L)
{
	useTimer = true;
	return 0;
}

static int l_disableTimer(lua_State* L)
{
	useTimer = false;
	return 0;
}

static int l_setTimer(lua_State* L)
{
	int timer = lua_tointeger(L, 1);

	startCountdown(timer);

	return 0;
}
static int l_win(lua_State* L)
{
	levelScene->win();
	return 0;
}

static int l_lose(lua_State* L)
{
	levelScene->lose();
	return 0;
}
static int l_waitFunc(lua_State* L)
{
	waitFunc = lua_tostring(L, 1);
	int ticks = lua_tointeger(L, 2);
	waitTimer.Start(ticks);
	return 0;
}
static int l_tpGuy(lua_State* L)
{
	float placex = lua_tonumber(L, 1);
	float placey = lua_tonumber(L, 2);

	guy.SetPos({ placex, placey });

	return 0;
}

static int l_setDarkness(lua_State* L)
{
	float darkness = lua_tonumber(L, 1);

	lightBuff.color = vec3(darkness, darkness, darkness);

	return 0;
}

class ThrusterLogo : public TScene
{
public:
	void Start() override
	{
		bool useLighting = false;
		frameBuffShader.SetUniformBool("useLighting", useLighting);
		TSound bootSound;
		bootSound.load("thrusterbootup.wav");
		TTexture thrusterLogoTex;
		thrusterLogoTex.loadImage("thrusterlogo.png");
		TTexture splashBGTex;
		splashBGTex.loadImage("splashbg.png");

		thrusterSprite.create(thrusterLogoTex, { 0, 0 });

		thrusterSprite.worldSize = { 0, 0, 480, 272 };

#if TBUILD_TYPE == TYPE_RELEASE
		wait.Start(8000);
#else
		wait.Start(4000);
#endif

		splashSprite.create(splashBGTex, { 0, 0 });
		splashSprite.worldSize = { 0, 0, 480, 272 };

		Camera.bgColor = vec4(1.0f, 1.0f, 1.0f, 1.0);

		splashShader = grabMainShader();
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
		splashShader.AttachShader("splashFrag.frag", GL_FRAGMENT_SHADER);
		splashShader.AttachShader("defaultVert.vert", GL_VERTEX_SHADER);
#endif
		
		splashShader.Create();

		bootSound.play();
	}

	void Update() override
	{
#if GRAPHICS_IMPLEMENTATION == G_IMPL_SDL2
	 	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	 	SDL_RenderClear(renderer);
		splashSprite.transform.y = cos(((float)SDL_GetTicks() / 1000.0f)) * 10;
#endif
		splashSprite.draw(splashShader);
		thrusterSprite.draw();

		if (wait.IsExpired())
		{
			GameSceneManager.SwitchToScene("Level");
		}
	}
	void Destroy() override
	{
		Camera.bgColor = vec4(0.0f, 0.0f, 0.0f, 1.0);
	}
private:
	float tint = 1.0f;
	TShader splashShader;
	TTimer wait;
	TSprite splashSprite;
	TSprite thrusterSprite;
};

#include <filesystem>

namespace fs = std::filesystem;

TPak PathToTPak(string path)
{
	TPak packed;
	const fs::path pathToShow{ path };

	for (const auto& entry : fs::directory_iterator(pathToShow)) {
		const auto filenameStr = entry.path().filename().string();
		if (entry.is_regular_file()) {
			packed.AddFile(LoadTFileFromLocalPath(path + filenameStr), filenameStr);
		}
	}
	return packed;
}

TSound boxSound;

inline double hires_time()
{
	return (double)SDL_GetPerformanceCounter() / 10000000.0f;
}

std::function<void()> loop;
void main_loop() { loop(); }

int main(int argc, char* args[])
{

	#ifdef __PSP__
	pspDebugScreenInit();
	setupExitCallback();
	#endif

	//PathToTPak("assets/").WriteFull("assets.tpak");

	//TAssetManager.SetLoadLocationToTPak("assets.tpak");

	TAssetManager.SetLoadLocationToPath("assets/");

	Thruster_Init(GameName, ScreenWidth, ScreenHeight);

	initPhysics();

	cpSpaceSetGravity(physSpace, cpvzero);

	cpSpaceSetDamping(physSpace, 0.001f);

	LI_Init();

	lua_pushcfunction(state, l_transToLevel);
	lua_setglobal(state, "transitionToLevel");

	lua_pushcfunction(state, l_addBox);
	lua_setglobal(state, "addBox");

	lua_pushcfunction(state, l_enableTimer);
	lua_setglobal(state, "enableTimer");

	lua_pushcfunction(state, l_disableTimer);
	lua_setglobal(state, "disableTimer");

	lua_pushcfunction(state, l_setTimer);
	lua_setglobal(state, "setTimer");

	lua_pushcfunction(state, l_makeTickTimer);
	lua_setglobal(state, "makeTickTimer");

	lua_pushcfunction(state, l_getTickTimer);
	lua_setglobal(state, "getTickTimer");

	lua_pushcfunction(state, l_win);
	lua_setglobal(state, "win");

	lua_pushcfunction(state, l_lose);
	lua_setglobal(state, "lose");

	lua_pushcfunction(state, l_waitFunc);
	lua_setglobal(state, "waitFunc");

	lua_pushcfunction(state, l_tpGuy);
	lua_setglobal(state, "tpGuy");

	lua_pushcfunction(state, l_setDarkness);
	lua_setglobal(state, "setDarkness");

	TTexture noiseTex;
	noiseTex.loadImage("noise.png");

	MainShader.Use();

	MainShader.SetUniformTTexture("noiseTex", noiseTex);

	frameBuffShader = grabMainShader();
#if GRAPHICS_IMPLEMENTATION == G_IMPL_OPENGL3
	frameBuffShader.AttachShader("frameBuffShader.frag", GL_FRAGMENT_SHADER);
	frameBuffShader.AttachShader("defaultVert.vert", GL_VERTEX_SHADER);
#endif

	TTexture slateTex;
	slateTex.loadImage("SlateTexture.png");

	frameBuffShader.Use();

	frameBuffShader.SetUniformTTexture("slateTex", slateTex);

	frameBuffShader.SetUniformTTexture("noiseTex", noiseTex);

	frameBuffShader.Create();

	dispenserTex.loadImage("machine.png");
	scoreTex.loadImage("score.png");
	timeTex.loadImage("time.png");
	tickDownSound.load("tickdown.wav");
	checkTex.loadImage("check.png");
	xTex.loadImage("x.png");
	logoTex.loadImage("logo.png");
	startSpaceTex.loadImage("pressStartSpace.png");
	beginLevelTex.loadImage("beginlevel.png");
	readyTex.loadImage("ready.png");
	loseTex.loadImage("youlose.png");
	beatTex.loadImage("youbeat.png");
	
	//themeMusic.load("theme.flac");

	LoadNumbers();

	level.Load();
	guy.Load();
	allBoxes.load();
	dispenser.create(dispenserTex, { 0, 0 });
	score.create(scoreTex, { 158, -98 });
	timeSprite.create(timeTex, { 0, 90 });
	checkSprite.create(checkTex, { 0, 0 });
	xSprite.create(xTex, { 0, 0 });
	logoSprite.create(logoTex, { 0, 10 });
	startSpaceSprite.create(startSpaceTex, { 0, -100 });
	beginLevelSprite.create(beginLevelTex, { 0, 0 });
	readySprite.create(readyTex, { 0, 0 });
	winSprite.create(beatTex, { 0, 0 });
	loseSprite.create(loseTex, { 0, 0 });

	guy.Create();
	level.Create();

	dispenser.worldSize.w = 1076;
	dispenser.worldSize.h = 408;

	score.worldSize.w = 207;
	score.worldSize.h = 60;
	score.attachedToCamera = true;

	timeSprite.worldSize.w = 113;
	timeSprite.worldSize.h = 79;
	timeSprite.attachedToCamera = true;

	checkSprite.worldSize = { 0, 0, 41, 44 };

	xSprite.worldSize = { 0, 0, 41, 44 };

	logoSprite.worldSize = { 0, 0, 413, 186 };
	logoSprite.attachedToCamera = true;

	startSpaceSprite.worldSize = { 0, 0, 282, 34 };
	startSpaceSprite.attachedToCamera = true;

	beginLevelSprite.attachedToCamera = true;
	beginLevelSprite.worldSize = { 0, 0, 279, 139 };

	readySprite.attachedToCamera = true;
	readySprite.worldSize = { 0, 0, 279, 139 };

	winSprite.attachedToCamera = true;
	winSprite.worldSize = { 0, 0, 279, 139 };

	loseSprite.attachedToCamera = true;
	loseSprite.worldSize = { 0, 0, 279, 139 };

	shared_ptr<ThrusterLogo> logoScene;

	logoScene = make_shared<ThrusterLogo>();

	levelScene = make_shared<LevelScene>();

	GameSceneManager.AddScene("Level", levelScene);

	GameSceneManager.AddScene("Logo", logoScene);

	

#if TBUILD_TYPE == TYPE_RELEASE
	GameSceneManager.SwitchToScene("Logo");
#else
	GameSceneManager.SwitchToScene("Level");
#endif


	gameCurrentFrame = 0;

	toggleFollow = false;
	isLastTicks = false;
	gameCountdown;
	oldCountdown = 0;

	TTexture lightCookie;
	lightCookie.loadImage("lightcookie.png");

	lightBuff.Init();

	lightBuff.color = vec3(0.05, 0.05, 0.05);

	whiteLight1.Create({ -74, -14 }, lightCookie);
	whiteLight1.sprite.worldSize = { 0, 0, 512, 512 };
	whiteLight1.color = vec4(1.000, 0.894, 0.769, 1.0f);
	whiteLight1.brightness = 0.9;

	
	redLight1.Create({ 47, 40 }, lightCookie);
	redLight1.sprite.worldSize = { 0, 0, 128, 128 };
	redLight1.color = vec4(0.698, 0.133, 0.133, 1.0f);
	
	greenLight1.Create({ 49, 25 }, lightCookie);
	greenLight1.sprite.worldSize = { 0, 0, 128, 128 };
	greenLight1.color = vec4(0.133, 0.545, 0.133, 1.0f);
	greenLight1.brightness = 0.5;

	
	purpleLight1.Create({ 89, 31 }, lightCookie);
	purpleLight1.sprite.worldSize = { 0, 0, 128, 128 };
	purpleLight1.color = vec4(0.600, 0.196, 0.800, 1.0f);
	purpleLight1.brightness = 1.0;

	
	purpleLight2.Create({ 210, 92 }, lightCookie);
	purpleLight2.sprite.worldSize = { 0, 0, 128, 128 };
	purpleLight2.color = vec4(0.600, 0.196, 0.800, 1.0f);
	purpleLight2.brightness = 1.0;

	
	orangeLight1.Create({ -255, 37 }, lightCookie);
	orangeLight1.sprite.worldSize = { 0, 0, 256, 256 };
	orangeLight1.color = vec4(0.957, 0.643, 0.376, 1.0f);
	orangeLight1.brightness = 1.0;

	uiLight1.Create({ 159, -90 }, lightCookie);
	uiLight1.sprite.worldSize = { 0, 0, 256, 256 };
	uiLight1.sprite.transform.w = 1.5;
	uiLight1.sprite.transform.h = 0.5;
	uiLight1.sprite.attachedToCamera = true;
	uiLight1.color = vec4(0.957, 0.643, 0.376, 1.0f);
	uiLight1.brightness = 1.0;

	uiLight2.Create({ 0, 102 }, lightCookie);
	uiLight2.sprite.worldSize = { 0, 0, 256, 256 };
	uiLight2.sprite.transform.w = 1.0;
	uiLight2.sprite.transform.h = 1.0;
	uiLight2.sprite.attachedToCamera = true;
	uiLight2.color = vec4(0.957, 0.643, 0.376, 1.0f);
	uiLight2.brightness = 1.0;

	uiLight3.Create({ 0, 0 }, lightCookie);
	uiLight3.sprite.worldSize = { 0, 0, 256, 256 };
	uiLight3.sprite.transform.w = 2;
	uiLight3.sprite.transform.h = 2;
	uiLight3.sprite.attachedToCamera = true;
	uiLight3.color = vec4(0.957, 0.643, 0.376, 1.0f);
	uiLight3.brightness = 1.0;

	Camera.Create();

	boxSound.load("bump.wav");
	tickDownLow.load("tickdownlow.wav");


#if TBUILD_TYPE == TYPE_RELEASE
	TWin_ToggleWindowed();
#endif

	vec2 mouseGuiPos = vec2(0.0f);

	int w, h;
	bool a;

	SDL_GetWindowSize(TWin_Window, &w, &h);
	GameWindowInfo.transform.w = (tfloat)w;
	GameWindowInfo.transform.h = (tfloat)h;

	loop = [&]
	{
		runningStatus = TWin_BeginFrame(gameCurrentFrame++);
		if (runningStatus == TWin_Exit) return;

		Thruster_Update();

		inputQuery = GetInput();

		joyAxis = { inputQuery.axis[Axis1X], inputQuery.axis[Axis1Y] };
		joyAxis = normalize(joyAxis);

#if GRAPHICS_IMPLEMENTATION == G_IMPL_SDL2
	 	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	 	SDL_RenderClear(renderer);
#endif

		Camera.Start(TWin_ScreenWidth, TWin_ScreenHeight, getScreenSize().w, getScreenSize().h);
#ifndef PLATFORM_NO_FRAMEBUFFER
		Camera.StartFrameAsRenderer();
#endif

		GameSceneManager.UpdateScenes();
#ifndef PLATFORM_NO_FRAMEBUFFER
		Camera.EndFrameAsRenderer();
#endif
		Camera.End();

#ifndef PLATFORM_NO_FRAMEBUFFER
		frameBuffShader.Use();

		frameBuffShader.SetUniformTTexture("lightBuffTex", lightBuff.GetTexture());
		
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		RenderCamera(Camera, TWin_ScreenWidth, TWin_ScreenHeight, getScreenSize().w, getScreenSize().h, frameBuffShader);
#endif

		TWin_EndFrame();
	};
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, true);
#else
    while(true) main_loop();
#endif

	TWin_DestroyWindow();

	#ifdef __PSP__
	sceKernelExitGame();
	#endif

	return EXIT_SUCCESS;
}
