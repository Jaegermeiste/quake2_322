/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// NeVo - protect gl_local.h
#ifndef __GL_LOCAL_H__
#define __GL_LOCAL_H__

#ifdef _WIN32
#  include <windows.h>
#endif

#include <stdio.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>

#ifndef __linux__
#ifndef GL_COLOR_INDEX8_EXT
#define GL_COLOR_INDEX8_EXT GL_COLOR_INDEX
#endif
#endif

#include "../client/ref.h"

//#define	REF_VERSION	"GL 0.01"
#define REF_VERSION "D3D 3.22.03"

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2


#ifndef __VIDDEF_T
#define __VIDDEF_T
typedef struct viddef_s
{
	unsigned		width, height;			// coordinates from main game
} viddef_t;
#endif

extern	viddef_t	vid;


/*

  skins will be outline flood filled and mip mapped
  pics and sprites with alpha will be outline flood filled
  pic won't be mip mapped

  model skin
  sprite frame
  wall texture
  pic

*/

typedef enum imagetype_e
{
	it_skin,
	it_sprite,
	it_wall,
	it_pic,
	it_sky
} imagetype_t;

typedef struct image_s
{
	char	name[MAX_QPATH];			// game path, including extension
	imagetype_t	type;
	int		width, height;				// source image
	int		upload_width, upload_height;	// after power of two and picmip
	int		registration_sequence;		// 0 = free
	struct msurface_s	*texturechain;	// for sort-by-texture world drawing
	int		texnum;						// gl texture binding
	float	sl, tl, sh, th;				// 0,0 - 1,1 unless part of the scrap
	qboolean	scrap;
	qboolean	has_alpha;

	qboolean paletted;
} image_t;

#define	TEXNUM_LIGHTMAPS	1024
#define	TEXNUM_SCRAPS		1152
#define	TEXNUM_IMAGES		1153

#define		MAX_GLTEXTURES	1024

// Truevision Targa header
typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;

// Definitions for image types
#define TGA_Null		0	// no image data
#define TGA_Map			1	// Uncompressed, color-mapped images
#define TGA_RGB			2	// Uncompressed, RGB images
#define TGA_Mono		3	// Uncompressed, black and white images
#define TGA_RLEMap		9	// Runlength encoded color-mapped images
#define TGA_RLERGB		10	// Runlength encoded RGB images
#define TGA_RLEMono		11	// Compressed, black and white images
#define TGA_CompMap		32	// Compressed color-mapped data, using Huffman, Delta, and runlength encoding
#define TGA_CompMap4	33	// Compressed color-mapped data, using Huffman, Delta, and runlength encoding. 4-pass quadtree-type process

// Definitions for interleave flag
#define TGA_IL_None		0	// non-interleaved
#define TGA_IL_Two		1	// two-way (even/odd) interleaving
#define TGA_IL_Four		2	// four way interleaving
#define TGA_IL_Reserved	3	// reserved

// Definitions for origin flag
#define TGA_O_UPPER		0	// Origin in lower left-hand corner
#define TGA_O_LOWER		1	// Origin in upper left-hand corner

#define MAXCOLORS 16384
// Truevision Targa header

//===================================================================

typedef enum rserr_e
{
	rserr_ok,

	rserr_invalid_fullscreen,
	rserr_invalid_mode,

	rserr_unknown
} rserr_t;

#include "dx_model.h"

void GL_BeginRendering (int *x, int *y, int *width, int *height);
void GL_EndRendering (void);

void GL_SetDefaultState( void );
void GL_UpdateSwapInterval( void );

extern	float	gldepthmin, gldepthmax;

typedef struct glvert_s
{
	float	x, y, z;
	float	s, t;
	float	r, g, b;
} glvert_t;


#define	MAX_LBM_HEIGHT		480

#define BACKFACE_EPSILON	0.01


//====================================================

extern	image_t		gltextures[MAX_GLTEXTURES];
extern	int			numgltextures;


extern	image_t		*r_notexture;
extern	image_t		*r_particletexture;
//--==OBSIDIAN UPDATE==--
//Based on work by Carbon14
extern  image_t     *r_shelltexture[5];     // c14 added shell texture

extern	entity_t	*currententity;
extern	model_t		*currentmodel;
extern	int			r_visframecount;
extern	int			r_framecount;
extern	cplane_t	frustum[4];
extern	int			c_brush_polys, c_alias_polys;

extern	int			gl_filter_min, gl_filter_max;

//
// view origin
//
extern	vec3_t	vup;
extern	vec3_t	vpn;
extern	vec3_t	vright;
extern	vec3_t	r_origin;

//
// screen size info
//
extern	refdef_t	r_newrefdef;
extern	int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern	cvar_t	*r_norefresh;
extern	cvar_t	*r_lefthand;
extern	cvar_t	*r_drawentities;
extern	cvar_t	*r_drawworld;
extern	cvar_t	*r_speeds;
extern	cvar_t	*r_fullbright;
extern	cvar_t	*r_novis;
extern	cvar_t	*r_nocull;
extern	cvar_t	*r_lerpmodels;

extern	cvar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level

extern cvar_t	*gl_vertex_arrays;

extern cvar_t	*gl_ext_swapinterval;
extern cvar_t	*gl_ext_palettedtexture;
extern cvar_t	*gl_ext_multitexture;
extern cvar_t	*gl_ext_pointparameters;
extern cvar_t	*gl_ext_compiled_vertex_array;

extern cvar_t	*gl_particle_min_size;
extern cvar_t	*gl_particle_max_size;
extern cvar_t	*gl_particle_size;
extern cvar_t	*gl_particle_att_a;
extern cvar_t	*gl_particle_att_b;
extern cvar_t	*gl_particle_att_c;

extern	cvar_t	*gl_nosubimage;
extern	cvar_t	*gl_bitdepth;
extern	cvar_t	*gl_mode;
extern	cvar_t	*gl_log;
extern	cvar_t	*gl_lightmap;
extern	cvar_t	*gl_shadows;
extern	cvar_t	*gl_dynamic;
extern  cvar_t  *gl_monolightmap;
extern	cvar_t	*gl_nobind;
extern	cvar_t	*gl_round_down;
extern	cvar_t	*gl_picmip;
extern	cvar_t	*gl_skymip;
extern	cvar_t	*gl_showtris;
extern	cvar_t	*gl_finish;
extern	cvar_t	*gl_ztrick;
extern	cvar_t	*gl_clear;
extern	cvar_t	*gl_cull;
extern	cvar_t	*gl_poly;
extern	cvar_t	*gl_texsort;
extern	cvar_t	*gl_polyblend;
extern	cvar_t	*gl_flashblend;
extern	cvar_t	*gl_lightmaptype;
extern	cvar_t	*gl_modulate;
extern	cvar_t	*gl_playermip;
extern	cvar_t	*gl_drawbuffer;
extern	cvar_t	*gl_3dlabs_broken;
extern  cvar_t  *gl_driver;
extern	cvar_t	*gl_swapinterval;
extern	cvar_t	*gl_texturemode;
extern	cvar_t	*gl_texturealphamode;
extern	cvar_t	*gl_texturesolidmode;
extern  cvar_t  *gl_saturatelighting;
extern  cvar_t  *gl_lockpvs;

extern	cvar_t	*vid_fullscreen;
extern	cvar_t	*vid_gamma;

extern	cvar_t	*intensity;

// NeVo - 3.22 cvars
extern	cvar_t	*r_2dalpha;
extern	cvar_t	*r_fluidwaves;
extern	cvar_t	*r_drawbboxes;
extern	cvar_t	*r_skyboxsize;
extern	cvar_t	*r_skyfog;
extern	cvar_t	*r_waterfog;
extern	cvar_t	*r_skyboxfog;
extern	cvar_t	*r_dlightcutoff;
extern	cvar_t	*r_stencil;
extern	cvar_t	*r_overbrightbits;
extern	cvar_t	*gl_ext_mtexcombine;	// Vic
extern	cvar_t	*r_wireframe;
extern	cvar_t	*r_wiremodels;
extern	cvar_t	*gl_ext_mipmap;
extern	cvar_t	*gl_ext_anisotropic;
extern	cvar_t	*r_anisotropylevels;
extern	cvar_t	*gl_ext_texture_compression; // Heffo - ARB Texture Compression
extern	cvar_t	*r_jpeg_quality;	// Heffo - JPEG Screenshots
extern	cvar_t	*r_hwgamma;
extern	cvar_t	*r_saturation;
extern	cvar_t	*r_dlight_viewblend;
extern	cvar_t	*r_spriteangle;
extern	cvar_t	*r_drawmenubg;
extern	cvar_t	*r_animmenubg;
extern	cvar_t	*r_shadowoffset;
extern	cvar_t	*r_projectshadows;
extern	cvar_t	*gl_ext_fog_coord;
extern	cvar_t	*r_fogdensity;	// NeVo - hack to get the client's fog level
extern	cvar_t	*r_antialias;
extern	cvar_t	*r_antialiaslevel;

// Obsidian shell hack
static float shell_size;
static float shell_alpha;
static float shell_fade;

// NeVo - fog information
#define FOGMODE_LINEAR	0
#define FOGMODE_EXP		1
#define FOGMODE_EXP2	2
typedef struct fogmode_s
{
	float red;
	float green;
	float blue;
	float alpha;
	float neardist;
	float fardist;
	float fogdensity;
	int   mode;
	qboolean  usecoords;	// Use coordinate fog if possible
} fogmode_t;
extern void R_Fog (fogmode_t fog);
// end fog

extern	int		gl_lightmap_format;
extern	int		gl_solid_format;
extern	int		gl_alpha_format;
extern	int		gl_tex_solid_format;
extern	int		gl_tex_alpha_format;

extern	int		c_visible_lightmaps;
extern	int		c_visible_textures;

extern	float	r_world_matrix[16];

void R_TranslatePlayerSkin (int playernum);
void GL_Bind (int texnum);
void GL_MBind( GLenum target, int texnum );
void GL_TexEnv( GLenum value );
void GL_EnableMultitexture( qboolean enable );
void GL_SelectTexture( GLenum );

void R_LightPoint (vec3_t p, vec3_t color);
void R_PushDlights (void);

//====================================================================

extern	model_t	*r_worldmodel;

extern	unsigned	d_8to24table[256];

extern	int		registration_sequence;


void V_AddBlend (float r, float g, float b, float a, float *v_blend);

int 	R_Init( void *hinstance, void *hWnd );
void	R_Shutdown( void );

void R_RenderView (refdef_t *fd);
void GL_ScreenShot_f (void);
void GL_ScreenShot_JPG_f (void);	// Heffo - JPG screenshots
//void GL_ScreenShot_PNG (void);	// Psychospaz - PNG screenshots
//void LoadPNG(char *filename, byte **pic, int *width, int *height);	// PNG file support
void R_DrawAliasModel (entity_t *e);
void R_DrawBrushModel (entity_t *e);
void R_DrawSpriteModel (entity_t *e);
void R_DrawBeam( entity_t *e );
void R_DrawWorld (void);
void R_RenderDlights (void);
void R_DrawAlphaSurfaces (void);
void R_RenderBrushPoly (msurface_t *fa);
void R_InitParticleTexture (void);
void Draw_InitLocal (void);
void GL_SubdivideSurface (msurface_t *fa);
qboolean R_CullBox (vec3_t mins, vec3_t maxs);
void R_RotateForEntity (entity_t *e);
void R_MarkLeaves (void);

glpoly_t *WaterWarpPolyVerts (glpoly_t *p);
void EmitWaterPolys (msurface_t *fa);
void R_AddSkySurface (msurface_t *fa);
void R_ClearSkyBox (void);
void R_DrawSkyBox (void);
void R_MarkLights (dlight_t *light, int bit, mnode_t *node);

#if 0
short LittleShort (short l);
short BigShort (short l);
int	LittleLong (int l);
float LittleFloat (float f);

char	*va(char *format, ...);
// does a varargs printf into a temp buffer
#endif

void COM_StripExtension (char *in, char *out);

void	Draw_GetPicSize (int *w, int *h, char *name);
void	Draw_Pic (int x, int y, char *name);
void	Draw_StretchPic (int x, int y, int w, int h, char *name);
void	Draw_Char (int x, int y, int c);
void	Draw_TileClear (int x, int y, int w, int h, char *name);
void	Draw_Fill (int x, int y, int w, int h, int c);
void	Draw_FadeScreen (void);
void	Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data);

void	R_BeginFrame( float camera_separation );
void	R_SwapBuffers( int );
void	R_SetPalette ( const unsigned char *palette);

int		Draw_GetPalette (void);

void GL_ResampleTexture (unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight);

struct image_s *R_RegisterSkin (char *name);

void LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height);
image_t *GL_LoadPic (char *name, byte *pic, int width, int height, imagetype_t type, int bits);
image_t	*GL_FindImage (char *name, imagetype_t type);
void	GL_TextureMode( char *string );
void	GL_ImageList_f (void);

void	GL_SetTexturePalette( unsigned palette[256] );

void	GL_InitImages (void);
void	GL_ShutdownImages (void);

void	GL_FreeUnusedImages (void);

void GL_TextureAlphaMode( char *string );
void GL_TextureSolidMode( char *string );

/*
** GL extension emulation functions
*/
void GL_DrawParticles( int n, const particle_t particles[], const unsigned colortable[768] );

/*
** GL config stuff
*/
#define GL_RENDERER_VOODOO		0x00000001
#define GL_RENDERER_VOODOO2   	0x00000002
#define GL_RENDERER_VOODOO_RUSH	0x00000004
#define GL_RENDERER_BANSHEE		0x00000008
#define	GL_RENDERER_3DFX		0x0000000F

#define GL_RENDERER_PCX1		0x00000010
#define GL_RENDERER_PCX2		0x00000020
#define GL_RENDERER_PMX			0x00000040
#define	GL_RENDERER_POWERVR		0x00000070

#define GL_RENDERER_PERMEDIA2	0x00000100
#define GL_RENDERER_GLINT_MX	0x00000200
#define GL_RENDERER_GLINT_TX	0x00000400
#define GL_RENDERER_3DLABS_MISC	0x00000800
#define	GL_RENDERER_3DLABS		0x00000F00

#define GL_RENDERER_REALIZM		0x00001000
#define GL_RENDERER_REALIZM2	0x00002000
#define	GL_RENDERER_INTERGRAPH	0x00003000

#define GL_RENDERER_3DPRO		0x00004000
#define GL_RENDERER_REAL3D		0x00008000
#define GL_RENDERER_RIVA128		0x00010000
#define GL_RENDERER_DYPIC		0x00020000

#define GL_RENDERER_V1000		0x00040000
#define GL_RENDERER_V2100		0x00080000
#define GL_RENDERER_V2200		0x00100000
#define	GL_RENDERER_RENDITION	0x001C0000

#define GL_RENDERER_O2          0x00100000
#define GL_RENDERER_IMPACT      0x00200000
#define GL_RENDERER_RE			0x00400000
#define GL_RENDERER_IR			0x00800000
#define GL_RENDERER_SGI			0x00F00000

#define GL_RENDERER_MCD			0x01000000
#define GL_RENDERER_OTHER		0x80000000

typedef struct dxconfig_s
{
	int         renderer;
	const char *renderer_string;
	const char *vendor_string;
	const char *version_string;
	const char *extensions_string;

	qboolean	allow_cds;
	// Vic - begin
	qboolean 	mtexcombine;
	// Vic - end
	qboolean	glmipmap;		// Gage - SGIS Mipmapping
	qboolean	anisotropic;		// NeVo - anisotropic filtering
	qboolean	texture_compression;	// Heffo - ARB texture compression
	qboolean	coordinatefog;	// NeVo - coordinate fog

	int width, height;

	int vertexmode;
	int displaydepth;
	int zbufferdepth;
	int stencildepth;
	qboolean have_stencil;
	qboolean gammaramp;
	qboolean allowdisplaydepthchange;
} dxconfig_t;

typedef struct dxstate_s
{
	float inverse_intensity;
	qboolean fullscreen;

	int     prev_mode;

	unsigned char *d_16to8table;

	int lightmap_textures;

	int	currenttextures[2];
	int currenttmu;

	unsigned char originalRedGammaTable[256];
	unsigned char originalGreenGammaTable[256];
	unsigned char originalBlueGammaTable[256];

	qboolean fogenabled;	// NeVo - fog is enabled
} dxstate_t;

extern dxconfig_t  dx_config;
extern dxstate_t   dx_state;

/*
====================================================================

IMPORTED FUNCTIONS

====================================================================
*/

extern	refimport_t	ri;


/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void		DXimp_BeginFrame( float camera_separation );
void		DXimp_EndFrame( void );
int 		DXimp_Init( void *hinstance, void *hWnd );
void		DXimp_Shutdown( void );
int     	DXimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen );
void		DXimp_AppActivate( qboolean active );
void		DXimp_EnableLogging( qboolean enable );
void		DXimp_LogNewFrame( void );

#endif // __GL_LOCAL_H__
