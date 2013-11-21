#include "gl_local.h"
#include <png.h>

typedef struct png_s 
{ // TPng = class(TGraphic)
	char *tmpBuf;
	int tmpi;
	long FBgColor; // DL Background color Added 30/05/2000
	int FTransparent; // DL Is this Image Transparent   Added 30/05/2000
	long FRowBytes;   //DL Added 30/05/2000
	double FGamma; //DL Added 07/06/2000
	double FScreenGamma; //DL Added 07/06/2000
	char *FRowPtrs; // DL Changed for consistancy 30/05/2000  
	char* Data; //property Data: pByte read fData;
	char* Title;
	char* Author;
	char* Description;
	int BitDepth;
	int BytesPerPixel;
	int ColorType;
	int Height;
	int Width;
	int Interlace;
	int Compression;
	int Filter;
	double LastModified;
	int Transparent;
} png_t;
png_t *my_png=0;
unsigned char* pngbytes;

/*
=============================================================

  PNG STUFF

=============================================================
*/

void InitializeDemData() 
{
	long* cvaluep; //ub
	long y;

	// Initialize Data and RowPtrs
	if(my_png->Data) 
	{
		free(my_png->Data);
		my_png->Data = 0;
	}
	if(my_png->FRowPtrs) 
	{
		free(my_png->FRowPtrs);
		my_png->FRowPtrs = 0;
	}
	my_png->Data = malloc(my_png->Height * my_png->FRowBytes ); // DL Added 30/5/2000
	my_png->FRowPtrs = malloc(sizeof(void*) * my_png->Height);

	if((my_png->Data)&&(my_png->FRowPtrs)) 
	{
		cvaluep = (long*)my_png->FRowPtrs;    
		for(y=0;y<my_png->Height;y++)
		{
			cvaluep[y] = (long)my_png->Data + ( y * (long)my_png->FRowBytes ); //DL Added 08/07/2000      
		}
	}
}

void mypng_struct_create() 
{
	if(my_png) return;
	my_png = malloc(sizeof(png_t));
	my_png->Data    = 0;
	my_png->FRowPtrs = 0;
	my_png->Height  = 0;
	my_png->Width   = 0;
	my_png->ColorType = PNG_COLOR_TYPE_RGB;
	my_png->Interlace = PNG_INTERLACE_NONE;
	my_png->Compression = PNG_COMPRESSION_TYPE_DEFAULT;
	my_png->Filter = PNG_FILTER_TYPE_DEFAULT;
}

void mypng_struct_destroy(qboolean keepData) 
{  
	if(!my_png) 
		return;
	if(my_png->Data && !keepData) 
		free(my_png->Data);
	if(my_png->FRowPtrs) 
		free(my_png->FRowPtrs);
	free(my_png);
	my_png = 0;
}

/*
=============================================================

  fReadData

=============================================================
*/

void PNGAPI fReadData(png_structp png,png_bytep data,png_size_t length) 
{ // called by pnglib
	int i;  
	for(i=0;i<length;i++) 
		data[i] = my_png->tmpBuf[my_png->tmpi++];    // give pnglib a some more bytes  
}

/*
=============================================================

  GL_ScreenShot_PNG - psychospaz

=============================================================
*/

void GL_ScreenShot_PNG (void)
{
	byte			*rgbdata;
	FILE			*file;
	char			picname[80], checkname[MAX_OSPATH];
	int				i, j, k;
	png_structp		png_ptr;
	png_infop		info_ptr;


	//not working yet...
	ri.Con_Printf (PRINT_ALL, "PNG Screenshot...(^iDisabled^r)\n"); 
	return;

	// Create the scrnshots directory if it doesn't exist
	Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot", ri.FS_Gamedir());
	Sys_Mkdir (checkname);

	for (i=0 ; i<=999 ; i++) 
	{
		Com_sprintf (picname, sizeof(picname), "quake2max%i%i%i.png", (int)(i/100)%10, (int)(i/10)%10, i%10);
		Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot/%s", ri.FS_Gamedir(), picname);
		file = fopen (checkname, "rb");
		if (!file)
			break;	// file doesn't exist
		fclose (file);
	} 
	if (i==1000) 
	{
		ri.Con_Printf (PRINT_ALL, "GL_ScreenShot_PNG: Couldn't create a file\n"); 
		return;
 	}

	// Allocate room for a copy of the framebuffer
	rgbdata = malloc(vid.width * vid.height * 3);
	if(!rgbdata)
		return;

	// Read the framebuffer into our storage
	qglReadPixels(0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, rgbdata);

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0,0,0);
	if (!png_ptr)
	{
		free(rgbdata);

		ri.Con_Printf (PRINT_ALL, "GL_ScreenShot_PNG: Couldn't create image (NO PNG_PTR)\n"); 
		return;
	}

	// Allocate/initialize the image information data.  REQUIRED
	info_ptr = png_create_info_struct(png_ptr);

	if (!info_ptr)
	{
		png_destroy_write_struct(&png_ptr,  0);
		free(rgbdata);

		ri.Con_Printf (PRINT_ALL, "GL_ScreenShot_PNG: Couldn't create image (NO INFO_PTR)\n"); 
		return;
	}

	if (setjmp(png_ptr->jmpbuf))
	{
		png_destroy_write_struct(&png_ptr, &info_ptr);
		free(rgbdata);

		ri.Con_Printf (PRINT_ALL, "GL_ScreenShot_PNG: Couldn't create image (BAD INFO)\n"); 
		return;
	}

	// Open the file for Binary Output
	file = fopen(checkname, "wb");
	if(!file)
	{
		ri.Con_Printf (PRINT_ALL, "GL_ScreenShot_PNG: Couldn't create a file\n"); 
		return;
 	}

	png_init_io(png_ptr, file);

	png_set_IHDR(png_ptr, info_ptr, 
		vid.width, 
		vid.height, 
		8, 
		PNG_COLOR_TYPE_RGB,		 
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE,
		PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);

	for (k = 0; k < vid.height; k++)
	{
		void *pointer = rgbdata + k*vid.width*3;
		png_write_row(png_ptr, pointer);
	}

	png_write_end(png_ptr, info_ptr);

	png_destroy_write_struct(&png_ptr, &info_ptr);

	// Close File
	fclose(file);

	// Free Temp Framebuffer
	free(rgbdata);

	// Done!
	ri.Con_Printf (PRINT_ALL, "Wrote %s\n", picname);
}

/*
=============================================================

  LoadPNG

=============================================================
*/

void LoadPNG (char *filename, byte **pic, int *width, int *height) 
{
	png_structp png;
	png_infop pnginfo;
	byte ioBuffer[8192];
	int len;
	byte *raw;
	*pic=0;

	len = ri.FS_LoadFile (filename, (void **)&raw);

	if (!raw)
	{
		ri.Con_Printf (PRINT_DEVELOPER, "Bad png file %s\n", filename);
		return;
	}

	if( png_sig_cmp(raw,0,4)) 
		return;  

	png = png_create_read_struct(PNG_LIBPNG_VER_STRING,0,0,0);
	if(!png) 
		return;

	pnginfo = png_create_info_struct( png );

	if(!pnginfo) 
	{
		png_destroy_read_struct(&png,&pnginfo,0);
		return;
	}
	png_set_sig_bytes(png,0/*sizeof( sig )*/);

	mypng_struct_create(); // creates the my_png struct

	my_png->tmpBuf = raw; //buf = whole file content
	my_png->tmpi=0; 

	png_set_read_fn(png,ioBuffer,fReadData );

	png_read_info( png, pnginfo );

	png_get_IHDR(png, pnginfo, &my_png->Width, &my_png->Height,&my_png->BitDepth, &my_png->ColorType, &my_png->Interlace, &my_png->Compression, &my_png->Filter );
	// ...removed bgColor code here...

	if (my_png->ColorType == PNG_COLOR_TYPE_PALETTE)  
		png_set_palette_to_rgb(png);
	if (my_png->ColorType == PNG_COLOR_TYPE_GRAY && my_png->BitDepth < 8) 
		png_set_gray_1_2_4_to_8(png);

	// Add alpha channel if present
	if(png_get_valid( png, pnginfo, PNG_INFO_tRNS ))
		png_set_tRNS_to_alpha(png); 

	// hax: expand 24bit to 32bit
	if (my_png->BitDepth == 8 && my_png->ColorType == PNG_COLOR_TYPE_RGB) 
		png_set_filler(png,255, PNG_FILLER_AFTER);

	if (( my_png->ColorType == PNG_COLOR_TYPE_GRAY) || (my_png->ColorType == PNG_COLOR_TYPE_GRAY_ALPHA ))
		png_set_gray_to_rgb( png );


	if (my_png->BitDepth < 8)
		png_set_expand (png);

	// update the info structure
	png_read_update_info( png, pnginfo );

	my_png->FRowBytes = png_get_rowbytes( png, pnginfo );
	my_png->BytesPerPixel = png_get_channels( png, pnginfo );  // DL Added 30/08/2000

	InitializeDemData();
	if((my_png->Data)&&(my_png->FRowPtrs)) 
		png_read_image(png, (png_bytepp)my_png->FRowPtrs);

	png_read_end(png, pnginfo); // read last information chunks

	png_destroy_read_struct(&png, &pnginfo, 0);


	//only load 32 bit by now...
	if (my_png->BitDepth == 8)
	{
		*pic= my_png->Data;
		*width = my_png->Width;
		*height = my_png->Height;
	}
	else
	{
		ri.Con_Printf (PRINT_DEVELOPER, "Bad png color depth: %s\n", filename);
		*pic = NULL;
		free(my_png->Data);
	}

	mypng_struct_destroy(true);
	ri.FS_FreeFile ((void *)raw);
}

//_____________________________________________________________________________
