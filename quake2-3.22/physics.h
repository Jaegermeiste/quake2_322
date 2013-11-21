/*
*/

/*
physics.h

NeVo - define the physical propeties of an object
that we use to accurately model its behavior
in the world

We use SI units for our calculations
*/

/*
These are standard values used when the physics engine is initialized.
*/
#define SI_GRAVITYACCEL			9.80665				// m/s^2
#define SI_GRAVITATIONALCONST	0.0000667259		// ml/(kg * s^2)
#define SI_SPEEDOFLIGHT			299792458			// m/s

/*
Some general math stuff
*/
#define	LN_2					0.6931472
#define LN_10					2.3025851
#define LOG_10_E				0.4342945
#define E						2.7182818
#define RAD						57.2957795		// degrees

/*
Some Macros
*/
#define density(a,b)			a/(b+0.00001)	// prevent div by zero
#define displacement(a)			VectorSubtract(a->origin, a->oldorigin, a->velocity)
#define velocity(a)				VectorSet(a->velocity, a->displacement[0]/FRAMETIME, a->displacement[1]/FRAMETIME, a->displacement[2]/FRAMETIME)
#define calcvelocity(a)			displacement(a);	velocity(a)

/*
These values are set when the engine is initialized. We do not
directly use the defines since we want to be able to adjust the
parameters... (eg. we are rarely on Earth.)
*/
typedef struct pstate_s
{
	float	gravity;
	float	airviscosity;
	float	waterviscosity;
	float	lavaviscosity;
	float	slimeviscosity;
} pstate_t;

// Physics Model //
typedef struct pmodel_s
{
	float	mass;			// in kg
	float	temp;			// in celsius
	vec3_t	lwh;			// in meters
	vec3_t	origin;			// current origin
	vec3_t	oldorigin;		// last origin
	vec3_t	displacement;	// displacement in each direction
	int		flags;			// computation flags
} pmodel_t;