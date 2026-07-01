#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <string.h>
#include "vshader_shbin.h"

#define CLEAR_COLOR 0x101018FF   // bleu nuit (RGBA8)

// Flags de transfert GPU -> framebuffer (config standard des exemples citro3d)
#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

// --- Géométrie du cube ---------------------------------------------------
// Chaque sommet porte sa position et sa couleur (RGB en float).
// 6 faces * 2 triangles * 3 sommets = 36 sommets, une couleur par face.
typedef struct { float position[3]; float color[3]; } vertex;

#define R {1.0f, 0.25f, 0.25f}   // +X rouge
#define G {0.25f, 1.0f, 0.35f}   // -X vert
#define B {0.30f, 0.45f, 1.0f}   // +Y bleu
#define Y {1.0f, 0.85f, 0.20f}   // -Y jaune
#define M {1.0f, 0.30f, 1.0f}    // +Z magenta
#define C {0.25f, 1.0f, 1.0f}    // -Z cyan

static const vertex vertex_list[] =
{
	// +X
	{ {+0.5f,-0.5f,-0.5f}, R }, { {+0.5f,+0.5f,-0.5f}, R }, { {+0.5f,+0.5f,+0.5f}, R },
	{ {+0.5f,-0.5f,-0.5f}, R }, { {+0.5f,+0.5f,+0.5f}, R }, { {+0.5f,-0.5f,+0.5f}, R },
	// -X
	{ {-0.5f,-0.5f,-0.5f}, G }, { {-0.5f,+0.5f,-0.5f}, G }, { {-0.5f,+0.5f,+0.5f}, G },
	{ {-0.5f,-0.5f,-0.5f}, G }, { {-0.5f,+0.5f,+0.5f}, G }, { {-0.5f,-0.5f,+0.5f}, G },
	// +Y
	{ {-0.5f,+0.5f,-0.5f}, B }, { {+0.5f,+0.5f,-0.5f}, B }, { {+0.5f,+0.5f,+0.5f}, B },
	{ {-0.5f,+0.5f,-0.5f}, B }, { {+0.5f,+0.5f,+0.5f}, B }, { {-0.5f,+0.5f,+0.5f}, B },
	// -Y
	{ {-0.5f,-0.5f,-0.5f}, Y }, { {+0.5f,-0.5f,-0.5f}, Y }, { {+0.5f,-0.5f,+0.5f}, Y },
	{ {-0.5f,-0.5f,-0.5f}, Y }, { {+0.5f,-0.5f,+0.5f}, Y }, { {-0.5f,-0.5f,+0.5f}, Y },
	// +Z
	{ {-0.5f,-0.5f,+0.5f}, M }, { {+0.5f,-0.5f,+0.5f}, M }, { {+0.5f,+0.5f,+0.5f}, M },
	{ {-0.5f,-0.5f,+0.5f}, M }, { {+0.5f,+0.5f,+0.5f}, M }, { {-0.5f,+0.5f,+0.5f}, M },
	// -Z
	{ {-0.5f,-0.5f,-0.5f}, C }, { {+0.5f,-0.5f,-0.5f}, C }, { {+0.5f,+0.5f,-0.5f}, C },
	{ {-0.5f,-0.5f,-0.5f}, C }, { {+0.5f,+0.5f,-0.5f}, C }, { {-0.5f,+0.5f,-0.5f}, C },
};

#define vertex_count (sizeof(vertex_list) / sizeof(vertex_list[0]))

// --- État global ---------------------------------------------------------
static DVLB_s* vshader_dvlb;
static shaderProgram_s program;
static int uLoc_projection, uLoc_modelView;
static C3D_Mtx projection;
static void* vbo_data;

static float angleX = 0.0f, angleY = 0.0f;

// Rendu d'une "vue" (un œil). iod = décalage inter-oculaire :
//  0 pour l'écran du bas / 2D, +/- valeur pour l'œil droit/gauche en 3D.
static void sceneRender(float iod)
{
	// Projection avec décalage stéréo. focLen = 2.0 (plan de convergence).
	Mtx_PerspStereoTilt(&projection, C3D_AngleFromDegrees(40.0f),
		C3D_AspectRatioTop, 0.01f, 1000.0f, iod, 2.0f, false);

	// Matrice modèle+vue : on recule le cube et on le fait tourner.
	C3D_Mtx modelView;
	Mtx_Identity(&modelView);
	Mtx_Translate(&modelView, 0.0f, 0.0f, -2.5f, true);
	Mtx_RotateX(&modelView, angleX, true);
	Mtx_RotateY(&modelView, angleY, true);

	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView, &modelView);

	C3D_DrawArrays(GPU_TRIANGLES, 0, vertex_count);
}

int main()
{
	gfxInitDefault();
	gfxSet3D(true);                 // active le mode 3D stéréoscopique
	consoleInit(GFX_BOTTOM, NULL);  // console d'infos sur l'écran du bas

	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	// Cibles de rendu : une par œil, sur l'écran du haut.
	C3D_RenderTarget* targetLeft  = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(targetLeft, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
	C3D_RenderTarget* targetRight = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(targetRight, GFX_TOP, GFX_RIGHT, DISPLAY_TRANSFER_FLAGS);

	// Chargement du vertex shader (compilé depuis vshader.v.pica).
	vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
	shaderProgramInit(&program);
	shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);
	C3D_BindProgram(&program);

	uLoc_projection = shaderInstanceGetUniformLocation(program.vertexShader, "projection");
	uLoc_modelView  = shaderInstanceGetUniformLocation(program.vertexShader, "modelView");

	// Description des attributs de sommet : v0 = position, v1 = couleur.
	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // position
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 3); // couleur

	// VBO en mémoire linéaire (accessible par le GPU).
	vbo_data = linearAlloc(sizeof(vertex_list));
	memcpy(vbo_data, vertex_list, sizeof(vertex_list));

	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, vbo_data, sizeof(vertex), 2, 0x10);

	// Étage de texture : on sort directement la couleur du sommet.
	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(env);
	C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

	// Test de profondeur (le PICA200 travaille en profondeur inversée).
	C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_ALL);
	C3D_CullFace(GPU_CULL_NONE);

	// Gyroscope (renvoie 0 sur émulateur, actif sur vraie console).
	HIDUSER_EnableGyroscope();

	printf("\x1b[1;1HCube 3D stereoscopique - citro3d");
	printf("\x1b[3;1HSTART : quitter");
	printf("\x1b[4;1HCurseur 3D : profondeur");
	printf("\x1b[5;1HGyroscope : incline la console");

	while (aptMainLoop())
	{
		hidScanInput();
		if (hidKeysDown() & KEY_START) break;

		// Rotation automatique + apport du gyroscope.
		angleY += 0.02f;
		angularRate gyro;
		hidGyroRead(&gyro);
		angleX += gyro.x / 3000.0f;
		angleY += gyro.y / 3000.0f;

		// Le curseur 3D physique règle la force du relief (0 = 2D).
		float slider = osGet3DSliderState();
		float iod = slider / 3.0f;

		printf("\x1b[7;1HCurseur 3D : %.2f   ", slider);

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C3D_RenderTargetClear(targetLeft, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
			C3D_FrameDrawOn(targetLeft);
			sceneRender(-iod);

			if (iod > 0.0f)
			{
				C3D_RenderTargetClear(targetRight, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
				C3D_FrameDrawOn(targetRight);
				sceneRender(iod);
			}
		C3D_FrameEnd(0);
	}

	// Nettoyage.
	HIDUSER_DisableGyroscope();
	linearFree(vbo_data);
	shaderProgramFree(&program);
	DVLB_Free(vshader_dvlb);
	C3D_Fini();
	gfxExit();
	return 0;
}
