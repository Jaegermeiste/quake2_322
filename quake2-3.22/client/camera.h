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

/*
================================
camera.h
Psychospaz's 3rd Person Chasecam
Enhanced by NeVo
================================
*/

#ifndef __CAMERA_H__	// Protect camera.h
#define __CAMERA_H__

#define CAMERA_VERSION  5.3

// Cameras
typedef enum camera_e {
	QCAMERA0,				// First Person Camera
	QCAMERA1,				// Third Person Chase Camera
	QCAMERA2,				// Animations Camera
	QCAMERA3,
	QCAMERA4,
	QCAMERA5,
	QCAMERA6,
	QCAMERA7,
	MAX_CAMERAS				// Maximum number of cameras
} camera_t;

typedef enum camtype_e {
	CAMTYPE_NONE,			// No type (unsupported)
	CAMTYPE_FIRSTPERSON,	// First person camera (through eyes)
	CAMTYPE_THIRDPERSON,	// Chase camera (from behind)
	CAMTYPE_ANIMATION,		// Animated camera
	CAMTYPE_BINOCULAR,		// Binoculars
	CAMTYPE_SCOPE,			// Scope
	CAMTYPE_SECURITY		// Security camera
} camtype_t;

typedef enum camlens_e {
	CAMLENS_DEFAULT,		// Normal lens
	CAMLENS_TELESCOPIC,		// Telescopic lens
	CAMLENS_WIDEANGLE,		// Wide Angle lens
	CAMLENS_BINOCULAR,		// Two lenses
	CAMLENS_PANORAMIC,		// Panoramic lens
	CAMLENS_FISHEYE			// Fish eye lens
} camlens_t;

typedef enum camdispopt_e {
	CAMDISPOPT_NONE,		// No coating
	CAMDISPOPT_IR,			// Draw the scene as infrared
	CAMDISPOPT_UV,			// Draw the scene as ultraviolet

	// The following functions require renderer support
	CAMDISPOPT_INKED,		// Draw the scene as inked (on white)
	CAMDISPOPT_INKONPAPER,	// Draw the scene as ink on paper (cream color, with texture)
	CAMDISPOPT_BLUEPRINT,	// Draw the scene like a blueprint
	CAMDISPOPT_NPR,			// Draw the scene NPR-style (black lines on white)
	CAMDISPOPT_INVNPR,		// Draw the scene as inverse NPR (white lines on black)
	CAMDISPOPT_CELSHADED,	// Draw the scene as cel-shaded
	CAMDISPOPT_FISHEYE,		// Draw the scene as if thru a fisheye lens
	CAMDISPOPT_GREYSCALE,	// Draw the scene greyscale
	CAMDISPOPT_WIREFRAME	// Draw the scene wireframe
} camdispopt_t;

typedef enum camanim_e {
	CAMANIM_NONE,			// No animation
	CAMANIM_SPAWN,			// Spawn animation
	CAMANIM_DEATH,			// Death animation
	CAMANIM_FALLING			// Falling animation
} camanim_t;

typedef enum camzoom_e {
	CAMZOOM_NONE,			// No zoom
	CAMZOOM_AUTO_IN,		// Auto zoom out
	CAMZOOM_AUTO_OUT,		// Auto zoom in
	CAMZOOM_MANUAL_IN,		// Manual zoom in
	CAMZOOM_MANUAL_OUT		// Manual zoom out
} camzoom_t;

typedef enum camaspect_e
{
	ASPECT_NONE,			// Unsupported
	ASPECT_NORMAL,			// TV, Normal Computer Monitors
	ASPECT_HDTV,			// HDTV's
	ASPECT_CINEMA			// Cinema Aspect
} camaspect_t;

// Camera parameter list
typedef enum camopt_e {
	CAM_NONE,				// Unsupported
	CAM_TYPE,				// Camera type
	CAM_VERSION,			// Camera Version
	CAM_LENS,				// Lens type
	CAM_DISPOPT,			// Render Flags
	CAM_ANIMATION,			// Animation type
	CAM_ZOOM,				// Zoom mode
	CAM_FILENAME,			// Camera's filename
	CAM_ASPECT,				// Camera's aspect ratio
	CAMMODE_DAMPED,			// Spring damped motion
	CAMMODE_SMART,			// Collision avoidance
	CAMMODE_CLIP,			// Collision detection
	CAMMODE_ALPHA,			// Viewer model alpha blending
	CAMMODE_FOCUS,			// Field of depth adjustment (Scopes and Binoculars)
	CAMMODE_DRIFT,			// Camera drifts slightly
	CAMMODE_ZOOM,			// Camera can be zoomed
	CAMMODE_MAGNIFY,		// Camera can be magnified
	CAMMODE_FLARE,			// Camera gets lens flares
	CAMMODE_KICK,			// Camera is affected by weapon recoil
	CAMMODE_JITTER,			// Camera is jittery like in a rocket launch
	CAMMODE_ASPECT			// Camera's aspect can be set
} camopt_t;

// Our plugins interface
typedef struct camplugins_s
{
	// Effects
	void (*Fisheye)			(void);
	void (*InkOnWhite)		(void);
	void (*InkOnPaper)		(void);
	void (*Blueprint)		(void);
	void (*NPR)				(void);
	void (*InvNPR)			(void);
	void (*CelShade)		(void);
	void (*Grayscale)		(void);
	void (*DepthOfField)	(void);
	void (*LensFlare)		(void);
	void (*WireFrame)		(void);

	// Animations
	void (*AnimDeath)		(void);
	void (*AnimFalling)		(void);
	void (*AnimSpawn)		(void);

	// Clear Settings
	void (*Clear)			(void);
} camplugins_t;

// Our interface
typedef struct camdevice_s
{
	void (*Activate)		(int cam);
	void (*ActivateLast)	(void);
	void (*Animate)			(int anim);
	void (*CreateCam)		(int cam, int type, char *filename);
	int  (*GetCurrentCam)	(void);
	void *(*GetOption)		(int cam, int mode);
	void (*LookAt)			(int cam, vec3_t lookat);
	void (*SetOption)		(int cam, int mode, void *option);
	void (*ShutdownCamera)	(int cam);
	void (*ShutdownCameras)	(void);			// Shut down cameras
	void (*Think)			(int updateextras);

	// These functions are provided due to common use
	void (*ZoomIn)			(void);			// Zoom in active camera
	void (*ZoomOut)			(void);			// Zoom out active camera
	void (*ToEyeCam)		(void);			// Switch to first person camera
	void (*ToChaseCam)		(void);			// Switch to chase camera
} camdevice_t;

// Device Properties
typedef struct camdevstate_s {
	int	DefinedCameras[MAX_CAMERAS];	// Array of defined cameras
	camera_t	ActiveCamera;					// Currently active camera
	camera_t	LastCamera;						// Last Active Camera
	float		viewermodelalpha;				// Player model transparency
	double		maxvertangle;					// Maximum vertical angle
	camzoom_t	zoommode;						// Current zoom mode
	int	locked;							// User can't adjust the camera
	int	Initialized;					// System has been initialized
	float		fov;							// Current FOV
	unsigned int oldtime;						// Old camera time
} camdevstate_t;

// Camera Properties
typedef struct camstate_s {

	// Camera Information
	camera_t	name;			// This camera's identifier
	camtype_t	type;			// Camera type
	float		version;		// Camera version
	camlens_t	lens;			// Lens type
	camdispopt_t dispopt;		// Lens coating
	camanim_t	animMode;		// Animation mode
	camaspect_t	aspect;			// Aspect Ratio 
	int	damped;			// Spring damped motion
	int	smart;			// Collision avoidance
	int	clip;			// Collision detection
	int	alpha;			// Viewer model alpha blending
	int	focus;			// Field of depth adjustment (Scopes and Binoculars)
	int	drift;			// Camera randomly drifts
	int	zoomable;		// Camera is zoomable
	int	magnifiable;	// Camera can be magnified
	int	flare;			// Camera gets lens flares
	int	kickable;		// Camera can be kicked
	int	jitter;			// Camera is jittery (like a rocket launch)
	int	aspectok;		// Camera has settable aspect ratio
	
	// World Camera Position
	vec3_t		origin;			// This camera's current origin (where it is)

	// Camera Target Positions
	vec3_t		target;			// This camera's destination origin (where it wants to be)
	vec3_t		oldTarget;		// Last target

	// Player Relative Camera Position
	vec3_t		position;		// Camera's position relative to player
	
	// Angular Camera Position
	vec3_t		angpos;			// Euler angular position

	// View angles
	vec3_t		viewangle;		// Euler view angle

	// Origin to look at
	vec3_t		lookat;			// Origin to look at

	// Trailing Distance Parameters
	double		mindist;		// Minimum distance from vieworg
	double		normdist;		// Normal distance from vieworg
	double		maxdist;		// Maximim distance from vieworg
	double		currentdist;	// Current distance from vieworg

	// Motion Damping
	float		tether;			// Tether distance

	// Collision Detection and Avoidance
	float		radius;			// This camera's virtual radius
    
	// Camera Definitions File
	char		*filename;		// Name of camera definition file

	// Model Transparency
	float		maxalphadist;	// Maximum distance to calculate alpha

	// Magnification
	float		magnification;
	float		maxmagnification;
	float		minmagnification;
	float		normmagnification;

	// Field of View
	float		fov;
} camstate_t;

#define MASK_CAMERA		(CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEADMONSTER|CONTENTS_PLAYERCLIP)
#define LOOKAT_SCALE	8000
#define	AUTOZOOM_UPF	5
#define MANUALZOOM_UPF	3
#define DRIFT_FACTOR	4
#define JITTER_FACTOR	4
#define VELOCITY_DAMP	1.0		//0.65
#define TELEPORT_DIST	256*8
#define MAX_FOV			179
#define MIN_FOV			1
#define CAMLERP_FRAC	0.05
#define CAM_PIXELLERP	1
#define SPRING_CONST	2.5f //4.5f //1.380658 // E^23
#define DAMP_CONST		0.5f //3.5f
#define QX				0
#define QY				1
#define QZ				2

////////////////////////////////////////
// Exported Camera Data and Functions //
////////////////////////////////////////

// Camera Device
extern	camdevice_t	camdevice;

// Plugins Manager
extern	camplugins_t camplugins;

// Cvars
extern	cvar_t	*cl_thirdPerson;
extern	cvar_t	*cl_thirdPersonAngle;
extern	cvar_t	*cl_thirdPersonDist;
extern	cvar_t	*vid_widescreen;	// aspect ratio support
extern	cvar_t	*vid_aspectscale;	// auto FOV increase for widescreen aspects
extern	cvar_t	*cl_thirdPersonDamp;

// Client Origin
vec3_t clientOrg; // Lerped origin of client for server->client side effects

// Camera System Functions
void Cam_InitSystem (void);
void Cam_Shutdown (void);

// Other Camera Related functions
void Cam_UpdateEntity (entity_t *ent);

// Console Commands (affect the current camera)
void Cam_Aspect_Cycle_f (void);
void Cam_Aspect_Norm_f (void);
void Cam_Aspect_HDTV_f (void);
void Cam_Aspect_Cinema_f (void);
void Cam_Reset_f (void);
void Cam_Toggle_f (void);
void Cam_ZoomIn_f (void);
void Cam_ZoomOut_f (void);
//====================================//

////////////////////////////
// Model and Skin Bugfixes//
////////////////////////////
extern	cvar_t	*gender;
extern	cvar_t	*model;
extern	cvar_t	*skin;
extern  cvar_t  *fov;
//========================//

#endif	// __CAMERA_H__