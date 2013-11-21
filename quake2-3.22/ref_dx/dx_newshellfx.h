/*
Copyright (C) 1997-2003 Bleeding Eye Studio, Bryan Miller.

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


//		--==OBSIDIAN UPDATE==--
/*--------------------------------------------------
New Shell FX for the openGL renderer of Quake2
By: Obsidian


This header contains a set of functions which will generate
data to be used in the rendering of new and various shell
effects for Quake2.
--------------------------------------------------*/


#include "dx_local.h"
//#include "../qcommon/qcommon.h"

#ifndef _GL_NEWSHELLFX_H_
#define _GL_NEWSHELLFX_H_


#define MAX_SHELL_TYPES 5
#define MAX_SHELL_STAGES 8
#define MAX_ACTIVE_SHELLS 129


typedef struct _shellStage{
	float Wait;

	float StartSize;
	float EndSize;
	float SizeDivisor;

	float StartAlpha;
	float EndAlpha;
	float AlphaDivisor;
}t_shellStage;


typedef struct _shellType{
	t_shellStage Stage[MAX_SHELL_STAGES];
	int NumStages;
	int StartStage;
	int Loaded;
}t_shellType;

typedef struct _shellID{
	char name[MAX_QPATH];
	float size;
	float alpha;
	float fading;
	int Stage;
	int Type;
}t_shellID;


t_shellType ShellType[MAX_SHELL_TYPES];
static int ShellTypesSet = 0;
t_shellID ActiveShells[MAX_ACTIVE_SHELLS];
static int NumActiveShells = 0;


//These five functions are helper functions and should not be used by the outside world.
int determineShellType(const int flags);	//return values and their meanings...
											//0 = God Shell
											//1 = Combo Invulnerability and Quad Damage Shell
											//2 = Invulnerability Shell
											//3 = Quad Damage Shell
											//4 = Unknown (Generic)


int activateShell(char *name);

//This should be the ONLY function called from the outside world. This does all the
//work needed.
void generateShellValues(int ShellIndex, float *size, float *alpha, float *fading, const int flags);




#endif //_GL_NEWSHELLFX_H_




