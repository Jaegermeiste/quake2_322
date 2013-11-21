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

#include "qcommon.h"
#include "../libs/zlib-1.2.8/zlib.h"

// define this to disallow any data but the demo pak file
//#define	NO_ADDONS

// if a packfile directory differs from this, it is assumed to be hacked
// Full version
#define	PAK0_CHECKSUM	0x40e614e0
// Demo
//#define	PAK0_CHECKSUM	0xb2c6d7ea
// OEM
//#define	PAK0_CHECKSUM	0x78e135c

/*
=============================================================================

QUAKE FILESYSTEM

=============================================================================
*/


//
// in memory
//

typedef struct packfile_s
{
	char	name[MAX_QPATH];
	int		filepos, filelen;
} packfile_t;

typedef struct pack_s
{
	char	filename[MAX_OSPATH];
	FILE	*handle;
	int		numfiles;
	packfile_t	*files;
} pack_t;

char	fs_gamedir[MAX_OSPATH];
cvar_t	*fs_basedir;
cvar_t	*fs_cddir;
cvar_t	*fs_gamedirvar;

typedef struct filelink_s
{
	struct filelink_s	*next;
	char	*from;
	int		fromlength;
	char	*to;
} filelink_t;

filelink_t	*fs_links;

typedef struct searchpath_s
{
	char	filename[MAX_OSPATH];
	pack_t	*pack;		// only one of filename / pack will be used
	struct searchpath_s *next;
} searchpath_t;

searchpath_t	*fs_searchpaths;
searchpath_t	*fs_base_searchpaths;	// without gamedirs


/*

All of Quake's data access is through a hierchal file system, but the contents
of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all
game directories.  The sys_* files pass this to host_init in quakeparms_t->basedir.
This can be overridden with the "-basedir" command line parm to allow code debugging
in a different directory.  The base directory is only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that
all generated files (savegames, screenshots, demos, config files) will be saved to.
This can be overridden with the "-game" command line parameter.  The game directory
can never be changed while quake is executing.  This is a precaution against having
a malicious server instruct clients to write files over areas they shouldn't.

*/

//		--==OBSIDIAN UPDATE==--
//This function is used to see if a file exists in either a pack file or the file system
//A return value of 0 (zero) means no file was found in either the pack files or file system
//A return value of 1 (one) means the specified file WAS found in either a pack file or file system
//NOTE: There is no way to distinguish between where it was found. That's the loader's job.
int FS_DoesFileExist(char *path){
	searchpath_t	*search;
	char			netpath[MAX_OSPATH];
	pack_t			*pak;
	int				i;
	//filelink_t		*link;
	FILE			*file;
	char			realPath[MAX_OSPATH];


	//if we have a period in the front of the string, chances are, the base directory is
	//part of the path. As such, we must strip it!!!
	memset(realPath, 0, MAX_OSPATH);
	if (path[0] == '.'){
		strcpy(realPath, strchr(strchr(path, '/')+1, '/')+1);
	}
	else{
		int rIndex = 0;
		for (i = 0; i < strlen(path); i++){
			if (path[i] == '/' && path[i+1] == '/')
				i++;

			realPath[rIndex] = path[i];
			rIndex++;
		}
	}


	for (search = fs_searchpaths ; search ; search = search->next)
	{
	// is the element a pak file?
		if (search->pack)
		{
		// look through all the pak file elements
			pak = search->pack;
			for (i=0 ; i<pak->numfiles ; i++)
				if (!Q_strcasecmp (pak->files[i].name, realPath))
				{	// found it!
					return 1;
				}
		}
		else
		{		
	// check a file in the directory tree
			
			Com_sprintf (netpath, sizeof(netpath), "%s/%s",search->filename, realPath);
			
			file = fopen (netpath, "rb");
			if (!file)
				continue;
			else{
				fclose(file);			
				return 1;
			}
		}
	}
	return 0;
}



//		--==OBSIDIAN UPDATE==--
//This function will search the pack files for a list of directories with the given findname
//PLEASE NOTE: Unlike with the list returned by FS_List files, the
//  list returned by FS_ListPackFiles does not need to be explicitly
//  destroyed after use due to the way it is being built.
char **FS_ListPackFiles(char *findname, int *numfiles){
	searchpath_t	*search;
	char			filepath[MAX_OSPATH];
	pack_t			*pak;
	int				i;
	int				x;
	int				j;
	//filelink_t		*link;
	int				nfiles;
	int				findex;
	int				step2;
	char**			list;
	char*			strloc;
	int				FoundWild = 0;
	int				FoundWildCont = 0;

	//getting the start point after the gamedir name
	if (findname[0] == '.'){
		strloc = strchr(strchr(findname, '/')+1, '/')+1;
		if (!strloc) return NULL;
	}
	else strloc = findname;

	if (strlen(strloc) > MAX_OSPATH)
		return NULL;

	memset(filepath, 0, MAX_OSPATH);
	for (i = 0; i < strlen(strloc); i++){
		if (strloc[i] == '*'){
			FoundWild = 1;
			if (strlen(strloc) > i + 2){
				if (strloc[i+1] == '.' && strloc[i+2] != '*'){
					FoundWildCont = 1;
					strcpy(filepath, strloc);
				}
			}
			break;
		}
		filepath[i] = strloc[i];
	}

	nfiles = 0;
	step2 = 0;
	findex = 0;
	list = NULL;

	for (x = 0; x < 2; x++){
		
		if (nfiles > 0){
			list = malloc( sizeof( char * ) * nfiles );
			memset( list, 0, sizeof( char * ) * nfiles );
			step2 = 1;
			*numfiles = nfiles;
		}
		else if (x == 1)
			break;

		for (search = fs_searchpaths ; search ; search = search->next)
		{
		// is the element a pak file?
			if (search->pack)
			{
			// look through all the pak file elements
				pak = search->pack;
				for (i=0 ; i<pak->numfiles ; i++){
					int FoundMatch = 1;
					int PastWild = 0;
					int wIndex = 0;
					if (strlen(pak->files[i].name) < strlen(filepath))
						continue;
					if (FoundWild == 0){
						if (strlen(pak->files[i].name) > strlen(filepath))
							continue;
					}
					else{
						if (strlen(pak->files[i].name) == strlen(filepath))
							continue;
					}
					for (j = 0; j < strlen(filepath); j++){
						if (FoundWildCont == 1){
							if (filepath[j] == '*' && PastWild == 0){
								PastWild = 1;
								j += 2;
							}
							if (PastWild == 1 && pak->files[i].name[wIndex] == '.'){
								wIndex++;
								PastWild = 2;
							}
							else if (PastWild == 1){
								wIndex++;
								j--;
							}

							if (PastWild != 1){
								if (pak->files[i].name[wIndex] != filepath[j]){
									FoundMatch = 0;
									break;
								}
								else wIndex++;
							}
						}
						else{
							if (pak->files[i].name[j] != filepath[j]){
								FoundMatch = 0;
								break;
							}
						}
					}

					
					if (FoundMatch == 1){
						if (FoundWildCont != 1){
							if (pak->files[i].name[strlen(pak->files[i].name) - 1] != '/')
								continue;
						}

						if (step2 == 0)
							nfiles++;
						else{
							list[findex] = pak->files[i].name;
							findex++;
						}
					}
				}
			}
		}
	}

	return list;
}


/*
================
FS_filelength
================
*/
int FS_filelength (FILE *f)
{
	int		pos;
	int		end;

	//pos = ftell (f);
	//fseek (f, 0, SEEK_END);
	//end = ftell (f);
	//fseek (f, pos, SEEK_SET);
	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}


/*
============
FS_CreatePath

Creates any directories needed to store the given filename
============
*/
void	FS_CreatePath (char *path)
{
	char	*ofs;
	
	for (ofs = path+1 ; *ofs ; ofs++)
	{
		if (*ofs == '/')
		{	// create the directory
			*ofs = 0;
			Sys_Mkdir (path);
			*ofs = '/';
		}
	}
}


/*
==============
FS_FCloseFile

For some reason, other dll's can't just cal fclose()
on files returned by FS_FOpenFile...
==============
*/
void FS_FCloseFile (FILE *f)
{
	//fclose (f);
	fclose(f);
}


// RAFAEL
/*
	Developer_searchpath
*/
int	Developer_searchpath (int who)
{
	
	int		ch;
	// PMM - warning removal
//	char	*start;
	searchpath_t	*search;
	
	if (who == 1) // xatrix
		ch = 'x';
	else if (who == 2)
		ch = 'r';

	for (search = fs_searchpaths ; search ; search = search->next)
	{
		if (strstr (search->filename, "xatrix"))
			return 1;

		if (strstr (search->filename, "rogue"))
			return 2;
/*
		start = strchr (search->filename, ch);

		if (start == NULL)
			continue;

		if (strcmp (start ,"xatrix") == 0)
			return (1);
*/
	}
	return (0);

}


/*
===========
FS_FOpenFile

Finds the file in the search path.
returns filesize and an open FILE *
Used for streaming data out of either a pak file or
a seperate file.
===========
*/
int file_from_pak = 0;
#ifndef NO_ADDONS
int FS_FOpenFile (char *filename, FILE **file)
{
	searchpath_t	*search;
	char			netpath[MAX_OSPATH];
	pack_t			*pak;
	int				i;
	filelink_t		*link;

	file_from_pak = 0;

	// check for links first
	for (link = fs_links ; link ; link=link->next)
	{
		if (!strncmp (filename, link->from, link->fromlength))
		{
			Com_sprintf (netpath, sizeof(netpath), "%s%s",link->to, filename+link->fromlength);
			//*file = fopen (netpath, "rb");
			*file = fopen (netpath, "rb");
			if (*file)
			{		
				Com_DPrintf ("link file: %s\n",netpath);
				return FS_filelength (*file);
			}
			return -1;
		}
	}

//
// search through the path, one element at a time
//
	for (search = fs_searchpaths ; search ; search = search->next)
	{
	// is the element a pak file?
		if (search->pack)
		{
		// look through all the pak file elements
			pak = search->pack;
			for (i=0 ; i<pak->numfiles ; i++)
				if (!Q_strcasecmp (pak->files[i].name, filename))
				{	// found it!
					file_from_pak = 1;
					Com_DPrintf ("PackFile: %s : %s\n",pak->filename, filename);
				// open a new file on the pakfile
					*file = fopen (pak->filename, "rb");
					if (!*file)
						Com_Error (ERR_FATAL, "Couldn't reopen %s", pak->filename);	
					fseek (*file, pak->files[i].filepos, SEEK_SET);	// FIXME: wrong seek fseek
					return pak->files[i].filelen;
				}
		}
		else
		{		
	// check a file in the directory tree
			
			Com_sprintf (netpath, sizeof(netpath), "%s/%s",search->filename, filename);
			
			*file = fopen (netpath, "rb");	// FIXME: wrong open fopen
			if (!*file)
				continue;
			
			Com_DPrintf ("FindFile: %s\n",netpath);
			return FS_filelength (*file);
		}
		
	}
	
	Com_DPrintf ("FindFile: can't find %s\n", filename);
	
	*file = NULL;
	return -1;
}

#else

// this is just for demos to prevent add on hacking

/*int FS_FOpenFile (char *filename, FILE **file)
{
	searchpath_t	*search;
	char			netpath[MAX_OSPATH];
	pack_t			*pak;
	int				i;

	file_from_pak = 0;

	// get config from directory, everything else from pak
	if (!strcmp(filename, "config.cfg") || !strncmp(filename, "players/", 8))
	{
		Com_sprintf (netpath, sizeof(netpath), "%s/%s",FS_Gamedir(), filename);
		
		*file = fopen (netpath, "rb");
		if (!*file)
			return -1;
		
		Com_DPrintf ("FindFile: %s\n",netpath);

		return FS_filelength (*file);
	}

	for (search = fs_searchpaths ; search ; search = search->next)
		if (search->pack)
			break;
	if (!search)
	{
		*file = NULL;
		return -1;
	}

	pak = search->pack;
	for (i=0 ; i<pak->numfiles ; i++)
		if (!Q_strcasecmp (pak->files[i].name, filename))
		{	// found it!
			file_from_pak = 1;
			Com_DPrintf ("PackFile: %s : %s\n",pak->filename, filename);
		// open a new file on the pakfile
			*file = fopen (pak->filename, "rb");
			if (!*file)
				Com_Error (ERR_FATAL, "Couldn't reopen %s", pak->filename);	
			fseek (*file, pak->files[i].filepos, SEEK_SET);
			return pak->files[i].filelen;
		}
	
	Com_DPrintf ("FindFile: can't find %s\n", filename);
	
	*file = NULL;
	return -1;
}*/

#endif


/*
=================
FS_ReadFile

Properly handles partial reads
=================
*/
void CDAudio_Stop(void);
//#define	MAX_READ	0x10000		// read in blocks of 64k
#define MAX_READ	256		// NeVo - zlib uses 256kb windows
void FS_Read (void *buffer, int len, FILE *f)
{
	int		block, remaining;
	int		read;
	byte	*buf;
	int		tries;

	buf = (byte *)buffer;

	// read in chunks for progress bar
	remaining = len;
	tries = 0;
	while (remaining)
	{
		block = remaining;
		if (block > MAX_READ)
			block = MAX_READ;
		read = fread (buf, 1, block, f);
		//read = fread(f, buf, block);
		if (read == 0)
		{
			// we might have been trying to read from a CD
			if (!tries)
			{
				tries = 1;
				CDAudio_Stop();
			}
			else
				Com_Error (ERR_FATAL, "FS_Read: 0 bytes read");
		}

		if (read == -1)
			Com_Error (ERR_FATAL, "FS_Read: -1 bytes read");

		// do some progress bar thing here...

		remaining -= read;
		buf += read;
	}
}

/*
==============
FS_InflateRead

This function should never be called directly
NeVo - and it is almost certainly slow
==============
*/
//int FS_InflateRead (FILE *f, void *buffer )
//{
//
//}

/*
============
FS_LoadFile

Filename are reletive to the quake search path
a null buffer will just return the file length without loading
============
*/
int FS_LoadFile (char *path, void **buffer)
{
	FILE	*h;
	byte	*buf;
	int		len;

	buf = NULL;	// quiet compiler warning

// look for it in the filesystem or pack files
	len = FS_FOpenFile (path, &h);

	if (!h)
	{
		if (buffer)
			*buffer = NULL;
		return -1;
	}
	
	if (!buffer)
	{
		fclose (h);
		return len;
	}

	buf = Z_Malloc(len);
	*buffer = buf;

	FS_Read (buf, len, h);

	FS_FCloseFile (h);

	return len;
}


/*
=============
FS_FreeFile
=============
*/
void FS_FreeFile (void *buffer)
{
	Z_Free (buffer);
}

/* 
================= 
FS_LoadPackFile 

Takes an explicit (not game tree related) path to a pak file. 

Loads the header and directory, adding the files at the beginning 
of the list so they override previous pack files. 
================= 
*/ 
pack_t *FS_LoadPackFile (char *packfile) 
{ 
   dpackheader_t	header; 
   dpak3header_t	temp; 
   int				i, numpackfiles, len; 
   packfile_t		*newfiles; 
   pack_t			*pack; 
   FILE				*packhandle; 
   dpackfile_t      info[MAX_FILES_IN_PACK]; 
   //unsigned			checksum; 

   packhandle = fopen(packfile, "rb"); //fopen
   if (!packhandle) 
      return NULL; 
   len = strlen(packfile); 

   if (!strcmp(packfile+len-4, ".pak")) 
   { 
      // NeVo - we have a regular QII .pak file 
      fread (&header, 1, sizeof(header), packhandle);
	  //fread (packhandle, &header, sizeof(header));
      if (LittleLong(header.ident) != IDPAKHEADER) 
         Com_Error (ERR_FATAL, "%s is not a packfile", packfile); 
      header.dirofs = LittleLong (header.dirofs); 
      header.dirlen = LittleLong (header.dirlen); 

      numpackfiles = header.dirlen / sizeof(dpackfile_t); 

      if (numpackfiles > MAX_FILES_IN_PACK) 
         Com_Error (ERR_FATAL, "%s has %i files", packfile, numpackfiles); 

      newfiles = Z_Malloc (numpackfiles * sizeof(packfile_t)); 

      fseek (packhandle, header.dirofs, SEEK_SET); 
      fread (info, 1, header.dirlen, packhandle); 
	  //fseek(packhandle, header.dirofs, SEEK_SET);
	  //fread(packhandle, info, header.dirlen);

      // crc the directory to check for modifications 
      //checksum = Com_BlockChecksum ((void *)info, header.dirlen); 

//#ifdef NO_ADDONS 
     // if (checksum != PAK0_CHECKSUM) 
      //   return NULL; 
//#endif 
      // parse the directory 
      for (i=0 ; i<numpackfiles ; i++) 
      { 
         strcpy (newfiles[i].name, info[i].name); 
         newfiles[i].filepos = LittleLong(info[i].filepos); 
         newfiles[i].filelen = LittleLong(info[i].filelen);
      } 
   } 
   else if (!strcmp(packfile+len-4, ".pk3") || !strcmp(packfile+len-4, ".zip")) 
   { 
	   // NeVo - We have a Quake III .pk3 or uncompressed .zip file 
	   /*       
	   NeVo - crcing the zip file is pointless 
	   // crc the directory to check for modifications 
	   checksum = Com_BlockChecksum ((void *)info, header.dirlen); 

	   #ifdef NO_ADDONS 
	   if (checksum != PAK0_CHECKSUM) 
	   return NULL; 
	   #endif 
	   */ 
	   for( i = 0; i < MAX_FILES_IN_PACK; ++i) 
	   { 
		   //-- Get the local header of the file.    
		   fread(&temp, sizeof(dpak3header_t), 1, packhandle);
		   //fread(packhandle, &temp, sizeof(dpak3header_t));

		   //-- Check if finished with pak file item collection. 
		   if (BigLong(temp.ident) == PAK3DIRHEADER) 
			   break; 

		   //-- Check if header signature is correct for the first item. 
		   if ((BigLong(temp.ident) != PAK3HEADER) && (i == 0)) 
			   Com_Error (ERR_FATAL, "%s is not a packfile", packfile); 

		   //-- Check if compression is used or any flags are set. 
		   //if ((temp.compression != 0) || (temp.flags != 0)) 
			   //Com_Error (ERR_FATAL, "%s contains errors or is compressed", packfile); 

		   //-- Get length of data area
		   info[i].filelen = temp.uncompressedSize;

		   //-- Get the data areas filename and add \0 to the end 
		   fread( &info[i].name, temp.filenameLength, 1, packhandle);
		   //fread(packhandle, &info[i].name, temp.filenameLength);
		   info[i].name[temp.filenameLength] = '\0'; 

		   //-- Get the offset of the data area 
		   //info[i].filepos = (ftell(packhandle) + temp.extraFieldLength);
		   info[i].filepos = (ftell(packhandle) + temp.extraFieldLength);

		   //-- Goto the next header 
		   //fseek( packhandle, (info[i].filelen + info[i].filepos), SEEK_SET);
		   fseek (packhandle, (info[i].filelen + info[i].filepos), SEEK_SET);
	   } 
      //-- Allocate space for array of packfile structures (filename, offset, length) 
      newfiles = Z_Malloc (i * sizeof(packfile_t)); 

      //-- The save the number of items collected from the zip file. 
      numpackfiles = i; 

      for (i=0 ; i < numpackfiles; i++) 
      { 
         strcpy (newfiles[i].name, info[i].name); 
         newfiles[i].filepos = info[i].filepos; 
         newfiles[i].filelen = info[i].filelen; 
      } 
   } 
   else 
   { 
      // Unrecognized file type! 
      Com_Printf ("Pack file type %s unrecognized\n", packfile+len-4); 
      return NULL; 
   } 
   pack = Z_Malloc (sizeof (pack_t)); 
   strcpy (pack->filename, packfile); 
   pack->handle = packhandle; 
   pack->numfiles = numpackfiles; 
   pack->files = newfiles; 

   Com_Printf ("Added packfile %s (%i files)\n", packfile, numpackfiles); 
   return pack; 
} 


/* 
================ 
FS_AddGameDirectory 

Sets fs_gamedir, adds the directory to the head of the path, 
then loads and adds pak1.pak pak2.pak ... 
------------------------------------ 
NeVo - heavily updated this function 
- Now loads up to 100 enumerated .pak files. (Pooy) 
- Now loads any and all .pak files. (Void) 
- Extended all functionality to include Quake III .pk3 and 
  uncompressed .zip files (NeVo/Nanook). 
Note: Just as higher enumerated files overwrite 
prior ones if they are the same, loose directory 
files overwrite .zip's which overwrite .pk3's 
which overwrite .pak's. 
------------------------------------ 
================ 
*/ 
void FS_AddGameDirectory (char *dir) 
{ 
   int            i; 
   searchpath_t   *search; 
   pack_t         *pak; 
   char         pakfile[MAX_OSPATH]; 
   //VoiD -S- *.pak support  
   //char *path = NULL; 
   char findname[1024]; 
   char **dirnames; 
   int ndirs; 
   char *tmp; 
   //VoiD -E- *.pack support 

   strcpy (fs_gamedir, dir); 

   // 
   // add the directory to the search path 
   // 
   search = Z_Malloc (sizeof(searchpath_t)); 
   strncpy (search->filename, dir, sizeof(search->filename)-1); 
   search->filename[sizeof(search->filename)-1] = 0; 
   search->next = fs_searchpaths; 
   fs_searchpaths = search; 

   // 
   // add any pak files in the format pak0.pak pak1.pak, ... 
   // 
   for (i=0; i<100; i++)   // Pooy - paks can now go up to 100 
   { 
      Com_sprintf (pakfile, sizeof(pakfile), "%s/pak%i.pak", dir, i); 
      pak = FS_LoadPackFile (pakfile); 
      if (!pak) 
         continue; 
      search = Z_Malloc (sizeof(searchpath_t)); 
      search->pack = pak; 
      search->next = fs_searchpaths; 
      fs_searchpaths = search;       
   } 
   // 
   // NeVo - pak3's! 
   // add any pk3 files in the format pak0.pk3 pak1.pk3, ... 
   // 
   for (i=0; i<100; i++)   // Pooy - paks can now go up to 100 
   { 
      Com_sprintf (pakfile, sizeof(pakfile), "%s/pak%i.pk3", dir, i); 
      pak = FS_LoadPackFile (pakfile); 
      if (!pak) 
         continue; 
      search = Z_Malloc (sizeof(searchpath_t)); 
      search->pack = pak; 
      search->next = fs_searchpaths; 
      fs_searchpaths = search;       
   } 
   //VoiD -S- *.pack support 
   for (i=0; i<3; i++) 
   { 
      // NeVo - Set filetype 
      switch (i) { 
         case 0: 
            // Standard Quake II pack file '.pak' 
            Com_sprintf( findname, sizeof(findname), "%s/%s", dir, ".pak" ); 
            break; 
         case 1: 
            // Quake III pack file '.pk3' 
            Com_sprintf( findname, sizeof(findname), "%s/%s", dir, ".pk3" ); 
            break; 
         case 2: 
            // Uncompressed zip file '.zip' 
            Com_sprintf( findname, sizeof(findname), "%s/%s", dir, ".zip" ); 
            break; 
         default: 
            // Standard Quake II pack file '.pak' 
            Com_sprintf( findname, sizeof(findname), "%s/%s", dir, ".pak" ); 
            break; 
      } 

      tmp = findname; 
      while ( *tmp != 0 ) 
      { 
         if ( *tmp == '\\' ) 
            *tmp = '/'; 
         tmp++; 
      } 
      if ( ( dirnames = FS_ListFiles( findname, &ndirs, 0, 0 ) ) != 0 ) 
      { 
         int i; 

         for ( i = 0; i < ndirs-1; i++ ) 
         { 
            if ( strrchr( dirnames[i], '/' ) ) 
            { 
               pak = FS_LoadPackFile (dirnames[i]); 
               if (!pak) 
                  continue; 
               search = Z_Malloc (sizeof(searchpath_t)); 
               search->pack = pak; 
               search->next = fs_searchpaths; 
               fs_searchpaths = search; 
            } 
            free( dirnames[i] ); 
         } 
         free( dirnames ); 

      } 
      //VoiD -E- *.pack support 
   } 
}

/*
============
FS_Gamedir

Called to find where to write a file (demos, savegames, etc)
============
*/
char *FS_Gamedir (void)
{
	if (*fs_gamedir)
		return fs_gamedir;
	else
		return BASEDIRNAME;
}

/*
=============
FS_ExecAutoexec
=============
*/
void FS_ExecAutoexec (void)
{
	char *dir;
	char name [MAX_QPATH];

	dir = Cvar_VariableString("gamedir");
	if (*dir)
		Com_sprintf(name, sizeof(name), "%s/%s/autoexec.cfg", fs_basedir->string, dir); 
	else
		Com_sprintf(name, sizeof(name), "%s/%s/autoexec.cfg", fs_basedir->string, BASEDIRNAME); 
	if (Sys_FindFirst(name, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM))
		Cbuf_AddText ("exec autoexec.cfg\n");
	Sys_FindClose();
}


/*
================
FS_SetGamedir

Sets the gamedir and path to a different directory.
================
*/
void FS_SetGamedir (char *dir)
{
	searchpath_t	*next;

	if (strstr(dir, "..") || strstr(dir, "/")
		|| strstr(dir, "\\") || strstr(dir, ":") )
	{
		Com_Printf ("Gamedir should be a single filename, not a path\n");
		return;
	}

	//
	// free up any current game dir info
	//
	while (fs_searchpaths != fs_base_searchpaths)
	{
		if (fs_searchpaths->pack)
		{
			fclose (fs_searchpaths->pack->handle);
			Z_Free (fs_searchpaths->pack->files);
			Z_Free (fs_searchpaths->pack);
		}
		next = fs_searchpaths->next;
		Z_Free (fs_searchpaths);
		fs_searchpaths = next;
	}

	//
	// flush all data, so it will be forced to reload
	//
	if (dedicated && !dedicated->value)
		Cbuf_AddText ("vid_restart\nsnd_restart\n");

	Com_sprintf (fs_gamedir, sizeof(fs_gamedir), "%s/%s", fs_basedir->string, dir);

	if (!strcmp(dir,BASEDIRNAME) || (*dir == 0))
	{
		Cvar_FullSet ("gamedir", "", CVAR_SERVERINFO|CVAR_NOSET);
		Cvar_FullSet ("game", "", CVAR_LATCH|CVAR_SERVERINFO);
	}
	else
	{
		Cvar_FullSet ("gamedir", dir, CVAR_SERVERINFO|CVAR_NOSET);
		if (fs_cddir->string[0])
			FS_AddGameDirectory (va("%s/%s", fs_cddir->string, dir) );
		FS_AddGameDirectory (va("%s/%s", fs_basedir->string, dir) );
	}
}


/*
================
FS_Link_f

Creates a filelink_t
================
*/
void FS_Link_f (void)
{
	filelink_t	*l, **prev;

	if (Cmd_Argc() != 3)
	{
		Com_Printf ("USAGE: link <from> <to>\n");
		return;
	}

	// see if the link already exists
	prev = &fs_links;
	for (l=fs_links ; l ; l=l->next)
	{
		if (!strcmp (l->from, Cmd_Argv(1)))
		{
			Z_Free (l->to);
			if (!strlen(Cmd_Argv(2)))
			{	// delete it
				*prev = l->next;
				Z_Free (l->from);
				Z_Free (l);
				return;
			}
			l->to = CopyString (Cmd_Argv(2));
			return;
		}
		prev = &l->next;
	}

	// create a new link
	l = Z_Malloc(sizeof(*l));
	l->next = fs_links;
	fs_links = l;
	l->from = CopyString(Cmd_Argv(1));
	l->fromlength = strlen(l->from);
	l->to = CopyString(Cmd_Argv(2));
}

/*
** FS_ListFiles
		--==OBSIDIAN UPDATE==--
This function has been modified to give pack file searching support while maintaining
directory tree file overriding presidense
*/
char **FS_ListFiles( char *findname, int *numfiles, unsigned musthave, unsigned canthave )
{
	//		--==OBSIDIAN UPDATE==--
	//Variables updated for pack files
	char *s;
	int nfiles = 0;
	int npfiles = 0; //Added to count pack files
	char **list = 0;
	char **packlist = NULL; //Added to hold found in pack files
	int x;

	//		--==OBSIDIAN UPDATE==--
	//Seeing if there are any matching pack file elements.
	packlist = FS_ListPackFiles(findname, &npfiles);
	nfiles = npfiles;

	s = Sys_FindFirst( findname, musthave, canthave );
	while ( s )
	{
		//		--==OBSIDIAN UPDATE==--
		//Updateing this if statement to take into account any files found in a pack file
		if ( s[strlen(s)-1] != '.' ){
			int a;
			int MatchFound = 0;
			char pequiv[MAX_OSPATH];
			Com_sprintf(pequiv, MAX_OSPATH, "%s/", strchr(strchr(s, '/') + 1, '/')+1);
			for (a = 0; a < npfiles; a++){
				if (!strcmp(pequiv, packlist[a])){
					MatchFound = 1;
					break;
				}
			}
			if (MatchFound == 0)
				nfiles++;
		}
		s = Sys_FindNext( musthave, canthave );
	}
	Sys_FindClose ();

	if ( !nfiles )
		return NULL;

	nfiles++; // add space for a guard
	*numfiles = nfiles;

	list = malloc( sizeof( char * ) * nfiles );
	memset( list, 0, sizeof( char * ) * nfiles );

	for (x = 0; x < npfiles; x++){
		char pfString[MAX_OSPATH];
		int FOc = 0;
		int si = 0;
		memset(pfString, 0, MAX_OSPATH);
		strcpy(pfString, findname);
		while (pfString[si] != '/' || FOc == 0){
			if (pfString[si] == '/')
				FOc = 1;
			si++;
		}
		si++;
		for (; si < MAX_OSPATH; si++)
			pfString[si] = 0;
		strcat(pfString, packlist[x]);
		if (pfString[strlen(pfString) - 1] == '/')
			pfString[strlen(pfString) - 1] = 0;
		list[x] = strdup(pfString);
	}

	s = Sys_FindFirst( findname, musthave, canthave );
	nfiles = npfiles;
	while ( s )
	{
		//		--==OBSIDIAN UPDATE==--
		//Updating this if statement to deal with pack searching
		if ( s[strlen(s)-1] != '.' )
		{
			int a;
			char pequiv[MAX_OSPATH];
			int MatchFound = 0;
			int MatchIndex = 0;
			Com_sprintf(pequiv, MAX_OSPATH, "%s/", strchr(strchr(s, '/') + 1, '/')+1);
			for (a = 0; a < npfiles; a++){
				if (!strcmp(pequiv, packlist[a])){
					MatchFound = 1;
					MatchIndex = a;
					break;
				}
			}
			if (MatchFound == 0){
				list[nfiles] = strdup( s );
#ifdef _WIN32
				strlwr( list[nfiles] );
#endif
				nfiles++;
			}
			else{
				if (list[MatchIndex]){
					free(list[MatchIndex]);
					list[MatchIndex] = 0;
				}
				list[MatchIndex] = strdup( s );
#ifdef _WIN32
				strlwr( list[nfiles] );
#endif
			}
		}
		s = Sys_FindNext( musthave, canthave );
	}
	Sys_FindClose ();
	if (packlist)
		free(packlist);

	return list;
}

/*
** FS_Dir_f
*/
void FS_Dir_f( void )
{
	char	*path = NULL;
	char	findname[1024];
	char	wildcard[1024] = "*.*";
	char	**dirnames;
	int		ndirs;

	if ( Cmd_Argc() != 1 )
	{
		strcpy( wildcard, Cmd_Argv( 1 ) );
	}

	while ( ( path = FS_NextPath( path ) ) != NULL )
	{
		char *tmp = findname;

		Com_sprintf( findname, sizeof(findname), "%s/%s", path, wildcard );

		while ( *tmp != 0 )
		{
			if ( *tmp == '\\' ) 
				*tmp = '/';
			tmp++;
		}
		Com_Printf( "Directory of %s\n", findname );
		Com_Printf( "----\n" );

		if ( ( dirnames = FS_ListFiles( findname, &ndirs, 0, 0 ) ) != 0 )
		{
			int i;

			for ( i = 0; i < ndirs-1; i++ )
			{
				if ( strrchr( dirnames[i], '/' ) )
					Com_Printf( "%s\n", strrchr( dirnames[i], '/' ) + 1 );
				else
					Com_Printf( "%s\n", dirnames[i] );

				free( dirnames[i] );
			}
			free( dirnames );
		}
		Com_Printf( "\n" );
	};
}

/*
============
FS_Path_f

============
*/
void FS_Path_f (void)
{
	searchpath_t	*s;
	filelink_t		*l;

	Com_Printf ("Current search path:\n");
	for (s=fs_searchpaths ; s ; s=s->next)
	{
		if (s == fs_base_searchpaths)
			Com_Printf ("----------\n");
		if (s->pack)
			Com_Printf ("%s (%i files)\n", s->pack->filename, s->pack->numfiles);
		else
			Com_Printf ("%s\n", s->filename);
	}

	Com_Printf ("\nLinks:\n");
	for (l=fs_links ; l ; l=l->next)
		Com_Printf ("%s : %s\n", l->from, l->to);
}

/*
================
FS_NextPath

Allows enumerating all of the directories in the search path
================
*/
char *FS_NextPath (char *prevpath)
{
	searchpath_t	*s;
	char			*prev;

	if (!prevpath)
		return fs_gamedir;

	prev = fs_gamedir;
	for (s=fs_searchpaths ; s ; s=s->next)
	{
		if (s->pack)
			continue;
		if (prevpath == prev)
			return s->filename;
		prev = s->filename;
	}

	return NULL;
}


/*
================
FS_InitFilesystem
================
*/
void FS_InitFilesystem (void)
{
	Cmd_AddCommand ("path", FS_Path_f);
	Cmd_AddCommand ("link", FS_Link_f);
	Cmd_AddCommand ("dir", FS_Dir_f );

	//
	// basedir <path>
	// allows the game to run from outside the data tree
	//
	fs_basedir = Cvar_Get ("basedir", ".", CVAR_NOSET);

	//
	// cddir <path>
	// Logically concatenates the cddir after the basedir for 
	// allows the game to run from outside the data tree
	//
	fs_cddir = Cvar_Get ("cddir", "", CVAR_NOSET);
	if (fs_cddir->string[0])
		FS_AddGameDirectory (va("%s/"BASEDIRNAME, fs_cddir->string) );

	//
	// start up with baseq2 by default
	//
	FS_AddGameDirectory (va("%s/"BASEDIRNAME, fs_basedir->string) );

	// any set gamedirs will be freed up to here
	fs_base_searchpaths = fs_searchpaths;

	// check for game override
	fs_gamedirvar = Cvar_Get ("game", "", CVAR_LATCH|CVAR_SERVERINFO);
	if (fs_gamedirvar->string[0])
		FS_SetGamedir (fs_gamedirvar->string);
}



