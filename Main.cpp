/*____________________________________________________________________
|
| File: main.cpp
|
| Description: Main module in program.
|
| Functions:  Program_Get_User_Preferences
|             Program_Init
|							 Init_Graphics
|								Set_Mouse_Cursor
|             Program_Run
|							 Init_Render_State
|             Program_Free
|             Program_Immediate_Key_Handler
|
| (C) Copyright 2013 Abonvita Software LLC.
| Licensed under the GX Toolkit License, Version 1.0.
|___________________________________________________________________*/

#define _MAIN_

/*___________________
|
| Include Files
|__________________*/

#include <first_header.h>
#include "dp.h"

#include <rom8x8.h>

#include "main.h"
#include "position.h"

/*___________________
|
| Type definitions
|__________________*/

typedef struct {
	unsigned resolution;
	unsigned bitdepth;
} UserPreferences;

/*___________________
|
| Function Prototypes
|__________________*/

static int Init_Graphics(unsigned resolution, unsigned bitdepth, unsigned stencildepth, int* generate_keypress_events);
static void Set_Mouse_Cursor();
static void Init_Render_State();

/*___________________
|
| Constants
|__________________*/

#define MAX_VRAM_PAGES  2
/*#define GRAPHICS_RESOLUTION  \
(                          \
gxRESOLUTION_640x480   | \
gxRESOLUTION_800x600   | \
gxRESOLUTION_1024x768  | \
gxRESOLUTION_1152x864  | \
gxRESOLUTION_1280x960  | \
gxRESOLUTION_1400x1050 | \
gxRESOLUTION_1440x1080 | \
gxRESOLUTION_1600x1200 | \
gxRESOLUTION_1152x720  | \
gxRESOLUTION_1280x800  | \
gxRESOLUTION_1440x900  | \
gxRESOLUTION_1680x1050 | \
gxRESOLUTION_1920x1200 | \
gxRESOLUTION_2048x1280 | \
gxRESOLUTION_1280x720  | \
gxRESOLUTION_1600x900  | \
gxRESOLUTION_1920x1080   \
)*/
#define GRAPHICS_RESOLUTION gxRESOLUTION_640x480
#define GRAPHICS_STENCILDEPTH 0
#define GRAPHICS_BITDEPTH (gxBITDEPTH_24 | gxBITDEPTH_32)

#define SCREENSHOT_FILENAME "screenshots\\screen"

#define AUTO_TRACKING    1
#define NO_AUTO_TRACKING 0

int lantern_light_on;
int dir_light_on;

/*____________________________________________________________________
|
| Function: Program_Get_User_Preferences
|
| Input: Called from CMainFrame::Init
| Output: Allows program to popup dialog boxes, etc. to get any user
|   preferences such as screen resolution.  Returns preferences via a
|   pointer.  Returns true on success, else false to quit the program.
|___________________________________________________________________*/

int Program_Get_User_Preferences(void** preferences)
{
	static UserPreferences user_preferences;

	if (gxGetUserFormat(GRAPHICS_DRIVER, GRAPHICS_RESOLUTION, GRAPHICS_BITDEPTH, &user_preferences.resolution, &user_preferences.bitdepth)) {
		*preferences = (void*)&user_preferences;
		return (1);
	}
	else
		return (0);
}

/*____________________________________________________________________
|
| Function: Program_Init
|
| Input: Called from CMainFrame::Start_Program_Thread()
| Output: Starts graphics mode.  Returns # of user pages available if
|       successful, else 0.
|___________________________________________________________________*/

int Program_Init(void* preferences, int* generate_keypress_events)
{
	UserPreferences* user_preferences = (UserPreferences*)preferences;
	int initialized = FALSE;

	if (user_preferences)
		initialized = Init_Graphics(user_preferences->resolution, user_preferences->bitdepth, GRAPHICS_STENCILDEPTH, generate_keypress_events);

	return (initialized);
}

/*____________________________________________________________________
|
| Function: Init_Graphics
|
| Input: Called from Program_Init()
| Output: Starts graphics mode.  Returns # of user pages available if
|       successful, else 0.
|___________________________________________________________________*/

static int Init_Graphics(unsigned resolution, unsigned bitdepth, unsigned stencildepth, int* generate_keypress_events)
{
	int num_pages;
	byte* font_data;
	unsigned font_size;

	/*____________________________________________________________________
	|
	| Init globals
	|___________________________________________________________________*/

	Pgm_num_pages = 0;
	Pgm_system_font = NULL;

	/*____________________________________________________________________
	|
	| Start graphics mode and event processing
	|___________________________________________________________________*/

	font_data = font_data_rom8x8;
	font_size = sizeof(font_data_rom8x8);

	// Start graphics mode                                      
	num_pages = gxStartGraphics(resolution, bitdepth, stencildepth, MAX_VRAM_PAGES, GRAPHICS_DRIVER);
	if (num_pages == MAX_VRAM_PAGES) {
		// Init system, drawing fonts 
		Pgm_system_font = gxLoadFontData(gxFONT_TYPE_GX, font_data, font_size);
		// Make system font the default drawing font 
		gxSetFont(Pgm_system_font);

		// Start event processing
		evStartEvents(evTYPE_MOUSE_LEFT_PRESS | evTYPE_MOUSE_RIGHT_PRESS |
			evTYPE_MOUSE_LEFT_RELEASE | evTYPE_MOUSE_RIGHT_RELEASE |
			evTYPE_MOUSE_WHEEL_BACKWARD | evTYPE_MOUSE_WHEEL_FORWARD |
			//                   evTYPE_KEY_PRESS | 
			evTYPE_RAW_KEY_PRESS | evTYPE_RAW_KEY_RELEASE,
			AUTO_TRACKING, EVENT_DRIVER);
		*generate_keypress_events = FALSE;  // true if using evTYPE_KEY_PRESS in the above mask

		// Set a custom mouse cursor
		Set_Mouse_Cursor();

		// Set globals
		Pgm_num_pages = num_pages;
	}

	return (Pgm_num_pages);
}

/*____________________________________________________________________
|
| Function: Set_Mouse_Cursor
|
| Input: Called from Init_Graphics()
| Output: Sets default mouse cursor.
|___________________________________________________________________*/

static void Set_Mouse_Cursor()
{
	gxColor fc, bc;

	// Set cursor to a medium sized red arrow
	fc.r = 255;
	fc.g = 0;
	fc.b = 0;
	fc.a = 0;
	bc.r = 1;
	bc.g = 1;
	bc.b = 1;
	bc.a = 0;
	msSetCursor(msCURSOR_MEDIUM_ARROW, fc, bc);
}

/*____________________________________________________________________
|
| Function: Draw_Screen
|
| Input: Called from Program_Run()
| Output: Displays various game screens
|___________________________________________________________________*/

gx3dObject* obj_screen;
static void Draw_Screen(gx3dTexture screen) {
	gx3dMatrix view_save, m, m1, m2;
	gx3d_GetViewMatrix(&view_save);
	gx3dVector tfrom = { 0,0,-1 }, tto = { 0,0,0 }, twup = { 0,1,0 };
	gx3d_CameraSetPosition(&tfrom, &tto, &twup, gx3d_CAMERA_ORIENTATION_LOOKTO_FIXED);
	gx3d_CameraSetViewMatrix();
	gx3d_DisableZBuffer();
	gx3d_EnableAlphaBlending();
	gx3d_GetTranslateMatrix(&m, 0, 0, .5);
	gx3d_GetRotateYMatrix(&m1, 0);
	gx3d_GetScaleMatrix(&m2, 0.085f, 0.085f, 0.085f);
	gx3d_MultiplyMatrix(&m, &m1, &m);
	gx3d_MultiplyMatrix(&m, &m2, &m);
	gx3d_SetObjectMatrix(obj_screen, &m);
	gx3d_SetTexture(0, screen);
	gx3d_DrawObject(obj_screen);
	gx3d_DisableAlphaBlending();
	gx3d_EnableZBuffer();
	gx3d_SetViewMatrix(&view_save);
}

/*____________________________________________________________________
|
| Function: Program_Run
|
| Input: Called from Program_Thread()
| Output: Runs program in the current video mode.  Begins with mouse
|   hidden.
|___________________________________________________________________*/


void Program_Run()
{
	int quit;
	evEvent event;
	gx3dDriverInfo dinfo;
	gxColor color;
	char str[256];

	static bool running = false;
	static bool walking = false;

	gx3dObject* obj_tree, * obj_skydome, * obj_ground, * obj_paper, * obj_slender, * obj_camera;

	gx3dMatrix m, m1, m2, m3, m4, m5;
	gx3dColor color3d_white = { 1, 1, 1, 0 };
	gx3dColor color3d_dim = { 0.1f, 0.1f, 0.1f };
	gx3dColor color3d_black = { 0, 0, 0, 0 };
	gx3dColor color3d_darkgray = { 0.3f, 0.3f, 0.3f, 0 };
	gx3dColor color3d_gray = { 0.5f, 0.5f, 0.5f, 0 };
	gx3dMaterialData material_default = {
	  { 1, 1, 1, 1 }, // ambient color
	  { 1, 1, 1, 1 }, // diffuse color
	  { 1, 1, 1, 1 }, // specular color
	  { 0, 0, 0, 0 }, // emissive color
	  10              // specular sharpness (0=disabled, 0.01=sharp, 10=diffused)
	};

	int startTime = time(NULL);

	// // How to use C++ print outupt 
	// string mystr;

	   //mystr = "SUPER!";
	   //char ss[256];
	   //strcpy (ss, mystr.c_str());
	   //debug_WriteFile (ss);

   /*____________________________________________________________________
   |
   | Print info about graphics driver to debug file.
   |___________________________________________________________________*/

	gx3d_GetDriverInfo(&dinfo);
	debug_WriteFile("_______________ Device Info ______________");
	sprintf(str, "max texture size: %dx%d", dinfo.max_texture_dx, dinfo.max_texture_dy);
	debug_WriteFile(str);
	sprintf(str, "max active lights: %d", dinfo.max_active_lights);
	debug_WriteFile(str);
	sprintf(str, "max user clip planes: %d", dinfo.max_user_clip_planes);
	debug_WriteFile(str);
	sprintf(str, "max simultaneous texture stages: %d", dinfo.max_simultaneous_texture_stages);
	debug_WriteFile(str);
	sprintf(str, "max texture stages: %d", dinfo.max_texture_stages);
	debug_WriteFile(str);
	sprintf(str, "max texture repeat: %d", dinfo.max_texture_repeat);
	debug_WriteFile(str);
	debug_WriteFile("__________________________________________");


	/*____________________________________________________________________
	|
	| Initialize the sound library
	|__________________________________________________________________*/

	snd_Init(22, 16, 2, 1, 1);
	snd_SetListenerDistanceFactorToFeet(snd_3D_APPLY_NOW);

	Sound s_forest, s_footsteps, s_running, s_paper, s_ouch, s_gameover, s_title, s_story1, s_wolves, s_survived, s_fire;

	s_forest = snd_LoadSound("wav\\forest.wav", snd_CONTROL_VOLUME, 0);
	s_footsteps = snd_LoadSound("wav\\walking.wav", snd_CONTROL_VOLUME, 0);
	s_running = snd_LoadSound("wav\\running.wav", snd_CONTROL_VOLUME, 0);
	s_paper = snd_LoadSound("wav\\paper.wav", snd_CONTROL_VOLUME, 0);
	s_ouch = snd_LoadSound("wav\\ouch.wav", snd_CONTROL_VOLUME, 0);
	s_gameover = snd_LoadSound("wav\\gameover.wav", snd_CONTROL_VOLUME, 0);
	s_title = snd_LoadSound("wav\\title.wav", snd_CONTROL_VOLUME, 0);
	s_story1 = snd_LoadSound("wav\\story1.wav", snd_CONTROL_VOLUME, 0);
	s_wolves = snd_LoadSound("wav\\wolves.wav", snd_CONTROL_VOLUME, 0);
	s_survived = snd_LoadSound("wav\\survived.wav", snd_CONTROL_VOLUME, 0);
	s_fire = snd_LoadSound("wav\\fire.wav", snd_CONTROL_3D, 0);

	/*____________________________________________________________________
	|
	| Initialize the graphics state
	|__________________________________________________________________*/

	// Set 2d graphics state
	Pgm_screen.xleft = 0;
	Pgm_screen.ytop = 0;
	Pgm_screen.xright = gxGetScreenWidth() - 1;
	Pgm_screen.ybottom = gxGetScreenHeight() - 1;
	gxSetWindow(&Pgm_screen);
	gxSetClip(&Pgm_screen);
	gxSetClipping(FALSE);

	// Set the 3D viewport
	gx3d_SetViewport(&Pgm_screen);
	// Init other 3D stuff
	Init_Render_State();

	/*____________________________________________________________________
	|
	| Init support routines
	|___________________________________________________________________*/

	gx3dVector heading, position;
	// Set starting camera position
	position.x = 0;
	position.y = 5;
	position.z = -100;
	// Set starting camera view direction (heading)
	heading.x = 0;  // {0,0,1} for cubic environment mapping to work correctly
	heading.y = 0;
	heading.z = 1;

	Position_Init(&position, &heading, RUN_SPEED);

	/*____________________________________________________________________
	|
	| Init 3D graphics
	|___________________________________________________________________*/

	// Set projection matrix
	float fov = 60; // degrees field of view
	float near_plane = 0.1f;
	float far_plane = 1000;
	gx3d_SetProjectionMatrix(fov, near_plane, far_plane);

	gx3d_SetFillMode(gx3d_FILL_MODE_GOURAUD_SHADED);

	// Clear the 3D viewport to all black
	color.r = 0;
	color.g = 0;
	color.b = 0;
	color.a = 0;

	/*____________________________________________________________________
	|
	| Load 3D models & init variables
	|___________________________________________________________________*/

	gx3dParticleSystem psys_fire = Script_ParticleSystem_Create("fire.gxps");

	// Load a 3D model																								
	gx3d_ReadLWO2File("Objects\\ptree6.lwo", &obj_tree, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	gx3d_ReadLWO2File("Objects\\skydome.lwo", &obj_skydome, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	gx3d_ReadLWO2File("Objects\\ground.lwo", &obj_ground, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	gx3d_ReadLWO2File("Objects\\billboard_paper.lwo", &obj_paper, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	gx3d_ReadLWO2File("Objects\\billboard_slender.lwo", &obj_slender, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
	gx3d_ReadLWO2File("Objects\\billboard_screen.lwo", &obj_screen, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);

	gx3dTexture tex_title_screen = gx3d_InitTexture_File("Objects\\Images\\Title.bmp", 0, 0);
	gx3dTexture tex_pause_screen = gx3d_InitTexture_File("Objects\\Images\\Pause.bmp", 0, 0);
	gx3dTexture tex_survive_screen = gx3d_InitTexture_File("Objects\\Images\\Won.bmp", 0, 0);
	gx3dTexture tex_gameover_screen = gx3d_InitTexture_File("Objects\\Images\\GameOver.bmp", 0, 0);
	gx3dTexture tex_firstpage_screen = gx3d_InitTexture_File("Objects\\Images\\Page1.bmp", 0, 0);
	gx3dTexture tex_story1_screen = gx3d_InitTexture_File("Objects\\Images\\story1.bmp", 0, 0);
	gx3dTexture tex_story2_screen = gx3d_InitTexture_File("Objects\\Images\\story2.bmp", 0, 0);
	gx3dTexture tex_tree = gx3d_InitTexture_File("Objects\\Images\\ptree_d512.bmp", "Objects\\Images\\ptree_d512_fa.bmp", 0);
	gx3dTexture tex_skydome = gx3d_InitTexture_File("Objects\\Images\\Night.bmp", 0, 0);
	gx3dTexture tex_ground = gx3d_InitTexture_File("Objects\\Images\\Ground.bmp", 0, 0);
	gx3dTexture tex_paper = gx3d_InitTexture_File("Objects\\Images\\Paper.bmp", "Objects\\Images\\Paper_FA.bmp", 0);
	gx3dTexture tex_slender = gx3d_InitTexture_File("Objects\\Images\\Slender.bmp", "Objects\\Images\\Slender_FA.bmp", 0);

	bool screen_change = true, screen_title = true, screen_story1 = true, screen_story2 = false, screen_survive = false, screen_gameover = false, screen_firstpage = false;
	const int NUM_TREES = 100, NUM_PAPER = 8, Slender = 1;
	int hp = 3, num_paper_touched = 0, take_screenshot;
	gx3dVector treePosition[NUM_TREES], paperPosition[NUM_PAPER], SlenderPosition[Slender];
	gx3dSphere paperSphere[NUM_PAPER], treeSphere[NUM_TREES], cameraSphere;
	gxRelation relation;
	boolean paperOnScreen[NUM_PAPER], paperDraw[NUM_PAPER];

	srand(time(0));
	for (int i = 0; i < Slender; i++)
	{
		SlenderPosition[i].x = (rand() % 151) - 75;
		SlenderPosition[i].y = 0;
		SlenderPosition[i].z = (rand() % 151) - 75;
		if (SlenderPosition[i].x == 0)
			SlenderPosition[i].x += 2;
		else if (SlenderPosition[i].z == 0)
			SlenderPosition[i].z += 2;
	}
	srand(time(0));
	for (int i = 0; i < NUM_PAPER; i++)
	{
		paperPosition[i].x = (rand() % 151) - 75;
		paperPosition[i].y = 1;
		paperPosition[i].z = (rand() % 151) - 75;

		if (paperPosition[i].x == 0 || paperPosition[i].x == SlenderPosition[i].x)
			paperPosition[i].x += 2;
		else if (paperPosition[i].z == 0 || paperPosition[i].z == SlenderPosition[i].z)
			paperPosition[i].z += 2;

		paperDraw[i] = true;
	}
	srand(time(0));
	for (int i = 0; i < NUM_TREES; i++) {

		treePosition[i].x = (rand() % 151) - 75;
		treePosition[i].y = 0;
		treePosition[i].z = (rand() % 151) - 75;
		if (treePosition[i].x == 0 || treePosition[i].x == paperPosition[i].x || treePosition[i].x == SlenderPosition[i].x)
			treePosition[i].x += 2;
		else if (treePosition[i].z == 0 || treePosition[i].z == paperPosition[i].z || treePosition[i].z == SlenderPosition[i].z)
			treePosition[i].z += 2;

		treeSphere[i] = obj_tree->bound_sphere;
		treeSphere[i].center.x = treePosition[i].x;
		treeSphere[i].center.y = treePosition[i].y;
		treeSphere[i].center.z = treePosition[i].z;
		treeSphere[i].radius *= 6.0f;  // adjust as needed
	}

	/*____________________________________________________________________
	|
	| create lights
	|___________________________________________________________________*/

	gx3dLight dir_light;
	gx3dLight lantern_light;
	gx3dLight fire_light;
	//gx3dLight light_flashlight;
	gx3dLightData light_data;
	gx3dLightData light_data2;

	light_data.light_type = gx3d_LIGHT_TYPE_DIRECTION;
	light_data.direction.diffuse_color.r = 1;
	light_data.direction.diffuse_color.g = 1;
	light_data.direction.diffuse_color.b = 1;
	light_data.direction.diffuse_color.a = 0;
	light_data.direction.specular_color.r = 1;
	light_data.direction.specular_color.g = 1;
	light_data.direction.specular_color.b = 1;
	light_data.direction.specular_color.a = 0;
	light_data.direction.ambient_color.r = 0;
	light_data.direction.ambient_color.g = 0;
	light_data.direction.ambient_color.b = 0;
	light_data.direction.ambient_color.a = 0;
	light_data.direction.dst.x = -1;
	light_data.direction.dst.y = -1;
	light_data.direction.dst.z = 0;
	dir_light = gx3d_InitLight(&light_data);

	light_data.light_type = gx3d_LIGHT_TYPE_POINT;
	light_data.point.diffuse_color.r = 1;
	light_data.point.diffuse_color.g = 1;
	light_data.point.diffuse_color.b = 1;
	light_data.point.diffuse_color.a = 0;
	light_data.point.specular_color.r = 1;
	light_data.point.specular_color.g = 1;
	light_data.point.specular_color.b = 1;
	light_data.point.specular_color.a = 0;
	light_data.point.ambient_color.r = 1;
	light_data.point.ambient_color.g = 1;
	light_data.point.ambient_color.b = 1;
	light_data.point.ambient_color.a = 0;
	light_data.point.range = 30;
	light_data.point.constant_attenuation = 0;
	light_data.point.linear_attenuation = 0.1f;
	light_data.point.quadratic_attenuation = 0;
	light_data.point.src.x = 0;
	light_data.point.src.y = 0;
	light_data.point.src.z = 0;
	fire_light = gx3d_InitLight(&light_data);

	light_data2.light_type = gx3d_LIGHT_TYPE_POINT;
	light_data2.point.diffuse_color.r = 1;
	light_data2.point.diffuse_color.g = 1;
	light_data2.point.diffuse_color.b = 1;
	light_data2.point.diffuse_color.a = 0;
	light_data2.point.specular_color.r = 1;
	light_data2.point.specular_color.g = 1;
	light_data2.point.specular_color.b = 1;
	light_data2.point.specular_color.a = 0;
	light_data2.point.ambient_color.r = 0;
	light_data2.point.ambient_color.g = 0;
	light_data2.point.ambient_color.b = 0;
	light_data2.point.ambient_color.a = 0;
	light_data2.point.range = 25;
	light_data2.point.constant_attenuation = 0;
	light_data2.point.linear_attenuation = 0; //0.1f;
	light_data2.point.quadratic_attenuation = 0;
	light_data2.point.src.x = position.x;
	light_data2.point.src.y = position.y + CHARACTER_HEIGHT - 1.5; // lantern held next to chest
	light_data2.point.src.z = position.z;
	lantern_light = gx3d_InitLight(&light_data2);

	//flashlight - needs work...
	/*gx3dVector direction, normalized_direction;
	direction.x = heading.x - position.x;
	direction.y = heading.y - position.y;
	direction.z = heading.z - position.z;
	gx3d_NormalizeVector(&direction, &normalized_direction);
	light_data.light_type = gx3d_LIGHT_TYPE_SPOT;
	light_data.spot.diffuse_color.r = 1;
	light_data.spot.diffuse_color.g = 1;
	light_data.spot.diffuse_color.b = 1;
	light_data.spot.diffuse_color.a = 0;
	light_data.spot.specular_color.r = 1;
	light_data.spot.specular_color.g = 1;
	light_data.spot.specular_color.b = 1;
	light_data.spot.specular_color.a = 0;
	light_data.spot.ambient_color.r = 0;
	light_data.spot.ambient_color.g = 0;
	light_data.spot.ambient_color.b = 0;
	light_data.spot.ambient_color.a = 0;
	light_data.spot.range = 250;
	light_data.spot.falloff = 1.0;
	light_data.spot.inner_cone_angle = 30;
	light_data.spot.outer_cone_angle = 45;
	light_data.spot.src.x = position.x;
	light_data.spot.src.y = position.y + CHARACTER_HEIGHT - 1.5;
	light_data.spot.src.z = position.z;
	light_data.spot.dst.x = position.x + normalized_direction.x;
	light_data.spot.dst.y = position.y + CHARACTER_HEIGHT - 1.5 + normalized_direction.y;
	light_data.spot.dst.z = position.z + normalized_direction.z;
	light_flashlight = gx3d_InitLight(&light_data);*/

	//gx3dVector light_position = { 10, 20, 0 }, xlight_position;
	//float angle = 0;

	/*____________________________________________________________________
	|
	| Flush input queue
	|___________________________________________________________________*/

	int move_x, move_y;	// mouse movement counters

	// Flush input queue
	evFlushEvents();
	// Zero mouse movement counters
	msGetMouseMovement(&move_x, &move_y);  // call this here so the next call will get movement that has occurred since it was called here                                    
	// Hide mouse cursor
	msHideMouse();

	/*____________________________________________________________________
	|
	| Main game loop
	|___________________________________________________________________*/

	// Variables
	unsigned elapsed_time, last_time, new_time;
	bool force_update;
	unsigned cmd_move;

	// Init loop variables
	cmd_move = 0;
	last_time = 0;
	force_update = false;

	lantern_light_on = 0, dir_light_on = 0;
	bool draw_wireframe = false, fastMovement = false;

	snd_SetSoundVolume(s_forest, 90);
	snd_SetSoundVolume(s_ouch, 90);
	snd_SetSoundVolume(s_footsteps, 90);
	snd_SetSoundVolume(s_running, 90);
	snd_SetSoundVolume(s_gameover, 75);
	snd_SetSoundVolume(s_paper, 75);
	snd_SetSoundVolume(s_wolves, 75);
	snd_SetSoundVolume(s_survived, 90);
	snd_SetSoundVolume(s_fire, 100);
	snd_SetSoundMode(s_fire, snd_3D_MODE_ORIGIN_RELATIVE, snd_3D_APPLY_NOW);
	snd_SetSoundPosition(s_fire, 0, 0, 0, snd_3D_APPLY_NOW);
	snd_SetSoundMinDistance(s_fire, 10, snd_3D_APPLY_NOW);
	snd_SetSoundMaxDistance(s_fire, 100, snd_3D_APPLY_NOW);
	snd_PlaySound(s_title, 1);

	// Game loop
	for (quit = FALSE; NOT quit; ) {

		take_screenshot = FALSE;

		/*____________________________________________________________________
		|
		| Update clock
		|___________________________________________________________________*/

		// Get the current time (# milliseconds since the program started)
		new_time = timeGetTime();
		// Compute the elapsed time (in milliseconds) since the last time through this loop
		if (last_time == 0)
			elapsed_time = 0;
		else
			elapsed_time = new_time - last_time;
		last_time = new_time;

		if (screen_change) {

			gx3d_ClearViewport(gx3d_CLEAR_SURFACE | gx3d_CLEAR_ZBUFFER, color, gx3d_MAX_ZBUFFER_VALUE, 0);
			// Start rendering in 3D           
			if (gx3d_BeginRender()) {
				// Set the default material
				gx3d_SetMaterial(&material_default);

				// Set  amount of ambient light
				gx3d_SetAmbientLight(color3d_white);

				if (screen_title) {

					Draw_Screen(tex_title_screen);

					if (evGetEvent(&event)) {
						if (event.type == evTYPE_RAW_KEY_PRESS) {
							if (event.keycode == evKY_ESC)
								quit = TRUE;
							else if (event.keycode == evKY_ENTER) {
								screen_title = false;
								screen_change = true;
							}
						}
					}
				}
				else if (screen_story2) {

					cmd_move = 0;

					if (!snd_IsPlaying(s_survived))
						snd_PlaySound(s_survived, 0);
					else if (snd_IsPlaying(s_forest))
						snd_StopSound(s_forest);
					else if (snd_IsPlaying(s_fire))
						snd_StopSound(s_fire);
					else if (snd_IsPlaying(s_footsteps))
						snd_StopSound(s_footsteps);
					else if (snd_IsPlaying(s_running))
						snd_StopSound(s_running);
					else if (snd_IsPlaying(s_wolves))
						snd_StopSound(s_wolves);
					else if (snd_IsPlaying(s_paper))
						snd_StopSound(s_paper);

					Draw_Screen(tex_story2_screen);

					if (evGetEvent(&event)) {
						if (event.type == evTYPE_RAW_KEY_PRESS) {
							if (event.keycode == evKY_ESC)
								quit = TRUE;
							else if (event.keycode == evKY_ENTER) {
								screen_story2 = false;
								screen_survive = true;
							}
						}
					}
				}
				else if (screen_survive) {
					// You Survived!

					cmd_move = 0;

					Draw_Screen(tex_survive_screen);

					if (evGetEvent(&event)) {
						if (event.type == evTYPE_RAW_KEY_PRESS) {
							if (event.keycode == evKY_ESC)
								quit = TRUE;
						}
					}
				}
				else if (screen_gameover) {
					// Game Over!

					cmd_move = 0;

					Draw_Screen(tex_gameover_screen);

					if (!snd_IsPlaying(s_gameover))
						snd_PlaySound(s_gameover, 0);
					else if (snd_IsPlaying(s_forest))
						snd_StopSound(s_forest);
					else if (snd_IsPlaying(s_fire))
						snd_StopSound(s_fire);
					else if (snd_IsPlaying(s_footsteps))
						snd_StopSound(s_footsteps);
					else if (snd_IsPlaying(s_running))
						snd_StopSound(s_running);
					else if (snd_IsPlaying(s_wolves))
						snd_StopSound(s_wolves);

					if (evGetEvent(&event)) {
						if (event.type == evTYPE_RAW_KEY_PRESS) {
							if (event.keycode == evKY_ESC)
								quit = TRUE;
						}
					}
				}
				else if (screen_firstpage) {

					cmd_move = 0;

					if (snd_IsPlaying(s_forest))
						snd_StopSound(s_forest);
					else if (snd_IsPlaying(s_fire))
						snd_StopSound(s_fire);
					else if (snd_IsPlaying(s_footsteps))
						snd_StopSound(s_footsteps);
					else if (snd_IsPlaying(s_running))
						snd_StopSound(s_running);
					else if (snd_IsPlaying(s_wolves))
						snd_StopSound(s_wolves);

					// Show page screen
					Draw_Screen(tex_firstpage_screen);

					if (evGetEvent(&event)) {
						if (event.type == evTYPE_RAW_KEY_PRESS) {
							if (event.keycode == evKY_ESC)
								quit = TRUE;
							else if (event.keycode == evKY_ENTER) {
								screen_firstpage = false;
								screen_change = false;
							}
						}
					}
				}
				else if (screen_story1) {
					// Show page screen
					Draw_Screen(tex_story1_screen);

					if (snd_IsPlaying(s_title))
						snd_StopSound(s_title);
					else if (!snd_IsPlaying(s_story1))
						snd_PlaySound(s_story1, 1);

					if (evGetEvent(&event)) {
						if (event.type == evTYPE_RAW_KEY_PRESS) {
							if (event.keycode == evKY_ESC)
								quit = TRUE;
							else if (event.keycode == evKY_ENTER) {
								screen_story1 = false;
								screen_change = true;
							}
						}
					}
				}
				else {

					cmd_move = 0;

					// Pause/Help Screen
					Draw_Screen(tex_pause_screen);

					if (snd_IsPlaying(s_story1))
						snd_StopSound(s_story1);

					// Enter to continue
					if (evGetEvent(&event)) {
						if (event.type == evTYPE_RAW_KEY_PRESS) {
							if (event.keycode == evKY_ESC)
								quit = TRUE;
							else if (event.keycode == evKY_ENTER)
							{
								screen_change = false;
							}
						}
					}
				}

				// Stop rendering
				gx3d_EndRender();

				// Page flip (so user can see it)
				gxFlipVisualActivePages(FALSE);
			}
		}
		else {

			/*____________________________________________________________________
			|
			| Process user input
			|___________________________________________________________________*/

			// Any event ready?
			if (evGetEvent(&event)) {
				// key press?
				if (event.type == evTYPE_RAW_KEY_PRESS) {
					// If ESC pressed, exit the program
					if (event.keycode == evKY_ESC)
						quit = TRUE;
					else if (event.keycode == 'w')
						cmd_move |= POSITION_MOVE_FORWARD;
					else if (event.keycode == 's')
						cmd_move |= POSITION_MOVE_BACK;
					else if (event.keycode == 'a')
						cmd_move |= POSITION_MOVE_LEFT;
					else if (event.keycode == 'd')
						cmd_move |= POSITION_MOVE_RIGHT;
					else if (event.keycode == 'f')
						draw_wireframe = true;
					else if (event.keycode == evKY_ENTER) {
						if (screen_change)
							screen_change = false;
						else
							screen_change = true;
					}
					else if (event.keycode == evKY_F1)
						take_screenshot = TRUE;
					else if (event.keycode == evKY_F3)
						lantern_light_on ^= 1;
					else if (event.keycode == evKY_F4)
						dir_light_on ^= 1;
					else if (event.keycode == evKY_SHIFT) {
						Position_Set_Speed(RUN_SPEED * (float)2.5);
						fastMovement = true;
					}
				}
				// key release?
				else if (event.type == evTYPE_RAW_KEY_RELEASE) {
					if (event.keycode == 'w')
						cmd_move &= ~(POSITION_MOVE_FORWARD);
					else if (event.keycode == 's')
						cmd_move &= ~(POSITION_MOVE_BACK);
					else if (event.keycode == 'a')
						cmd_move &= ~(POSITION_MOVE_LEFT);
					else if (event.keycode == 'd')
						cmd_move &= ~(POSITION_MOVE_RIGHT);
					else if (event.keycode == 'f')
						draw_wireframe = false;
					else if (event.keycode == evKY_SHIFT) {
						Position_Set_Speed(RUN_SPEED);
						fastMovement = false;
					}
				}
				// mouse left press?
				else if (event.type == evTYPE_MOUSE_LEFT_PRESS) {
					// Play sound effect for the press
					// ADD CODE HERE
					// Perform a ray test on all visible targets
					gx3dRay viewVector;
					viewVector.origin = position;
					viewVector.direction = heading;
					bool paper_hit = false;
					int i;
					for (i = 0; i < NUM_PAPER && !paper_hit; i++) {
						if (paperOnScreen[i]) {
							gxRelation rel = gx3d_Relation_Ray_Sphere(&viewVector, 2.5, &paperSphere[i]);
							if (rel != gxRELATION_OUTSIDE) {

								if (!snd_IsPlaying(s_paper))
									snd_PlaySound(s_paper, 0);

								// Remove this paper from the game
								paperDraw[i] = false;
								num_paper_touched++;
								if (num_paper_touched >= 5) {// pickup at least 5/8 pages to win
									num_paper_touched = 5;
									screen_change = true;
									screen_story2 = true;
								}
								else if (num_paper_touched == 1)
									screen_change = true;
								screen_firstpage = true;

								paper_hit = true;
							}
						}
					}
				}
				switch (cmd_move) {
				case 0:
					snd_StopSound(s_footsteps);
					snd_StopSound(s_running);
					break;
				default:
					if (fastMovement) {
						snd_StopSound(s_footsteps);
						if (!snd_IsPlaying(s_running))
							snd_PlaySound(s_running, 1);
					}
					else {
						snd_StopSound(s_running);
						if (!snd_IsPlaying(s_footsteps))
							snd_PlaySound(s_footsteps, 1);
					}
					break;
				}
			}
			// Check for camera movement (via mouse)
			msGetMouseMovement(&move_x, &move_y);

			/*____________________________________________________________________
			|
			| Update camera view
			|___________________________________________________________________*/

			if (light_data2.point.src.x != position.x)
				light_data2.point.src.x = position.x;
			else if (light_data2.point.src.z != position.z)
				light_data2.point.src.z = position.z;
			gx3d_UpdateLight(lantern_light, &light_data2);

			// COLLISION DETECTION - MORE RESEARCH REQUIRED
			//gx3dTrajectory cameraTrajectory;

			//// Set camera's collision sphere properties 
			//cameraSphere.center = position;
			//cameraSphere.radius = 6.0f;	// adjust as needed	

			//// Set camera's trajectory (initially, it's unchanged) 
			//cameraTrajectory.direction = heading;
			//cameraTrajectory.velocity = RUN_SPEED;

			//// Check for collisions with trees 
			//int collisionDetected = 0;
			//float dtime = 1;
			//float distance = RUN_SPEED * dtime;
			//for (int i = 0; i < NUM_TREES && !collisionDetected; i++) {
			//	float pTime;
			//	if (gx3d_Collide_Sphere_StaticSphere(&cameraSphere, &cameraTrajectory, dtime, &treeSphere[i], &pTime) == gxRELATION_INTERSECT) {
			//		position.x -= cameraTrajectory.direction.x * distance * (1.0f - pTime);
			//		position.y -= cameraTrajectory.direction.y * distance * (1.0f - pTime);
			//		position.z -= cameraTrajectory.direction.z * distance * (1.0f - pTime);

			//		// Update the camera's collision sphere center 
			//		cameraSphere.center = position;

			//		// Update the camera's trajectory 
			//		cameraTrajectory.direction = heading;
			//		cameraTrajectory.velocity = RUN_SPEED;

			//		collisionDetected = 1;
			//		break;
			//	}
			//}

			//// Update camera position and trajectory if no collision 

			//if (!collisionDetected) {
			//	// Update the camera's position 
			//	position.x += heading.x * distance;
			//	position.y += heading.y * distance;
			//	position.z += heading.z * distance;

			//	// Update the camera's collision sphere center 
			//	cameraSphere.center = position;

			//	// Update the camera's trajectory 
			//	cameraTrajectory.direction = heading;
			//	cameraTrajectory.velocity = RUN_SPEED;
			//}

			bool position_changed, camera_changed, collision;
			Position_Update(elapsed_time, cmd_move, move_y, move_x, force_update,
				&position_changed, &camera_changed, &collision, &position, &heading);

			snd_SetListenerPosition(position.x, position.y, position.z, snd_3D_APPLY_NOW);
			snd_SetListenerOrientation(heading.x, heading.y, heading.z, 0, 1, 0, snd_3D_APPLY_NOW);

			/*____________________________________________________________________
			|
			| Draw 3D graphics
			|___________________________________________________________________*/

			gx3d_SetFogColor(0, 0, 0);
			gx3d_SetLinearPixelFog(15, 150);

			// Render the screen
			gx3d_ClearViewport(gx3d_CLEAR_SURFACE | gx3d_CLEAR_ZBUFFER, color, gx3d_MAX_ZBUFFER_VALUE, 0);
			// Start rendering in 3D
			if (gx3d_BeginRender()) {
				// Set the default material
				gx3d_SetMaterial(&material_default);
				gx3d_SetAmbientLight(color3d_dim);
				//Enable Fog
				gx3d_EnableFog();

				// Play Forest Audio
				if (!snd_IsPlaying(s_forest))
					snd_PlaySound(s_forest, 1);
				else if (!snd_IsPlaying(s_fire))
					snd_PlaySound(s_fire, 1);

				const int SOUND_CHANCE = 10; // % chance of sound playing

				// generate a random number between 0 and 99
				srand(time(0));
				int random_num = rand() % 100;

				// check if the number is less than or equal to SOUND_CHANCE
				if (random_num <= SOUND_CHANCE) {
					if (!snd_IsPlaying(s_wolves))
						snd_PlaySound(s_wolves, 0); // wolves howling
				}

				// Draw ground
				gx3d_GetTranslateMatrix(&m, 0, 0, 0);
				gx3d_SetObjectMatrix(obj_ground, &m);
				gx3d_SetTexture(0, tex_ground);
				gx3d_DrawObject(obj_ground, 0);

				// Draw skydome
				gx3d_SetAmbientLight(color3d_white);
				gx3d_GetScaleMatrix(&m1, 200, 100, 200);
				gx3d_GetTranslateMatrix(&m2, 0, 0, 0);
				gx3d_MultiplyMatrix(&m1, &m2, &m);
				gx3d_SetObjectMatrix(obj_skydome, &m);
				gx3d_SetTexture(0, tex_skydome);
				gx3d_DrawObject(obj_skydome, 0);

				// Enable alpha blending and testing
				gx3d_EnableAlphaBlending();
				gx3d_EnableAlphaTesting(128);
				// Enable a fire light
				gx3d_EnableLight(fire_light);
				gx3d_SetAmbientLight(color3d_dim);

				// Draw a tree
				for (int i = 0; i < NUM_TREES; i++) {
					gx3d_GetTranslateMatrix(&m, treePosition[i].x, 0, treePosition[i].z);
					gx3d_SetObjectMatrix(obj_tree, &m);
					gx3d_SetTexture(0, tex_tree);
					gx3d_DrawObject(obj_tree, 0);
				}

				// Draw a paper
				static gx3dVector billboard_normal = { 0, 0, 1 };
				for (int i = 0; i < NUM_PAPER; i++) {
					paperOnScreen[i] = false;
					gxRelation relation;
					relation = gx3d_Relation_Sphere_Frustum(&treeSphere[i]);
					if (relation != gxRELATION_OUTSIDE) {

						if (paperDraw[i]) {
							// check the bounding volume of the paper with the view frustum
							if (true) {
								gx3d_GetScaleMatrix(&m1, 1, 1, 1);
								gx3d_GetBillboardRotateYMatrix(&m2, &billboard_normal, &heading);
								gx3d_GetTranslateMatrix(&m3, paperPosition[i].x, paperPosition[i].y, paperPosition[i].z);
								gx3d_MultiplyMatrix(&m1, &m2, &m);
								gx3d_MultiplyMatrix(&m, &m3, &m);
								gx3d_SetObjectMatrix(obj_paper, &m);
								gx3d_SetTexture(0, tex_paper);
								gx3d_DrawObject(obj_paper, 0);
								paperOnScreen[i] = true;
							}
							// Update bounding spheres of each paper
							paperSphere[i] = obj_paper->bound_sphere;
							paperSphere[i].center.x *= 6;
							paperSphere[i].center.y *= 6;
							paperSphere[i].center.z *= 6;
							paperSphere[i].center.x = paperPosition[i].x;
							paperSphere[i].center.y = paperPosition[i].y;
							paperSphere[i].center.z = paperPosition[i].z;
							paperSphere[i].radius *= 6;
						}
					}
				}

				// Move position of slender
				srand(time(0));
				for (int i = 0; i < Slender; i++) {
					// Get direction vector from Slender to camera with random variation
					gx3dVector dir = {
						position.x - SlenderPosition[i].x + (float(rand()) / RAND_MAX - 0.5f) * 0.1f,
						position.y - 0,
						position.z - SlenderPosition[i].z + (float(rand()) / RAND_MAX - 0.5f) * 0.1f
					};
					gx3dVector normalizedVector;
					// Normalize direction vector
					gx3d_NormalizeVector(&dir, &normalizedVector);
					// Calculate movement speed
					float speed = 0.005f; // adjust as needed
					// Move Slender towards camera
					SlenderPosition[i].x += dir.x * speed;
					SlenderPosition[i].y += 0;
					SlenderPosition[i].z += dir.z * speed;

					if (SlenderPosition[i].x > 150)
						SlenderPosition[i].x *= -1;
					else if (SlenderPosition[i].x < -150)
						SlenderPosition[i].x *= -1;
					else if (SlenderPosition[i].z > 150)
						SlenderPosition[i].z *= -1;
					else if (SlenderPosition[i].z < -150)
						SlenderPosition[i].z *= -1;

					gx3dVector diff;

					// calculate distance between Slenderman and camera
					gx3d_SubtractVector(&position, &SlenderPosition[i], &diff);
					float distance = gx3d_VectorMagnitude(&diff);

					// check if Slenderman is within a certain distance from the camera, will trigger Game Over!
					if (distance <= 10) {
						screen_change = true;
						screen_gameover = true;
					}
				}

				// Draw SlenderMan
				static gx3dVector billboard_normal2 = { 0, 0, 1 };
				for (int i = 0; i < Slender; i++) {
					gx3d_GetScaleMatrix(&m1, 6, 6, 6);
					gx3d_GetBillboardRotateYMatrix(&m2, &billboard_normal2, &heading);
					gx3d_GetTranslateMatrix(&m3, SlenderPosition[i].x, SlenderPosition[i].y, SlenderPosition[i].z);
					gx3d_MultiplyMatrix(&m1, &m2, &m);
					gx3d_MultiplyMatrix(&m, &m3, &m);
					gx3d_SetObjectMatrix(obj_slender, &m);
					gx3d_SetTexture(0, tex_slender);
					gx3d_DrawObject(obj_slender, 0);
				}

				// Disable Fog
				gx3d_DisableFog();

				// Disable alpha blending
				gx3d_DisableAlphaBlending();
				gx3d_DisableAlphaTesting();

				/*____________________________________________________________________
				|
				| Take Screenshot
				|___________________________________________________________________*/

				// Take screenshot
				if (take_screenshot) {
					char str[32];
					char buff[50];
					static int screenshot_count = 0;
					strcpy(str, SCREENSHOT_FILENAME);
					strcat(str, itoa(screenshot_count++, buff, 10));
					strcat(str, ".bmp");
					gxWriteBMPFile(str);
				}

				/*____________________________________________________________________
				|
				| Update Lantern
				|___________________________________________________________________*/

				if (NOT lantern_light_on) {
					gx3d_DisableLight(lantern_light);
				}
				else {
					gx3d_EnableLight(lantern_light);
				}

				/*____________________________________________________________________
				|
				| Update Directional Light
				|___________________________________________________________________*/

				if (NOT dir_light_on) {
					gx3d_DisableLight(dir_light);
				}
				else {
					gx3d_EnableLight(dir_light);
				}

				/*____________________________________________________________________
				|
				| Draw fire particle system
				|___________________________________________________________________*/
				// Set ambient lighting for fire
				gx3d_SetAmbientLight(color3d_white);
				gx3d_EnableAlphaBlending();
				gx3d_GetTranslateMatrix(&m, 0, -0.5, 0);
				gx3d_SetParticleSystemMatrix(psys_fire, &m);
				gx3d_UpdateParticleSystem(psys_fire, elapsed_time);
				gx3d_DrawParticleSystem(psys_fire, &heading, draw_wireframe);
				gx3d_DisableLight(fire_light);
				gx3d_DisableAlphaBlending();

				/*____________________________________________________________________
				|
				| Draw 2D graphics on top of 3D and Process Paper Markers
				|___________________________________________________________________*/

				// Save current view matrix
				gx3dMatrix view_save;
				gx3d_GetViewMatrix(&view_save);

				// Set new view matrix
				gx3dVector tfrom = { 0,0,-1 }, tto = { 0,0,0 }, twup = { 0,1,0 };
				gx3d_CameraSetPosition(&tfrom, &tto, &twup, gx3d_CAMERA_ORIENTATION_LOOKTO_FIXED);
				gx3d_CameraSetViewMatrix();

				// Draw 2D icons at top of screen
				if (num_paper_touched) {
					gx3d_DisableZBuffer();
					gx3d_EnableAlphaBlending();
					for (int i = 0; i < num_paper_touched; i++) {
						gx3d_GetScaleMatrix(&m1, 0.025f, 0.025f, 0.025f);
						gx3d_GetRotateYMatrix(&m2, 180);
						gx3d_GetTranslateMatrix(&m3, -0.55 + (0.033 * i), 0.38, 0);
						gx3d_MultiplyMatrix(&m1, &m2, &m);
						gx3d_MultiplyMatrix(&m, &m3, &m);
						gx3d_SetObjectMatrix(obj_paper, &m);
						gx3d_SetTexture(0, tex_paper);
						gx3d_DrawObject(obj_paper, 0);
					}
					gx3d_DisableAlphaBlending();
					gx3d_EnableZBuffer();
				}
				// Restore view matrix
				gx3d_SetViewMatrix(&view_save);

				// Stop rendering
				gx3d_EndRender();

				// Page flip (so user can see it)
				gxFlipVisualActivePages(FALSE);
			}
		}
	}
	/*____________________________________________________________________
	|
	| Free stuff and exit
	|___________________________________________________________________*/

	gx3d_FreeLight(dir_light);
	gx3d_FreeLight(lantern_light);
	gx3d_FreeLight(fire_light);
	gx3d_FreeParticleSystem(psys_fire);
	gx3d_FreeAllObjects();
	gx3d_FreeAllTextures();
	snd_Free();
}

/*____________________________________________________________________
|
| Function: Init_Render_State
|
| Input: Called from Program_Run()
| Output: Initializes general 3D render state.
|___________________________________________________________________*/

static void Init_Render_State()
{
	// Enable zbuffering
	gx3d_EnableZBuffer();

	// Enable lighting
	gx3d_EnableLighting();

	// Set the default alpha blend factor
	gx3d_SetAlphaBlendFactor(gx3d_ALPHABLENDFACTOR_SRCALPHA, gx3d_ALPHABLENDFACTOR_INVSRCALPHA);

	// Init texture addressing mode - wrap in both u and v dimensions
	gx3d_SetTextureAddressingMode(0, gx3d_TEXTURE_DIMENSION_U | gx3d_TEXTURE_DIMENSION_V, gx3d_TEXTURE_ADDRESSMODE_WRAP);
	gx3d_SetTextureAddressingMode(1, gx3d_TEXTURE_DIMENSION_U | gx3d_TEXTURE_DIMENSION_V, gx3d_TEXTURE_ADDRESSMODE_WRAP);
	// Texture stage 0 default blend operator and arguments
	gx3d_SetTextureColorOp(0, gx3d_TEXTURE_COLOROP_MODULATE, gx3d_TEXTURE_ARG_TEXTURE, gx3d_TEXTURE_ARG_CURRENT);
	gx3d_SetTextureAlphaOp(0, gx3d_TEXTURE_ALPHAOP_SELECTARG1, gx3d_TEXTURE_ARG_TEXTURE, 0);
	// Texture stage 1 is off by default
	gx3d_SetTextureColorOp(1, gx3d_TEXTURE_COLOROP_DISABLE, 0, 0);
	gx3d_SetTextureAlphaOp(1, gx3d_TEXTURE_ALPHAOP_DISABLE, 0, 0);

	// Set default texture coordinates
	gx3d_SetTextureCoordinates(0, gx3d_TEXCOORD_SET0);
	gx3d_SetTextureCoordinates(1, gx3d_TEXCOORD_SET1);

	// Enable trilinear texture filtering
	gx3d_SetTextureFiltering(0, gx3d_TEXTURE_FILTERTYPE_TRILINEAR, 0);
	gx3d_SetTextureFiltering(1, gx3d_TEXTURE_FILTERTYPE_TRILINEAR, 0);
}

/*____________________________________________________________________
|
| Function: Program_Free
|
| Input: Called from CMainFrame::OnClose()
| Output: Exits graphics mode.
|___________________________________________________________________*/

void Program_Free()
{
	// Stop event processing 
	evStopEvents();
	// Return to text mode 
	if (Pgm_system_font)
		gxFreeFont(Pgm_system_font);
	gxStopGraphics();
}
