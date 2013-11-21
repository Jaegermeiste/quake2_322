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

//=============================================================================
/* Sound Menu */

// NeVo - This is all the structures and general layout of our sound menu
static menuframework_t	s_sound_menu;

static menulist_t		s_sound_system_list;
static menulist_t		s_sound_quality_list;
static menulist_t		s_sound_bitdepth_list;

static menulist_t		s_sound_cdmusic_box;
static menulist_t		s_sound_cdshuffle_box;

static menuslider_t		s_sound_sfxvolume_slider;
static menuslider_t		s_sound_musicvolume_slider;

static menuslider_t		s_sound_balance_slider;	// In terms of % right
static menulist_t		s_sound_swapstereo_box;

static menuaction_t		s_sound_defaults_action;
static menuaction_t		s_sound_optionsmenu_action;

static void SoundSetMenuItemValues( void )
{
	s_sound_system_list.curvalue		= s_soundsystem->value;
	s_sound_quality_list.curvalue		= s_soundquality->value;
	s_sound_bitdepth_list.curvalue		= !s_loadas8bit->value;

	s_sound_sfxvolume_slider.curvalue	= s_volume->value * 10;
	s_sound_musicvolume_slider.curvalue	= s_musicvolume->value * 10;
	s_sound_balance_slider.curvalue		= s_balance->value;
	s_sound_swapstereo_box.curvalue		= s_swapstereo->value;

	s_sound_cdmusic_box.curvalue 		= !cd_nocd->value;
	s_sound_cdshuffle_box.curvalue		= cd_shuffle->value;
}

static void UpdateSoundParamsFunc ( void *unused )
{
	Cvar_SetValue ( "s_volume", s_sound_sfxvolume_slider.curvalue / 10 );
	Cvar_SetValue ( "s_musicvolume", s_sound_musicvolume_slider.curvalue / 10 );
	Cvar_SetValue ( "s_balance", s_sound_balance_slider.curvalue);
	Cvar_SetValue ( "s_swapstereo", s_sound_swapstereo_box.curvalue / 10 );
}

static void UpdateCDMusicFunc( void *unused )
{
	Cvar_SetValue ( "cd_nocd", !s_sound_cdmusic_box.curvalue );
	Cvar_SetValue ( "cd_shuffle", s_sound_cdshuffle_box.curvalue );

	if (s_sound_cdmusic_box.curvalue)
	{
	  CDAudio_Init();
	  if (s_sound_cdshuffle_box.curvalue)
	    {
	      CDAudio_RandomPlay();
	    }
	  else
	    {
	      CDAudio_Play(atoi(cl.configstrings[CS_CDTRACK]), true);
	    }
	}
	else
	{
	  CDAudio_Stop();
	}
}

static void UpdateSoundSystemFunc( void *unused )
{
	Cvar_SetValue ( "s_soundsystem", s_sound_system_list.curvalue );
	Cvar_SetValue ( "s_soundquality", s_sound_quality_list.curvalue );
	Cvar_SetValue ( "s_loadas8bit", !s_sound_bitdepth_list.curvalue );

	switch ( (int)s_soundquality->value )
	{
	case 0:
		Cvar_SetValue( "s_khz", 8 );
		break;
	case 1:
		Cvar_SetValue( "s_khz", 11 );
		break;
	case 2:
		Cvar_SetValue( "s_khz", 22 );
		break;
	case 3:
		Cvar_SetValue( "s_khz", 44 );
		break;
	case 4:
		Cvar_SetValue( "s_khz", 48 );
		break;
	case 5:
		Cvar_SetValue( "s_khz", 96 );
		break;
	default:
		Cvar_SetValue( "s_khz", 22 );
		break;
	}

	M_DrawTextBox( 8, 120 - 48, 36, 3 );
	M_Print( 16 + 16, 120 - 48 + 8,  "Restarting the sound system. This" );
	M_Print( 16 + 16, 120 - 48 + 16, "could take up to a minute, so" );
	M_Print( 16 + 16, 120 - 48 + 24, "please be patient." );

	// the text box won't show up unless we do a buffer swap
	re.EndFrame();

	UpdateSoundParamsFunc ( NULL );
	UpdateCDMusicFunc ( NULL );

	CL_Snd_Restart_f();
}

static void UpdateSoundFunc( void *unused )
{	
	// This is used to update everything at once
	UpdateSoundParamsFunc ( NULL );
	UpdateSoundSystemFunc ( NULL );
	UpdateCDMusicFunc ( NULL );
}

static void SoundResetDefaultsFunc( void *unused )
{
	Cvar_Set ( "s_soundsystem",		"2" );		// DirectSound8
	Cvar_Set ( "s_soundquality",	"3" );		// 44 kHz
	Cvar_Set ( "s_loadas8bit",		"0" );		// 16 Bit
	Cvar_Set ( "s_volume",			"0.7" );	// 70% volume
	Cvar_Set ( "s_musicvolume",		"0.25" );	// 25% volume
	Cvar_Set ( "s_balance",			"5" );		// centered
	Cvar_Set ( "s_swapstereo",		"0" );		// normal
	Cvar_Set ( "cd_nocd",			"0" );		// cd music enabled
	Cvar_Set ( "cd_shuffle",		"0" );		// cd random play disabled

	SoundSetMenuItemValues();
	UpdateSoundFunc ( NULL );
}

void Sound_MenuInit( void )
{
	static const char *cd_music_items[] =
	{
		"Disabled",
		"Enabled",
		0
	};

	static const char *cd_shuffle_items[] =
	  {
	    "Disabled",
	    "Enabled",
	    0
	  };

	static const char *quality_items[] =
	{
		"Extra Low ( 8 kHz)",
		"Low       (11 kHz)",
		"Normal    (22 kHz)",
		"High      (44 kHz)",
		"Higher    (48 kHz)",
		"Extreme   (96 kHz)",
		0
	};

	static const char *bitdepth_items[] =
	{
		"8 Bit",
		"16 Bit",
		0
	};

	static const char *soundsystem_items[] =
	{
		"No Sound",
		"WaveOut Sound",
		"DirectSound 8",
		//"Open AL",
		0
	};

	static const char *stereoswap_items[] =
	{
		"Normal",
		"Reversed",
		0
	};

	/*
	** configure controls menu and menu items
	*/
	s_sound_menu.x = viddef.width * 0.50;
	s_sound_menu.y = (viddef.height * 0.50) - 58;
	s_sound_menu.nitems = 0;

	s_sound_system_list.generic.type			= MTYPE_SPINCONTROL;
	s_sound_system_list.generic.x				= 0;
	s_sound_system_list.generic.y				= 0;
	s_sound_system_list.generic.name			= "Sound System";
	s_sound_system_list.generic.callback		= UpdateSoundSystemFunc;
	s_sound_system_list.itemnames				= soundsystem_items;
	s_sound_system_list.curvalue				= Cvar_VariableValue("s_soundsystem");

	s_sound_quality_list.generic.type			= MTYPE_SPINCONTROL;
	s_sound_quality_list.generic.x				= 0;
	s_sound_quality_list.generic.y				= 10;
	s_sound_quality_list.generic.name			= "Sample Rate";
	s_sound_quality_list.generic.callback		= UpdateSoundSystemFunc;
	s_sound_quality_list.itemnames				= quality_items;
	s_sound_quality_list.curvalue				= Cvar_VariableValue("s_soundquality");

	s_sound_bitdepth_list.generic.type			= MTYPE_SPINCONTROL;
	s_sound_bitdepth_list.generic.x				= 0;
	s_sound_bitdepth_list.generic.y				= 20;
	s_sound_bitdepth_list.generic.name			= "Sound Quality";
	s_sound_bitdepth_list.generic.callback		= UpdateSoundSystemFunc;
	s_sound_bitdepth_list.itemnames				= bitdepth_items;
	s_sound_bitdepth_list.curvalue				= Cvar_VariableValue("s_loadas8bit");

	s_sound_sfxvolume_slider.generic.type		= MTYPE_SLIDER;
	s_sound_sfxvolume_slider.generic.x			= 0;
	s_sound_sfxvolume_slider.generic.y			= 40;
	s_sound_sfxvolume_slider.generic.name		= "Effects Volume";
	s_sound_sfxvolume_slider.generic.callback	= UpdateSoundParamsFunc;
	s_sound_sfxvolume_slider.minvalue			= 0;
	s_sound_sfxvolume_slider.maxvalue			= 10;
	s_sound_sfxvolume_slider.curvalue			= Cvar_VariableValue( "s_volume" ) * 10;

	s_sound_musicvolume_slider.generic.type		= MTYPE_SLIDER;
	s_sound_musicvolume_slider.generic.x		= 0;
	s_sound_musicvolume_slider.generic.y		= 50;
	s_sound_musicvolume_slider.generic.name		= "Music Volume";
	s_sound_musicvolume_slider.generic.callback	= UpdateSoundParamsFunc;
	s_sound_musicvolume_slider.minvalue			= 0;
	s_sound_musicvolume_slider.maxvalue			= 10;
	s_sound_musicvolume_slider.curvalue			= Cvar_VariableValue( "s_musicvolume" ) * 10;

	s_sound_balance_slider.generic.type			= MTYPE_SLIDER;
	s_sound_balance_slider.generic.x			= 0;
	s_sound_balance_slider.generic.y			= 60;
	s_sound_balance_slider.generic.name			= "Stereo Balance";
	s_sound_balance_slider.generic.callback		= UpdateSoundParamsFunc;
	s_sound_balance_slider.minvalue				= 0;
	s_sound_balance_slider.maxvalue				= 10;
	s_sound_balance_slider.curvalue				= Cvar_VariableValue( "s_balance" );

	s_sound_swapstereo_box.generic.type			= MTYPE_SPINCONTROL;
	s_sound_swapstereo_box.generic.x			= 0;
	s_sound_swapstereo_box.generic.y			= 70;
	s_sound_swapstereo_box.generic.name			= "Swap Stereo";
	s_sound_swapstereo_box.generic.callback		= UpdateSoundParamsFunc;
	s_sound_swapstereo_box.itemnames			= stereoswap_items;
	s_sound_swapstereo_box.curvalue 			= Cvar_VariableValue("s_swapstereo");

	s_sound_cdmusic_box.generic.type			= MTYPE_SPINCONTROL;
	s_sound_cdmusic_box.generic.x				= 0;
	s_sound_cdmusic_box.generic.y				= 90;
	s_sound_cdmusic_box.generic.name			= "CD Music";
	s_sound_cdmusic_box.generic.callback		= UpdateCDMusicFunc;
	s_sound_cdmusic_box.itemnames				= cd_music_items;
	s_sound_cdmusic_box.curvalue 				= !Cvar_VariableValue("cd_nocd");

	s_sound_cdshuffle_box.generic.type			= MTYPE_SPINCONTROL;
	s_sound_cdshuffle_box.generic.x				= 0;
	s_sound_cdshuffle_box.generic.y				= 100;
	s_sound_cdshuffle_box.generic.name			= "CD Shuffle";
	s_sound_cdshuffle_box.generic.callback		= UpdateCDMusicFunc;
	s_sound_cdshuffle_box.itemnames				= cd_shuffle_items;
	s_sound_cdshuffle_box.curvalue				= Cvar_VariableValue("cd_shuffle");

	s_sound_defaults_action.generic.type		= MTYPE_ACTION;
	s_sound_defaults_action.generic.x			= 0;
	s_sound_defaults_action.generic.y			= 120;
	s_sound_defaults_action.generic.name		= "Reset Defaults";
	s_sound_defaults_action.generic.callback	= SoundResetDefaultsFunc;

	s_sound_optionsmenu_action.generic.type		= MTYPE_ACTION;
	s_sound_optionsmenu_action.generic.x		= 0;
	s_sound_optionsmenu_action.generic.y		= 130;
	s_sound_optionsmenu_action.generic.name		= "Options Menu";
	s_sound_optionsmenu_action.generic.callback	= M_Menu_Options_f;

	SoundSetMenuItemValues();

	Menu_AddItem( &s_sound_menu, ( void * ) &s_sound_system_list );
	Menu_AddItem( &s_sound_menu, ( void * ) &s_sound_quality_list );
	Menu_AddItem( &s_sound_menu, ( void * ) &s_sound_bitdepth_list );
	Menu_AddItem( &s_sound_menu, ( void * ) &s_sound_sfxvolume_slider );
	Menu_AddItem( &s_sound_menu, ( void * ) &s_sound_musicvolume_slider );
	Menu_AddItem( &s_sound_menu, ( void * ) &s_sound_balance_slider );
	Menu_AddItem( &s_sound_menu, ( void * ) &s_sound_swapstereo_box );
	Menu_AddItem( &s_sound_menu, ( void * ) &s_sound_cdmusic_box );
	Menu_AddItem( &s_sound_menu, ( void * ) &s_sound_cdshuffle_box );
	Menu_AddItem( &s_sound_menu, ( void * ) &s_sound_defaults_action );
	Menu_AddItem( &s_sound_menu, ( void * ) &s_sound_optionsmenu_action );
}

void Sound_MenuDraw (void)
{
	Menu_AdjustCursor( &s_sound_menu, 1 );
	Menu_Draw( &s_sound_menu );
	M_Banner( "m_banner_options" );
}

const char *Sound_MenuKey( int key )
{
	return Default_MenuKey( &s_sound_menu, key );
}

void M_Menu_Sound_f (void)
{
	Sound_MenuInit();
	M_PushMenu ( Sound_MenuDraw, Sound_MenuKey );
}

