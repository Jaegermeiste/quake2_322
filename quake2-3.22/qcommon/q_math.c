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
/* q_math.c */

#include "../game/q_shared.h"

/*
***********************************
			QUATERNIONS
***********************************
*/

void AnglesToQuat (const vec3_t angles, vec4_t quat)
{
	vec3_t a;
	float cr, cp, cy, sr, sp, sy, cpcy, spsy;

	a[PITCH] = (M_PI/360.0) * angles[PITCH];
	a[YAW] = (M_PI/360.0) * angles[YAW];
	a[ROLL] = (M_PI/360.0) * angles[ROLL];

	cr = cos(a[ROLL]);
	cp = cos(a[PITCH]);
	cy = cos(a[YAW]);

	sr = sin(a[ROLL]);
	sp = sin(a[PITCH]);
	sy = sin(a[YAW]);

	cpcy = cp * cy;
	spsy = sp * sy;
	quat[0] = cr * cpcy + sr * spsy; // w
	quat[1] = sr * cpcy - cr * spsy; // x
	quat[2] = cr * sp * cy + sr * cp * sy; // y
	quat[3] = cr * cp * sy - sr * sp * cy; // z
}

void QuatToAxis(vec4_t q, vec3_t axis[3])
{
	float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

	x2 = q[1] + q[1];
	y2 = q[2] + q[2];
	z2 = q[3] + q[3];
	xx = q[1] * x2;
	xy = q[1] * y2;
	xz = q[1] * z2;
	yy = q[2] * y2;
	yz = q[2] * z2;
	zz = q[3] * z2;
	wx = q[0] * x2;
	wy = q[0] * y2;
	wz = q[0] * z2;

	axis[0][0] = 1.0 - (yy + zz);
	axis[1][0] = xy - wz;
	axis[2][0] = xz + wy;

	axis[0][1] = xy + wz;
	axis[1][1] = 1.0 - (xx + zz);
	axis[2][1] = yz - wx;

	axis[0][2] = xz - wy;
	axis[1][2] = yz + wx;
	axis[2][2] = 1.0 - (xx + yy);
}

void QuatMul(const vec4_t q1, const vec4_t q2, vec4_t res)
{
	float A, B, C, D, E, F, G, H;

	A = (q1[0] + q1[1])*(q2[0] + q2[1]);
	B = (q1[3] - q1[2])*(q2[2] - q2[3]);
	C = (q1[0] - q1[1])*(q2[2] + q2[3]);
	D = (q1[2] + q1[3])*(q2[0] - q2[1]);
	E = (q1[1] + q1[3])*(q2[1] + q2[2]);
	F = (q1[1] - q1[3])*(q2[1] - q2[2]);
	G = (q1[0] + q1[2])*(q2[0] - q2[3]);
	H = (q1[0] - q1[2])*(q2[0] + q2[3]);

	res[0] = B + (H - E - F + G)*0.5;
	res[1] = A - (E + F + G + H)*0.5;
	res[2] = C + (E - F + G - H)*0.5;
	res[3] = D + (E - F - G + H)*0.5;
}

void QuatToAngles (const vec4_t q, vec3_t a)
{
	vec4_t q2;
	q2[0] = q[0]*q[0];
	q2[1] = q[1]*q[1];
	q2[2] = q[2]*q[2];
	q2[3] = q[3]*q[3];
	a[ROLL] = (180.0/M_PI)*atan2 (2*(q[2]*q[3] + q[1]*q[0]) , (-q2[1] - q2[2] + q2[3] + q2[0]));
	a[PITCH] = (180.0/M_PI)*asin (-2*(q[1]*q[3] - q[2]*q[0]));
	a[YAW] = (180.0/M_PI)*atan2 (2*(q[1]*q[2] + q[3]*q[0]) , (q2[1] - q2[2] - q2[3] + q2[0]));
}
