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
#ifndef _SOUND_H_
#define _SOUND_H_

struct sfx_s;

extern cvar_t	*s_volume;
extern cvar_t	*s_nosound;
extern cvar_t	*s_loadas8bit;
extern cvar_t	*s_khz;
extern cvar_t	*s_show;
extern cvar_t	*s_mixahead;
extern cvar_t	*s_testsound;
extern cvar_t	*s_primary;
extern cvar_t	*s_soundsystem;
extern cvar_t	*s_soundquality;
extern cvar_t	*s_balance;
extern cvar_t	*s_musicvolume;	// Vic
extern cvar_t	*s_swapstereo;	// Vic

void S_Init (void);
void S_Shutdown (void);

// if origin is NULL, the sound will be dynamically sourced from the entity
void S_StartSound (vec3_t origin, int entnum, int entchannel, struct sfx_s *sfx, float fvol,  float attenuation, float timeofs);
void S_StartLocalSound (char *s);

void S_RawSamples (int samples, int rate, int width, int channels, byte *data);

void S_StopAllSounds(void);
void S_Update (vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up);

void S_Activate (int active);

void S_BeginRegistration (void);
struct sfx_s *S_RegisterSound (char *sample);
void S_EndRegistration (void);

struct sfx_s *S_FindName (char *name, int create);

/* NeVo - WinAmp Integration */
void S_WinAmp_Init (void);
void S_WinAmp_Shutdown (void);
void S_WinAmp_Frame (void);

// the sound code makes callbacks to the client for entitiy position
// information, so entities can be dynamically re-spatialized
void CL_GetEntitySoundOrigin (int entnum, vec3_t org);

#endif	// _SOUND_H_
