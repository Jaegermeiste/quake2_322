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
#include "../client/client.h"
#include "../client/qmenu.h"

#define REF_SOFT	0
#define REF_OPENGL	1
#define REF_3DFX	2
#define REF_POWERVR	3
#define REF_VERITE	4
#define REF_D3D9	5	// NeVo

extern cvar_t *vid_ref;
extern cvar_t *vid_fullscreen;
extern cvar_t *vid_gamma;
extern cvar_t *scr_viewsize;

static cvar_t *gl_mode;
static cvar_t *gl_driver;
static cvar_t *gl_picmip;
static cvar_t *gl_ext_palettedtexture;
static cvar_t *gl_finish;

static cvar_t *sw_mode;
static cvar_t *sw_stipplealpha;

cvar_t	*ui_fogenabled;	// NeVo - this is a hack

extern void M_ForceMenuOff( void );

/*
====================================================================

MENU INTERACTION

====================================================================
*/
#define SOFTWARE_MENU 0
#define OPENGL_MENU   1
#define DIRECT3D_MENU 2		// NeVo
#define MENUTYPES	2

static menuframework_t  s_software_menu;
static menuframework_t	s_opengl_menu;
static menuframework_t	s_direct3d_menu;	// NeVo
static menuframework_t *s_current_menu;
static int				s_current_menu_index;

static menulist_t		s_mode_list[MENUTYPES];
static menulist_t		s_ref_list[MENUTYPES];
static menuslider_t		s_tq_slider;
static menuslider_t		s_screensize_slider[MENUTYPES];
static menuslider_t		s_brightness_slider[MENUTYPES];
static menulist_t  		s_fs_box[MENUTYPES];
static menulist_t  		s_stipple_box;
static menulist_t  		s_paletted_texture_box;
static menulist_t  		s_finish_box;
static menulist_t		s_aspect_ratio_list[MENUTYPES];		// NeVo - aspect ratio support
static menuaction_t		s_apply_action[MENUTYPES];
static menuaction_t		s_cancel_action[MENUTYPES];
static menuaction_t		s_defaults_action[MENUTYPES];

// NeVo - v3non requested these
static menulist_t		s_overbrights_list;
static menulist_t		s_fogtoggle_list;
static menulist_t		s_cameramode_list[MENUTYPES];

static void DriverCallback( void *unused )
{
	s_ref_list[!s_current_menu_index].curvalue = s_ref_list[s_current_menu_index].curvalue;

	if ( s_ref_list[s_current_menu_index].curvalue == SOFTWARE_MENU )
	{
		s_current_menu = &s_software_menu;
		s_current_menu_index = SOFTWARE_MENU;
	}
	else if ( s_ref_list[s_current_menu_index].curvalue == OPENGL_MENU )
	{
		s_current_menu = &s_opengl_menu;
		s_current_menu_index = OPENGL_MENU;
	}
	else if ( s_ref_list[s_current_menu_index].curvalue == DIRECT3D_MENU )	// NeVo
	{
		s_current_menu = &s_direct3d_menu;
		s_current_menu_index = DIRECT3D_MENU;
	}

}

static void ScreenSizeCallback( void *s )
{
	menuslider_t *slider = ( menuslider_t * ) s;

	Cvar_SetValue( "viewsize", slider->curvalue * 10 );
}

// NeVo - obsolete
static void AspectRatioCallback( void *s )
{
	menulist_t *list = ( menulist_t * ) s;

	Cvar_SetValue( "vid_widescreen", list->curvalue );
}

static void BrightnessCallback( void *s )
{
	menuslider_t *slider = ( menuslider_t * ) s;
	float gamma;

	if ( s_current_menu_index == SOFTWARE_MENU )
		s_brightness_slider[1].curvalue = s_brightness_slider[0].curvalue;
	else
		s_brightness_slider[0].curvalue = s_brightness_slider[1].curvalue;

	gamma = ( 0.8 - ( slider->curvalue/10.0 - 0.5 ) ) + 0.5;
	Cvar_SetValue( "vid_gamma", gamma );
}

static void ResetDefaults( void *unused )
{
	VID_MenuInit();
}

static void ApplyChanges( void *unused )
{
	float gamma;

	/*
	** make values consistent
	*/
	s_fs_box[!s_current_menu_index].curvalue = s_fs_box[s_current_menu_index].curvalue;
	s_brightness_slider[!s_current_menu_index].curvalue = s_brightness_slider[s_current_menu_index].curvalue;
	s_ref_list[!s_current_menu_index].curvalue = s_ref_list[s_current_menu_index].curvalue;

	/*
	** invert sense so greater = brighter, and scale to a range of 0.5 to 1.3
	*/
	gamma = ( 0.8 - ( s_brightness_slider[s_current_menu_index].curvalue/10.0 - 0.5 ) ) + 0.5;

	Cvar_SetValue( "vid_gamma", gamma );
	Cvar_SetValue( "sw_stipplealpha", s_stipple_box.curvalue );
	Cvar_SetValue( "gl_picmip", 3 - s_tq_slider.curvalue );
	Cvar_SetValue( "vid_fullscreen", s_fs_box[s_current_menu_index].curvalue );
	Cvar_SetValue( "gl_ext_palettedtexture", s_paletted_texture_box.curvalue );
	Cvar_SetValue( "gl_finish", s_finish_box.curvalue );
	Cvar_SetValue( "sw_mode", s_mode_list[SOFTWARE_MENU].curvalue );
	Cvar_SetValue( "gl_mode", s_mode_list[OPENGL_MENU].curvalue );
	Cvar_SetValue( "vid_widescreen", s_aspect_ratio_list[s_current_menu_index].curvalue );	// NeVo - aspect ratio support

	// NeVo - fog toggle, overbright mode, and camera modes
	Cvar_SetValue ("ui_fogenabled", s_fogtoggle_list.curvalue);
	// NeVo - giant fucking hack
	if (ui_fogenabled->value)
	{
		Cvar_SetValue ("r_skyfog", 1.0);
		Cvar_SetValue ("r_waterfog", 1.0);
	}
	else
	{
		Cvar_SetValue ("r_skyfog", 0.0);
		Cvar_SetValue ("r_waterfog", 0.0);
	}
	Cvar_SetValue ("r_overbrightbits", s_overbrights_list.curvalue*2.0);
	Cvar_SetValue ("cl_thirdPerson", s_cameramode_list[s_current_menu_index].curvalue);

	switch ( s_ref_list[s_current_menu_index].curvalue )
	{
	case REF_SOFT:
		Cvar_Set( "vid_ref", "soft" );
		break;
	case REF_OPENGL:
		Cvar_Set( "vid_ref", "gl" );
		Cvar_Set( "gl_driver", "opengl32" );
		break;
	case REF_3DFX:
		Cvar_Set( "vid_ref", "gl" );
		Cvar_Set( "gl_driver", "3dfxgl" );
		break;
	case REF_POWERVR:
		Cvar_Set( "vid_ref", "gl" );
		Cvar_Set( "gl_driver", "pvrgl" );
		break;
	case REF_VERITE:
		Cvar_Set( "vid_ref", "gl" );
		Cvar_Set( "gl_driver", "veritegl" );
		break;
	case REF_D3D9:		// NeVo
		Cvar_Set( "vid_ref", "d3d9" );
		break;
	default:			// NeVo - default to software
		Cvar_Set( "vid_ref", "soft" );
		break;
	}

	/*
	** update appropriate stuff if we're running OpenGL and gamma
	** has been modified
	*/
	if ( stricmp( vid_ref->string, "gl" ) == 0 )
	{
		if ( vid_gamma->modified )
		{
			vid_ref->modified = 1;
			if ( stricmp( gl_driver->string, "3dfxgl" ) == 0 )
			{
				char envbuffer[1024];
				float g;

				vid_ref->modified = 1;

				g = 2.00 * ( 0.8 - ( vid_gamma->value - 0.5 ) ) + 1.0F;
				Com_sprintf( envbuffer, sizeof(envbuffer), "SSTV2_GAMMA=%f", g );
				putenv( envbuffer );
				Com_sprintf( envbuffer, sizeof(envbuffer), "SST_GAMMA=%f", g );
				putenv( envbuffer );

				vid_gamma->modified = 0;
			}
		}

		if ( gl_driver->modified )
			vid_ref->modified = 1;
	}

	//M_ForceMenuOff();
	VID_MenuInit();
}

/*
** VID_MenuInit
*/
void VID_MenuInit( void )
{
	static const char *resolutions[] = 
	{
		"[320 240  ]",
		"[400 300  ]",
		"[512 384  ]",
		"[640 480  ]",
		"[800 600  ]",
		"[960 720  ]",
		"[1024 480 ]",		// c14 new mode - Sony PictureBook C1VFK support
		"[1024 768 ]",
		"[1152 864 ]",
		"[1280 768 ]",		// David M. Pochron
		"[1280 960 ]",
		"[1280 1024]",		// TheInvisible
		"[1600 1200]",
		"[2048 1536]",
		// NeVo - World Video Resolutions
		"[Std. NTSC 601   ]",		// Std NTSC 601		720x540
		"[Std. NTSC DV/DVD]",		// Std NTSC DV/DVD	720x534
		"[Wide NTSC 601   ]",		// Wide NTSC 601	864x486
		"[Wide NTSC DV/DVD]",		// Wide NTSC DV/DVD	864x480
		"[Std PAL         ]",		// Std PAL			768x576
		"[Wide PAL        ]",		// Wide PAL			1024x576
		"[HDTV 720P       ]",		// HDTV 720P		1280x720
		"[HDTV 1080i      ]",		// HDTV 1080i		1920x1080
		0
	};
	static const char *refs[] =
	{
		"[Software      ]",
		"[System OpenGL ]",		// NeVo
		"[3Dfx OpenGL   ]",
		"[PowerVR OpenGL]",
		"[Rendition OpenGL]",	// NeVo
//		"[Direct 3D 9.0	]",		// NeVo
		0
	};
	static const char *yesno_names[] =
	{
		"No",
		"Yes",
		0
	};
	static const char *hilo_names[] =
	{
		"Low",
		"High",
		0
	};
	// NeVo - aspect ratio support
	static const char *aspect_ratios[] =
	{
		"[1.33:1 - Default]",
		"[1.78:1 - HDTV   ]",
		"[2.35:1 - Cinema ]",
		0
	};

	// NeVo - overbrights list
	static const char *overbright_modes[] =
	{
		"Off",
		"Low",
		"High",
		0
	};

	// NeVo - camera modes
	static const char *cam_modes[] =
	{
		"First Person",
		"Third Person",
		0
	};

	int i;

	ui_fogenabled = Cvar_Get ("ui_fogenabled", "1", CVAR_ARCHIVE);	// NeVo - this is a goddamn hack

	if ( !gl_driver )
		gl_driver = Cvar_Get( "gl_driver", "opengl32", 0 );
	if ( !gl_picmip )
		gl_picmip = Cvar_Get( "gl_picmip", "0", 0 );
	if ( !gl_mode )
		gl_mode = Cvar_Get( "gl_mode", "3", 0 );
	if ( !sw_mode )
		sw_mode = Cvar_Get( "sw_mode", "0", 0 );
	if ( !gl_ext_palettedtexture )
		gl_ext_palettedtexture = Cvar_Get( "gl_ext_palettedtexture", "1", CVAR_ARCHIVE );
	if ( !gl_finish )
		gl_finish = Cvar_Get( "gl_finish", "0", CVAR_ARCHIVE );

	if ( !sw_stipplealpha )
		sw_stipplealpha = Cvar_Get( "sw_stipplealpha", "0", CVAR_ARCHIVE );

	s_mode_list[SOFTWARE_MENU].curvalue = sw_mode->value;
	s_mode_list[OPENGL_MENU].curvalue = gl_mode->value;

	if ( !scr_viewsize )
		scr_viewsize = Cvar_Get ("viewsize", "100", CVAR_ARCHIVE);

	s_screensize_slider[SOFTWARE_MENU].curvalue = scr_viewsize->value/10;
	s_screensize_slider[OPENGL_MENU].curvalue = scr_viewsize->value/10;

	if ( strcmp( vid_ref->string, "soft" ) == 0 )
	{
		s_current_menu_index = SOFTWARE_MENU;
		s_ref_list[0].curvalue = s_ref_list[1].curvalue = REF_SOFT;
	}
	else if ( strcmp( vid_ref->string, "gl" ) == 0 )
	{
		s_current_menu_index = OPENGL_MENU;
		if ( strcmp( gl_driver->string, "3dfxgl" ) == 0 )
			s_ref_list[s_current_menu_index].curvalue = REF_3DFX;
		else if ( strcmp( gl_driver->string, "pvrgl" ) == 0 )
			s_ref_list[s_current_menu_index].curvalue = REF_POWERVR;
		else if ( strcmp( gl_driver->string, "opengl32" ) == 0 )
			s_ref_list[s_current_menu_index].curvalue = REF_OPENGL;
		else if ( strcmp( gl_driver->string, "veritegl" ) == 0 )		// NeVo
			s_ref_list[s_current_menu_index].curvalue = REF_VERITE;
		else
			s_ref_list[s_current_menu_index].curvalue = REF_OPENGL;
	}

	s_software_menu.x		= viddef.width * 0.50;
	s_software_menu.y		= (viddef.height * 0.50) - 58;
	s_software_menu.nitems	= 0;
	s_opengl_menu.x			= viddef.width * 0.50;
	s_opengl_menu.y			= (viddef.height * 0.50) - 58;
	s_opengl_menu.nitems	= 0;

	for ( i = 0; i < 2; i++ )
	{
		s_ref_list[i].generic.type = MTYPE_SPINCONTROL;
		s_ref_list[i].generic.name = "Driver";
		s_ref_list[i].generic.x = 0;
		s_ref_list[i].generic.y = 0;
		s_ref_list[i].generic.callback = DriverCallback;
		s_ref_list[i].itemnames = refs;

		s_mode_list[i].generic.type = MTYPE_SPINCONTROL;
		s_mode_list[i].generic.name = "Video Mode";
		s_mode_list[i].generic.x = 0;
		s_mode_list[i].generic.y = 10;
		s_mode_list[i].itemnames = resolutions;

		s_screensize_slider[i].generic.type	= MTYPE_SLIDER;
		s_screensize_slider[i].generic.x		= 0;
		s_screensize_slider[i].generic.y		= 20;
		s_screensize_slider[i].generic.name	= "Screen Size";
		s_screensize_slider[i].minvalue = 3;
		s_screensize_slider[i].maxvalue = 12;
		s_screensize_slider[i].generic.callback = ScreenSizeCallback;

		s_brightness_slider[i].generic.type	= MTYPE_SLIDER;
		s_brightness_slider[i].generic.x	= 0;
		s_brightness_slider[i].generic.y	= 30;
		s_brightness_slider[i].generic.name	= "Brightness";
		s_brightness_slider[i].generic.callback = BrightnessCallback;
		s_brightness_slider[i].minvalue = 5;
		s_brightness_slider[i].maxvalue = 13;
		s_brightness_slider[i].curvalue = ( 1.3 - vid_gamma->value + 0.5 ) * 10;

		s_fs_box[i].generic.type = MTYPE_SPINCONTROL;
		s_fs_box[i].generic.x	= 0;
		s_fs_box[i].generic.y	= 40;
		s_fs_box[i].generic.name	= "Fullscreen";
		s_fs_box[i].itemnames = yesno_names;
		s_fs_box[i].curvalue = vid_fullscreen->value;

		// NeVo - camera modes
		s_cameramode_list[i].generic.type = MTYPE_SPINCONTROL;
		s_cameramode_list[i].generic.x	= 0;
		s_cameramode_list[i].generic.y	= 50;
		s_cameramode_list[i].generic.name	= "Camera Mode";
		s_cameramode_list[i].itemnames = cam_modes;
		s_cameramode_list[i].curvalue = Cvar_VariableValue("cl_thirdPerson->value");

		// NeVo - aspect ratio support
		s_aspect_ratio_list[i].generic.type = MTYPE_SPINCONTROL;
		s_aspect_ratio_list[i].generic.x	= 0;
		s_aspect_ratio_list[i].generic.y	= 110;
		s_aspect_ratio_list[i].generic.name = "Aspect Ratio";
		s_aspect_ratio_list[i].itemnames = aspect_ratios;
		s_aspect_ratio_list[i].curvalue	= Cvar_VariableValue("vid_widescreen");
		//s_aspect_ratio_list[i].generic.callback = AspectRatioCallback;	// NeVo - obsolete

		s_defaults_action[i].generic.type = MTYPE_ACTION;
		s_defaults_action[i].generic.name = "Reset to Defaults";
		s_defaults_action[i].generic.x    = 0;
		s_defaults_action[i].generic.y    = 120;
		s_defaults_action[i].generic.callback = ResetDefaults;

		s_apply_action[i].generic.type = MTYPE_ACTION;
		s_apply_action[i].generic.name = "Apply Changes";
		s_apply_action[i].generic.x    = 0;
		s_apply_action[i].generic.y    = 130;
		s_apply_action[i].generic.callback = ApplyChanges;

		s_cancel_action[i].generic.type = MTYPE_ACTION;
		s_cancel_action[i].generic.name = "Options Menu";
		s_cancel_action[i].generic.x    = 0;
		s_cancel_action[i].generic.y    = 140;
		s_cancel_action[i].generic.callback = M_Menu_Options_f;
	}

	s_stipple_box.generic.type = MTYPE_SPINCONTROL;
	s_stipple_box.generic.x	= 0;
	s_stipple_box.generic.y	= 60;
	s_stipple_box.generic.name	= "Dither Alpha";
	s_stipple_box.curvalue = sw_stipplealpha->value;
	s_stipple_box.itemnames = yesno_names;

	s_tq_slider.generic.type	= MTYPE_SLIDER;
	s_tq_slider.generic.x		= 0;
	s_tq_slider.generic.y		= 60;
	s_tq_slider.generic.name	= "Texture Quality";
	s_tq_slider.minvalue = 0;
	s_tq_slider.maxvalue = 3;
	s_tq_slider.curvalue = 3-gl_picmip->value;

	s_paletted_texture_box.generic.type = MTYPE_SPINCONTROL;
	s_paletted_texture_box.generic.x	= 0;
	s_paletted_texture_box.generic.y	= 70;
	s_paletted_texture_box.generic.name	= "Texture Resolution";
	s_paletted_texture_box.itemnames = hilo_names;
	s_paletted_texture_box.curvalue = gl_ext_palettedtexture->value;

	s_finish_box.generic.type = MTYPE_SPINCONTROL;
	s_finish_box.generic.x	= 0;
	s_finish_box.generic.y	= 80;
	s_finish_box.generic.name	= "Sync Every Frame";
	s_finish_box.curvalue = gl_finish->value;
	s_finish_box.itemnames = yesno_names;

	// Overbrights mode
	s_overbrights_list.generic.type = MTYPE_SPINCONTROL;
	s_overbrights_list.generic.x	= 0;
	s_overbrights_list.generic.y	= 90;
	s_overbrights_list.generic.name	= "Overbrights";
	s_overbrights_list.curvalue = Cvar_VariableValue("r_overbrightbits")*0.5;
	s_overbrights_list.itemnames = overbright_modes;

	// Fog mode
	s_fogtoggle_list.generic.type = MTYPE_SPINCONTROL;
	s_fogtoggle_list.generic.x	= 0;
	s_fogtoggle_list.generic.y	= 100;
	s_fogtoggle_list.generic.name	= "Fog Enabled";
	s_fogtoggle_list.curvalue = ui_fogenabled->value;
	s_fogtoggle_list.itemnames = yesno_names;

	Menu_AddItem( &s_software_menu, ( void * ) &s_ref_list[SOFTWARE_MENU] );
	Menu_AddItem( &s_software_menu, ( void * ) &s_mode_list[SOFTWARE_MENU] );
	Menu_AddItem( &s_software_menu, ( void * ) &s_screensize_slider[SOFTWARE_MENU] );
	Menu_AddItem( &s_software_menu, ( void * ) &s_brightness_slider[SOFTWARE_MENU] );
	Menu_AddItem( &s_software_menu, ( void * ) &s_fs_box[SOFTWARE_MENU] );
	Menu_AddItem( &s_software_menu, ( void * ) &s_stipple_box );

	Menu_AddItem( &s_opengl_menu, ( void * ) &s_ref_list[OPENGL_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_mode_list[OPENGL_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_screensize_slider[OPENGL_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_brightness_slider[OPENGL_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_fs_box[OPENGL_MENU] );

	// NeVo - camera modes
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_cameramode_list[OPENGL_MENU] );
	Menu_AddItem( &s_software_menu, ( void * ) &s_cameramode_list[SOFTWARE_MENU] );

	Menu_AddItem( &s_opengl_menu, ( void * ) &s_tq_slider );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_paletted_texture_box );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_finish_box );

	// NeVo - fog and overbrights
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_overbrights_list );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_fogtoggle_list );

	// NeVo - aspect ratio support
	Menu_AddItem( &s_software_menu, ( void * ) &s_aspect_ratio_list[SOFTWARE_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_aspect_ratio_list[OPENGL_MENU] );

	Menu_AddItem( &s_software_menu, ( void * ) &s_defaults_action[SOFTWARE_MENU] );
	Menu_AddItem( &s_software_menu, ( void * ) &s_apply_action[SOFTWARE_MENU] );
	Menu_AddItem( &s_software_menu, ( void * ) &s_cancel_action[SOFTWARE_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_defaults_action[OPENGL_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_apply_action[OPENGL_MENU] );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_cancel_action[OPENGL_MENU] );

	Menu_Center( &s_software_menu );
	Menu_Center( &s_opengl_menu );
	s_opengl_menu.x -= 8;
	s_software_menu.x -= 8;
}

/*
================
VID_MenuDraw
================
*/
void VID_MenuDraw (void)
{
	int w, h;

	if ( s_current_menu_index == 0 )
		s_current_menu = &s_software_menu;
	else
		s_current_menu = &s_opengl_menu;

	s_current_menu->x		= viddef.width * 0.50;
	s_current_menu->y		= (viddef.height * 0.50) - 58;

	/*
	** move cursor to a reasonable starting position
	*/
	Menu_AdjustCursor( s_current_menu, 1 );

	/*
	** draw the menu
	*/
	Menu_Draw( s_current_menu );

	/*
	** draw the banner
	*/
	re.DrawGetPicSize( &w, &h, "m_banner_video" );
	re.DrawPic( viddef.width / 2 - w / 2, viddef.height /2 - 110, "m_banner_video" );
}

/*
================
VID_MenuKey
================
*/
extern void M_PopMenu (void);
const char *VID_MenuKey( int key )
{
	menuframework_t *m = s_current_menu;
	static const char *insound = "misc/menu1.wav";
	static const char *movesound = "misc/menu2.wav";	// NeVo
	static const char *exitsound = "misc/menu3.wav";	// NeVo
	const char *sound = NULL;

	switch ( key )
	{
	case K_ESCAPE:
		M_PopMenu();	// NeVo
		return exitsound;	// NeVo
		break;
	case K_KP_UPARROW:
	case K_UPARROW:
		m->cursor--;
		Menu_AdjustCursor( m, -1 );
		sound = movesound;
		break;
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		m->cursor++;
		Menu_AdjustCursor( m, 1 );
		sound = movesound;
		break;
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
		Menu_SlideItem( m, -1 );
		sound = movesound;
		break;
	case K_KP_RIGHTARROW:
	case K_RIGHTARROW:
		Menu_SlideItem( m, 1 );
		sound = movesound;
		break;
	case K_KP_ENTER:
	case K_ENTER:
		if ( !Menu_SelectItem( m ) )
			ApplyChanges( NULL );
		sound = movesound;
		break;
	}

	return sound;
}


