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

#include "client.h"

/*
camplugs.c

All plugins must return void and accept void parameters.
All necessary information should be accessible through
the camdevice.Get function(s) or directly through the cameras[]
array. To install a plugin, it must be written respecting the
above parameters, and then in CamPlugins_RegisterPlugins, the
appropriate call must be changed from CamPlugins_Void to
CamPlugins_YourPlugin. All plugin names should begin with
CamPlugins_Xxxxx, for continuity. Animations should begin
with CamPlugins_AnimXxxxx, for continuity. The camplugins_t
structure is provided for convenience:

// Our plugins interface
typedef struct camplugins_s
{
	void *Fisheye		(void);
	void *InkOnWhite	(void);
	void *InkOnPaper	(void);
	void *Blueprint		(void);
	void *NPR			(void);
	void *InvNPR		(void);
	void *CelShade		(void);
	void *Grayscale		(void);
	void *DepthOfField	(void);
	void *LensFlare		(void);
	void *WireFrame		(void);

	// Animations
	void *AnimDeath		(void);
	void *AnimFalling	(void);
	void *AnimSpawn		(void);
} camplugins_t;
*/

// Useful for plugins
extern	camstate_t	*cameras[MAX_CAMERAS];
extern void CamEngine_SetAngularPosition (int cam, float yaw, float pitch, float roll);

// Function Prototypes //
void CamPlugins_RegisterPlugins (void);
void CamPlugins_ClearSettings (void);
void CamPlugins_Void (void);
void CamPlugins_AnimDeath (void);
void CamPlugins_WireFrame (void);

// Wireframe Plugin
cvar_t	*r_wireframe;
cvar_t	*r_wiremodel;

/*
==========================
CamPlugins_RegisterPlugins

Register the plugin functions with the system
==========================
*/
void CamPlugins_RegisterPlugins (void)
{
	// Effects
	camplugins.Blueprint	=	CamPlugins_Void;
	camplugins.CelShade		=	CamPlugins_Void;
	camplugins.DepthOfField	=	CamPlugins_Void;
	camplugins.Fisheye		=	CamPlugins_Void;
	camplugins.Grayscale	=	CamPlugins_Void;
	camplugins.InkOnPaper	=	CamPlugins_Void;
	camplugins.InkOnWhite	=	CamPlugins_Void;
	camplugins.InvNPR		=	CamPlugins_Void;
	camplugins.LensFlare	=	CamPlugins_Void;
	camplugins.NPR			=	CamPlugins_Void;
	camplugins.WireFrame	=	CamPlugins_WireFrame;

	// Animations
	camplugins.AnimDeath	=	CamPlugins_AnimDeath;
	camplugins.AnimFalling	=	CamPlugins_Void;
	camplugins.AnimSpawn	=	CamPlugins_Void;

	// Clear Settings
	camplugins.Clear		=	CamPlugins_ClearSettings;

	// Individual Plugin Data

		// Wireframe
		r_wireframe = Cvar_Get ("r_wireframe", "0", 0);
		r_wiremodel = Cvar_Get ("r_wiremodel", "0", 0);
}

/*
========================
CamPlugins_ClearSettings

Turn effects off
========================
*/
void CamPlugins_ClearSettings (void)
{
	// Turn off wireframe
	r_wireframe->value = 0;
	r_wiremodel->value = 0;
}

/*
===============
CamPlugins_Void

Empty function
===============
*/
void CamPlugins_Void (void)
{
	CamPlugins_ClearSettings ();
	return;
}

/*
====================
CamPlugins_AnimDeath

Death Animation (Similar to Deus Ex
====================
*/
void CamPlugins_AnimDeath (void)
{
	int cam = camdevice.GetCurrentCam();
	camdevice.LookAt(cam, cl.refdef.vieworg);
	camdevice.ToChaseCam();
	CamEngine_SetAngularPosition(cam, cameras[cam]->angpos[YAW]+1, cameras[cam]->angpos[PITCH], cameras[cam]->angpos[ROLL]);
}

/*
====================
CamPlugins_WireFrame

Draw in wireframe
====================
*/
void CamPlugins_WireFrame (void)
{
	CamPlugins_ClearSettings ();
	r_wireframe->value = 1;
	r_wiremodel->value = 1;
	return;
}