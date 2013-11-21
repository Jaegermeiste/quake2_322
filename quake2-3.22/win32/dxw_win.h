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
#ifndef _WIN32
#  error You should not be including this file on this platform
#endif

#ifndef __DXW_WIN_H__
#define __DXW_WIN_H__

#include <d3d9.h>
#include <d3dx9.h>

typedef enum dx_vertexmode_s
{
	DX_HARDWARE = 0,
	DX_MIXED,
	DX_SOFTWARE,
	DX_HWPUREDEVICE,
	DX_MAX_VERTEXMODES
} dx_vertexmode_t;

typedef struct winstate_s
{
	HINSTANCE	hInstance;
	void		*wndproc;
	HDC			hDC;			// handle to device context
	HWND		hWnd;			// handle to window
} winstate_t;

extern winstate_t winstate;

// Direct3D9
LPDIRECT3D9				d3dObj9;
LPDIRECT3DDEVICE9		d3dDev9;
LPDIRECT3DVERTEXBUFFER9	d3dvb;
LPDIRECT3DVERTEXBUFFER9	d3dvbQuad;	// A static buffer for Quads
LPDIRECT3DVERTEXBUFFER9	d3dvbTris;	// A static buffer for Triangles
D3DPRESENT_PARAMETERS	d3dpp;
D3DVIEWPORT9			d3dView;

// Matrices
LPD3DXMATRIXSTACK		matStack;
D3DXMATRIX				WorldMatrix;
D3DXMATRIX				ProjectionMatrix;
D3DXMATRIX				ViewMatrix;

// Our texture stages
typedef enum QSTAGES_s
{
	QSTAGE0 = 0,
	QSTAGE1,
	QSTAGE2,
	QSTAGE3,
	QSTAGE4,
	QSTAGE5,
	QSTAGE6,
	QSTAGE7,
	MAXSTAGES	// Must be 8 for DX9
} QSTAGES;

// Our FVF
// FIXME: Add specular?
#define Q_FVF (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1|D3DFVF_DIFFUSE)

typedef struct QVERTEX_s
{
    FLOAT	x, y, z;	// vertex
	FLOAT	nx, ny, nz;	// normal
    FLOAT   tu, tv;		// texcoords
	DWORD	diffuse;	// diffuse color
} QVERTEX;

#endif
