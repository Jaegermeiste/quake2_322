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
// r_misc.c

#include "dx_local.h"
#include <jpeglib.h> //Heffo - JPEG Screenshots

/*
==================
R_InitParticleTexture
==================
*/
byte	dottexture[8][8] =
{
	{0  ,1  ,2  ,4  ,4  ,2  ,1  ,0  },
	{1  ,4  ,8  ,16 ,16 ,8  ,4  ,1  },
	{2  ,8  ,32 ,64 ,64 ,32 ,8  ,2  },
	{4  ,16 ,64 ,255,255,64 ,16 ,4  },
	{4  ,16 ,64 ,255,255,64 ,16 ,4  },
	{2  ,8  ,32 ,64 ,64 ,32 ,8  ,2  },
	{1  ,4  ,8  ,16 ,16 ,8  ,4  ,1  },
	{0  ,1  ,2  ,4  ,4  ,2  ,1  ,0  },
};	// Carbon14

extern image_t *Draw_FindPic(char *name);
void R_InitParticleTexture (void)
{
	int		x,y;
	byte	data[8][8][4];

	///
	// particle texture
	//
	for (x=0 ; x<8 ; x++)
	{
		for (y=0 ; y<8 ; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = dottexture[x][y]; // c14 Just this line changes
		}
	}
	r_particletexture = Draw_FindPic("particle");   //c14 add this line

    if (!r_particletexture) {                                 //c14 add this line
		r_particletexture = GL_LoadPic ("***particle***", (byte *)data, 8, 8, it_sprite, 32);
    }                                                         //c14 add this line

	//
	// also use this for bad textures, but without alpha
	//
	for (x=0 ; x<8 ; x++)
	{
		for (y=0 ; y<8 ; y++)
		{
			data[y][x][0] = dottexture[x&3][y&3]*255;
			data[y][x][1] = 0; // dottexture[x&3][y&3]*255;
			data[y][x][2] = 0; //dottexture[x&3][y&3]*255;
			data[y][x][3] = 255;
		}
	}
	r_notexture = GL_LoadPic ("***r_notexture***", (byte *)data, 8, 8, it_wall, 32);

	//		--==OBSIDIAN UPDATE==--
	// Based on work by Carbon14
	r_shelltexture[0] = Draw_FindPic("shell/shell0");
	r_shelltexture[1] = Draw_FindPic("shell/shell1");
	r_shelltexture[2] = Draw_FindPic("shell/shell2");
	r_shelltexture[3] = Draw_FindPic("shell/shell3");
	r_shelltexture[4] = Draw_FindPic("shell/shell4");
	for (x = 0; x < 5; x++){
		if (!r_shelltexture[x])
		{
			r_shelltexture[x] = GL_LoadPic ("***shell***", (byte *)data, 8, 8, it_sprite, 32);
		}
	}
}


/* 
============================================================================== 
 
						SCREEN SHOTS 
 
============================================================================== 
*/ 

/* 
================== 
GL_ScreenShot_JPG
By Robert 'Heffo' Heffernan
================== 
*/
void GL_ScreenShot_JPG_f (void)
{
	struct jpeg_compress_struct		cinfo;
	struct jpeg_error_mgr			jerr;
	byte							*rgbdata;
	JSAMPROW						s[1];
	FILE							*file;
	char							picname[80], checkname[MAX_OSPATH];
	int								i, offset;
	LPDIRECT3DSURFACE9				screenshot;

	// Create the scrnshots directory if it doesn't exist
	Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot", ri.FS_Gamedir());
	Sys_Mkdir (checkname);

	// Find a file name to save it to 
	strcpy(picname,"quake00.jpg");

	for (i=0 ; i<=99 ; i++) 
	{ 
		picname[5] = i/10 + '0'; 
		picname[6] = i%10 + '0'; 
		Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot/%s", ri.FS_Gamedir(), picname);
		file = fopen (checkname, "rb");
		if (!file)
			break;	// file doesn't exist
		fclose (file);
	} 
	if (i==100) 
	{
		ri.Con_Printf (PRINT_ALL, "SCR_JPGScreenShot_f: Couldn't create a file\n"); 
		return;
 	}

	// Open the file for Binary Output
	file = fopen(checkname, "wb");
	if(!file)
	{
		ri.Con_Printf (PRINT_ALL, "GL_ScreenShotJPG_f: Couldn't create a file\n"); 
		return;
 	}

	// Allocate room for a copy of the framebuffer
	rgbdata = malloc(vid.width * vid.height * 3);
	if(!rgbdata)
	{
		fclose(file);
		return;
	}

	// Read the framebuffer into our storage
	//qglReadPixels(0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, rgbdata);
	d3dDev9->GetFrontBufferData(0, screenshot);

	// Initialise the JPEG compression object
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, file);

	// Setup JPEG Parameters
	cinfo.image_width = vid.width;
	cinfo.image_height = vid.height;
	cinfo.in_color_space = JCS_RGB;
	cinfo.input_components = 3;
	jpeg_set_defaults(&cinfo);
	if((r_jpeg_quality->value >= 101) || (r_jpeg_quality->value <= 0))
		ri.Cvar_Set("r_jpeg_quality", "85");
	jpeg_set_quality(&cinfo, r_jpeg_quality->value, TRUE);

	// Start Compression
	jpeg_start_compress(&cinfo, true);

	// Feed Scanline data
	offset = (cinfo.image_width * cinfo.image_height * 3) - (cinfo.image_width * 3);
	while(cinfo.next_scanline < cinfo.image_height)
	{
		s[0] = &rgbdata[offset - (cinfo.next_scanline * (cinfo.image_width * 3))];
		jpeg_write_scanlines(&cinfo, s, 1);
	}

	// Finish Compression
	jpeg_finish_compress(&cinfo);

	// Destroy JPEG object
	jpeg_destroy_compress(&cinfo);

	// Close File
	fclose(file);

	// Free Temp Framebuffer
	free(rgbdata);

	// Done!
	ri.Con_Printf (PRINT_ALL, "Wrote %s\n", picname);
}

/* 
================== 
GL_ScreenShot_f
================== 
*/  
void GL_ScreenShot_f (void) 
{
	byte		*buffer;
	char		picname[80]; 
	char		checkname[MAX_OSPATH];
	int			i, c, temp;
	FILE		*f;

	// create the scrnshots directory if it doesn't exist
	Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot", ri.FS_Gamedir());
	Sys_Mkdir (checkname);

// 
// find a file name to save it to 
// 
	strcpy(picname,"quake00.tga");

	for (i=0 ; i<=99 ; i++) 
	{ 
		picname[5] = i/10 + '0'; 
		picname[6] = i%10 + '0'; 
		Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot/%s", ri.FS_Gamedir(), picname);
		f = fopen (checkname, "rb");
		if (!f)
			break;	// file doesn't exist
		fclose (f);
	} 
	if (i==100) 
	{
		ri.Con_Printf (PRINT_ALL, "SCR_ScreenShot_f: Couldn't create a file\n"); 
		return;
 	}


	buffer = malloc(vid.width*vid.height*3 + 18);
	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = vid.width&255;
	buffer[13] = vid.width>>8;
	buffer[14] = vid.height&255;
	buffer[15] = vid.height>>8;
	buffer[16] = 24;	// pixel size

	qglReadPixels (0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, buffer+18 ); 

	// swap rgb to bgr
	c = 18+vid.width*vid.height*3;
	for (i=18 ; i<c ; i+=3)
	{
		temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

	f = fopen (checkname, "wb");
	fwrite (buffer, 1, c, f);
	fclose (f);

	free (buffer);
	ri.Con_Printf (PRINT_ALL, "Wrote %s\n", picname);
} 

/*
** GL_Strings_f
*/
void GL_Strings_f( void )
{
	ri.Con_Printf (PRINT_ALL, "GL_VENDOR: %s\n", gl_config.vendor_string );
	ri.Con_Printf (PRINT_ALL, "GL_RENDERER: %s\n", gl_config.renderer_string );
	ri.Con_Printf (PRINT_ALL, "GL_VERSION: %s\n", gl_config.version_string );
	ri.Con_Printf (PRINT_ALL, "GL_EXTENSIONS: %s\n", gl_config.extensions_string );
}

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void )
{
	//qglClearColor (1,0, 0.5 , 0.5); // NeVo - fucking ugly color!
	qglClearColor (0,0,0,1);	// NeVo - clear to black
	qglCullFace(GL_FRONT);
	qglEnable(GL_TEXTURE_2D);

	qglEnable(GL_ALPHA_TEST);
	qglAlphaFunc(GL_GREATER, 0.666);

	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_CULL_FACE);
	qglDisable (GL_BLEND);

	qglColor4f (1,1,1,1);

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglShadeModel (GL_FLAT);

	GL_TextureMode( gl_texturemode->string );
	GL_TextureAlphaMode( gl_texturealphamode->string );
	GL_TextureSolidMode( gl_texturesolidmode->string );

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_TexEnv( GL_REPLACE );

	if ( qglPointParameterfEXT )
	{
		float attenuations[3];

		attenuations[0] = gl_particle_att_a->value;
		attenuations[1] = gl_particle_att_b->value;
		attenuations[2] = gl_particle_att_c->value;

		qglEnable( GL_POINT_SMOOTH );
		qglPointParameterfEXT( GL_POINT_SIZE_MIN_EXT, gl_particle_min_size->value );
		qglPointParameterfEXT( GL_POINT_SIZE_MAX_EXT, gl_particle_max_size->value );
		qglPointParameterfvEXT( GL_DISTANCE_ATTENUATION_EXT, attenuations );
	}

	if ( qglColorTableEXT && gl_ext_palettedtexture->value )
	{
		qglEnable( GL_SHARED_TEXTURE_PALETTE_EXT );

		GL_SetTexturePalette( d_8to24table );
	}

	GL_UpdateSwapInterval();
}

void GL_UpdateSwapInterval( void )
{
	if ( gl_swapinterval->modified )
	{
		gl_swapinterval->modified = false;

		if ( !gl_state.stereo_enabled ) 
		{
#ifdef _WIN32
			if ( qwglSwapIntervalEXT )
				qwglSwapIntervalEXT( gl_swapinterval->value );
#endif
		}
	}
}

/*
=================
R_ComputeFogCoord

Cipher the fog coordinate
=================
*/
void R_ComputeFogCoord (vec3_t point)
{
	float myCoord;
	//glFogCoordfEXT(myCoord);
}

/*
===========
R_Fog

NeVo - draw fog in an area
===========
*/
void R_Fog (fogmode_t fog)
{
	float colors[4];

	// disable fog for now
	qglDisable(GL_FOG);

	// Set up our colors
	colors[0] = fog.red;
	colors[1] = fog.green;
	colors[2] = fog.blue;
	colors[3] = fog.alpha;

	// Set our mode
	switch (fog.mode)
	{
	case FOGMODE_LINEAR:
		qglFogi(GL_FOG_MODE, GL_LINEAR);
		break;
	case FOGMODE_EXP:
		qglFogi(GL_FOG_MODE, GL_EXP);
		break;
	case FOGMODE_EXP2:
		qglFogi(GL_FOG_MODE, GL_EXP2);
		break;
	default:
		qglFogi(GL_FOG_MODE, GL_LINEAR);
		break;
	}

	if (gl_config.coordinatefog && fog.usecoords)
		qglFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);
	else
		qglFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);

	// Hint for nicest fog possible
	qglHint(GL_FOG_HINT, GL_NICEST);

	qglFogfv(GL_FOG_COLOR, colors);
	qglFogf(GL_FOG_START, fog.neardist); 
	qglFogf(GL_FOG_END, fog.fardist);
	qglFogf(GL_FOG_DENSITY, fog.fogdensity);

	// Reenable fog
	if (gl_state.fogenabled)
		qglEnable(GL_FOG);
}