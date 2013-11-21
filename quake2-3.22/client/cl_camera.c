/*
cl_camera.c
 
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

/*
Psychospaz's Chase Camera Code
NeVo - exported the entire original chasecam into this file on 01 Sept, 2002
NeVo - performed massive updates to this code on 28, 29, and 30 Nov, 2002 (yes, thanksgiving)
NeVo - got rid of animations on 31 Nov, 2002
NeVo - more massive updates on 02 and 03 Dec, 2002
NeVo - brought animations back on 03 Dec, 2002
NeVo - performed gigantic update and bugfix from 03 to 07 Dec, 2002.
NeVo - brought in loading of camera definitions thru .cdf files 07 Dec, 2002
NeVo - disabled .cdf files 08 Dec, 2002
NeVo - dampened camera motion 11 Dec, 2002
NeVo - dumped animations again 27 Dec, 2002
NeVo - massive interface overhaul 27, 28 Dec, 2002
NeVo - began final phase of project 26 Jan, 2003
NeVo - almost finished code 12 Feb, 2003
NeVo - plugins system implemented 13 Feb, 2003
NeVo - started reimplementing animations 15 Feb, 2003
NeVo - animations implemented as plugins 15 Feb, 2003
NeVo - lookat system implemented 15 Feb, 2003
NeVo - back to this code 01 April, 2003
NeVo - implemented camera prediction 01 April, 2003
NeVo - hopefully fixed spastic camera bug 01 April, 2003
NeVo - extended camera prediction 01 April, 2003
NeVo - removed extended camera prediction 02 April, 2003
NeVo - definitely fixed spastic camera 02 April, 2003

A note on multiple cameras: Extra cameras could be used to switch fixed
views (Alone in the Dark style), used for special animations,
camera angles in demos, etc. To use multiple cameras the extra
camera(s) properties must be defined. The default camera is QCAMERA0.

This code is based off of Psychospaz's original chasecam tutorial and the
Quake2Max 0.42 source code. NeVo does not claim credit for code
written by Psychospaz. This is merely an advanced implementation of the
original under the GPL.

Camera Version 5.3 - 28 Feb, 2003 (NeVo)
*/

#include "client.h"

// Default cameras
camdevice_t		camdevice;				// Main Camera Device
camdevstate_t	camdevstate;			// Device State
camstate_t		*cameras [MAX_CAMERAS];	// Array of cameras
camplugins_t	camplugins;				// Plugins Manager

// Cvars
cvar_t	*cl_thirdPerson;
cvar_t	*cl_thirdPersonAngle;
cvar_t	*cl_thirdPersonDist;
cvar_t	*vid_widescreen;
cvar_t	*vid_aspectscale;	// auto increase of FOV for wide aspects
cvar_t	*cl_thirdPersonDamp;

/************************
** Function Prototypes **
************************/
float absval (float a);
void CamSystem_Activate (int cam);
void CamSystem_ActivateLast (void);
void CamSystem_AnimForce (int anim);
void CamSystem_CreateCam (int cam, int type, char *file);
void *CamSystem_GetOption (int cam, int mode);
void CamSystem_LookAt (int cam, vec3_t lookat);
int CamSystem_ReturnCam (void);
void CamSystem_SetOption (int cam, int mode, void *option);
void CamSystem_ShutdownCam (int cam);
void CamSystem_ShutdownCams (void);
void CamSystem_Think (qboolean updateextras);
void CamSystem_ToChase (void);
void CamSystem_ToEyes (void);
void CamSystem_ZoomIn (void);
void CamSystem_ZoomOut (void);
void CamEngine_AddEntAlpha (entity_t *ent);
void CamEngine_AnimThink (int cam);
void CamEngine_AspectThink (int cam);
void CamEngine_BinocOverlay (void);
void CamEngine_CalcAlpha (int cam);
void CamEngine_CalcLookat (int cam, player_state_t *ps, player_state_t *ops);
void CamEngine_Clip (int cam);
void CamEngine_DirectView (int cam);
void CamEngine_DrawOverlays (int cam);
void CamEngine_Drift (int cam);
void CamEngine_Focus (int cam);
void CamEngine_FOV (int cam);
void CamEngine_InitPosition (vec3_t myViewOrg);
void CamEngine_Jitter (int cam);
void CamEngine_Kick (int cam);
void CamEngine_LensFlare (int cam);
void CamEngine_LensThink (int cam);
qboolean CamEngine_LoadFromFile (int cam, char *file);
void CamEngine_ParseState (int cam);
void CamEngine_ScopeOverlay (void);
void CamEngine_SecurityOverlay (void);
void CamEngine_SetAngularPosition (int cam, float yaw, float pitch, float roll);
void CamEngine_SetViewAngle (int cam, float yaw, float pitch, float roll);
void CamEngine_SmartTarget (int cam);
void CamEngine_Damp (int cam);
void CamEngine_Think (int cam);
trace_t CamEngine_Trace (vec3_t start, vec3_t end, float size, int contentmask);
void CamEngine_UpdatePrimary (int cam);
void CamEngine_UpdateTarget (int cam);
void CamEngine_Zoom (int cam);
void Cam_InitSystem (void);
void Cam_Shutdown (void);
void Cam_UpdateEntity (entity_t *ent);
void Cam_Aspect_Cycle_f (void);
void Cam_Aspect_Norm_f (void);
void Cam_Aspect_HDTV_f (void);
void Cam_Aspect_Cinema_f (void);
void Cam_Reset_f (void);
void Cam_Toggle_f (void);
void Cam_ZoomIn_f (void);
void Cam_ZoomOut_f (void);
extern void CamPlugins_RegisterPlugins (void);
extern void CL_AddViewWeapon (player_state_t *ps, player_state_t *ops);
extern void vectoangles2 (vec3_t value1, vec3_t angles);

#define DEG2RAD( a ) ( a * M_PI ) / 180.0F

/*
======
absval
======
*/
float absval (float a)
{
	if (a < 0.0) a=-a;
	return a;
}

/*
||||||||||||||||||||||||||||||||||||
			Camera System
||||||||||||||||||||||||||||||||||||
This section contains the camera system functions.
*/

/*
==================
CamSystem_Activate

Swtich cameras
==================
*/
void CamSystem_Activate (int cam)
{
	// If the camera is defined, switch to it
	if (camdevstate.DefinedCameras[cam])
	{
		camdevstate.LastCamera = camdevice.GetCurrentCam();
		camdevstate.ActiveCamera = cam;
	}
}

/*
======================
CamSystem_ActivateLast

Swtich cameras
======================
*/
void CamSystem_ActivateLast (void)
{
	// If the camera is defined, switch to it
	if (camdevstate.DefinedCameras[camdevstate.LastCamera])
	{
		int oldcam = camdevice.GetCurrentCam();
		camdevstate.ActiveCamera = camdevstate.LastCamera;
		camdevstate.LastCamera = oldcam;
	}
}

/*
===================
CamSystem_AnimForce

Run appropriate animation
===================
*/
void CamSystem_AnimForce (int anim)
{
	switch (anim)
	{
	case CAMANIM_SPAWN:
		camplugins.AnimSpawn();
		break;
	case CAMANIM_DEATH:
		camplugins.AnimDeath();
		break;
	case CAMANIM_FALLING:
		camplugins.AnimFalling();
		break;
	case CAMANIM_NONE:
	default:
		break;
	}
}

/*
===================
CamSystem_CreateCam

Create a camera
===================
*/
void CamSystem_CreateCam (int cam, int type, char *file)
{
	if (camdevstate.DefinedCameras[cam])
		return;

	// Try loading a definitions file
	if (!CamEngine_LoadFromFile(cam, file))
	{
		vec3_t myViewOrg;
		CamEngine_InitPosition(myViewOrg);

		/*
		If loading a camera definition file fails
		use these hardcoded definitions
		*/

		// Allocate some ram
		cameras[cam] = malloc (sizeof (camstate_t));

		// Camera Information
		cameras[cam]->name = cam;
		cameras[cam]->type = type;
		cameras[cam]->version = CAMERA_VERSION;
		cameras[cam]->lens = CAMLENS_DEFAULT;
		cameras[cam]->dispopt = CAMDISPOPT_NONE;
		cameras[cam]->animMode = CAMANIM_NONE;
		cameras[cam]->damped = false;
		cameras[cam]->smart = false;
		cameras[cam]->clip = false;
		cameras[cam]->alpha = false;
		cameras[cam]->focus = false;
		cameras[cam]->kickable = false;
		cameras[cam]->zoomable = false;
		cameras[cam]->magnifiable = false;
		cameras[cam]->drift = false;
		cameras[cam]->flare = false;
		cameras[cam]->jitter = false;
		cameras[cam]->aspectok = false;
		VectorClear(cameras[cam]->angpos);			// Euler angular position

		switch (type) {
			case CAMTYPE_SECURITY:
				cameras[cam]->lens = CAMLENS_WIDEANGLE;
				break;
			case CAMTYPE_SCOPE:
				cameras[cam]->lens = CAMLENS_TELESCOPIC;
				cameras[cam]->focus = true;			// Field of depth adjustment (Scopes and Binoculars)
				break;
			case CAMTYPE_BINOCULAR:
				cameras[cam]->lens = CAMLENS_BINOCULAR;
				cameras[cam]->focus = true;			// Field of depth adjustment (Scopes and Binoculars)
				break;
			case CAMTYPE_ANIMATION:
				cameras[cam]->animMode = CAMANIM_NONE;
				break;
			case CAMTYPE_FIRSTPERSON:
				cameras[cam]->aspect = vid_widescreen->value + 1;
				cameras[cam]->aspectok = true;
				cameras[cam]->currentdist = 0.0;
				cameras[cam]->maxdist = 0.0;
				cameras[cam]->mindist = 0.0;
				cameras[cam]->normdist = 0.0;
				cameras[cam]->fov = fov->value;
				cameras[cam]->kickable = true;
				cameras[cam]->magnification = 1.0;
				cameras[cam]->maxmagnification = 1.0;
				cameras[cam]->minmagnification = 1.0;
				cameras[cam]->normmagnification = 1.0;
				cameras[cam]->maxalphadist = 0.0;
				cameras[cam]->radius = 1.0;
				cameras[cam]->tether = 0.00001;
				VectorCopy (myViewOrg, cameras[cam]->origin);
				VectorCopy (myViewOrg, cameras[cam]->target);
				VectorCopy (myViewOrg, cameras[cam]->oldTarget);
				break;
			case CAMTYPE_THIRDPERSON:
				cameras[cam]->alpha = true;
				cameras[cam]->aspect = vid_widescreen->value + 1;
				cameras[cam]->aspectok = true;
				cameras[cam]->clip = true;
				cameras[cam]->currentdist = cl_thirdPersonDist->value;
				cameras[cam]->mindist = 3.0;
				cameras[cam]->maxdist = cl_thirdPersonDist->value + cameras[cam]->mindist;
				cameras[cam]->normdist = (cl_thirdPersonDist->value * 0.5) + cameras[cam]->mindist;
				cameras[cam]->damped = true;
				cameras[cam]->flare = true;
				cameras[cam]->fov = fov->value;
				cameras[cam]->magnification = 1.0;
				cameras[cam]->maxmagnification = 1.0;
				cameras[cam]->minmagnification = 1.0;
				cameras[cam]->magnifiable = false;
				cameras[cam]->normmagnification = 1.0;
				cameras[cam]->maxalphadist = 26.5;
				cameras[cam]->radius = 8.0;
				cameras[cam]->smart = true;
				cameras[cam]->tether = 3.8;		// No more than 4.0, or prediction gets choppy
				cameras[cam]->angpos[PITCH] = cl_thirdPersonAngle->value;
				cameras[cam]->zoomable = true;
				VectorCopy (myViewOrg, cameras[cam]->origin);
				VectorCopy (myViewOrg, cameras[cam]->target);
				VectorCopy (myViewOrg, cameras[cam]->oldTarget);
			default:
				break;
		}

		// Player Relative Camera Position
		VectorClear(cameras[cam]->position);		// Camera's position relative to player

		// View angles
		VectorClear(cameras[cam]->viewangle);		// Euler view angle

		// Camera Definitions File
		cameras[cam]->filename = file;				// Name of camera definition file

		VectorCopy (cl.refdef.vieworg, cameras[cam]->origin);
	}

	camdevstate.DefinedCameras[cam] = true;
	camdevice.Activate(cam);
}

/*
===================
CamSystem_GetOption

Return value of parameter
===================
*/
void *CamSystem_GetOption (int cam, int mode)
{
	switch (mode) {
		case (int)CAM_TYPE:					// Camera type
			return (int)cameras[cam]->type;
		case (int)CAM_LENS:					// Lens type
			return (int)cameras[cam]->lens;
		case (int)CAM_DISPOPT:				// Render Flags
			return (int)cameras[cam]->dispopt;
		case (int)CAM_ANIMATION:			// Animation type
			return (int)cameras[cam]->animMode;
		case (int)CAM_ZOOM:					// Zoom mode
			return (int)camdevstate.zoommode;
		case (int)CAM_FILENAME:				// Load camera from file
			return cameras[cam]->filename;
		case (int)CAMMODE_DAMPED:			// Spring damped motion
			return (int)cameras[cam]->damped;
		case (int)CAMMODE_SMART:			// Collision avoidance
			return (int)cameras[cam]->smart;
		case (int)CAMMODE_CLIP:				// Collision detection
			return (int)cameras[cam]->clip;
		case (int)CAMMODE_ALPHA:			// Viewer model alpha blending
			return (int)cameras[cam]->alpha;
		case (int)CAMMODE_FOCUS:			// Field of depth adjustment (Scopes and Binoculars)
			return (int)cameras[cam]->focus;
		case (int)CAMMODE_DRIFT:			// Drift mode
			return (int)cameras[cam]->drift;
		case (int)CAMMODE_ZOOM:				// Zoomable
			return (int)cameras[cam]->zoomable;
		case (int)CAMMODE_MAGNIFY:			// Magnifiable
			return (int)cameras[cam]->magnifiable;
		case (int)CAMMODE_FLARE:			// Lens Flares
			return (int)cameras[cam]->flare;
		case (int)CAMMODE_KICK:				// Recoil, Pain
			return (int)cameras[cam]->kickable;
		case (int)CAMMODE_JITTER:			// Jerky camera
			return (int)cameras[cam]->jitter;
		case (int)CAM_ASPECT:				// Aspect ratio
			return (int)cameras[cam]->aspect;
		case (int)CAMMODE_ASPECT:			// Aspect Ratio
			return (int)cameras[cam]->aspectok;
	}
	return NULL;
}

/*
================
CamSystem_LookAt

Set what to look at
================
*/
void CamSystem_LookAt (int cam, vec3_t lookat)
{
	VectorCopy(lookat, cameras[cam]->lookat);
}

/*
===================
CamSystem_ReturnCam

Return index of active camera
===================
*/
int CamSystem_ReturnCam (void)
{
	return camdevstate.ActiveCamera;
}

/*
===================
CamSystem_SetOption

Set parameter to value
===================
*/
void CamSystem_SetOption (int cam, int mode, void *option)
{
	if (!option)
		return;

	switch (mode) {
		case (int)CAM_TYPE:					// Camera type
			cameras[cam]->type = (int)option;
			break;
		case (int)CAM_LENS:					// Lens type
			cameras[cam]->lens = (int)option;
			break;
		case (int)CAM_DISPOPT:				// Render flags
			cameras[cam]->dispopt = (int)option;
			break;
		case (int)CAM_ANIMATION:			// Animation type
			cameras[cam]->animMode = (int)option;
			break;
		case (int)CAM_ZOOM:					// Zoom mode
			camdevstate.zoommode = (int)option;
			break;
		case (int)CAM_FILENAME:				// Load camera from file
			cameras[cam]->filename = (char *)option;
			break;
		case (int)CAMMODE_DAMPED:			// Spring damped motion
			cameras[cam]->damped = (int)option;
			break;
		case (int)CAMMODE_SMART:			// Collision avoidance
			cameras[cam]->smart = (int)option;
			break;
		case (int)CAMMODE_CLIP:				// Collision detection
			cameras[cam]->clip = (int)option;
			break;
		case (int)CAMMODE_ALPHA:			// Viewer model alpha blending
			cameras[cam]->alpha = (int)option;
			break;
		case (int)CAMMODE_FOCUS:			// Field of depth adjustment (Scopes and Binoculars)
			cameras[cam]->focus = (int)option;
			break;
		case (int)CAMMODE_DRIFT:			// Drift mode
			cameras[cam]->drift = (int)option;
			break;
		case (int)CAMMODE_ZOOM:				// Zoomable
			cameras[cam]->zoomable = (int)option;
			break;
		case (int)CAMMODE_MAGNIFY:			// Magnifiable
			cameras[cam]->magnifiable = (int)option;
			break;
		case (int)CAMMODE_FLARE:			// Lens Flares
			cameras[cam]->flare = (int)option;
			break;
		case (int)CAMMODE_KICK:				// Recoil, Pain
			cameras[cam]->kickable = (int)option;
			break;
		case (int)CAMMODE_JITTER:			// Jerky camera
			cameras[cam]->jitter = (int)option;
			break;
		case (int)CAM_ASPECT:				// Camera aspect
			cameras[cam]->aspect = (int)option;
			break;
		case (int)CAMMODE_ASPECT:			// Camera aspect
			cameras[cam]->aspectok = (int)option;
			break;
	}
	return;
}

/*
=====================
CamSystem_ShutdownCam

Shutdown a camera
=====================
*/
void CamSystem_ShutdownCam (int cam)
{
	// We can't shut down the default cameras manually
	if (!(cam > QCAMERA2))
		return;

	free (cameras[cam]);
	camdevstate.DefinedCameras[cam] = false;
	camdevice.Activate(QCAMERA0);
}

/*
======================
CamSystem_ShutdownCams

Shutdown all cameras
!! This should only ever be called at client shutdown !!
======================
*/
void CamSystem_ShutdownCams (void)
{
	int i;
	for (i = 0; i < MAX_CAMERAS; i++)
		free (cameras[i]);

	//free (cameras);
}

/*
===============
CamSystem_Think

Process cameras
Set updateextras to true to parse all cameras
===============
*/
void CamSystem_Think (qboolean updateextras)
{
	int cam = camdevice.GetCurrentCam();

	CamEngine_ParseState (cam);

	if (updateextras)
	{
		int i;
		// Update all cameras
		for (i = 0; i < MAX_CAMERAS; i++)
		{
			CamEngine_Think (i);
		}
	}
	else
	{
		// Always update the default cameras to prevent switching artifacts
		CamEngine_Think (QCAMERA0);
		CamEngine_Think (QCAMERA1);

		if ((cam != QCAMERA0) && (cam != QCAMERA1))
		{
			// Update the current camera
			CamEngine_Think (cam);
		}
	}
}

/*
NeVo
TODO: The behavior of these two functions is unpredicatable
when switching from an outside camera (like a spectator
or security cam)... 
*/

/*
=================
CamSystem_ToChase

Switch to chase camera
=================
*/
void CamSystem_ToChase (void)
{
	int cam = camdevice.GetCurrentCam();

	cl_thirdPerson->value = 1;

	if (cam == QCAMERA1)	// If already chase, this doesn't apply
		return;

	camdevice.Activate (QCAMERA1);
	camdevice.SetOption (QCAMERA1, CAM_ZOOM, (void *)CAMZOOM_AUTO_OUT);
}

/*
================
CamSystem_ToEyes

Switch to first person view
================
*/
void CamSystem_ToEyes (void)
{
	int cam = camdevice.GetCurrentCam();

	cl_thirdPerson->value = 0;

	if (cam == QCAMERA0)	// If first person, this doesn't apply
		return;

	camdevice.SetOption (cam, CAM_ZOOM, (void *)CAMZOOM_AUTO_IN);	// NeVo - another bug fix
}

/*
================
CamSystem_ZoomIn

Zoom camera in
================
*/
void CamSystem_ZoomIn (void)
{
	int cam = camdevice.GetCurrentCam();

	camdevice.SetOption (cam, CAM_ZOOM, (void *)CAMZOOM_MANUAL_IN);
}

/*
=================
CamSystem_ZoomOut

Zoom camera out
=================
*/
void CamSystem_ZoomOut (void)
{
	int cam = camdevice.GetCurrentCam();

	camdevice.SetOption (cam, CAM_ZOOM, (void *)CAMZOOM_MANUAL_OUT);
}

/*
|||||||||||||||||||||||||||||||||||||
			Camera Engine		
|||||||||||||||||||||||||||||||||||||
This section contains the camera engine functions.
*/

/*
=====================
CamEngine_AddEntAlpha
=====================
*/
void CamEngine_AddEntAlpha (entity_t *ent)
{
	if (ent->alpha == 0)
		ent->alpha = camdevstate.viewermodelalpha;
	else
		ent->alpha *= camdevstate.viewermodelalpha;	// Set entity alpha

	if (ent->alpha < 1.0)						// If translucent, notify refresh
		ent->flags |= RF_TRANSLUCENT;
}

/*
===================
CamEngine_AnimThink

Run appropriate animation
===================
*/
void CamEngine_AnimThink (int cam)
{
	// FIXME: Do animation parsing stuff here
	// FIXME: Trace for falling death, no godmode. Q3 uses a death brush... Q2 doesn't have
	int anim = 0;

	switch (anim)
	{
	case CAMANIM_SPAWN:
		camplugins.AnimSpawn();
		break;
	case CAMANIM_DEATH:
		camplugins.AnimDeath();
		break;
	case CAMANIM_FALLING:
		camplugins.AnimFalling();
		break;
	case CAMANIM_NONE:
	default:
		break;
	}
}

/*
=====================
CamEngine_AspectThink

NeVo - this eats SCR_CalcVRect
NeVo - found bug. If this is called
more than once per frame, the static
variables get fucked up. :-/ This
is important because we always update
both QCAMERA0 and QCAMERA1 every frame.
=====================
*/
void CamEngine_AspectThink (int cam)
{
	int width, height;
	static int oldheight, oldwidth;
	static float lerp;
	float size;

	// NeVo - fix dumb ass bug
	if (cam != camdevice.GetCurrentCam())
		return;

	// bound viewsize
	if (scr_viewsize->value < 40)
		Cvar_Set ("viewsize","40");
	if (scr_viewsize->value > 100)
		Cvar_Set ("viewsize","100");

	// Set size
	size = scr_viewsize->value;

	// Set width
	width = viddef.width;

	// NeVo - aspect ratio code
	switch ((int) cameras[cam]->aspect )
	{
	case ASPECT_HDTV:
		// 1.77:1 Ratio
		height = ( width / 16 ) * 9;
		break;
	case ASPECT_CINEMA:
		// 2.35:1 Ratio
		height = width / 2.35;	// NeVo - there is no good way to get this to be exact
		break;
	case ASPECT_NONE:
		cameras[cam]->aspect = ASPECT_NORMAL;
	case ASPECT_NORMAL:
		// 1.33:1 Ratio
		//height = ( width / 4 ) * 3;
		height = viddef.height;
		break;
	}

	if (!oldheight)
	{
		oldheight = height;
		oldwidth = width;
	}

	// interpolate aspect ratio transitions and screen size adjustments
	if (( height != oldheight ) && ( lerp <= 1.0 ))
	{
		height = height * ( size / 100 );
		width = width * ( size / 100 );
		scr_vrect.height = oldheight + lerp * ( height - oldheight );
		scr_vrect.width = oldwidth + lerp * ( width - oldwidth );
		lerp += CAMLERP_FRAC;
	}
	else
	{
		scr_vrect.height = height * ( size / 100 );
		oldheight = scr_vrect.height;
		scr_vrect.width = width * ( size / 100 );
		oldwidth = scr_vrect.width;
		lerp = 0.0;
	}

	// Make sure we don't break the system limits
	/*if (scr_vrect.height > viddef.height)
		scr_vrect.height = viddef.height;
	else if (scr_vrect.height < (viddef.height * 0.4))
		scr_vrect.height = viddef.height * 0.4;
	if (scr_vrect.width > viddef.width)
		scr_vrect.width = viddef.width;
	else if (scr_vrect.width < (viddef.width * 0.4))
		scr_vrect.width = viddef.width * 0.4;*/

	scr_vrect.width &= ~7;
	scr_vrect.height &= ~1;

	scr_vrect.x = ( viddef.width * 0.5 ) - ( scr_vrect.width * 0.5 );
	scr_vrect.y = ( viddef.height * 0.5 ) - ( scr_vrect.height * 0.5 );
}

/*
======================
CamEngine_BinocOverlay

Draw overlay data for binoculars
======================
*/
void CamEngine_BinocOverlay (void)
{
	// Draw silkscreen cutout
	//re.DrawStretchPic (0, 0, cl.refdef.width, cl.refdef.height, "binoc_overlay");

	// Compass
		// TODO: Do an optional compass/direction thing here

	// Rangefinder
		// TODO: Do an optional rangefinder thing here

	// Magnification
	DrawString (((cl.refdef.width / 8) * 7 )- 24,
		((cl.refdef.height /4) *3) - 8, 
		va("1.2f%", (int)cameras[camdevice.GetCurrentCam()]->magnification));
}

/*
===================
CamEngine_CalcAlpha

Calculate player model alpha
===================
*/
void CamEngine_CalcAlpha (int cam)
{
	vec3_t	displacement;
	float	dist;

	if(!camdevice.GetOption(cam, CAMMODE_ALPHA))
		return;

	// Calculate displacement from camera to player head
	VectorSubtract (cl.refdef.vieworg, cameras[cam]->origin, displacement);
	dist = VectorLength(displacement);

	// Set alpha to one by default
	camdevstate.viewermodelalpha = 1.0;

	// Prevent division by zero
	if (cameras[cam]->maxalphadist == 0.0)
		cameras[cam]->maxalphadist = 0.1;
	
	// Make alpha from being zoomed too close
	if (dist <= cameras[cam]->maxalphadist)
		camdevstate.viewermodelalpha = dist / cameras[cam]->maxalphadist;

	// Clamp
	if (camdevstate.viewermodelalpha < 0.0)
		camdevstate.viewermodelalpha = 0.0;
	else if (camdevstate.viewermodelalpha > 1.0)	// Never more solid than solid
		camdevstate.viewermodelalpha = 1.0;
}

/*
====================
CamEngine_CalcLookat

Cipher View Angles
====================
*/
void CamEngine_CalcLookat (int cam, player_state_t *ps, player_state_t *ops)
{
	int lerp = cl.lerpfrac;
	//trace_t	tr;
	int i;

	// if not running a demo or on a locked frame, add the local angle movement
	if ( cl.frame.playerstate.pmove.pm_type < PM_DEAD )
	{	// use predicted values
		for (i=0 ; i<3 ; i++)
			cl.refdef.viewangles[i] = cl.predicted_angles[i];
	}
	else
	{	// just use interpolated values
		for (i=0 ; i<3 ; i++)
			cl.refdef.viewangles[i] = LerpAngle (ops->viewangles[i], ps->viewangles[i], lerp);
	}

	AngleVectors (cl.refdef.viewangles, cl.v_forward, cl.v_right, cl.v_up);

	// Figure out what we are looking at
	VectorMA(cl.refdef.vieworg, 512, cl.v_forward, cameras[cam]->lookat);
	
	// NeVo - this trace makes for really choppy camera
	//tr = CamEngine_Trace (cl.refdef.vieworg, cameras[cam]->lookat, cameras[cam]->radius, MASK_SOLID);
	//VectorCopy(tr.endpos, cameras[cam]->lookat);
}

/*
==============
CamEngine_Clip

Clip camera
==============
*/
void CamEngine_Clip (int cam)
{
	trace_t tr;

	if (!camdevice.GetOption(cam, CAMMODE_CLIP))
		return;

	// Clip the camera
	tr = CamEngine_Trace (cl.refdef.vieworg, cameras[cam]->origin, cameras[cam]->radius, MASK_SOLID);
	VectorCopy(tr.endpos, cameras[cam]->origin);
}

/*
====================
CamEngine_DirectView

Adjust View Angles
====================
Parts based on RTCW source
*/
void CamEngine_DirectView (int cam)
{
	vec3_t	focusPoint;

	VectorSubtract(cameras[cam]->lookat, cameras[cam]->origin, focusPoint);
	VectorNormalize(focusPoint);
	vectoangles2(focusPoint, cameras[cam]->viewangle);
}

/*
======================
CamEngine_DrawOverlays

Draw overlays on the screen by camera type
======================
*/
void CamEngine_DrawOverlays (int cam)
{
	switch ((int)camdevice.GetOption(cam, CAM_TYPE)) {
	case CAMTYPE_BINOCULAR:
		CamEngine_BinocOverlay ();
		break;
	case CAMTYPE_SCOPE:	
		CamEngine_ScopeOverlay ();
		break;
	case CAMTYPE_SECURITY:
		CamEngine_SecurityOverlay ();
		break;
	case CAMTYPE_THIRDPERSON:		
	case CAMTYPE_ANIMATION:		
	case CAMTYPE_FIRSTPERSON:
	default:
		break;
	}
}

/*
===============
CamEngine_Drift

Slowly drift the camera
===============
*/
void CamEngine_Drift (int cam)
{
	trace_t	tr;

	if (!camdevice.GetOption(cam, CAMMODE_DRIFT))
		return;

	cameras[cam]->target[0] += ((rand()&DRIFT_FACTOR) * 0.25);
	cameras[cam]->target[0] -= ((rand()&DRIFT_FACTOR) * 0.25);
	cameras[cam]->target[1] += ((rand()&DRIFT_FACTOR) * 0.25);
	cameras[cam]->target[1] -= ((rand()&DRIFT_FACTOR) * 0.25);
	cameras[cam]->target[2] += ((rand()&DRIFT_FACTOR) * 0.25);
	cameras[cam]->target[2] -= ((rand()&DRIFT_FACTOR) * 0.25);

	// Clip the target
	tr = CamEngine_Trace (cameras[cam]->target, cameras[cam]->target, cameras[cam]->radius, MASK_SOLID);
	VectorCopy(tr.endpos, cameras[cam]->target);
}

/*
===============
CamEngine_Focus

Do depth of field adjustments
===============
*/
void CamEngine_Focus (int cam)
{
	if(!camdevice.GetOption(cam, CAMMODE_FOCUS))
		return;

	camplugins.DepthOfField();
}

/*
=============
CamEngine_FOV

Do field of view adjustments
=============
*/
void CamEngine_FOV (int cam)
{
	static float	lerp, old_fov;
	float			calc_fov;

	if (!camdevstate.Initialized)
		return;
	
	if (cameras[cam]->magnifiable)
		calc_fov = (cameras[cam]->fov + (vid_aspectscale->value * (cameras[cam]->aspect - 1))) / cameras[cam]->magnification;
	else
		calc_fov = cameras[cam]->fov + (vid_aspectscale->value * (cameras[cam]->aspect - 1));

	if (!old_fov)
		old_fov = calc_fov;

	// interpolate field of view
	if (( calc_fov != old_fov ) && ( lerp <= 1.0 ))
	{
		//cl.refdef.fov_x = ops->fov + lerp * (ps->fov - ops->fov);
		camdevstate.fov = old_fov + lerp * ( calc_fov - old_fov );
		cl.refdef.fov_x = camdevstate.fov;
		lerp += CAMLERP_FRAC;
	}
	else
	{
		cl.refdef.fov_x = camdevstate.fov = old_fov = calc_fov;
		lerp = 0.0;
	}

	// Make sure we don't break the system limits
	if (cl.refdef.fov_x > MAX_FOV)
		cl.refdef.fov_x = MAX_FOV;
	else if (cl.refdef.fov_x < MIN_FOV)
		cl.refdef.fov_x = MIN_FOV;
}

/*
======================
CamEngine_InitPosition

Return current vieworg position
NeVo - this exists to fix the camera init bug
======================
*/
void CamEngine_InitPosition (vec3_t myViewOrg)
{
	int			i;
	float		lerp, backlerp;
	centity_t	*ent;
	frame_t		*oldframe;
	player_state_t	*ps, *ops;

	// find the previous frame to interpolate from
	ps = &cl.frame.playerstate;
	i = (cl.frame.serverframe - 1) & UPDATE_MASK;
	oldframe = &cl.frames[i];
	if (oldframe->serverframe != cl.frame.serverframe-1 || !oldframe->valid)
		oldframe = &cl.frame;		// previous frame was dropped or invalid
	ops = &oldframe->playerstate;

	// see if the player entity was teleported this frame
	if ( Q_fabs(ops->pmove.origin[0] - ps->pmove.origin[0]) > TELEPORT_DIST
		|| Q_fabs(ops->pmove.origin[1] - ps->pmove.origin[1]) > TELEPORT_DIST
		|| Q_fabs(ops->pmove.origin[2] - ps->pmove.origin[2]) > TELEPORT_DIST)
		ops = ps;		// don't interpolate

	ent = &cl_entities[cl.playernum+1];
	lerp = cl.lerpfrac;

	// calculate the origin - modified by Psychospaz for 3rd person camera
	if ((cl_predict->value) && !(cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION))
	{	// use predicted values
		unsigned	delta;

		backlerp = 1.0 - lerp;
		for (i=0 ; i<3 ; i++)
		{
			myViewOrg[i] = cl.predicted_origin[i] + ops->viewoffset[i] 
				+ cl.lerpfrac * (ps->viewoffset[i] - ops->viewoffset[i])
				- backlerp * cl.prediction_error[i];
		}

		// smooth out stair climbing
		delta = cls.realtime - cl.predicted_step_time;
		if (delta < 100)
			myViewOrg[2] -= cl.predicted_step * (100 - delta) * 0.01;
	}
	else
	{	// just use interpolated values
		for (i=0 ; i<3 ; i++)
			myViewOrg[i] = ops->pmove.origin[i]*0.125 + ops->viewoffset[i] 
				+ lerp * (ps->pmove.origin[i]*0.125 + ps->viewoffset[i] 
				- (ops->pmove.origin[i]*0.125 + ops->viewoffset[i]) );
	}
}

/*
================
CamEngine_Jitter

Rapidly jerk the camera
================
*/
void CamEngine_Jitter (int cam)
{
	trace_t	tr;

	if (!camdevice.GetOption(cam, CAMMODE_JITTER))
		return;

	cameras[cam]->origin[0] += ((rand()&JITTER_FACTOR) * 0.4);
	cameras[cam]->origin[0] -= ((rand()&JITTER_FACTOR) * 0.4);
	cameras[cam]->origin[1] += ((rand()&JITTER_FACTOR) * 0.4);
	cameras[cam]->origin[1] -= ((rand()&JITTER_FACTOR) * 0.4);
	cameras[cam]->origin[2] += ((rand()&JITTER_FACTOR) * 0.4);
	cameras[cam]->origin[2] -= ((rand()&JITTER_FACTOR) * 0.4);

	// Clip the target
	tr = CamEngine_Trace (cameras[cam]->origin, cameras[cam]->origin, cameras[cam]->radius, MASK_SOLID);
	VectorCopy(tr.endpos, cameras[cam]->origin);
}

/*
==============
CamEngine_Kick

Kick the camera
==============
*/
void CamEngine_Kick (int cam)
{
	int i, lerp = cl.lerpfrac;
	player_state_t	*ps, *ops;
	frame_t	*oldframe;

	if (!cameras[cam]->kickable)
		return;

	// find the previous frame to interpolate from
	ps = &cl.frame.playerstate;
	i = (cl.frame.serverframe - 1) & UPDATE_MASK;
	oldframe = &cl.frames[i];
	if (oldframe->serverframe != cl.frame.serverframe-1 || !oldframe->valid)
		oldframe = &cl.frame;		// previous frame was dropped or invalid
	ops = &oldframe->playerstate;

	for (i=0 ; i<3 ; i++)
		cl.refdef.viewangles[i] *= LerpAngle (ops->kick_angles[i], ps->kick_angles[i], lerp);
}

/*
===================
CamEngine_LensFlare

Draw lens flares
===================
*/
void CamEngine_LensFlare (int cam)
{
	if (!camdevice.GetOption(cam, CAMMODE_FLARE))
		return;

	camplugins.LensFlare();
}

/*
===================
CamEngine_LensThink

Adjust view based on lens properties
===================
*/
void CamEngine_LensThink (int cam)
{
	// Change refdef based on lens type
	switch ((int)camdevice.GetOption(cam, CAM_LENS)) {
		case CAMLENS_TELESCOPIC:
			// Set the refdef to a square in the middle of the screen and zoom it
			scr_vrect.x = cl.refdef.width / 4;
			scr_vrect.width = (cl.refdef.width / 4) * 3;
			scr_vrect.y = cl.refdef.width / 4;
			scr_vrect.height = (cl.refdef.width / 4) * 3;
			scr_vrect.width &= ~7;
			scr_vrect.height &= ~1;
			break;
		case CAMLENS_WIDEANGLE:
			// Set the refdef to be letterbox and zoom it
			scr_vrect.x = 0;
			scr_vrect.width = cl.refdef.width;
			scr_vrect.y = cl.refdef.height / 4;
			scr_vrect.height = (cl.refdef.height / 4) * 3;
			scr_vrect.width &= ~7;
			scr_vrect.height &= ~1;
			break;
		case CAMLENS_BINOCULAR:
			// Set the refdef to 3/4 screen width and 1/2 height and zoom it
			scr_vrect.x = cl.refdef.width / 8;
			scr_vrect.width = (cl.refdef.width / 8) * 7;
			scr_vrect.y = cl.refdef.height / 4;
			scr_vrect.height = (cl.refdef.height / 4) * 3;
			scr_vrect.width &= ~7;
			scr_vrect.height &= ~1;
			break;
		case CAMLENS_FISHEYE:	// FIXME: Obsolete?
			camdevice.SetOption(cam, CAM_DISPOPT, (void *)CAMDISPOPT_FISHEYE);
			break;
		case CAMLENS_PANORAMIC:	// FIXME: Obsolete?
			// Set the aspect to letterbox
			camdevice.SetOption(cam, CAM_ASPECT, (void *)ASPECT_CINEMA);
		case CAMLENS_DEFAULT:
		default:
			CamEngine_AspectThink(cam);
			break;
	}

	// Change render effects by display option
	switch ((int)camdevice.GetOption(cam, CAM_DISPOPT)) {
		case CAMDISPOPT_IR:
			cl.refdef.rdflags &= RDF_IRGOGGLES;
			break;
		case CAMDISPOPT_UV:
			cl.refdef.rdflags &= RDF_UVGOGGLES;
			break;

		// The following modes require refresh support
		case CAMDISPOPT_INKED:		// Draw the scene as inked (on white)
			camplugins.InkOnWhite();
			break;
		case CAMDISPOPT_INKONPAPER:	// Draw the scene as ink on paper (cream color, with texture)
			camplugins.InkOnPaper();
			break;
		case CAMDISPOPT_BLUEPRINT:	// Draw the scene like a blueprint
			camplugins.Blueprint();
			break;
		case CAMDISPOPT_NPR:		// Draw the scene NPR-style (black lines on white)
			camplugins.NPR();
			break;
		case CAMDISPOPT_INVNPR:		// Draw the scene as inverse NPR (white lines on black)
			camplugins.InvNPR();
			break;
		case CAMDISPOPT_CELSHADED:	// Draw the scene as cel-shaded
			camplugins.CelShade();
			break;
		case CAMDISPOPT_FISHEYE:	// Draw the scene as a fisheye lens
			camplugins.Fisheye();
			break;
		case CAMDISPOPT_GREYSCALE:	// Draw the scene greyscale
			camplugins.Grayscale();
			break;
		case CAMDISPOPT_WIREFRAME:	// Draw the scene wireframe
			camplugins.WireFrame();
			break;
		case CAMDISPOPT_NONE:
		default:
			break;
	}

	// Adjust Focus
	CamEngine_Focus (cam);

	// Lens Flares
	CamEngine_LensFlare(cam);
}

/*
=====================
CamEngine_LoadFromFile

Load camera values from a .cdf file
=====================
*/
qboolean CamEngine_LoadFromFile (int cam, char *file) {
	// FIXME: Write this when the camera is finished
	if (!file)
		return false;

	return false;
}

/*
====================
CamEngine_ParseState

Cipher player state
====================
*/
void CamEngine_ParseState (int cam)
{
	if (cl_thirdPerson->modified)
	{
		Cam_Toggle_f ();
		cl_thirdPerson->modified = false;
	}

	CamEngine_AnimThink(cam);
}

/*
======================
CamEngine_ScopeOverlay

Draw scope overlay
======================
*/
void CamEngine_ScopeOverlay (void)
{
	// Draw silkscreen cutout
	//re.DrawStretchPic (0, 0, cl.refdef.width, cl.refdef.height, "scope_overlay");

	// Rangefinder

	// Magnification
	DrawString (((cl.refdef.width / 4) * 3 )- 24,
		((cl.refdef.height /4) *3) - 8, 
		va("1.2f%", (int)cameras[camdevice.GetCurrentCam()]->magnification));
}

/*
=========================
CamEngine_SecurityOverlay

Draw security camera overlay
=========================
*/
void CamEngine_SecurityOverlay (void)
{
	// Draw silkscreen cutout
	//re.DrawStrecthPic (0, 0, cl.refdef.width, cl.refdef.height, "security_overlay");
}

/*
============================
CamEngine_SetAngularPosition

Rotate camera around
============================
*/

void CamEngine_SetAngularPosition (int cam, float yaw, float pitch, float roll) 
{

	// Adjust yaw //
	cameras[cam]->angpos[YAW] += yaw;

	// Infinite Bound //
	if (cameras[cam]->angpos[YAW] > 360.0)
		cameras[cam]->angpos[YAW] -= 360.0;
	else if (cameras[cam]->angpos[YAW] <= 0.0)
		cameras[cam]->angpos[YAW] += 360.0;

	// Adjust pitch //
	cameras[cam]->angpos[PITCH] += pitch;

	// Clamp
	if (cameras[cam]->angpos[PITCH] > camdevstate.maxvertangle)
		cameras[cam]->angpos[PITCH] = camdevstate.maxvertangle;
	else if (cameras[cam]->angpos[PITCH] < -camdevstate.maxvertangle)
		cameras[cam]->angpos[PITCH] = -camdevstate.maxvertangle;
}

/*
======================
CamEngine_SetViewAngle

Rotate camera around
======================
*/

void CamEngine_SetViewAngle (int cam, float yaw, float pitch, float roll) {

	// Adjust yaw //
	cameras[cam]->viewangle[YAW] += yaw;

	// Adjust pitch //
	cameras[cam]->viewangle[PITCH] += pitch;

	// Adjust roll //
	cameras[cam]->viewangle[ROLL] += roll;

	// Infinite Bound //
	if (cameras[cam]->viewangle[YAW] > 360.0)
		cameras[cam]->viewangle[YAW] -= 360.0;
	else if (cameras[cam]->viewangle[YAW] <= 0.0)
		cameras[cam]->viewangle[YAW] += 360.0;
	if (cameras[cam]->viewangle[PITCH] > 360.0)
		cameras[cam]->viewangle[PITCH] -= 360.0;
	else if (cameras[cam]->viewangle[PITCH] <= 0.0)
		cameras[cam]->viewangle[PITCH] += 360.0;
	if (cameras[cam]->viewangle[ROLL] > 360.0)
		cameras[cam]->viewangle[ROLL] -= 360.0;
	else if (cameras[cam]->viewangle[ROLL] <= 0.0)
		cameras[cam]->viewangle[ROLL] += 360.0;
}

/*
=====================
CamEngine_SmartTarget

Collision Avoidance
=====================
*/
void CamEngine_SmartTarget (int cam)
{
	vec3_t predictedNextTarget, predictedOffset, midpoint;
	trace_t tr;
	int	i;

	if (!camdevice.GetOption(cam, CAMMODE_SMART))
		return;

	/*
	NeVo - added crude prediction for cameras in motion. This should smooth
	cameras going through doorways and stuff like that.
	*/
	if (!VectorCompare(cameras[cam]->target, cameras[cam]->oldTarget))
	{
		// Roughly predict ahead once
		VectorSubtract (cameras[cam]->target, cameras[cam]->oldTarget, predictedNextTarget);
		VectorAdd (predictedNextTarget, cameras[cam]->target, predictedNextTarget);
		tr = CamEngine_Trace (cameras[cam]->target, predictedNextTarget, cameras[cam]->radius, MASK_SOLID);

		// Influence the target based on our prediction
		for (i=0 ; i<3 ; i++)
			cameras[cam]->target[i] = cameras[cam]->target[i] + ((1.0-tr.fraction) * (tr.endpos[i]-(cameras[cam]->target[i])));

		/*// Roughly predict further ahead
		VectorSubtract (tr.endpos, cameras[cam]->target, predictedNextTarget);
		VectorAdd (predictedNextTarget, tr.endpos, predictedNextTarget);
		tr = CM_BoxTrace (cl.refdef.vieworg, predictedNextTarget, mins, maxs, 0, MASK_SOLID);
		VectorSubtract (tr.endpos, predictedNextTarget, predictedOffset);
		VectorScale (predictedOffset, 0.63, predictedOffset);

		// Influence the target based on our prediction
		VectorAdd (cameras[cam]->target, predictedOffset, cameras[cam]->target);*/
	}

	tr = CamEngine_Trace (cameras[cam]->origin, cameras[cam]->target, cameras[cam]->radius, MASK_SOLID);

	/*
	NeVo - now make sure the path from the camera origin to the
	camera target is clear, and adjust the camera target appropriately
	*/
	while (tr.fraction != 1.0) 
	{
		VectorSubtract(cameras[cam]->target, cameras[cam]->origin, midpoint);
		VectorScale (midpoint, 0.5, midpoint);
		VectorAdd (cameras[cam]->origin, midpoint, midpoint);
		tr = CamEngine_Trace (cameras[cam]->origin, midpoint, cameras[cam]->radius, MASK_SOLID);
		if (!VectorCompare(tr.endpos, midpoint))
			VectorCopy (midpoint, cameras[cam]->target);
	}
}

/*
==============
CamEngine_Damp

Damp camera motion
==============
*/
void CamEngine_Damp (int cam)
{
	vec3_t displacement;
	vec3_t velocity;
	float displen, magnitude;

	// Force damping when drift is on
	if (!camdevice.GetOption(cam, CAMMODE_DRIFT))
	{
		// Return if damping is disabled
		if (!camdevice.GetOption(cam, CAMMODE_DAMPED))
		{
			VectorCopy ( cameras[cam]->target, cameras[cam]->origin );
			return;
		}
		else if ( VectorCompare( cameras[cam]->target, cameras[cam]->origin ))
			return;		// Return if identical
	}

	// Update Displacement
	VectorSubtract ( cameras[cam]->origin, cameras[cam]->target, displacement );
	displen = VectorLength ( displacement );

	// Update Velocity
	VectorSubtract ( cameras[cam]->oldTarget, cameras[cam]->target, velocity);
	VectorScale ( velocity, cls.frametime * VELOCITY_DAMP, velocity );

	// Update Magnitude
	magnitude = SPRING_CONST *
				(cameras[cam]->tether - displen) +
				DAMP_CONST *
				(DotProduct(displacement, velocity) /
				displen);

    // Update Camera
	VectorNormalize (displacement);
	VectorScale (displacement, magnitude * cls.frametime, displacement);
	VectorAdd (cameras[cam]->origin, displacement, cameras[cam]->origin);
}

/*
===============
CamEngine_Think

Do some real work
===============
*/
void CamEngine_Think (int cam)
{
	// Primary update
	CamEngine_UpdatePrimary(cam);

	// Update target position
	CamEngine_UpdateTarget (cam);

	// Aim at where client is looking
	CamEngine_DirectView (cam);

	// Drift camera
	CamEngine_Drift (cam);

	// Dampen motion
	CamEngine_Damp (cam);

	// Calculate Alpha
	CamEngine_CalcAlpha (cam);

	// Adjust camera lens
	CamEngine_LensThink (cam);

	// Draw overlays
	CamEngine_DrawOverlays (cam);

	// Adjust FOV
	CamEngine_FOV (cam);

	// Jitter camera
	CamEngine_Jitter (cam);

	// Adjust camera position
	CamEngine_Clip (cam);

	// Update View
	if (cam == camdevice.GetCurrentCam())
	{
		VectorCopy (cameras[cam]->origin, cl.refdef.vieworg);
		AngleVectors(cameras[cam]->viewangle, cl.v_forward, cl.v_right, cl.v_up);
		VectorCopy(cameras[cam]->viewangle, cl.refdef.viewangles);

		// Kick the camera
		CamEngine_Kick (cam);
	}
}

/*
===============
CamEngine_Trace

Run a trace for collision detection
===============
*/
trace_t CamEngine_Trace (vec3_t start, vec3_t end, float size, int contentmask)
{
	// This stuff is for Box Traces
	vec3_t maxs, mins;

	VectorSet(maxs, size, size, size);
	VectorSet(mins, -size, -size, -size);

	return CM_BoxTrace (start, end, mins, maxs, 0, contentmask);
}

/*
=======================
CamEngine_UpdatePrimary

Update vieworg to first person position
Note: at one point, this used to be CL_CalcViewValues
=======================
*/
void CamEngine_UpdatePrimary (int cam)
{
	int			i;
	float		lerp, backlerp;
	centity_t	*ent;
	frame_t		*oldframe;
	player_state_t	*ps, *ops;

	// find the previous frame to interpolate from
	ps = &cl.frame.playerstate;
	i = (cl.frame.serverframe - 1) & UPDATE_MASK;
	oldframe = &cl.frames[i];
	if (oldframe->serverframe != cl.frame.serverframe-1 || !oldframe->valid)
		oldframe = &cl.frame;		// previous frame was dropped or invalid
	ops = &oldframe->playerstate;

	// see if the player entity was teleported this frame
	if ( Q_fabs(ops->pmove.origin[0] - ps->pmove.origin[0]) > TELEPORT_DIST
		|| Q_fabs(ops->pmove.origin[1] - ps->pmove.origin[1]) > TELEPORT_DIST
		|| Q_fabs(ops->pmove.origin[2] - ps->pmove.origin[2]) > TELEPORT_DIST)
		ops = ps;		// don't interpolate

	ent = &cl_entities[cl.playernum+1];
	lerp = cl.lerpfrac;

	// calculate the origin - modified by Psychospaz for 3rd person camera
	if ((cl_predict->value) && !(cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION))
	{	// use predicted values
		unsigned	delta;

		backlerp = 1.0 - lerp;
		for (i=0 ; i<3 ; i++)
		{
			cl.refdef.vieworg[i] = cl.predicted_origin[i] + ops->viewoffset[i] 
				+ cl.lerpfrac * (ps->viewoffset[i] - ops->viewoffset[i])
				- backlerp * cl.prediction_error[i];

			if (cameras[cam]->type == CAMTYPE_THIRDPERSON)	// NeVo - fix annoying bug
			{
				//this smooths out platform riding
				cl.predicted_origin[i] -= backlerp * cl.prediction_error[i];
			}
		}

		// smooth out stair climbing
		delta = cls.realtime - cl.predicted_step_time;
		if (delta < 100)
		{
			cl.refdef.vieworg[2] -= cl.predicted_step * (100 - delta) * 0.01;
			if (cameras[cam]->type == CAMTYPE_THIRDPERSON)	// NeVo - fix annoying bug
				cl.predicted_origin[2] -= cl.predicted_step * (100 - delta) * 0.01;
		}
	}
	else
	{	// just use interpolated values
		for (i=0 ; i<3 ; i++)
			cl.refdef.vieworg[i] = ops->pmove.origin[i]*0.125 + ops->viewoffset[i] 
				+ lerp * (ps->pmove.origin[i]*0.125 + ps->viewoffset[i] 
				- (ops->pmove.origin[i]*0.125 + ops->viewoffset[i]) );
	}

	// Get our target viewpoint
	CamEngine_CalcLookat (cam, ps, ops);

	// don't interpolate blend color
	for (i=0 ; i<4 ; i++)
		cl.refdef.blend[i] = ps->blend[i];

	// Return if we aren't active
	if (cam != camdevice.GetCurrentCam())
		return;

	// If the camera is a first person camera, draw the vweap
	if ((int)camdevice.GetOption(cam, CAM_TYPE) == CAMTYPE_FIRSTPERSON)
		CL_AddViewWeapon (ps, ops);
}

/*
======================
CamEngine_UpdateTarget

Update target position
======================

*/
void CamEngine_UpdateTarget (int cam)
{
	trace_t tr;
	float radius, theta, phi;

	// Cipher zoom and magnification
	CamEngine_Zoom (cam);

	// Set old target origin
	VectorCopy (cameras[cam]->target, cameras[cam]->oldTarget);
	
	// Calculate relative target position
	radius	= (float)cameras[cam]->currentdist;

	// First Person Aspect Adjustment
	if (cameras[cam]->type == CAMTYPE_FIRSTPERSON)
	{
		// Tweak the distance so the vweap isn't cut out of view
		radius += (cameras[cam]->aspect - 1) * 0.60;
	}

	// NeVo - prevent the camera from being too far away
	if (radius > cameras[cam]->maxdist)
		radius = cameras[cam]->maxdist;

	// NeVo - this may STILL suffer from Gimbal Lock, use quaternions someday
	theta	= DEG2RAD (cameras[cam]->angpos[YAW]);
	phi		= DEG2RAD (cameras[cam]->angpos[PITCH]);
	cameras[cam]->position[QX] = radius * cos ( theta ) * cos ( phi );	// X
	cameras[cam]->position[QY] = radius * sin ( theta );				// Y - may need a *sin(phi) or *cos(phi)
	cameras[cam]->position[QZ] = radius * cos ( theta ) * sin ( phi );	// Z

	// Update target using head origin
	VectorMA(cl.refdef.vieworg, -cameras[cam]->position[QX], cl.v_forward, cameras[cam]->target);
	VectorMA(cameras[cam]->target, cameras[cam]->position[QY], cl.v_right, cameras[cam]->target);
	VectorMA(cameras[cam]->target, cameras[cam]->position[QZ], cl.v_up, cameras[cam]->target);
	
	// Clip the target
	tr = CamEngine_Trace (cl.refdef.vieworg, cameras[cam]->target, cameras[cam]->radius, MASK_SOLID);
	VectorCopy( tr.endpos, cameras[cam]->target);

	// Smart collision avoidance
	CamEngine_SmartTarget (cam);
}

/*
==============
CamEngine_Zoom
  
Zoom camera in or out
==============
*/
void CamEngine_Zoom (int cam)
{
	// Return if we aren't active
	if (cam != camdevice.GetCurrentCam())
		return;

	// Adjust position
	if (cameras[cam]->zoomable)
	{
		switch (camdevstate.zoommode)
		{
		case CAMZOOM_AUTO_IN:
			cameras[cam]->currentdist -= AUTOZOOM_UPF;
			if (cameras[cam]->currentdist <= cameras[cam]->mindist)
			{
				camdevstate.zoommode = CAMZOOM_NONE;
				cameras[cam]->currentdist = cameras[cam]->mindist;
				camdevice.Activate(QCAMERA0);
			}
			break;
		case CAMZOOM_AUTO_OUT:
			cameras[cam]->currentdist += AUTOZOOM_UPF;
			if (cameras[cam]->currentdist >= cameras[cam]->normdist)
			{
				camdevstate.zoommode = CAMZOOM_NONE;
				cameras[cam]->currentdist = cameras[cam]->normdist;
			}
			break;
		case CAMZOOM_MANUAL_IN:
			cameras[cam]->currentdist -= MANUALZOOM_UPF;
			camdevstate.zoommode = CAMZOOM_NONE;
			if (cameras[cam]->currentdist <= cameras[cam]->mindist)
			{
				cameras[cam]->currentdist = cameras[cam]->mindist;
				camdevice.Activate(QCAMERA0);
			}
			break;
		case CAMZOOM_MANUAL_OUT:
			cameras[cam]->currentdist += MANUALZOOM_UPF;
			camdevstate.zoommode = CAMZOOM_NONE;
			if (cameras[cam]->currentdist >= cameras[cam]->maxdist)
				cameras[cam]->currentdist = cameras[cam]->maxdist;
			break;
		case CAMZOOM_NONE:
		default:
			camdevstate.zoommode = CAMZOOM_NONE;
			break;
		}
	}

	// Adjust magnification
	if (cameras[cam]->magnifiable)
	{
		switch (camdevstate.zoommode)
		{
		case CAMZOOM_AUTO_IN:
			cameras[cam]->magnification += AUTOZOOM_UPF;
			if (cameras[cam]->magnification >= cameras[cam]->normmagnification)
			{
				camdevstate.zoommode = CAMZOOM_NONE;
				cameras[cam]->magnification = cameras[cam]->normmagnification;
			}
			break;
		case CAMZOOM_AUTO_OUT:
			cameras[cam]->magnification -= AUTOZOOM_UPF;
			if (cameras[cam]->magnification <= cameras[cam]->minmagnification)
			{
				camdevstate.zoommode = CAMZOOM_NONE;
				cameras[cam]->magnification = cameras[cam]->minmagnification;
				camdevice.ActivateLast();
			}
			break;
		case CAMZOOM_MANUAL_IN:
			cameras[cam]->magnification += MANUALZOOM_UPF;
			camdevstate.zoommode = CAMZOOM_NONE;
			if (cameras[cam]->magnification >= cameras[cam]->maxmagnification)
				cameras[cam]->magnification = cameras[cam]->maxmagnification;
			break;
		case CAMZOOM_MANUAL_OUT:
			cameras[cam]->magnification -= MANUALZOOM_UPF;
			camdevstate.zoommode = CAMZOOM_NONE;
			if (cameras[cam]->magnification <= cameras[cam]->minmagnification)
				cameras[cam]->magnification = cameras[cam]->minmagnification;
			break;
		case CAMZOOM_NONE:
		default:
			camdevstate.zoommode = CAMZOOM_NONE;
			break;
		}
	}
}
 
/*
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
					Camera Export Functions
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
This section contains the camera export functions.
*/

/*
==============
Cam_InitSystem

Initialize the camera system
==============
*/
void Cam_InitSystem (void)
{
	int i;

	/*		Initialize camera device to system		*/
	camdevice.Activate			= CamSystem_Activate;
	camdevice.ActivateLast		= CamSystem_ActivateLast;
	camdevice.Animate			= CamSystem_AnimForce;
	camdevice.CreateCam			= CamSystem_CreateCam;
	camdevice.GetCurrentCam		= CamSystem_ReturnCam;
	camdevice.SetOption			= CamSystem_SetOption;
	camdevice.GetOption			= CamSystem_GetOption;
	camdevice.ShutdownCamera	= CamSystem_ShutdownCam;
	camdevice.ShutdownCameras	= CamSystem_ShutdownCams;
	camdevice.Think				= CamSystem_Think;
	camdevice.ToChaseCam		= CamSystem_ToChase;
	camdevice.ToEyeCam			= CamSystem_ToEyes;
	camdevice.ZoomIn			= CamSystem_ZoomIn;
	camdevice.ZoomOut			= CamSystem_ZoomOut;
	camdevice.LookAt			= CamSystem_LookAt;

	// Register Cvars
	cl_thirdPerson				= Cvar_Get ( "cl_thirdPerson",		"0",	CVAR_ARCHIVE );
	cl_thirdPersonDist			= Cvar_Get ( "cl_thirdPersonDist",	"40",	CVAR_ARCHIVE );
	cl_thirdPersonAngle			= Cvar_Get ( "cl_thirdPersonAngle", "30",	CVAR_ARCHIVE );
	vid_widescreen				= Cvar_Get ( "vid_widescreen",		"0",	CVAR_ARCHIVE );
	vid_aspectscale				= Cvar_Get ( "vid_aspectscale",		"3",	CVAR_ARCHIVE );
	cl_thirdPersonDamp			= Cvar_Get ( "cl_thirdPersonDamp",	"1",	CVAR_ARCHIVE );

	// Initialize Camera Device State
	camdevstate.ActiveCamera = QCAMERA0;
	for (i = 0; i < MAX_CAMERAS; i++)
		camdevstate.DefinedCameras[i] = false;
	camdevstate.LastCamera = QCAMERA0;
	camdevstate.locked = false;
	camdevstate.maxvertangle = 90;
	camdevstate.viewermodelalpha = 1.0;
	camdevstate.zoommode = CAMZOOM_NONE;

	// Register Plugins
	CamPlugins_RegisterPlugins();

	// Create default cameras, set current camera to first person
	camdevice.CreateCam	(QCAMERA0, CAMTYPE_FIRSTPERSON, NULL);
	camdevice.CreateCam	(QCAMERA1, CAMTYPE_THIRDPERSON, NULL);
	//camdevice.CreateCam	(QCAMERA2, CAMTYPE_ANIMATION, NULL);
	camdevice.Activate	((int)cl_thirdPerson->value);
	
	// Add commands
	Cmd_AddCommand ("camaspect",	Cam_Aspect_Cycle_f);
	Cmd_AddCommand ("camaspectnorm",Cam_Aspect_Norm_f);
	Cmd_AddCommand ("camaspectHDTV",Cam_Aspect_HDTV_f);
	Cmd_AddCommand ("camaspectcin",	Cam_Aspect_Cinema_f);
	Cmd_AddCommand ("camtoggle",	Cam_Toggle_f);
	Cmd_AddCommand ("camzoomin",	Cam_ZoomIn_f);
	Cmd_AddCommand ("camzoomout",	Cam_ZoomOut_f);
	Cmd_AddCommand ("camreset",		Cam_Reset_f);

	camdevstate.Initialized = true;
}

/*
============
Cam_Shutdown

Shutdown camera system
============
*/
void Cam_Shutdown (void) {
	Cmd_RemoveCommand ("camtoggle");
	Cmd_RemoveCommand ("camzoomin");
	Cmd_RemoveCommand ("camzoomout");
	Cmd_RemoveCommand ("camreset");
	Cmd_RemoveCommand ("camaspect");
	Cmd_RemoveCommand ("camaspectnorm");
	Cmd_RemoveCommand ("camaspectHDTV");
	Cmd_RemoveCommand ("camaspectcin");

	// Shutdown cameras
	camdevice.ShutdownCameras();

	camdevstate.Initialized = false;

	// Free the device
	//free ( camdevice );
}

/*
================
Cam_UpdateEntity

Update the player entity
================
*/
void Cam_UpdateEntity (entity_t *ent)
{
	int i;

	// If this entity is the player
	if (ent->flags & RF_VIEWERMODEL) 
	{	
		// Update the player's origin 
		for (i = 0; i < 3 ;i++)
			clientOrg[i] = ent->oldorigin[i] = ent->origin[i] = cl.predicted_origin[i];
		
		// If not a first person camera
		if (!((int)camdevice.GetOption(camdevice.GetCurrentCam(), CAM_TYPE) == CAMTYPE_FIRSTPERSON))
		{
			// Update player model alpha
			CamEngine_AddEntAlpha(ent);
			ent->flags &= ~RF_VIEWERMODEL;
		}
	}
}

/*
||||||||||||||||||||||||||||||||||||||||||||||
				Console Commands
||||||||||||||||||||||||||||||||||||||||||||||
This section contains the console commands.
These only affect the active camera.
*/

/*
==================
Cam_Aspect_Cycle_f

Cycle camera aspects
==================
*/
void Cam_Aspect_Cycle_f (void)
{
	int cam, aspect;
	
	cam = camdevice.GetCurrentCam();

	// Return if aspect can't be set
	if (!camdevice.GetOption(cam, CAMMODE_ASPECT))
		return;

	aspect = (int)camdevice.GetOption(cam, CAM_ASPECT);

	switch (aspect)
	{
	case ASPECT_NORMAL:
		camdevice.SetOption(cam, CAM_ASPECT, (void *)ASPECT_HDTV);
		vid_widescreen->value = 1;
		break;
	case ASPECT_HDTV:
		camdevice.SetOption(cam, CAM_ASPECT, (void *)ASPECT_CINEMA);
		vid_widescreen->value = 2;
		break;
	case ASPECT_CINEMA:
		camdevice.SetOption(cam, CAM_ASPECT, (void *)ASPECT_NORMAL);
		vid_widescreen->value = 0;
		break;
	}
}

/*
=================
Cam_Aspect_Norm_f

Set camera aspect to normal
=================
*/
void Cam_Aspect_Norm_f (void)
{
	int cam = camdevice.GetCurrentCam();

	// Return if aspect can't be set
	if (!camdevice.GetOption(cam, CAMMODE_ASPECT))
		return;

	camdevice.SetOption(cam, CAM_ASPECT, (void *)ASPECT_NORMAL);
	vid_widescreen->value = 0;
}

/*
=================
Cam_Aspect_HDTV_f

Set camera aspect to HDTV
=================
*/
void Cam_Aspect_HDTV_f (void)
{
	int cam = camdevice.GetCurrentCam();

	// Return if aspect can't be set
	if (!camdevice.GetOption(cam, CAMMODE_ASPECT))
		return;

	camdevice.SetOption(cam, CAM_ASPECT, (void *)ASPECT_HDTV);
	vid_widescreen->value = 1;
}

/*
===================
Cam_Aspect_Cinema_f

Set camera aspect to cinematic
===================
*/
void Cam_Aspect_Cinema_f (void)
{
	int cam = camdevice.GetCurrentCam();

	// Return if aspect can't be set
	if (!camdevice.GetOption(cam, CAMMODE_ASPECT))
		return;

	camdevice.SetOption(cam, CAM_ASPECT, (void *)ASPECT_CINEMA);
	vid_widescreen->value = 2;
}

/*
===========
Cam_Reset_f

Reset camera
===========
*/
void Cam_Reset_f (void)
{
	int cam;
	
	if (!camdevstate.Initialized)
		return;
	
	cam = camdevice.GetCurrentCam();

	// Return if locked
	  if (camdevstate.locked)
		  return;

	camdevstate.DefinedCameras[cam] = false;
	camdevice.CreateCam(cam,
		(int)camdevice.GetOption(cam, CAM_TYPE),
		camdevice.GetOption(cam, CAM_FILENAME));
}
  
/*
=================
Cam_Toggle_f
  
Toggle Third Person Camera
=================
*/
  void Cam_Toggle_f (void)
  {
	  int cam = camdevice.GetCurrentCam();

	  // Return if locked
	  if (camdevstate.locked)
		  return;

	  // NeVo - finally fixed this dumb ass bug
	  if ((int)camdevice.GetOption(cam, CAM_TYPE) == CAMTYPE_FIRSTPERSON)
	  {
		  // We are in first person, so switch to chase camera
		  camdevice.ToChaseCam();
		  return;
	  }
	  else if ((int)camdevice.GetOption(cam, CAM_TYPE) == CAMTYPE_THIRDPERSON)
	  {
		  // We are a chase camera, so switch to first person
		  camdevice.ToEyeCam();
		  return;
	  }
	  else
	  {
		  // Otherwise just switch to first person
		  camdevice.Activate(QCAMERA0);
		  return;
	  }
  }
  
/*
=================
Cam_ZoomIn_f
  
Zoom camera in
=================
*/
  void Cam_ZoomIn_f (void) {
	  // Return if locked
	  if (camdevstate.locked)
		  return;
	  camdevice.ZoomIn ();
  }
  
/*
==================
Cam_ZoomOut_f
  
Zoom camera out
==================
*/
  void Cam_ZoomOut_f (void) {
	  // Return if locked
	  if (camdevstate.locked)
		  return;
	  camdevice.ZoomOut ();
  }
