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
// snd_mem.c: sound caching

#include "client.h"
#include "snd_loc.h"
#include "../libs/mpglib/mpg123.h" //Heffo - MP3 Audio Support
#include "../libs/mpglib/mpglib.h"     //Heffo - MP3 Audio Support
sfxcache_t *S_LoadMP3Sound (sfx_t *s);  //Heffo - MP3 Audio Support

//int			cache_full_cycle; // Vic - stereo sound
//byte *S_Alloc (int size); // Vic - stereo sound

/*
================
ResampleSfx	// Vic - stereo sound
================
*/
void ResampleSfx (sfxcache_t *sc, byte *data, char *name)
{
	int i, outcount, srcsample, srclength, samplefrac, fracstep;

	// this is usually 0.5 (128), 1 (256), or 2 (512)
	fracstep = ((double) sc->speed / (double) dma.speed) * 256.0;

	srclength = sc->length << sc->stereo;
	outcount = (double) sc->length * (double) dma.speed / (double) sc->speed;

	sc->length = outcount;
	if (sc->loopstart != -1)
		sc->loopstart = (double) sc->loopstart * (double) dma.speed / (double) sc->speed;

	sc->speed = dma.speed;

// resample / decimate to the current source rate
	if (fracstep == 256)
	{
		if (sc->width == 1) // 8bit
			for (i = 0;i < srclength;i++)
				((signed char *)sc->data)[i] = ((unsigned char *)data)[i] - 128;
		else
			for (i = 0;i < srclength;i++)
				((short *)sc->data)[i] = LittleShort (((short *)data)[i]);
	}
	else
	{
// general case
		Com_DPrintf("ResampleSfx: resampling sound %s\n", name);
		samplefrac = 0;

		if ((fracstep & 255) == 0) // skipping points on perfect multiple
		{
			srcsample = 0;
			fracstep >>= 8;
			if (sc->width == 2)
			{
				short *out = (void *)sc->data, *in = (void *)data;
				if (sc->stereo)
				{
					fracstep <<= 1;
					for (i=0 ; i<outcount ; i++)
					{
						*out++ = LittleShort (in[srcsample  ]);
						*out++ = LittleShort (in[srcsample+1]);
						srcsample += fracstep;
					}
				}
				else
				{
					for (i=0 ; i<outcount ; i++)
					{
						*out++ = LittleShort (in[srcsample  ]);
						srcsample += fracstep;
					}
				}
			}
			else
			{
				signed char *out = (void *)sc->data;
				unsigned char *in = (void *)data;

				if (sc->stereo)
				{
					fracstep <<= 1;
					for (i=0 ; i<outcount ; i++)
					{
						*out++ = in[srcsample  ] - 128;
						*out++ = in[srcsample+1] - 128;
						srcsample += fracstep;
					}
				}
				else
				{
					for (i=0 ; i<outcount ; i++)
					{
						*out++ = in[srcsample  ] - 128;
						srcsample += fracstep;
					}
				}
			}
		}
		else
		{
			int sample;
			int a, b;
			if (sc->width == 2)
			{
				short *out = (void *)sc->data, *in = (void *)data;

				if (sc->stereo)
				{
					for (i=0 ; i<outcount ; i++)
					{
						srcsample = (samplefrac >> 8) << 1;
						a = in[srcsample  ];
						if (srcsample+2 >= srclength)
							b = 0;
						else
							b = in[srcsample+2];
						sample = (((b - a) * (samplefrac & 255)) >> 8) + a;
						*out++ = (short) sample;
						a = in[srcsample+1];
						if (srcsample+2 >= srclength)
							b = 0;
						else
							b = in[srcsample+3];
						sample = (((b - a) * (samplefrac & 255)) >> 8) + a;
						*out++ = (short) sample;
						samplefrac += fracstep;
					}
				}
				else
				{
					for (i=0 ; i<outcount ; i++)
					{
						srcsample = samplefrac >> 8;
						a = in[srcsample  ];
						if (srcsample+1 >= srclength)
							b = 0;
						else
							b = in[srcsample+1];
						sample = (((b - a) * (samplefrac & 255)) >> 8) + a;
						*out++ = (short) sample;
						samplefrac += fracstep;
					}
				}
			}
			else
			{
				signed char *out = (void *)sc->data;
				unsigned char *in = (void *)data;
				if (sc->stereo) // LordHavoc: stereo sound support
				{
					for (i=0 ; i<outcount ; i++)
					{
						srcsample = (samplefrac >> 8) << 1;
						a = (int) in[srcsample  ] - 128;
						if (srcsample+2 >= srclength)
							b = 0;
						else
							b = (int) in[srcsample+2] - 128;
						sample = (((b - a) * (samplefrac & 255)) >> 8) + a;
						*out++ = (signed char) sample;
						a = (int) in[srcsample+1] - 128;
						if (srcsample+2 >= srclength)
							b = 0;
						else
							b = (int) in[srcsample+3] - 128;
						sample = (((b - a) * (samplefrac & 255)) >> 8) + a;
						*out++ = (signed char) sample;
						samplefrac += fracstep;
					}
				}
				else
				{
					for (i=0 ; i<outcount ; i++)
					{
						srcsample = samplefrac >> 8;
						a = (int) in[srcsample  ] - 128;
						if (srcsample+1 >= srclength)
							b = 0;
						else
							b = (int) in[srcsample+1] - 128;
						sample = (((b - a) * (samplefrac & 255)) >> 8) + a;
						*out++ = (signed char) sample;
						samplefrac += fracstep;
					}
				}
			}
		}
	}
}

//=============================================================================

/*
==============
S_LoadSound
==============
*/
sfxcache_t *S_LoadSound (sfx_t *s)
{
    char	namebuffer[MAX_QPATH];
	byte	*data;
	wavinfo_t	info;
	int		len;
//	float	stepscale;
	sfxcache_t	*sc;
	int		size;
	char	*name;

	if (!s->name[0] || s->name[0] == '*')
		return NULL;

// see if still in memory
	sc = s->cache;
	if (sc)
		return sc;

//Com_Printf ("S_LoadSound: %x\n", (int)stackbuf);
// load it in
	if (s->truename)
		name = s->truename;
	else
		name = s->name;

	if (name[0] == '#') 
		Com_sprintf (namebuffer, sizeof(namebuffer), &name[1]); // Vic - stereo sound
    else 
		Com_sprintf (namebuffer, sizeof(namebuffer), "sound/%s", name);

//	Com_Printf ("loading %s\n",namebuffer);

	//Heffo - MP3 Audio Support
	len = strlen(name);                // Find the length of the filename
	if(!strcmp(name+(len-4), ".mp3"))    // compare the last 4 characters of the filename
		return S_LoadMP3Sound(s);  // An MP3, so return the MP3 loader's return result

	size = FS_LoadFile (namebuffer, (void **)&data);

	if (!data)
	{
		Com_DPrintf ("Couldn't load %s\n", namebuffer);
		return NULL;
	}

	info = GetWavinfo (s->name, data, size);
	if (info.channels < 1 || info.channels > 2)	// Vic - stereo sound
	{
		Com_Printf ("%s has an invalid number of channels\n", s->name);
		FS_FreeFile (data);
		return NULL;
	}

//	stepscale = (float)info.rate / dma.speed;	
//	len = info.samples / stepscale;

	// calculate resampled length
	len = (int) ((double) info.samples * (double) dma.speed / (double) info.rate);
	len = len * info.width * info.channels;

	sc = s->cache = Z_Malloc (len + sizeof(sfxcache_t));
	if (!sc)
	{
		FS_FreeFile (data);
		return NULL;
	}
	
	sc->length = info.samples;
	sc->loopstart = info.loopstart;
	sc->speed = info.rate;
	sc->width = info.width;
	sc->stereo = (info.channels == 2);
	sc->music = !strncmp (namebuffer, "music/", 6);

	// force loopstart if it's a music file
	if ( sc->music && (sc->loopstart == -1) ) {
		sc->loopstart = 0;
	}

	ResampleSfx (sc, data + info.dataofs, s->name);

	FS_FreeFile (data);

	return sc;
}



/*
===============================================================================

WAV loading

===============================================================================
*/


byte	*data_p;
byte 	*iff_end;
byte 	*last_chunk;
byte 	*iff_data;
int 	iff_chunk_len;


short GetLittleShort(void)
{
	short val = 0;
	val = *data_p;
	val = val + (*(data_p+1)<<8);
	data_p += 2;
	return val;
}

int GetLittleLong(void)
{
	int val = 0;
	val = *data_p;
	val = val + (*(data_p+1)<<8);
	val = val + (*(data_p+2)<<16);
	val = val + (*(data_p+3)<<24);
	data_p += 4;
	return val;
}

void FindNextChunk(char *name)
{
	while (1)
	{
		data_p = last_chunk;

		if (data_p >= iff_end)
		{	// didn't find the chunk
			data_p = NULL;
			return;
		}
		
		data_p += 4;
		iff_chunk_len = GetLittleLong();
		if (iff_chunk_len < 0)
		{
			data_p = NULL;
			return;
		}
//		if (iff_chunk_len > 1024*1024)
//			Sys_Error ("FindNextChunk: %i length is past the 1 meg sanity limit", iff_chunk_len);
		data_p -= 8;
		last_chunk = data_p + 8 + ( (iff_chunk_len + 1) & ~1 );
		if (!strncmp(data_p, name, 4))
			return;
	}
}

void FindChunk(char *name)
{
	last_chunk = iff_data;
	FindNextChunk (name);
}


void DumpChunks(void)
{
	char	str[5];
	
	str[4] = 0;
	data_p = iff_data;
	do
	{
		memcpy (str, data_p, 4);
		data_p += 4;
		iff_chunk_len = GetLittleLong();
		Com_Printf ("0x%x : %s (%d)\n", (int)(data_p - 4), str, iff_chunk_len);
		data_p += (iff_chunk_len + 1) & ~1;
	} while (data_p < iff_end);
}

/*
============
GetWavinfo
============
*/
wavinfo_t GetWavinfo (char *name, byte *wav, int wavlength)
{
	wavinfo_t	info;
	int     i;
	int     format;
	int		samples;

	memset (&info, 0, sizeof(info));

	if (!wav)
		return info;
		
	iff_data = wav;
	iff_end = wav + wavlength;

// find "RIFF" chunk
	FindChunk("RIFF");
	if (!(data_p && !strncmp((char *)data_p+8, "WAVE", 4)))
	{
		Com_Printf("Missing RIFF/WAVE chunks\n");
		return info;
	}

// get "fmt " chunk
	iff_data = data_p + 12;
// DumpChunks ();

	FindChunk("fmt ");
	if (!data_p)
	{
		Com_Printf("Missing fmt chunk\n");
		return info;
	}
	data_p += 8;
	format = GetLittleShort();
	if (format != 1)
	{
		Com_Printf("Microsoft PCM format only\n");
		return info;
	}

	info.channels = GetLittleShort();
	info.rate = GetLittleLong();
	data_p += 4+2;
	info.width = GetLittleShort() / 8;

// get cue chunk
	FindChunk("cue ");
	if (data_p)
	{
		data_p += 32;
		info.loopstart = GetLittleLong();
//		Com_Printf("loopstart=%d\n", sfx->loopstart);

	// if the next chunk is a LIST chunk, look for a cue length marker
		FindNextChunk ("LIST");
		if (data_p)
		{
			if (!strncmp ((char *)data_p + 28, "mark", 4))
			{	// this is not a proper parse, but it works with cooledit...
				data_p += 24;
				i = GetLittleLong ();	// samples in loop
				info.samples = info.loopstart + i;
				//				Com_Printf("looped length: %i\n", i);
			}
		}
	}
	else
		info.loopstart = -1;

	// find data chunk
	FindChunk("data");
	if (!data_p)
	{
		Com_Printf("Missing data chunk\n");
		return info;
	}

	data_p += 4;
	samples = GetLittleLong () / info.width / info.channels;

	if (info.samples)
	{
		if (samples < info.samples)
			Com_Error (ERR_DROP, "Sound %s has a bad loop length", name);
	}
	else
		info.samples = samples;

	info.dataofs = data_p - wav;

	return info;
}

/*
===============================================================================

MP3 Audio Support
By Robert 'Heffo' Heffernan

===============================================================================
*/
#define TEMP_BUFFER_SIZE 0x100000 // 1Mb Temporary buffer for storing the decompressed MP3 data

// MP3_New - Allocate & initialise a new mp3_t structure
mp3_t *MP3_New (char *filename)
{
	mp3_t *mp3;

	// Allocate a piece of ram the same size as the mp3_t structure
	mp3 = malloc(sizeof(mp3_t));
	if(!mp3)
		return NULL; // Couldn't allocate it, so return nothing

	// Wipe the freshly allocated mp3_t structure
	memset(mp3, 0, sizeof(mp3_t));

	// Load the MP3 file via the filesystem routines to ensure PAK file compatability
	// set the mp3->mp3data pointer to the memory location of the newly loaded file
	// and the mp3->mp3size value to the size of the file
	mp3->mp3size = FS_LoadFile(filename, (void **)&mp3->mp3data);
	if(!mp3->mp3data)
	{
		// The File couldn't be loaded so free the memory used by the mp3_t structure
		free(mp3);

		// Then return nothing;
		return NULL;
	}

	// The mp3_t structure was allocated, and the file loaded, so return the structure
	return mp3;
}

// MP3_Free - Release an mp3_t structure, and free all memory used by it
void MP3_Free (mp3_t *mp3)
{
	// Was an mp3_t to be released passed in?
	if(!mp3)
		return; // Nope, so just return

	// Does the mp3_t still have the file loaded?
	if(mp3->mp3data)
		FS_FreeFile(mp3->mp3data); // Yes, so free the file

	// Does the mp3_t have any raw audio data allocated?
	if(mp3->rawdata)
		free(mp3->rawdata); // Yes, so free the raw data

	// All other memory has been freed, so free the mp3_t itself
	free(mp3);
}

// ExtendRawBuffer - Allocates & extends a buffer for the raw decompressed audio data
int ExtendRawBuffer (mp3_t *mp3, int size)
{
	byte *newbuf;

	// Was a valid mp3_t provided?
	if(!mp3)
		return 0; // Nope, so return a failure

	// Are we missing a buffer for the raw data?
	if(!mp3->rawdata)
	{
		// Yes, so make one the size requested
		mp3->rawdata = malloc(size);
		if(!mp3->rawdata)
			return 0; // We couldn't allocate the memory, so return a failure

		// Set the size of the buffer and return success
		mp3->rawsize = size;
		return 1;
	}

	// Make a new buffer the size of the old one, plus the extra size requested
	newbuf = malloc(mp3->rawsize + size);
	if(!newbuf)
		return 0; // We couldn't allocate the memory, so return a failure

	// Copy the contents of the old buffer into the new one
	memcpy(newbuf, mp3->rawdata, mp3->rawsize);

	// Increase the buffer size by the amount requested
	mp3->rawsize += size;

	// Release the old buffer
	free(mp3->rawdata);

	// Point the mp3_t's rawbuffer to the address of the new buffer
	mp3->rawdata = newbuf;

	// Everything went okay, so return success
	return 1;
}

int MP3_Process (mp3_t *mp3)
{
	struct mpstr	mpeg;
	byte			sbuff[8192];
	byte			*tbuff, *tb;
	int				size, ret, tbuffused;

	// Was a valid mp3_t provided?
	if(!mp3)
		return 0; // Nope, so return a failure

	// Do we have any MP3 data to decompress?
	if(!mp3->mp3data)
		return 0; // Nope, so return a failure

	// Allocate room for a large tempoary decode buffer
	tbuff = malloc(TEMP_BUFFER_SIZE);
	if(!tbuff)
		return 0; // Couldn't allocate the room, so return a failure

	// Initialise the MP3 decoder
	InitMP3(&mpeg);

	// Decode the 1st frame of MP3 data, and store it in the tempoary buffer
	ret = decodeMP3(&mpeg, mp3->mp3data, mp3->mp3size, tbuff, TEMP_BUFFER_SIZE, &size);

	// Copy the MP3s format details like samplerate, and number of channels
	mp3->samplerate = freqs[mpeg.fr.sampling_frequency];
	mp3->channels = mpeg.fr.stereo;

	// Set a pointer to the start of the tempoary buffer and then offset it by the number of
	// bytes decoded. Also set the amout of the temp buffer that has been used
	tb = tbuff + size;
	tbuffused = size;

	// Start a loop that ends when the MP3 decoder fails to return successfully
	while(ret == MP3_OK)
	{
		// Decode the next frame of MP3 data into a smaller tempoary buffer
		ret = decodeMP3(&mpeg, NULL, 0, sbuff, 8192, &size);

		// Check to see if we have enough room in the large tempoary buffer to store the new
		// sample data
		if(tbuffused+size >= TEMP_BUFFER_SIZE)
		{
			// Nope, so extend the mp3_t structures raw sample buffer by the amount of the
			// large temp buffer that has been used
			if(!ExtendRawBuffer(mp3, tbuffused))
			{
				// Failed to extend the sample buffer, so free the large temp buffer
				free(tbuff);

				// Shutdown the MP3 decoder
				ExitMP3(&mpeg);

				// Return a failure
				return 0;
			}

			// Copy the large tempoary buffer into the freshly extended sample buffer
			memcpy(mp3->rawdata + mp3->rawpos, tbuff, tbuffused);

			// Reset the large tempoary buffer, and extend the reported size of the sample
			// buffer
			tbuffused = 0; tb = tbuff;
			mp3->rawpos = mp3->rawsize;
		}

		// Copy the small tempoary buffer into the large tempoary buffer, and adjust the
		// amount used
		memcpy(tb, sbuff, size);
		tb+=size; tbuffused+=size;
	}

	// All decoding has been done so flush the large tempoary buffer into the sample buffer
	// Extend the mp3_t structures raw sample buffer by the amount of the large temp buffer
	// that has been used
	if(!ExtendRawBuffer(mp3, tbuffused))
	{
		// Failed to extend the sample buffer, so free the large temp buffer
		free(tbuff);

		// Shutdown the MP3 decoder
		ExitMP3(&mpeg);

		// Return a failure
		return 0;
	}

	// Copy the large tempoary buffer into the freshly extended sample buffer
	memcpy(mp3->rawdata + mp3->rawpos, tbuff, tbuffused);

	// Calculate the number of samples based on the size of the decompressed MP3 data
	// divided by two for 16bit per sample, then divided by the number of channels in the MP3
	mp3->samples = (mp3->rawsize / 2) / mp3->channels;

	// Free the large temporary buffer
	free(tbuff);

	// Shutdown the MP3 decoder
	ExitMP3(&mpeg);

	// Everything worked, so return success
	return 1;
}	

// S_LoadMP3Sound - A modified version of S_LoadSound that supports MP3 files
sfxcache_t *S_LoadMP3Sound (sfx_t *s)
{
	char	namebuffer[MAX_QPATH];
	mp3_t	*mp3;
	float	stepscale;
	sfxcache_t	*sc;
	char	*name;

	// Workout the filename being opened
	if (s->truename)
		name = s->truename;
	else
		name = s->name;

	if (name[0] == '#')
		strcpy(namebuffer, &name[1]);
	else
		Com_sprintf (namebuffer, sizeof(namebuffer), "sound/%s", name);


	// Allocate an mp3_t structure for the new MP3 file
	mp3 = MP3_New(namebuffer);
	if(!mp3)
	{
		// Couldn't allocate it, so show an error and return nothing
		Com_DPrintf ("Couldn't load %s\n", namebuffer);
		return NULL;
	}

	// Process the mp3_t and check to see if something went wrong
	if(!MP3_Process(mp3))
	{
		// Something did, so free the mp3_t, show an error and return nothing
		MP3_Free(mp3);
		Com_DPrintf ("Couldn't load %s\n", namebuffer);
		return NULL;
	}

	/*// Make sure the MP3 is a mono file, reject it if it isn't
	if(mp3->channels == 2)
	{
		// It's a stereo file, so free it, show an error and return nothing
		MP3_Free(mp3);
		Com_Printf ("%s is a stereo sample\n",s->name);
		return NULL;
	}*/

	// NeVo - I am such an idiot... can't believe I forgot to enable the stereo here

	// Make sure the MP3 is a valid file, reject it if it isn't
	if ((mp3->channels < 1) || (mp3->channels > 2))	// Idle
	{
		// It's an invalid file, so free it, show an error and return nothing
		MP3_Free(mp3);
		Com_Printf ("%s is an invalid sample\n", s->name);	// NeVo
		return NULL;
	}

	// Work out a scale to bring the samplerate of the MP3 and Quake2's playback rate inline
	stepscale = (float)mp3->samplerate / dma.speed;

	// Allocate a piece of zone memory to store the decompressed & scaled MP3 aswell as it's
	// attatched sfxcache_t structure
	sc = s->cache = Z_Malloc ((mp3->rawsize / stepscale) + sizeof(sfxcache_t));
	if (!sc)
	{
		// Couldn't allocate the zone, so free the MP3, show an error and return nothing
		MP3_Free(mp3);
		Com_DPrintf ("Couldn't load %s\n", namebuffer);
		return NULL;
	}

	// Copy a few details of the MP3 into the sfxcache_t structure
	sc->length = mp3->samples;
	sc->loopstart = -1;
	sc->speed = mp3->samplerate;
	sc->width = 2;	// Always 16 Bit?
	sc->stereo = mp3->channels;

	// Resample the decompressed MP3 to match Quake II's samplerate
	ResampleSfx (sc, mp3->rawdata, s->name);	// NeVo/Idle

	// Free the mp3_t structure as it's nolonger needed
	MP3_Free(mp3);

	// Return the sfxcache_t structure for the decoded & processed MP3
	return sc;
}


