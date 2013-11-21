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
NeVo
Now uses audio through DirectX 8 and 9
*/
#include <float.h>

#include "../client/client.h"
#include "../client/snd_loc.h"
#include "winquake.h"

#define CHANNELS	2		// Stereo
#define SAMPLEBITS	16

HRESULT (WINAPI *pDirectSoundCreate8)(LPCGUID FAR *lpcGuidDevice, LPDIRECTSOUND8 FAR *ppDS8, LPUNKNOWN  FAR *pUnkOuter);

// 64K is > 1 second at 16-bit, 22050 Hz
#define WAV_BUFFERS						64
#define	WAV_MASK						0x3F
#define	WAV_BUFFER_SIZE					0x0400
#define SECONDARY_BUFFER_SIZE			0x10000

typedef enum sndinitstat_e {SIS_SUCCESS, SIS_FAILURE, SIS_NOTAVAIL} sndinitstat;

//cvar_t	*s_wavonly;
cvar_t	*s_buffersize;
//cvar_t	*s_openal;
cvar_t	*s_soundsystem;
cvar_t	*s_soundquality;

static int	dsound_init;
static int openal_init;
static int	wav_init;
static int	snd_firsttime = 1, snd_isdirect, snd_isopenal, snd_iswave;
static int	primary_format_set;

// starts at 0 for disabled
static int	snd_buffer_count = 0;
static int	sample16;
static int	snd_sent, snd_completed;

/* 
 * Global variables. Must be visible to window-procedure function 
 *  so it can unlock and free the data block after it has been played. 
 */ 

HANDLE		hData;
HPSTR		lpData, lpData2;

HGLOBAL		hWaveHdr;
LPWAVEHDR	lpWaveHdr;

HWAVEOUT    hWaveOut; 

WAVEOUTCAPS	wavecaps;

DWORD	gSndBufSize;

MMTIME		mmstarttime;

LPDIRECTSOUND8 pDS = NULL;				// Sound Device
LPDIRECTSOUNDBUFFER	pDSPBuf = NULL;		// Primary Buffer
//LPDIRECTSOUNDBUFFER8 pDSBuf;			// Secondary Buffer
LPDIRECTSOUNDBUFFER	pDSBuf = NULL;		// Secondary Buffer

/*
NeVo
myDirectSoundIID is our bitch. It is used to
pass the IID's to DirectX because the compiler
is stupid and typecasts don't work. It must be
set to the address of the IID, and then passed
to whatever function needs the IID.
*/
qIID myDirectSoundIID;

HINSTANCE hInstDS;

sndinitstat SNDDMA_InitDirect (void);
int SNDDMA_InitWav (void);

void FreeSound( void );

static const char *DSoundError( int error )
{
	switch ( error )
	{
	case DSERR_ACCESSDENIED:
        return "DSERR_ACCESSDENIED";
	case DSERR_ALLOCATED:
		return "DSERR_ALLOCATED";
	case DSERR_ALREADYINITIALIZED:
		return "DSERR_ALREADYINITIALIZED";
	case DSERR_BADFORMAT:
		return "DSERR_BADFORMAT";
	case DSERR_BADSENDBUFFERGUID:
		return "DSERR_BADSENDBUFFERGUID";
	case DSERR_BUFFERLOST:
		return "DSERR_BUFFERLOST";
	case DSERR_BUFFERTOOSMALL:
		return "DSERR_BUFFERTOOSMALL";
	case DSERR_CONTROLUNAVAIL:
		return "DSERR_CONTROLUNAVAIL";
	case DSERR_DS8_REQUIRED:
		return "DSERR_DS8_REQUIRED";
	case DSERR_FXUNAVAILABLE:
		return "DSERR_FXUNAVAILABLE";
	case DSERR_GENERIC:
		return "DSERR_GENERIC";
	case DSERR_INVALIDCALL:
		return "DSERR_INVALIDCALL";
	case DSERR_INVALIDPARAM:
		return "DSERR_INVALIDPARAM";
	case DSERR_NOAGGREGATION:
		return "DSERR_NOAGGREGATION";
	case DSERR_NODRIVER:
		return "DSERR_NODRIVER";
	case DSERR_NOINTERFACE:
		return "DSERR_NOINTERFACE";
	case DSERR_OBJECTNOTFOUND:
		return "DSERR_OBJECTNOTFOUND";
	case DSERR_OTHERAPPHASPRIO:
		return "DSERR_OTHERAPPHASPRIO";
	case DSERR_OUTOFMEMORY:
		return "DSERR_OUTOFMEMORY";
	case DSERR_PRIOLEVELNEEDED:
		return "DSERR_PRIOLEVELNEEDED";
	case DSERR_SENDLOOP:
		return "DSERR_SENDLOOP";
	case DSERR_UNINITIALIZED:
		return "DSERR_UNINITIALIZED";
	case DSERR_UNSUPPORTED:
		return "DSERR_UNSUPPORTED";
	}

	return "Unknown DirectSound Error";
}

/*
** DS_CreateBuffers
*/
static int DS_CreateBuffers( void )
{
	DSBUFFERDESC	dsbuf;
	DSBCAPS			dsbcaps;
	WAVEFORMATEX	pformat, format;
	DWORD			dwWrite;

	memset (&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = dma.channels;
    format.wBitsPerSample = dma.samplebits;
    format.nSamplesPerSec = dma.speed;
    format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
    format.cbSize = 0;
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign; 

	Com_Printf( "Creating DirectSound 8 buffers\n" );

	Com_Printf("...setting PRIORITY coop level: " );
#ifdef QDSNDCOMPILERHACK
	if ( DS_OK != pDS->lpVtbl->SetCooperativeLevel( pDS, cl_hwnd, DSSCL_PRIORITY ) )
#else
	if ( DS_OK != pDS->SetCooperativeLevel( cl_hwnd, DSSCL_PRIORITY ) )
#endif
	{
		Com_Printf ("failed\n");
		FreeSound ();
		return 0;
	}
	Com_Printf("ok\n" );

// get access to the primary buffer, if possible, so we can set the
// sound hardware format
	memset (&dsbuf, 0, sizeof(dsbuf));
	dsbuf.dwSize = sizeof(DSBUFFERDESC);
	dsbuf.dwFlags = DSBCAPS_PRIMARYBUFFER; // | DSBCAPS_CTRL3D;
	dsbuf.dwBufferBytes = 0;
	dsbuf.lpwfxFormat = NULL;

	memset(&dsbcaps, 0, sizeof(dsbcaps));
	dsbcaps.dwSize = sizeof(dsbcaps);
	primary_format_set = 0;
	
	if (!pDSPBuf)
	{
		Com_Printf( "...creating primary buffer: " );
#ifdef QDSNDCOMPILERHACK
		if (DS_OK == pDS->lpVtbl->CreateSoundBuffer(pDS, &dsbuf, &pDSPBuf, NULL))
#else
		if (DS_OK == pDS->CreateSoundBuffer(&dsbuf, &pDSPBuf, NULL))
#endif
		{
			pformat = format;

			Com_Printf( "ok\n" );
#ifdef QDSNDCOMPILERHACK
			if (DS_OK != pDSPBuf->lpVtbl->SetFormat (pDSPBuf, &pformat))
#else
			if (DS_OK != pDSPBuf->SetFormat (&pformat))
#endif
			{
				if (snd_firsttime)
					Com_Printf ("...setting primary sound format: failed\n");
			}
			else
			{
				if (snd_firsttime)
					Com_Printf ("...setting primary sound format: ok\n");

				primary_format_set = 1;
			}
		}
		else
			Com_Printf( "failed\n" );
	}

	if ( !primary_format_set || !s_primary->value)
	{
		// create the secondary buffer we'll actually work with
		memset (&dsbuf, 0, sizeof(DSBUFFERDESC));
		dsbuf.dwSize = sizeof(DSBUFFERDESC);
		dsbuf.dwFlags = DSBCAPS_CTRLFREQUENCY | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCHARDWARE; //| DSBCAPS_MUTE3DATMAXDISTANCE; // | DSBCAPS_CTRL3D;	
		//dsbuf.dwFlags |= DSBCAPS_CTRLVOLUME;			// Allow volume control
		//dsbuf.dwBufferBytes = format.nAvgBytesPerSec * SECONDARY_BUFFER_LEN_SECONDS;
		dsbuf.dwBufferBytes = SECONDARY_BUFFER_SIZE * s_buffersize->value;
		dsbuf.lpwfxFormat = &format;
		//dsbuf.guid3DAlgorithm = DS3DALG_DEFAULT;
		//dsbuf.guid3DAlgorithm = DS3DALG_HRTF_FULL;	// Use high quality 3D processing

		memset(&dsbcaps, 0, sizeof(dsbcaps));
		dsbcaps.dwSize = sizeof(dsbcaps);

		Com_Printf( "...creating secondary buffer: " );

		//pDSPBuf->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*) pDSBuf);
		//myDirectSoundIID = &IID_IDirectSoundBuffer8;
		//pDSPBuf->lpVtbl->QueryInterface(pDSPBuf, myDirectSoundIID, &pDSBuf);

		if (!pDSBuf)
		{
			//MessageBox (NULL, "Got Here!\n\n", "Quake II", MB_OK | MB_SETFOREGROUND | MB_ICONEXCLAMATION);
#ifdef QDSNDCOMPILERHACK
			if (DS_OK != pDS->lpVtbl->CreateSoundBuffer(pDS, &dsbuf, &pDSBuf, NULL))
#else
			if (DS_OK != pDS->CreateSoundBuffer(&dsbuf, &pDSBuf, NULL))
#endif
			//if (DS_OK != pDSPBuf->lpVtbl->QueryInterface(pDSPBuf, myDirectSoundIID, &pDSBuf))
			{
				Com_Printf( "failed\n" );
				Com_Printf( "Attempting to locate buffer in software...\n");
				dsbuf.dwFlags = DSBCAPS_CTRLFREQUENCY | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCSOFTWARE;
				//pDSBuf->lpVtbl->Release(pDSBuf);
				pDSBuf = NULL;
#ifdef QDSNDCOMPILERHACK
				if (DS_OK != pDS->lpVtbl->CreateSoundBuffer(pDS, &dsbuf, &pDSBuf, NULL))
#else
				if (DS_OK != pDS->CreateSoundBuffer(&dsbuf, &pDSBuf, NULL))
#endif
				//if (DS_OK != pDSPBuf->lpVtbl->QueryInterface(pDSPBuf, myDirectSoundIID, &pDSBuf))
				{
					// NeVo - try switching to primary
					s_primary->value = 1;
					Com_Printf( "failed\n" );
					Com_Printf( "Attempting fallback to primary buffer...\n");
					//pDSBuf->lpVtbl->Release(pDSBuf);
					pDSBuf = NULL;
					return DS_CreateBuffers();
					//FreeSound ();
					//return 0;
				}
				Com_Printf( "ok\n" );
				Com_Printf( "Sound Buffer in Software\n" );
			}
			else
			{
			Com_Printf( "ok\n" );
			Com_Printf( "Sound Buffer in Hardware\n" );
			}
		}

		dma.channels = format.nChannels;
		dma.samplebits = format.wBitsPerSample;
		dma.speed = format.nSamplesPerSec;

#ifdef QDSNDCOMPILERHACK
		if (DS_OK != pDSBuf->lpVtbl->GetCaps (pDSBuf, &dsbcaps))
#else
		if (DS_OK != pDSBuf->GetCaps (&dsbcaps))
#endif
		{
			Com_Printf ("*** GetCaps failed ***\n");
			FreeSound ();
			return 0;
		}

		Com_Printf ("...using secondary sound buffer\n");
	}
	else
	{
		Com_Printf( "...using primary buffer\n" );

		Com_Printf( "...setting WRITEPRIMARY coop level: " );
#ifdef QDSNDCOMPILERHACK
		if (DS_OK != pDS->lpVtbl->SetCooperativeLevel (pDS, cl_hwnd, DSSCL_WRITEPRIMARY))
#else
		if (DS_OK != pDS->SetCooperativeLevel (cl_hwnd, DSSCL_WRITEPRIMARY))
#endif
		{
			Com_Printf( "failed\n" );
			FreeSound ();
			return 0;
		}
		Com_Printf( "ok\n" );

#ifdef QDSNDCOMPILERHACK
		if (DS_OK != pDSPBuf->lpVtbl->GetCaps (pDSPBuf, &dsbcaps))
#else
		if (DS_OK != pDSPBuf->GetCaps (&dsbcaps))
#endif
		{
			Com_Printf ("*** GetCaps failed ***\n");
			return 0;
		}

		pDSBuf = pDSPBuf;
	}

	// Make sure mixer is active
	//if ( !s_primary->value)
#ifdef QDSNDCOMPILERHACK
		pDSBuf->lpVtbl->Play(pDSBuf, 0, 0, DSBPLAY_LOOPING);
#else
	pDSBuf->Play (0, 0, DSBPLAY_LOOPING);
#endif
	//else
	//	pDSPBuf->lpVtbl->Play(pDSPBuf, 0, 0, DSBPLAY_LOOPING);
	
	if (snd_firsttime)
		Com_Printf("   %d channel(s)\n"
		               "   %d bits/sample\n"
					   "   %d bytes/sec\n",
					   dma.channels, dma.samplebits, dma.speed);
	
	gSndBufSize = dsbcaps.dwBufferBytes;

	/* we don't want anyone to access the buffer directly w/o locking it first. */
	lpData = NULL; 

	//if ( !s_primary->value)
	//{
#ifdef QDSNDCOMPILERHACK
		pDSBuf->lpVtbl->Stop(pDSBuf);
		pDSBuf->lpVtbl->GetCurrentPosition(pDSBuf, &mmstarttime.u.sample, &dwWrite);
		pDSBuf->lpVtbl->Play(pDSBuf, 0, 0, DSBPLAY_LOOPING);
#else
		pDSBuf->Stop();
		pDSBuf->GetCurrentPosition(&mmstarttime.u.sample, &dwWrite);
		pDSBuf->Play(0, 0, DSBPLAY_LOOPING);
#endif
	//}
	//else
	//{
	//	pDSPBuf->lpVtbl->Stop(pDSPBuf);
	//	pDSPBuf->lpVtbl->GetCurrentPosition(pDSPBuf, &mmstarttime.u.sample, &dwWrite);
	//	pDSPBuf->lpVtbl->Play(pDSPBuf, 0, 0, DSBPLAY_LOOPING);
	//}
	
	dma.samples = gSndBufSize/(dma.samplebits/8);
	dma.samplepos = 0;
	dma.submission_chunk = 1;
	dma.buffer = (unsigned char *) lpData;
	sample16 = (dma.samplebits/8) - 1;

	return 1;
}

/*
** DS_DestroyBuffers
*/
static void DS_DestroyBuffers( void )
{
	Com_Printf( "Destroying DirectSound 8 buffers\n" );
	if ( pDS )
	{
		Com_Printf( "...setting NORMAL coop level\n" );
#ifdef QDSNDCOMPILERHACK
		pDS->lpVtbl->SetCooperativeLevel( pDS, cl_hwnd, DSSCL_NORMAL );
#else
		pDS->SetCooperativeLevel(cl_hwnd, DSSCL_NORMAL);
#endif
	}

	if ( pDSBuf )
	{
		Com_Printf( "...stopping and releasing secondary buffer\n" );
#ifdef QDSNDCOMPILERHACK
		pDSBuf->lpVtbl->Stop(pDSBuf);
		pDSBuf->lpVtbl->Release(pDSBuf);
#else
		pDSBuf->Stop();
		pDSBuf->Release();
#endif
	}
	
	// only release primary buffer if it's not also the mixing buffer we just released
	if ( pDSPBuf && ( pDSBuf != pDSPBuf ) )
	{
		Com_Printf( "...releasing primary buffer\n" );
#ifdef QDSNDCOMPILERHACK
		pDSPBuf->lpVtbl->Stop(pDSPBuf);
		pDSPBuf->lpVtbl->Release(pDSPBuf);
#else
		pDSPBuf->Stop();
		pDSPBuf->Release();
#endif
	}

	pDSBuf = NULL;
	pDSPBuf = NULL;

	dma.buffer = NULL;
}

/*
==================
FreeSound
==================
*/
void FreeSound (void)
{
	int		i;

	Com_Printf( "Shutting down sound system\n" );

	if ( pDS )
		DS_DestroyBuffers();

	if ( hWaveOut )
	{
		Com_Printf( "...resetting waveOut\n" );
		waveOutReset (hWaveOut);

		if (lpWaveHdr)
		{
			Com_Printf( "...unpreparing headers\n" );
			for (i=0 ; i< WAV_BUFFERS ; i++)
				waveOutUnprepareHeader (hWaveOut, lpWaveHdr+i, sizeof(WAVEHDR));
		}

		Com_Printf( "...closing waveOut\n" );
		waveOutClose (hWaveOut);

		if (hWaveHdr)
		{
			Com_Printf( "...freeing WAV header\n" );
			GlobalUnlock(hWaveHdr);
			GlobalFree(hWaveHdr);
		}

		if (hData)
		{
			Com_Printf( "...freeing WAV buffer\n" );
			GlobalUnlock(hData);
			GlobalFree(hData);
		}

	}

	if ( pDS )
	{
		Com_Printf( "...releasing DirectSound 8 object\n" );
#ifdef QDSNDCOMPILERHACK
		pDS->lpVtbl->Release(pDS);
#else
		pDS->Release();
#endif
	}

	if ( hInstDS )
	{
		Com_Printf( "...freeing dsound.dll\n" );
		FreeLibrary( hInstDS );
		hInstDS = NULL;
	}

	pDS = NULL;
	pDSBuf = NULL;
	pDSPBuf = NULL;
	hWaveOut = 0;
	hData = 0;
	hWaveHdr = 0;
	lpData = NULL;
	lpWaveHdr = NULL;
	dsound_init = 0;
	wav_init = 0;
}

/*
==================
SNDDMA_InitDirect

Direct-Sound support
==================
*/
sndinitstat SNDDMA_InitDirect (void)
{
	DSCAPS			dscaps;
	HRESULT			hresult;

	dma.channels = CHANNELS;
	dma.samplebits = SAMPLEBITS;

	Com_Printf( "Initializing DirectSound 8\n");

	if ( !hInstDS )
	{
		Com_Printf( "...loading dsound.dll: " );

		hInstDS = LoadLibrary("dsound.dll");
		
		if (hInstDS == NULL)
		{
			Com_Printf ("failed\n");
			return SIS_FAILURE;
		}

		Com_Printf ("ok\n");
		pDirectSoundCreate8 = GetProcAddress(hInstDS,"DirectSoundCreate8");

		if (!pDirectSoundCreate8)
		{
			Com_Printf ("*** couldn't get DirectSound 8 process address ***\n");
			return SIS_FAILURE;
		}
	}

	Com_Printf( "...creating DirectSound 8 object: " );
	while ( ( hresult = pDirectSoundCreate8( NULL, &pDS, NULL ) ) != DS_OK )
	{
		if (hresult != DSERR_ALLOCATED)
		{
			Com_Printf( "failed\n" );
			return SIS_FAILURE;
		}

		if (MessageBox (NULL,
						"The sound hardware is in use by another application.\n\n"
					    "Select Retry to try to start sound again or Cancel to run with no sound.",
						"Sound not available",
						MB_RETRYCANCEL | MB_SETFOREGROUND | MB_ICONEXCLAMATION) != IDRETRY)
		{
			Com_Printf ("failed, hardware already in use\n" );
			return SIS_NOTAVAIL;
		}
	}
	Com_Printf( "ok\n" );

	dscaps.dwSize = sizeof(dscaps);

#ifdef QDSNDCOMPILERHACK
	if ( DS_OK != pDS->lpVtbl->GetCaps( pDS, &dscaps ) )
#else
	if ( DS_OK != pDS->GetCaps( &dscaps ) )
#endif
	{
		Com_Printf ("*** couldn't get DirectSound 8 caps ***\n");
	}

	if ( dscaps.dwFlags & DSCAPS_EMULDRIVER )
	{
		Com_Printf ("...no DirectSound 8 driver found\n" );
		FreeSound();
		return SIS_FAILURE;
	}

	if ( !DS_CreateBuffers() )
		return SIS_FAILURE;

	dsound_init = 1;

	Com_Printf("...completed successfully\n" );

	return SIS_SUCCESS;
}

/*
==================
SNDDMA_InitWav

Crappy windows multimedia base
==================
*/
int SNDDMA_InitWav (void)
{
	WAVEFORMATEX  format; 
	int				i;
	HRESULT			hr;

	Com_Printf( "Initializing wave sound\n" );
	
	snd_sent = 0;
	snd_completed = 0;

	dma.channels = CHANNELS;
	dma.samplebits = SAMPLEBITS;

	memset (&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = dma.channels;
	format.wBitsPerSample = dma.samplebits;
	format.nSamplesPerSec = dma.speed;
	format.nBlockAlign = format.nChannels
		*format.wBitsPerSample / 8;
	format.cbSize = 0;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign; 
	
	/* Open a waveform device for output using window callback. */ 
	Com_Printf ("...opening waveform device: ");
	while ((hr = waveOutOpen((LPHWAVEOUT)&hWaveOut, WAVE_MAPPER, 
					&format, 
					0, 0L, CALLBACK_NULL)) != MMSYSERR_NOERROR)
	{
		if (hr != MMSYSERR_ALLOCATED)
		{
			Com_Printf ("failed\n");
			return 0;
		}

		if (MessageBox (NULL,
						"The sound hardware is in use by another app.\n\n"
					    "Select Retry to try to start sound again or Cancel to run Quake 2 with no sound.",
						"Sound not available",
						MB_RETRYCANCEL | MB_SETFOREGROUND | MB_ICONEXCLAMATION) != IDRETRY)
		{
			Com_Printf ("hw in use\n" );
			return 0;
		}
	} 
	Com_Printf( "ok\n" );

	/* 
	 * Allocate and lock memory for the waveform data. The memory 
	 * for waveform data must be globally allocated with 
	 * GMEM_MOVEABLE and GMEM_SHARE flags. 

	*/ 
	Com_Printf ("...allocating waveform buffer: ");
	gSndBufSize = WAV_BUFFERS*WAV_BUFFER_SIZE;
	hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, gSndBufSize); 
	if (!hData) 
	{ 
		Com_Printf( " failed\n" );
		FreeSound ();
		return 0; 
	}
	Com_Printf( "ok\n" );

	Com_Printf ("...locking waveform buffer: ");
	lpData = GlobalLock(hData);
	if (!lpData)
	{ 
		Com_Printf( " failed\n" );
		FreeSound ();
		return 0; 
	} 
	memset (lpData, 0, gSndBufSize);
	Com_Printf( "ok\n" );

	/* 
	 * Allocate and lock memory for the header. This memory must 
	 * also be globally allocated with GMEM_MOVEABLE and 
	 * GMEM_SHARE flags. 
	 */ 
	Com_Printf ("...allocating waveform header: ");
	hWaveHdr = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, 
		(DWORD) sizeof(WAVEHDR) * WAV_BUFFERS); 

	if (hWaveHdr == NULL)
	{ 
		Com_Printf( "failed\n" );
		FreeSound ();
		return 0; 
	} 
	Com_Printf( "ok\n" );

	Com_Printf ("...locking waveform header: ");
	lpWaveHdr = (LPWAVEHDR) GlobalLock(hWaveHdr); 

	if (lpWaveHdr == NULL)
	{ 
		Com_Printf( "failed\n" );
		FreeSound ();
		return 0; 
	}
	memset (lpWaveHdr, 0, sizeof(WAVEHDR) * WAV_BUFFERS);
	Com_Printf( "ok\n" );

	/* After allocation, set up and prepare headers. */ 
	Com_Printf ("...preparing headers: ");
	for (i=0 ; i<WAV_BUFFERS ; i++)
	{
		lpWaveHdr[i].dwBufferLength = WAV_BUFFER_SIZE; 
		lpWaveHdr[i].lpData = lpData + i*WAV_BUFFER_SIZE;

		if (waveOutPrepareHeader(hWaveOut, lpWaveHdr+i, sizeof(WAVEHDR)) !=
				MMSYSERR_NOERROR)
		{
			Com_Printf ("failed\n");
			FreeSound ();
			return 0;
		}
	}
	Com_Printf ("ok\n");

	dma.samples = gSndBufSize/(dma.samplebits/8);
	dma.samplepos = 0;
	dma.submission_chunk = 512;
	dma.buffer = (unsigned char *) lpData;
	sample16 = (dma.samplebits/8) - 1;

	wav_init = 1;

	return 1;
}

/*
==================
SNDDMA_Init

Try to find a sound device to mix for.
Returns 0 if nothing is found.
==================
*/
int SNDDMA_Init(void)
{
	sndinitstat	stat;

	memset ((void *)&dma, 0, sizeof (dma));

	s_soundsystem = Cvar_Get ("s_soundsystem", "1", CVAR_ARCHIVE );
	s_soundquality = Cvar_Get ("s_soundquality", "2", CVAR_ARCHIVE );
	s_buffersize = Cvar_Get ("s_buffersize", "1", CVAR_ARCHIVE);

	dsound_init = wav_init = 0;

	stat = SIS_FAILURE;	// assume DirectSound won't initialize

	switch ((long int)s_khz->value)
	{
	case 96:
		dma.speed = 96000;
		break;
	case 48:
		dma.speed = 48000;
		break;
	case 44:
		dma.speed = 44100;
		break;
	case 22:
		dma.speed = 22050;
		break;
	case 11:
		dma.speed = 11025;
		break;
	case 8:
		dma.speed = 8000;
		break;
	default:
		dma.speed = 22050;	// Default to ok quality
		break;
	}

	/* Init DirectSound */
	if (s_soundsystem->value == 2)
	{
		//if (snd_firsttime || snd_isdirect)
		//{
			stat = SNDDMA_InitDirect ();

			if (stat == SIS_SUCCESS)
			{
				snd_isdirect = 1;

				//if (snd_firsttime)
					Com_Printf ("DirectSound 8 initialization succeeded\n" );
			}
			else
			{
				snd_isdirect = 0;
				Com_Printf ("*** DirectSound 8 initialization failed ***\n");
			}
		//}
	}
	/* Init WaveOut */
	else if (s_soundsystem->value == 1)
	{
		//if (snd_firsttime || snd_iswave)
		//{

			snd_iswave = SNDDMA_InitWav ();

			if (snd_iswave)
			{
				if (snd_firsttime)
					Com_Printf ("Wave sound init succeeded\n");
			}
			else
			{
				Com_Printf ("Wave sound init failed\n");
			}
		//}
	}
	/* No Audio */
	else if (s_soundsystem->value == 0)
		S_Shutdown();

	snd_firsttime = 0;

	snd_buffer_count = 1;

	if (!dsound_init && !wav_init)
	{
		Com_Printf ("*** No sound device initialized ***\n");
	}

	// NeVo - initialize Winamp Integration
	S_WinAmp_Init();

	if (s_soundsystem->value == 0)
		return 0;

	return 1;
}

/*
==============
SNDDMA_GetDMAPos

return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
===============
*/
int SNDDMA_GetDMAPos(void)
{
	MMTIME	mmtime;
	int		s = 0;
	DWORD	dwWrite;

	if (dsound_init) 
	{
		mmtime.wType = TIME_SAMPLES;
		//if (!s_primary->value)
#ifdef QDSNDCOMPILERHACK
			pDSBuf->lpVtbl->GetCurrentPosition(pDSBuf, &mmtime.u.sample, &dwWrite);
#else
			pDSBuf->GetCurrentPosition(&mmtime.u.sample, &dwWrite);
#endif
		//else
		//	pDSPBuf->lpVtbl->GetCurrentPosition(pDSPBuf, &mmtime.u.sample, &dwWrite);
		s = mmtime.u.sample - mmstarttime.u.sample;
	}
	else if (wav_init)
	{
		s = snd_sent * WAV_BUFFER_SIZE;
	}


	s >>= sample16;

	s &= (dma.samples-1);

	return s;
}

/*
==============
SNDDMA_BeginPainting

Makes sure dma.buffer is valid
===============
*/
DWORD	locksize;
void SNDDMA_BeginPainting (void)
{
	int		reps;
	DWORD	dwSize2;
	DWORD	*pbuf, *pbuf2;
	HRESULT	hresult;
	DWORD	dwStatus;

	// if the buffer was lost or stopped, restore it and/or restart it
	//if (!s_primary->value)
	//{
		if (!pDSBuf)
			return;

#ifdef QDSNDCOMPILERHACK
		if (pDSBuf->lpVtbl->GetStatus (pDSBuf, &dwStatus) != DS_OK)
#else
		if (pDSBuf->GetStatus (&dwStatus) != DS_OK)
#endif
			Com_Printf ("Couldn't get sound buffer status\n");

		if (dwStatus & DSBSTATUS_BUFFERLOST)
#ifdef QDSNDCOMPILERHACK
			pDSBuf->lpVtbl->Restore (pDSBuf);
#else
			pDSBuf->Restore ();
#endif

		if (!(dwStatus & DSBSTATUS_PLAYING))
#ifdef QDSNDCOMPILERHACK
			pDSBuf->lpVtbl->Play(pDSBuf, 0, 0, DSBPLAY_LOOPING);
#else
			pDSBuf->Play(0, 0, DSBPLAY_LOOPING);
#endif
	/*}
	else
	{
		if (!pDSPBuf)
			return;

		if (pDSPBuf->lpVtbl->GetStatus (pDSPBuf, &dwStatus) != DS_OK)
			Com_Printf ("Couldn't get sound buffer status\n");

		if (dwStatus & DSBSTATUS_BUFFERLOST)
			pDSPBuf->lpVtbl->Restore (pDSPBuf);

		if (!(dwStatus & DSBSTATUS_PLAYING))
			pDSPBuf->lpVtbl->Play(pDSPBuf, 0, 0, DSBPLAY_LOOPING);
	}*/
	
	// lock the dsound buffer

	reps = 0;
	dma.buffer = NULL;
	//if (!s_primary->value)
	//{
#ifdef QDSNDCOMPILERHACK
		while ((hresult = pDSBuf->lpVtbl->Lock(pDSBuf, 0, gSndBufSize, &pbuf, &locksize, 
			&pbuf2, &dwSize2, 0)) != DS_OK)
#else
		while ((hresult = pDSBuf->Lock(0, gSndBufSize, &pbuf, &locksize, 
			&pbuf2, &dwSize2, 0)) != DS_OK)
#endif
		{
			if (hresult != DSERR_BUFFERLOST)
			{
				Com_Printf( "S_TransferStereo16: Lock failed with error '%s'\n", DSoundError( hresult ) );
				S_Shutdown ();
				return;
			}
			else
			{
#ifdef QDSNDCOMPILERHACK
				pDSBuf->lpVtbl->Restore(pDSBuf);
#else
				pDSBuf->Restore();
#endif
			}

			if (++reps > 2)
				return;
		}
	/*}
	else
	{
		while ((hresult = pDSPBuf->lpVtbl->Lock(pDSPBuf, 0, gSndBufSize, &pbuf, &locksize, 
			&pbuf2, &dwSize2, 0)) != DS_OK)
		{
			if (hresult != DSERR_BUFFERLOST)
			{
				Com_Printf( "S_TransferStereo16: Lock failed with error '%s'\n", DSoundError( hresult ) );
				S_Shutdown ();
				return;
			}
			else
			{
				pDSPBuf->lpVtbl->Restore(pDSPBuf);
			}

			if (++reps > 2)
				return;
		}
	}*/
	dma.buffer = (unsigned char *)pbuf;
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
Also unlocks the dsound buffer
===============
*/
void SNDDMA_Submit(void)
{
	LPWAVEHDR	h;
	int			wResult;

	if (!dma.buffer)
		return;

	// unlock the dsound buffer
	if (pDSBuf)// && !s_primary->value)
#ifdef QDSNDCOMPILERHACK
		pDSBuf->lpVtbl->Unlock(pDSBuf, dma.buffer, locksize, NULL, 0);
#else
		pDSBuf->Unlock(dma.buffer, locksize, NULL, 0);
#endif
	//else
	//	pDSPBuf->lpVtbl->Unlock(pDSPBuf, dma.buffer, locksize, NULL, 0);
	
	if (!wav_init)
		return;

	//
	// find which sound blocks have completed
	//
	while (1)
	{
		if ( snd_completed == snd_sent )
		{
			Com_Printf ("Sound overrun\n");
			break;
		}

		if ( ! (lpWaveHdr[ snd_completed & WAV_MASK].dwFlags & WHDR_DONE) )
		{
			break;
		}

		snd_completed++;	// this buffer has been played
	}

//Com_Printf ("completed %i\n", snd_completed);
	//
	// submit a few new sound blocks
	//
	while (((snd_sent - snd_completed) >> sample16) < 8)
	{
		h = lpWaveHdr + ( snd_sent&WAV_MASK );
	if (paintedtime/256 <= snd_sent)
		break;	//	Com_Printf ("submit overrun\n");
//Com_Printf ("send %i\n", snd_sent);
		snd_sent++;
		/* 
		 * Now the data block can be sent to the output device. The 
		 * waveOutWrite function returns immediately and waveform 
		 * data is sent to the output device in the background. 
		 */ 
		wResult = waveOutWrite(hWaveOut, h, sizeof(WAVEHDR)); 

		if (wResult != MMSYSERR_NOERROR)
		{ 
			Com_Printf ("Failed to write block to device\n");
			FreeSound ();
			return; 
		} 
	}
}

/*
==============
SNDDMA_Shutdown

Reset the sound device for exiting
===============
*/
void SNDDMA_Shutdown(void)
{
	FreeSound ();

	// NeVo - shutdown Winamp Integration
	S_WinAmp_Shutdown();
}


/*
===========
S_Activate

Called when the main window gains or loses focus.
The window have been destroyed and recreated
between a deactivate and an activate.
===========
*/
void S_Activate (int active)
{
	if ( active )
	{
		if ( pDS && cl_hwnd && snd_isdirect )
		{
			DS_CreateBuffers();
		}
	}
	else
	{
		if ( pDS && cl_hwnd && snd_isdirect )
		{
			DS_DestroyBuffers();
		}
	}
}

