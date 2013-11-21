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
** GLW_IMP.C
**
** This file contains ALL Win32 specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_Shutdown
** GLimp_SwitchFullscreen
**
*/
#include <assert.h>
#include <windows.h>
#include "../ref_dx/dx_local.h"
#include "dxw_win.h"
#include "winquake.h"

static qboolean DXimp_SwitchFullscreen( int width, int height );
qboolean DXimp_InitDX (void);

// MrG - BeefQuake - hardware gammaramp
WORD original_ramp[3][256];
WORD gamma_ramp[3][256];

winstate_t winstate;

extern cvar_t *vid_fullscreen;
extern cvar_t *vid_ref;

/*
** VID_CreateWindow
*/
#define	WINDOW_CLASS_NAME	"Quake II"	// NeVo

qboolean VID_CreateWindow( int width, int height, qboolean fullscreen )
{
	WNDCLASS		wc;
	RECT			r;
	cvar_t			*vid_xpos, *vid_ypos;
	int				stylebits;
	int				x, y, w, h;
	int				exstyle;
	BYTE ANDmaskCursor[] = {0xFF};	// NeVo - Hidden Cursor: One
	BYTE XORmaskCursor[] = {0x00};	// NeVo - Hidden Cursor: Zero

	/* Register the frame class */
    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC)winstate.wndproc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = winstate.hInstance;
    wc.hIcon         = 0;
    //wc.hCursor       = LoadCursor (winstate.hInstance, IDC_ARROW);
	// NeVo - get rid of windows cursor (transparent)
	wc.hCursor		 = CreateCursor (winstate.hInstance, 0, 0, 1, 1, ANDmaskCursor, XORmaskCursor);
	wc.hbrBackground = (void *)COLOR_GRAYTEXT;
    wc.lpszMenuName  = 0;
    wc.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClass (&wc) )
		ri.Sys_Error (ERR_FATAL, "Couldn't register window class");

	if (fullscreen)
	{
		exstyle = WS_EX_TOPMOST;
		stylebits = WS_POPUP|WS_VISIBLE;
	}
	else
	{
		exstyle = 0;
		stylebits = WINDOW_STYLE;
	}

	r.left = 0;
	r.top = 0;
	r.right  = width;
	r.bottom = height;

	AdjustWindowRect (&r, stylebits, FALSE);

	w = r.right - r.left;
	h = r.bottom - r.top;

	if (fullscreen)
	{
		x = 0;
		y = 0;
	}
	else
	{
		vid_xpos = ri.Cvar_Get ("vid_xpos", "0", 0);
		vid_ypos = ri.Cvar_Get ("vid_ypos", "0", 0);
		x = vid_xpos->value;
		y = vid_ypos->value;
	}

	winstate.hWnd = CreateWindowEx (
		 exstyle, 
		 WINDOW_CLASS_NAME,
		 "Quake II",
		 stylebits,
		 x, y, w, h,
		 NULL,
		 NULL,
		 winstate.hInstance,
		 NULL);

	if (!winstate.hWnd)
		ri.Sys_Error (ERR_FATAL, "Couldn't create window");
	
	ShowWindow( winstate.hWnd, SW_SHOW );
	UpdateWindow( winstate.hWnd );

	// init all the dx stuff for the window
	if (!DXimp_InitDX ())
	{
		ri.Con_Printf( PRINT_ALL, "VID_CreateWindow() - DXimp_InitDX failed\n");
		return false;
	}

	SetForegroundWindow( winstate.hWnd );
	SetFocus( winstate.hWnd );

	// let the sound and input subsystems know about the new window
	ri.Vid_NewWindow (width, height);

	return true;
}


/*
** GLimp_SetMode
*/
rserr_t DXimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
{
	int width, height;
	const char *win_fs[] = { "W", "FS" };

	ri.Con_Printf( PRINT_ALL, "Initializing Direct3D 9 Display\n");

	ri.Con_Printf (PRINT_ALL, "...setting mode %d:", mode );

	if ( !ri.Vid_GetModeInfo( &width, &height, mode ) )
	{
		ri.Con_Printf( PRINT_ALL, " invalid mode\n" );
		return rserr_invalid_mode;
	}

	ri.Con_Printf( PRINT_ALL, " %d %d %s\n", width, height, win_fs[fullscreen] );

	// destroy the existing window
	if (winstate.hWnd)
	{
		DXimp_Shutdown ();
	}

	// do a CDS if needed
	if ( fullscreen )
	{
		DEVMODE dm;

		ri.Con_Printf( PRINT_ALL, "...attempting fullscreen\n" );

		memset( &dm, 0, sizeof( dm ) );

		dm.dmSize = sizeof( dm );

		dm.dmPelsWidth  = width;
		dm.dmPelsHeight = height;
		dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;

		if ( gl_bitdepth->value != 0 )
		{
			dm.dmBitsPerPel = gl_bitdepth->value;
			dm.dmFields |= DM_BITSPERPEL;
			ri.Con_Printf( PRINT_ALL, "...using gl_bitdepth of %d\n", ( int ) gl_bitdepth->value );
		}
		else
		{
			HDC hdc = GetDC( NULL );
			int bitspixel = GetDeviceCaps( hdc, BITSPIXEL );

			ri.Con_Printf( PRINT_ALL, "...using desktop display depth of %d\n", bitspixel );

			ReleaseDC( 0, hdc );
		}

		ri.Con_Printf( PRINT_ALL, "...calling CDS: " );
		if ( ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) == DISP_CHANGE_SUCCESSFUL )
		{
			*pwidth = width;
			*pheight = height;

			dx_state.fullscreen = true;

			ri.Con_Printf( PRINT_ALL, "ok\n" );

			if ( !VID_CreateWindow (width, height, true) )
				return rserr_invalid_mode;

			return rserr_ok;
		}
		else
		{
			*pwidth = width;
			*pheight = height;

			ri.Con_Printf( PRINT_ALL, "failed\n" );

			ri.Con_Printf( PRINT_ALL, "...calling CDS assuming dual monitors:" );

			dm.dmPelsWidth = width * 2;
			dm.dmPelsHeight = height;
			dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

			if ( gl_bitdepth->value != 0 )
			{
				dm.dmBitsPerPel = gl_bitdepth->value;
				dm.dmFields |= DM_BITSPERPEL;
			}

			/*
			** our first CDS failed, so maybe we're running on some weird dual monitor
			** system 
			*/
			if ( ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) != DISP_CHANGE_SUCCESSFUL )
			{
				ri.Con_Printf( PRINT_ALL, " failed\n" );

				ri.Con_Printf( PRINT_ALL, "...setting windowed mode\n" );

				ChangeDisplaySettings( 0, 0 );

				*pwidth = width;
				*pheight = height;
				dx_state.fullscreen = false;
				if ( !VID_CreateWindow (width, height, false) )
					return rserr_invalid_mode;
				return rserr_invalid_fullscreen;
			}
			else
			{
				ri.Con_Printf( PRINT_ALL, " ok\n" );
				if ( !VID_CreateWindow (width, height, true) )
					return rserr_invalid_mode;

				dx_state.fullscreen = true;
				return rserr_ok;
			}
		}
	}
	else
	{
		ri.Con_Printf( PRINT_ALL, "...setting windowed mode\n" );

		ChangeDisplaySettings( 0, 0 );

		*pwidth = width;
		*pheight = height;
		dx_state.fullscreen = false;
		if ( !VID_CreateWindow (width, height, false) )
			return rserr_invalid_mode;
	}

	return rserr_ok;
}

/*
** GLimp_Shutdown
*/
void DXimp_Shutdown( void )
{
	SetDeviceGammaRamp (winstate.hDC, original_ramp);	// MrG - BeefQuake - hardware gammaramp

	if (matStack)
	{
		matStack->lpVtbl->Release(&matStack);
		matStack = NULL;
	}
	if (d3dvb)
	{
		d3dvb->lpVtbl->Release (&d3dvb);
		d3dvb = NULL;
	}
	if (d3dvbQuad)
	{
		d3dvbQuad->lpVtbl->Release (&d3dvbQuad);
		d3dvbQuad = NULL;
	}
	if (d3dvbTris)
	{
		d3dvbTris->lpVtbl->Release (&d3dvbTris);
		d3dvbTris = NULL;
	}
	if (d3dDev9)
	{
		d3dDev9->lpVtbl->Release (&d3dDev9);
		d3dDev9 = NULL;
	}
	if (d3dObj9)
	{
		d3dObj9->lpVtbl->Release (&d3dObj9);
		d3dObj9 = NULL;
	}

	if (winstate.hDC)
	{
		if ( !ReleaseDC( winstate.hWnd, winstate.hDC ) )
			ri.Con_Printf( PRINT_ALL, "ref_dx::R_Shutdown() - ReleaseDC failed\n" );
		winstate.hDC   = NULL;
	}
	if (winstate.hWnd)
	{
		ShowWindow (winstate.hWnd, SW_HIDE);
		DestroyWindow (	winstate.hWnd );		// Tomaz - leftover button bug fix
		winstate.hWnd = NULL;
	}

	UnregisterClass (WINDOW_CLASS_NAME, winstate.hInstance);

	if ( dx_state.fullscreen )
	{
		ChangeDisplaySettings( 0, 0 );
		dx_state.fullscreen = false;
	}
}


/*
** GLimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of OpenGL.  Under Win32 this means dealing with the pixelformats and
** doing the wgl interface stuff.
*/
qboolean DXimp_Init( void *hinstance, void *wndproc )
{
#define OSR2_BUILD_NUMBER 1111

	OSVERSIONINFO	vinfo;

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	dx_config.allowdisplaydepthchange = false;

	if ( GetVersionEx( &vinfo) )
	{
		if ( vinfo.dwMajorVersion > 4 )
		{
			dx_config.allowdisplaydepthchange = true;
		}
		else if ( vinfo.dwMajorVersion == 4 )
		{
			if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
			{
				dx_config.allowdisplaydepthchange = true;
			}
			else if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
			{
				if ( LOWORD( vinfo.dwBuildNumber ) >= OSR2_BUILD_NUMBER )
				{
					dx_config.allowdisplaydepthchange = true;
				}
			}
		}
	}
	else
	{
		ri.Con_Printf( PRINT_ALL, "DXimp_Init() - GetVersionEx failed\n" );
		return false;
	}

	winstate.hInstance = ( HINSTANCE ) hinstance;
	winstate.wndproc = wndproc;

	return true;
}

void DXimp_GetCaps (void)
{
	D3DCAPS9	caps;

	if (!d3dObj9)
		return;

	// Get the caps
	d3dObj9->lpVtbl->GetDeviceCaps(&d3dObj9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);

	if (caps.Caps2 & D3DCAPS2_CANAUTOGENMIPMAP)
		dx_config.automipmap = true;
	else
		dx_config.automipmap = false;

D3DCAPS2_DYNAMICTEXTURES
D3DCAPS2_FULLSCREENGAMMA
D3DDEVCAPS_NPATCHES
D3DDEVCAPS_PUREDEVICE
D3DDEVCAPS_QUINTICRTPATCHES
D3DDEVCAPS_RTPATCHES
D3DPRASTERCAPS_ANISOTROPY
D3DPRASTERCAPS_DITHER
D3DPRASTERCAPS_DEPTHBIAS
D3DPRASTERCAPS_FOGRANGE
D3DPRASTERCAPS_FOGTABLE
D3DPRASTERCAPS_FOGVERTEX
D3DPRASTERCAPS_MIPMAPLODBIAS
D3DPRASTERCAPS_MULTISAMPLE_TOGGLE
D3DPRASTERCAPS_SCISSORTEST
D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS
D3DPRASTERCAPS_ZFOG
D3DPRASTERCAPS_ZTEST
D3DPTEXTURECAPS_ALPHA
D3DPTEXTURECAPS_ALPHAPALETTE
D3DPTEXTURECAPS_CUBEMAP
D3DPTEXTURECAPS_CUBEMAP_POW2
D3DPTEXTURECAPS_MIPCUBEMAP
D3DPTEXTURECAPS_MIPMAP
D3DPTEXTURECAPS_MIPVOLUMEMAP
D3DPTEXTURECAPS_PERSPECTIVE
D3DPTEXTURECAPS_POW2
D3DPTEXTURECAPS_VOLUMEMAP
D3DPTEXTURECAPS_VOLUMEMAP_POW2

}

qboolean DXimp_InitDX (void)
{
	HRESULT hr;

	// Create our Object
	if( NULL == (d3dObj9 = Direct3DCreate9(D3D_SDK_VERSION)))
	{
		MessageBox(winstate.hWnd, "Direct3DCreate9 Failed", "Fatal Error", MB_OK | MB_ICONERROR);
        goto fail;
	}

	// Set our device properties
	ZeroMemory( &d3dpp, sizeof(d3dpp));

	// Set our depth stencil buffer and render target format
	if( FAILED( d3dObj9->lpVtbl->CheckDeviceFormat( 
		&d3dObj9,
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,  
		D3DFMT_A8R8G8B8,  
		D3DUSAGE_DEPTHSTENCIL, 
		D3DRTYPE_SURFACE,
		D3DFMT_D24S8 ) ) )
	{
		if( FAILED( d3dObj9->lpVtbl->CheckDeviceFormat( 
			&d3dObj9,
			D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,  
			D3DFMT_A8R8G8B8,  
			D3DUSAGE_DEPTHSTENCIL, 
			D3DRTYPE_SURFACE,
			D3DFMT_D32 ) ) )
		{
			if( FAILED( d3dObj9->lpVtbl->CheckDeviceFormat( 
				&d3dObj9,
				D3DADAPTER_DEFAULT,
				D3DDEVTYPE_HAL,  
				D3DFMT_A8R8G8B8,  
				D3DUSAGE_DEPTHSTENCIL, 
				D3DRTYPE_SURFACE,
				D3DFMT_D16 ) ) )
			{
				if( FAILED( d3dObj9->lpVtbl->CheckDeviceFormat( 
					&d3dObj9,
					D3DADAPTER_DEFAULT,
					D3DDEVTYPE_HAL,  
					D3DFMT_X8R8G8B8,  
					D3DUSAGE_DEPTHSTENCIL, 
					D3DRTYPE_SURFACE,
					D3DFMT_D24S8 ) ) )
				{
					if( FAILED( d3dObj9->lpVtbl->CheckDeviceFormat( 
						&d3dObj9,
						D3DADAPTER_DEFAULT,
						D3DDEVTYPE_HAL,  
						D3DFMT_X8R8G8B8,  
						D3DUSAGE_DEPTHSTENCIL, 
						D3DRTYPE_SURFACE,
						D3DFMT_D32 ) ) )
					{
						if( FAILED( d3dObj9->lpVtbl->CheckDeviceFormat( 
							&d3dObj9,
							D3DADAPTER_DEFAULT,
							D3DDEVTYPE_HAL,  
							D3DFMT_X8R8G8B8,  
							D3DUSAGE_DEPTHSTENCIL, 
							D3DRTYPE_SURFACE,
							D3DFMT_D16 ) ) )
						{
							if( FAILED( d3dObj9->lpVtbl->CheckDeviceFormat( 
								&d3dObj9,
								D3DADAPTER_DEFAULT,
								D3DDEVTYPE_HAL,  
								D3DFMT_R5G6B5,  
								D3DUSAGE_DEPTHSTENCIL, 
								D3DRTYPE_SURFACE,
								D3DFMT_D24S8 ) ) )
							{
								if( FAILED( d3dObj9->lpVtbl->CheckDeviceFormat( 
									&d3dObj9,
									D3DADAPTER_DEFAULT,
									D3DDEVTYPE_HAL,  
									D3DFMT_R5G6B5,  
									D3DUSAGE_DEPTHSTENCIL, 
									D3DRTYPE_SURFACE,
									D3DFMT_D32 ) ) )
								{
									if( FAILED( d3dObj9->lpVtbl->CheckDeviceFormat( 
										&d3dObj9,
										D3DADAPTER_DEFAULT,
										D3DDEVTYPE_HAL,  
										D3DFMT_R5G6B5,  
										D3DUSAGE_DEPTHSTENCIL, 
										D3DRTYPE_SURFACE,
										D3DFMT_D16 ) ) )
									{
										MessageBox(winstate.hWnd, "d3dObj9->CheckDeviceFormat Failed\n"
											"Try setting your desktop resolution to 16 bit (Medium, High Color)",
											"Fatal Error", MB_OK | MB_ICONERROR);
										goto fail;
									}
									else
									{
										d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
										d3dpp.BackBufferFormat = D3DFMT_R5G6B5;
										dx_config.displaydepth = 16;
										dx_config.have_stencil = false;
									}
								}
								else
								{
									d3dpp.AutoDepthStencilFormat = D3DFMT_D32;
									d3dpp.BackBufferFormat = D3DFMT_R5G6B5;
									dx_config.displaydepth = 16;
									dx_config.have_stencil = false;
								}
							}
							else
							{
								d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
								d3dpp.BackBufferFormat = D3DFMT_R5G6B5;
								dx_config.displaydepth = 16;
								dx_config.have_stencil = true;
							}
						}
						else
						{
							d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
							d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
							dx_config.displaydepth = 32;
							dx_config.have_stencil = false;
						}
					}
					else
					{
						d3dpp.AutoDepthStencilFormat = D3DFMT_D32;
						d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
						dx_config.displaydepth = 32;
						dx_config.have_stencil = false;
					}
				}
				else
				{
					d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
					d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
					dx_config.displaydepth = 32;
					dx_config.have_stencil = true;
				}
			}
			else
			{
				d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
				d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
				dx_config.displaydepth = 32;
				dx_config.have_stencil = false;
			}
		}
		else
		{
			d3dpp.AutoDepthStencilFormat = D3DFMT_D32;
			d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
			dx_config.displaydepth = 32;
			dx_config.have_stencil = false;
		}
	}
	else
	{
		d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
		d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
		dx_config.displaydepth = 32;
		dx_config.have_stencil = true;
	}

	// Decide fullscreen
	if (dx_state.fullscreen)
		d3dpp.Windowed = FALSE;
	else
		d3dpp.Windowed = TRUE;

	// FSAA
	if (r_antialias->value)
	{
		int quality;
		if (r_antialiaslevel->value < 2)
			r_antialiaslevel->value = 2;

		do
		{
			if( SUCCEEDED(d3dObj9->lpVtbl->CheckDeviceMultiSampleType( &d3dObj9, D3DADAPTER_DEFAULT, 
				D3DDEVTYPE_HAL , d3dpp.BackBufferFormat, d3dpp.Windowed, 
				(int)r_antialiaslevel->value, &quality ) ) )
			{
				d3dpp.MultiSampleQuality = quality-1;
				d3dpp.MultiSampleType = r_antialiaslevel->value;
			}
			r_antialiaslevel->value--;
		} while (r_antialiaslevel->value > 1);

		if (r_antialiaslevel->value < 2)
		{
			r_antialiaslevel->value = 2;
			r_antialias->value = 0;
		}
	}
	else
	{
		// No multisampling
		d3dpp.MultiSampleQuality = 0;
		d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	}

	// Set up the rest of the parameters
	d3dpp.BackBufferCount = 1;										// One Back Buffer
	d3dpp.BackBufferHeight = dx_config.height;						// Use current window height
	d3dpp.BackBufferWidth = dx_config.width;						// Use current window width
	d3dpp.EnableAutoDepthStencil = TRUE;							// Automatic depth and stencil buffer management
	d3dpp.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;				// Clear the depth stencil every time
	if ( d3dpp.Windowed )
		d3dpp.FullScreen_RefreshRateInHz = 0;						// Use current refresh rate
	else
		d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT; // Use default refresh rate
	d3dpp.hDeviceWindow = winstate.hWnd;							// Our window
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;		// Use default presentation rate
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;						// Overwrite the front buffer

	// Create our Device
	d3dDev9 = NULL;
	hr = d3dObj9->lpVtbl->CreateDevice( &d3dObj9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, winstate.hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &d3dDev9 );
	if ( FAILED ( hr ))
	{
		hr = d3dObj9->lpVtbl->CreateDevice( &d3dObj9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, winstate.hWnd,
			D3DCREATE_MIXED_VERTEXPROCESSING, &d3dpp, &d3dDev9 );
		if ( FAILED ( hr ))
		{
			hr = d3dObj9->lpVtbl->CreateDevice( &d3dObj9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, winstate.hWnd,
				D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3dDev9 );
			if ( FAILED ( hr ))
			{
				hr = d3dObj9->lpVtbl->CreateDevice( &d3dObj9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, winstate.hWnd,
					D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE, &d3dpp, &d3dDev9 );
				if (FAILED ( hr ))
				{
					MessageBox(winstate.hWnd, "d3dObj9->CreateDevice Failed\n"
						"Try setting your desktop resolution to 16 bit (Medium, High Color)",
						"Fatal Error", MB_OK | MB_ICONERROR);
					goto fail;
				}
				else
				{
					dx_config.vertexmode = DX_HWPUREDEVICE;
				}
			}
			else 
			{
				dx_config.vertexmode = DX_SOFTWARE;
			}
		}
		else 
		{
			dx_config.vertexmode = DX_MIXED;
		}
	}
	else 
	{
		dx_config.vertexmode = DX_HARDWARE;
	}

	// FSAA
	if (r_antialias->value)
		d3dDev9->lpVtbl->SetRenderState( &d3dDev9, D3DRS_MULTISAMPLEANTIALIAS, TRUE);
	else
		d3dDev9->lpVtbl->SetRenderState( &d3dDev9, D3DRS_MULTISAMPLEANTIALIAS, FALSE);

	// MrG - BeefQuake - hardware gammaramp
	ZeroMemory(original_ramp, sizeof(original_ramp));
	dx_config.gammaramp = GetDeviceGammaRamp(winstate.hDC, original_ramp);
	if (!r_hwgamma->value)
		dx_config.gammaramp=false;
	if (dx_config.gammaramp)
		vid_gamma->modified=true;

	// Set up quad and tris vertex buffers
	if( FAILED( d3dDev9->lpVtbl->CreateVertexBuffer( &d3dDev9, 3*sizeof(QVERTEX),	// Tris buffer
		D3DUSAGE_DYNAMIC, Q_FVF, D3DPOOL_DEFAULT, &d3dvbTris, NULL ) ) )
	{
		MessageBox(winstate.hWnd, "d3dDev9->CreateVertexBuffer Failed\n",
			"Fatal Error", MB_OK | MB_ICONERROR);
		goto fail;
	}
	if( FAILED( d3dDev9->lpVtbl->CreateVertexBuffer( &d3dDev9, 4*sizeof(QVERTEX),	// Quad buffer
		D3DUSAGE_DYNAMIC, Q_FVF, D3DPOOL_DEFAULT, &d3dvbQuad, NULL ) ) )
	{
		MessageBox(winstate.hWnd, "d3dDev9->>CreateVertexBuffer Failed\n",
			"Fatal Error", MB_OK | MB_ICONERROR);
		goto fail;
	}

	// FIXME: Set up matrix stack

	/*
	** print out PFD specifics 
	*/
	ri.Con_Printf( PRINT_ALL, "DX PFD: Color(%i-bits) Z/Depth(%i-bit) Stencil(%i-bit)\n",
		dx_config.displaydepth, dx_config.zbufferdepth, dx_config.stencildepth );
	switch (dx_config.vertexmode)
	{
		case DX_HARDWARE:
			ri.Con_Printf( PRINT_ALL, "Hardware Vertex Processing\n");
			break;
		case DX_MIXED:
			ri.Con_Printf( PRINT_ALL, "Mixed Hardware/Software Vertex Processing\n");
			break;
		case DX_SOFTWARE:
			ri.Con_Printf( PRINT_ALL, "Software Vertex Processing\n");
			break;
		case DX_HWPUREDEVICE:
			ri.Con_Printf( PRINT_ALL, "Hardware Pure Device Vertex Processing\n");
			break;
		default:
			ri.Con_Printf( PRINT_ALL, "Unknown Vertex Processing Mode\n");
			break;
	}
	if (r_antialias->value)
	{
		ri.Con_Printf( PRINT_ALL, "Full Scene Anti-Aliasing (Multisampling) Enabled.\n");
		ri.Con_Printf( PRINT_ALL, "Multisampling Passes: %i\n", (int)r_antialiaslevel->value);
	}
	else
	{
		ri.Con_Printf( PRINT_ALL, "Full Scene Anti-Aliasing (Multisampling) Disabled.\n");
	}

	return true;

fail:
	DXimp_Shutdown();
	return false;
}

/*
** GLimp_BeginFrame
*/
void DXimp_BeginFrame( float camera_separation )
{
	if ( gl_bitdepth->modified )
	{
		if ( gl_bitdepth->value != 0 && !dx_config.allowdisplaydepthchange )
		{
			ri.Cvar_SetValue( "gl_bitdepth", 0 );
			ri.Con_Printf( PRINT_ALL, "gl_bitdepth disabled\n" );
		}
		gl_bitdepth->modified = false;
	}
}

/*
** GLimp_EndFrame
** 
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
void DXimp_EndFrame (void)
{
	if ( FAILED( d3dDev9->lpVtbl->Present( &d3dDev9, NULL, NULL, NULL, NULL ) ) )
		ri.Sys_Error( ERR_FATAL, "DXimp_EndFrame() - d3dDev9->Present failed!\n" );
}

// MrG - BeefQuake - hardware gammaramp
void UpdateGammaRamp()
{
	int i,o;
	if (dx_config.gammaramp) {
		memcpy(gamma_ramp, original_ramp, sizeof(original_ramp));
		for (o=0; o<3; o++)
			for (i=0; i<256; i++) {
				signed int v;
				v = 255 * pow ( (i+0.5) * 0.0039138943248532289628180039138943, vid_gamma->value ) + 0.5;
				if (v > 255) v=255;
				if (v < 0) v=0;
				gamma_ramp[o][i]=((WORD)v) << 8;
			}
		SetDeviceGammaRamp(winstate.hDC, gamma_ramp);
	}
}

/*
** GLimp_AppActivate
*/
void DXimp_AppActivate( qboolean active )
{
	if ( active )
	{
		SetForegroundWindow( winstate.hWnd );
		ShowWindow( winstate.hWnd, SW_RESTORE );
	}
	else
	{
		if ( vid_fullscreen->value )
			ShowWindow( winstate.hWnd, SW_MINIMIZE );
	}
}
