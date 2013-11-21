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
// winquake.h: Win32-specific Quake header file

#include <windows.h>

#include <dsound.h>

/*
NeVo
This is a hack to get our DirectX components to
eat the IID's, since the Microsoft compiler is dumb and
doesn't realize that IID's and GUID's are the same
*/
typedef const IID * qIID;

/*
NeVo
Intel compiler is a lot more strict than the Microsoft compiler...
It properly recognizes COM objects. Since the Microsoft compiler
is dumb, we have a #define switch for the two compilers. THere
is a similar sort of thing in ref_soft.
// TODO: Since the Intel compiler works properly, we could use advanced 3D positional sound buffers
*/
#define QDSNDCOMPILERHACK

#define	WINDOW_STYLE	(WS_OVERLAPPED|WS_BORDER|WS_CAPTION|WS_VISIBLE)

extern	HINSTANCE	global_hInstance;

extern DWORD gSndBufSize;

extern HWND			cl_hwnd;
extern int		ActiveApp, Minimized;

void IN_Activate (int active);
void IN_MouseEvent (int mstate);

extern int		window_center_x, window_center_y;
extern RECT		window_rect;
