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
#include <float.h>
#include "../ref_dx/dx_local.h"
#include "dxw_win.h"

// Direct3D9
LPDIRECT3D9				d3dObj9;
LPDIRECT3DDEVICE9		d3dDev9;
LPDIRECT3DVERTEXBUFFER9	d3dvb;
D3DPRESENT_PARAMETERS	d3dpp;
D3DVIEWPORT9			d3dView;

// Matrices
LPD3DXMATRIXSTACK		matStack;
D3DXMATRIX				WorldMatrix;
D3DXMATRIX				ProjectionMatrix;
D3DXMATRIX				ViewMatrix;

// Materials
D3DMATERIAL9			waterMat;
D3DMATERIAL9			glassMat;
D3DMATERIAL9			skyMat;
D3DMATERIAL9			concreteMat;
D3DMATERIAL9			plasmaMat;
D3DMATERIAL9			creatureMat;
D3DMATERIAL9			defaultMat;

// Water Material //
//qglColor4f ( 1 - 0.3333, 1 - 0.4511, 1 - 0.5451, 0.75);	// NeVo - made color match murky fog
// This is our murky color
waterMat.Diffuse.r = 1.0f - 0.3333f;
waterMat.Diffuse.g = 1.0f - 0.4511f;
waterMat.Diffuse.b = 1.0f - 0.5451f;
waterMat.Diffuse.a = 0.76f;
// Match the murky color
waterMat.Ambient.r = 1.0f - 0.3333f;
waterMat.Ambient.g = 1.0f - 0.4511f;
waterMat.Ambient.b = 1.0f - 0.5451f;
waterMat.Ambient.a = 1.0f;
// Bright specular highlights... roughly proportional per channel
waterMat.Specular.r = 1.0f;
waterMat.Specular.g = 0.9f;
waterMat.Specular.b = 0.8f;
waterMat.Specular.a = 1.0f;
waterMat.Power = 50.0f;			// This controls the specular intensity
// We want our water to look contaminated... let it emit a low, low light
waterMat.Emissive.r = 1.0f - 0.3333f;
waterMat.Emissive.g = 1.0f - 0.4511f;
waterMat.Emissive.b = 1.0f - 0.5451f;
waterMat.Emissive.a = 0.17f;	// This ends up controlling the intenstiy of the emitted light

// Glass Material //
// Blue-Green Glass, 60% see through
glassMat.Diffuse.r = 0.2f;
glassMat.Diffuse.g = 0.6f;
glassMat.Diffuse.b = 0.9f;
glassMat.Diffuse.a = 0.40f;
// Set the RGBA for ambient reflection.
glassMat.Ambient.r = 0.5f;
glassMat.Ambient.g = 0.0f;
glassMat.Ambient.b = 0.5f;
glassMat.Ambient.a = 1.0f;
// Set the color and sharpness of specular highlights.
glassMat.Specular.r = 1.0f;
glassMat.Specular.g = 1.0f;
glassMat.Specular.b = 1.0f;
glassMat.Specular.a = 0.7f;
glassMat.Power = 30.0f;
// Set the RGBA for emissive color.
glassMat.Emissive.r = 0.0f;
glassMat.Emissive.g = 0.0f;
glassMat.Emissive.b = 0.0f;
glassMat.Emissive.a = 0.0f;

// Sky Material //
// The sky is red-orange
skyMat.Diffuse.r = 0.8f;
skyMat.Diffuse.g = 0.1f;
skyMat.Diffuse.b = 0.1f;
skyMat.Diffuse.a = 1.0f;	// Cant see through the skybox
// Set the RGBA for ambient reflection.
skyMat.Ambient.r = 0.5f;
skyMat.Ambient.g = 0.5f;
skyMat.Ambient.b = 0.5f;
skyMat.Ambient.a = 1.0f;
// Don't hilite the sky
skyMat.Specular.r = 0.0f;
skyMat.Specular.g = 0.0f;
skyMat.Specular.b = 0.0f;
skyMat.Specular.a = 0.0f;
skyMat.Power = 0.0f;
// Slight red emissive
skyMat.Emissive.r = 0.2f;
skyMat.Emissive.g = 0.0f;
skyMat.Emissive.b = 0.0f;
skyMat.Emissive.a = 0.1f;

// Concrete Material //
// Dull Gray
concreteMat.Diffuse.r = 0.5f;
concreteMat.Diffuse.g = 0.5f;
concreteMat.Diffuse.b = 0.5f;
concreteMat.Diffuse.a = 0.5f;
// Set the RGBA for ambient reflection.
concreteMat.Ambient.r = 0.5f;
concreteMat.Ambient.g = 0.5f;
concreteMat.Ambient.b = 0.5f;
concreteMat.Ambient.a = 0.5f;
// No emission or reflectivity
concreteMat.Specular.r = 0.0f;
concreteMat.Specular.g = 0.0f;
concreteMat.Specular.b = 0.0f;
concreteMat.Specular.a = 0.0f;
concreteMat.Power = 0.0f;
concreteMat.Emissive.r = 0.0f;
concreteMat.Emissive.g = 0.0f;
concreteMat.Emissive.b = 0.0f;
concreteMat.Emissive.a = 0.0f;

// Plasma Material //
// Near white
plasmaMat.Diffuse.r = 0.9f;
plasmaMat.Diffuse.g = 0.9f;
plasmaMat.Diffuse.b = 0.9f;
plasmaMat.Diffuse.a = 0.9f;
// Near white
plasmaMat.Ambient.r = 0.9f;
plasmaMat.Ambient.g = 0.9f;
plasmaMat.Ambient.b = 0.9f;
plasmaMat.Ambient.a = 0.9f;
// Strong specular // FIXME: this is emitting... should it be shiny?
plasmaMat.Specular.r = 1.0f;
plasmaMat.Specular.g = 1.0f;
plasmaMat.Specular.b = 1.0f;
plasmaMat.Specular.a = 1.0f;
plasmaMat.Power = 50.0f;
// Strong white emission
plasmaMat.Emissive.r = 0.9f;
plasmaMat.Emissive.g = 0.9f;
plasmaMat.Emissive.b = 0.9f;
plasmaMat.Emissive.a = 0.9f;

// Creature Material //
// Not sure what color this makes...
creatureMat.Diffuse.r = 0.7f;
creatureMat.Diffuse.g = 0.6f;
creatureMat.Diffuse.b = 0.6f;
creatureMat.Diffuse.a = 0.6f;
// Set the RGBA for ambient reflection.
creatureMat.Ambient.r = 0.7f;
creatureMat.Ambient.g = 0.4f;
creatureMat.Ambient.b = 0.5f;
creatureMat.Ambient.a = 0.5f;
// Set the color and sharpness of specular highlights.
creatureMat.Specular.r = 1.0f;
creatureMat.Specular.g = 1.0f;
creatureMat.Specular.b = 1.0f;
creatureMat.Specular.a = 1.0f;
creatureMat.Power = 1.0f;	// Weak reflectivity
// No emission
creatureMat.Emissive.r = 0.0f;
creatureMat.Emissive.g = 0.0f;
creatureMat.Emissive.b = 0.0f;
creatureMat.Emissive.a = 0.0f;

// Default Material //
// Set the RGBA for diffuse reflection.
defaultMat.Diffuse.r = 0.9f;
defaultMat.Diffuse.g = 0.9f;
defaultMat.Diffuse.b = 0.9f;
defaultMat.Diffuse.a = 0.3f;
// Set the RGBA for ambient reflection.
defaultMat.Ambient.r = 0.9f;
defaultMat.Ambient.g = 0.9f;
defaultMat.Ambient.b = 0.9f;
defaultMat.Ambient.a = 0.3f;
// Set the color and sharpness of specular highlights.
defaultMat.Specular.r = 1.0f;
defaultMat.Specular.g = 1.0f;
defaultMat.Specular.b = 1.0f;
defaultMat.Specular.a = 1.0f;
defaultMat.Power = 1.0f;
// Set the RGBA for emissive color.
defaultMat.Emissive.r = 0.0f;
defaultMat.Emissive.g = 0.0f;
defaultMat.Emissive.b = 0.0f;
defaultMat.Emissive.a = 0.0f;

// Our texture stages
typedef enum QSTAGES_s
{
	QSTAGE0,
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
#define Q_FVF (D3DFVF_XYZ|D3DFVF_TEX1|D3DFVF_DIFFUSE)

typedef struct QVERTEX_s
{
    FLOAT	x, y, z;	// vertex
    FLOAT   tu, tv;		// texcoords
	DWORD	diffuse;	// diffuse color
} QVERTEX;



