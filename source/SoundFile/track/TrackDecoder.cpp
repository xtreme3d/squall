/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>,
 *  		Adam Goode  	 <adam@evdebs.org> (endian and char fixes for PPC)
*/

#include <math.h>   	//for GCCFIX
#include "stdafx.h"
#include "TrackDecoder.h"

// включения
#include <windows.h>
#include "AbstractDecoder.h"
#include "AbstractSoundFile.h"

class CAbstractSoundFile;

// External decompressors
extern DWORD ITReadBits(DWORD& bitbuf, UINT& bitnum, LPBYTE& ibuf, CHAR n);
extern void ITUnpack8Bit(signed char* pSample, DWORD dwLen, LPBYTE lpMemFile,
	DWORD dwMemLength, BOOL b215);
extern void ITUnpack16Bit(signed char* pSample, DWORD dwLen, LPBYTE lpMemFile,
	DWORD dwMemLength, BOOL b215);


#define MAX_PACK_TABLES		3


// Compression table
static signed char UnpackTable[MAX_PACK_TABLES][16] =
//--------------------------------------------
{
	// CPU-generated dynamic table
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	// u-Law table
	{0, 1, 2, 4, 8, 16, 32, 64, -1, -2, -4, -8, -16, -32, -48, -64},
	// Linear table
	{0, 1, 2, 3, 5, 7, 12, 19, -1, -2, -3, -5, -7, -12, -19, -31}
};


//////////////////////////////////////////////////////////
// CDecompressTrack

CDecompressTrack::CDecompressTrack(WAVEFORMATEX* pcm_format, bool& flag,
	CAbstractSoundFile* a)
	: CAbstractDecompressor(pcm_format, flag, a)
	//----------------------
{
	flag = false;

	m_nType = MOD_TYPE_NONE;
	m_dwSongFlags = 0;
	m_nChannels = 0;
	m_nMixChannels = 0;
	m_nSamples = 0;
	m_nInstruments = 0;
	m_nPatternNames = 0;
	m_lpszPatternNames = NULL;
	m_lpszSongComments = NULL;
	m_nFreqFactor = m_nTempoFactor = 128;
	m_nMasterVolume = 128;
	m_nMinPeriod = 0x20;
	m_nMaxPeriod = 0x7FFF;
	m_nRepeatCount = 0;
	memset(Chn, 0, sizeof(Chn));
	memset(ChnMix, 0, sizeof(ChnMix));
	memset(Ins, 0, sizeof(Ins));
	memset(ChnSettings, 0, sizeof(ChnSettings));
	memset(Headers, 0, sizeof(Headers));
	memset(Order, 0xFF, sizeof(Order));
	memset(Patterns, 0, sizeof(Patterns));
	memset(m_szNames, 0, sizeof(m_szNames));
	memset(m_MixPlugins, 0, sizeof(m_MixPlugins));

	FileBuffer = malloc(a->length());
	a->read(FileBuffer, a->length());

	if (Create((LPCBYTE) FileBuffer, a->length())) {
		// преобразуем данные для Direct X (иначе Direct X не сможет создать буфер)
		pcm_format->wFormatTag = 1;
		pcm_format->wBitsPerSample = (WORD) gnBitsPerSample;
		pcm_format->nSamplesPerSec = gdwMixingFreq;
		pcm_format->nChannels = (WORD) gnChannels;
		pcm_format->nBlockAlign = (pcm_format->nChannels * pcm_format->wBitsPerSample) >>
			3;
		pcm_format->nAvgBytesPerSec = pcm_format->nBlockAlign * pcm_format->nSamplesPerSec;

		// Создание дополнительных данных
		CreateOrderInfo();

		// Создание буфера для распаковки патерна
		PatternSampleBuffer = (char *)
			malloc(MaxSamplesInPattern * pcm_format->nBlockAlign);
		memset(PatternSampleBuffer,
			0,
			MaxSamplesInPattern * pcm_format->nBlockAlign);

		// Распаковка первого патерна
		SetCurrentOrder(0);

		flag = true;
		free(FileBuffer);
	}
}


CDecompressTrack::~CDecompressTrack()
	//-----------------------
{
	Destroy();
}


BOOL CDecompressTrack::Create(LPCBYTE lpStream, DWORD dwMemLength)
	//----------------------------------------------------------
{
	int i;

	m_nType = MOD_TYPE_NONE;
	m_dwSongFlags = 0;
	m_nChannels = 0;
	m_nMixChannels = 0;
	m_nSamples = 0;
	m_nInstruments = 0;
	m_nFreqFactor = m_nTempoFactor = 128;
	m_nMasterVolume = 128;
	m_nDefaultGlobalVolume = 256;
	m_nGlobalVolume = 256;
	m_nOldGlbVolSlide = 0;
	m_nDefaultSpeed = 6;
	m_nDefaultTempo = 125;
	m_nPatternDelay = 0;
	m_nFrameDelay = 0;
	m_nNextRow = 0;
	m_nRow = 0;
	m_nPattern = 0;
	m_nCurrentPattern = 0;
	m_nNextPattern = 0;
	m_nRestartPos = 0;
	m_nMinPeriod = 16;
	m_nMaxPeriod = 32767;
	m_nSongPreAmp = 0x30;
	m_nPatternNames = 0;
	m_nMaxOrderPosition = 0;
	m_lpszPatternNames = NULL;
	m_lpszSongComments = NULL;
	memset(Ins, 0, sizeof(Ins));
	memset(ChnMix, 0, sizeof(ChnMix));
	memset(Chn, 0, sizeof(Chn));
	memset(Headers, 0, sizeof(Headers));
	memset(Order, 0xFF, sizeof(Order));
	memset(Patterns, 0, sizeof(Patterns));
	memset(m_szNames, 0, sizeof(m_szNames));
	memset(m_MixPlugins, 0, sizeof(m_MixPlugins));
	ResetMidiCfg();

	for (UINT npt = 0; npt < MAX_PATTERNS; npt++)
		PatternSize[npt] = 64;

	for (UINT nch = 0; nch < MAX_BASECHANNELS; nch++) {
		ChnSettings[nch].nPan = 128;
		ChnSettings[nch].nVolume = 64;
		ChnSettings[nch].dwFlags = 0;
		ChnSettings[nch].szName[0] = 0;
	}

	if (lpStream) {
		if ((!ReadXM(lpStream, dwMemLength)) &&
			(!ReadIT(lpStream, dwMemLength)) &&
			(!ReadS3M(lpStream, dwMemLength)) &&
			(!ReadSTM(lpStream, dwMemLength)) &&
			(!ReadMed(lpStream, dwMemLength)) &&
			(!ReadMTM(lpStream, dwMemLength)) &&
			(!ReadMDL(lpStream, dwMemLength)) &&
			(!ReadDBM(lpStream, dwMemLength)) &&
			(!Read669(lpStream, dwMemLength)) &&
			(!ReadFAR(lpStream, dwMemLength)) &&
			(!ReadAMS(lpStream, dwMemLength)) &&
			(!ReadOKT(lpStream, dwMemLength)) &&
			(!ReadPTM(lpStream, dwMemLength)) &&
			(!ReadUlt(lpStream, dwMemLength)) &&
			(!ReadDMF(lpStream, dwMemLength)) &&
			(!ReadDSM(lpStream, dwMemLength)) &&
			(!ReadUMX(lpStream, dwMemLength)) &&
			(!ReadAMF(lpStream, dwMemLength)) &&
			(!ReadPSM(lpStream, dwMemLength)) &&
			(!ReadMT2(lpStream, dwMemLength)) &&
			(!ReadMod(lpStream, dwMemLength)))
			m_nType = MOD_TYPE_NONE;
	}
	// Adjust song names
	for (i = 0; i < MAX_SAMPLES; i++) {
		LPSTR p = m_szNames[i];
		int j = 31;
		p[j] = 0;
		while ((j >= 0) && (p[j] <= ' '))
			p[j--] = 0;
		while (j >= 0) {
			if (((BYTE) p[j]) < ' ')
				p[j] = ' ';
			j--;
		}
	}
	// Adjust channels
	for (i = 0; i < MAX_BASECHANNELS; i++) {
		if (ChnSettings[i].nVolume > 64)
			ChnSettings[i].nVolume = 64;
		if (ChnSettings[i].nPan > 256)
			ChnSettings[i].nPan = 128;
		Chn[i].nPan = ChnSettings[i].nPan;
		Chn[i].nGlobalVol = ChnSettings[i].nVolume;
		Chn[i].dwFlags = ChnSettings[i].dwFlags;
		Chn[i].nVolume = 256;
		Chn[i].nCutOff = 0x7F;
	}
	// Checking instruments
	MODINSTRUMENT* pins = Ins;

	for (i = 0; i < MAX_INSTRUMENTS; i++, pins++) {
		if (pins->pSample) {
			if (pins->nLoopEnd > pins->nLength)
				pins->nLoopEnd = pins->nLength;
			if (pins->nLoopStart + 3 >= pins->nLoopEnd) {
				pins->nLoopStart = 0;
				pins->nLoopEnd = 0;
			}
			if (pins->nSustainEnd > pins->nLength)
				pins->nSustainEnd = pins->nLength;
			if (pins->nSustainStart + 3 >= pins->nSustainEnd) {
				pins->nSustainStart = 0;
				pins->nSustainEnd = 0;
			}
		} else {
			pins->nLength = 0;
			pins->nLoopStart = 0;
			pins->nLoopEnd = 0;
			pins->nSustainStart = 0;
			pins->nSustainEnd = 0;
		}
		if (!pins->nLoopEnd)
			pins->uFlags &= ~CHN_LOOP;
		if (!pins->nSustainEnd)
			pins->uFlags &= ~CHN_SUSTAINLOOP;
		if (pins->nGlobalVol > 64)
			pins->nGlobalVol = 64;
	}
	// Check invalid instruments
	while ((m_nInstruments > 0) && (!Headers[m_nInstruments]))
		m_nInstruments--;
	// Set default values
	if (m_nSongPreAmp < 0x20)
		m_nSongPreAmp = 0x20;
	if (m_nDefaultTempo < 32)
		m_nDefaultTempo = 125;
	if (!m_nDefaultSpeed)
		m_nDefaultSpeed = 6;
	m_nMusicSpeed = m_nDefaultSpeed;
	m_nMusicTempo = m_nDefaultTempo;
	m_nGlobalVolume = m_nDefaultGlobalVolume;
	m_nNextPattern = 0;
	m_nCurrentPattern = 0;
	m_nPattern = 0;
	m_nBufferCount = 0;
	m_nTickCount = m_nMusicSpeed;
	m_nNextRow = 0;
	m_nRow = 0;
	if ((m_nRestartPos >= MAX_ORDERS) ||
		(Order[m_nRestartPos] >= MAX_PATTERNS))
		m_nRestartPos = 0;
	// Load plugins
	/*	if (gpMixPluginCreateProc)
		{
			for (UINT iPlug=0; iPlug<MAX_MIXPLUGINS; iPlug++)
			{
				if ((m_MixPlugins[iPlug].Info.dwPluginId1)
				 || (m_MixPlugins[iPlug].Info.dwPluginId2))
				{
					gpMixPluginCreateProc(&m_MixPlugins[iPlug]);
					if (m_MixPlugins[iPlug].pMixPlugin)
					{
						m_MixPlugins[iPlug].pMixPlugin->RestoreAllParameters();
					}
				}
			}
		}*/
	if (m_nType) {
		UINT maxpreamp = 0x10 + (m_nChannels*8);
		if (maxpreamp > 100)
			maxpreamp = 100;
		if (m_nSongPreAmp > maxpreamp)
			m_nSongPreAmp = maxpreamp;
		return TRUE;
	}
	return FALSE;
}


BOOL CDecompressTrack::Destroy()

	//------------------------
{
	int i;
	for (i = 0; i < MAX_PATTERNS; i++)
		if (Patterns[i]) {
			FreePattern(Patterns[i]);
			Patterns[i] = NULL;
		}
	m_nPatternNames = 0;
	if (m_lpszPatternNames) {
		delete m_lpszPatternNames;
		m_lpszPatternNames = NULL;
	}
	if (m_lpszSongComments) {
		delete m_lpszSongComments;
		m_lpszSongComments = NULL;
	}
	for (i = 1; i < MAX_SAMPLES; i++) {
		MODINSTRUMENT* pins =& Ins[i];
		if (pins->pSample) {
			FreeSample(pins->pSample);
			pins->pSample = NULL;
		}
	}
	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		if (Headers[i]) {
			delete Headers[i];
			Headers[i] = NULL;
		}
	}
	for (i = 0; i < MAX_MIXPLUGINS; i++) {
		if ((m_MixPlugins[i].nPluginDataSize) && (m_MixPlugins[i].pPluginData)) {
			m_MixPlugins[i].nPluginDataSize = 0;
			delete [] (signed char *) m_MixPlugins[i].pPluginData;
			m_MixPlugins[i].pPluginData = NULL;
		}
		m_MixPlugins[i].pMixState = NULL;
		if (m_MixPlugins[i].pMixPlugin) {
			m_MixPlugins[i].pMixPlugin->Release();
			m_MixPlugins[i].pMixPlugin = NULL;
		}
	}
	m_nType = MOD_TYPE_NONE;
	m_nChannels = m_nSamples = m_nInstruments = 0;
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
// Memory Allocation

MODCOMMAND* CDecompressTrack::AllocatePattern(UINT rows, UINT nchns)
	//------------------------------------------------------------
{
	MODCOMMAND* p = new MODCOMMAND[rows* nchns];
	if (p)
		memset(p, 0, rows * nchns * sizeof(MODCOMMAND));

	return p;
}

void CDecompressTrack::FreePattern(LPVOID pat)
	//--------------------------------------
{
	if (pat)
		delete [] (signed char *) pat;
}


signed char* CDecompressTrack::AllocateSample(UINT nbytes)
	//-------------------------------------------
{
	signed char * p = (signed char *)
		GlobalAllocPtr(GHND, (nbytes + 39) & ~ 7);
	if (p)
		p += 16;
	return p;
}


void CDecompressTrack::FreeSample(LPVOID p)
	//-----------------------------------
{
	if (p) {
		GlobalFreePtr(((LPSTR) p) - 16);
	}
}


//////////////////////////////////////////////////////////////////////////
// Misc functions

void CDecompressTrack::ResetMidiCfg()
	//-----------------------------
{
	memset(&m_MidiCfg, 0, sizeof(m_MidiCfg));
	lstrcpy(&m_MidiCfg.szMidiGlb[MIDIOUT_START * 32], "FF");
	lstrcpy(&m_MidiCfg.szMidiGlb[MIDIOUT_STOP * 32], "FC");
	lstrcpy(&m_MidiCfg.szMidiGlb[MIDIOUT_NOTEON * 32], "9c n v");
	lstrcpy(&m_MidiCfg.szMidiGlb[MIDIOUT_NOTEOFF * 32], "9c n 0");
	lstrcpy(&m_MidiCfg.szMidiGlb[MIDIOUT_PROGRAM * 32], "Cc p");
	lstrcpy(&m_MidiCfg.szMidiSFXExt[0], "F0F000z");
	for (int iz = 0; iz < 16; iz++)
		wsprintf(&m_MidiCfg.szMidiZXXExt[iz * 32], "F0F001%02X", iz * 8);
}


UINT CDecompressTrack::GetNumChannels() const
	//-------------------------------------
{
	UINT n = 0;
	for (UINT i = 0; i < m_nChannels; i++)
		if (ChnSettings[i].nVolume)
			n++;
	return n;
}


UINT CDecompressTrack::GetSongComments(LPSTR s, UINT len, UINT linesize)
	//----------------------------------------------------------------
{
	LPCSTR p = m_lpszSongComments;
	if (!p)
		return 0;
	UINT i = 2, ln = 0;
	if ((len) && (s))
		s[0] = '\x0D';
	if ((len > 1) && (s))
		s[1] = '\x0A';
	while ((*p) && (i + 2 < len)) {
		BYTE c = (BYTE) * p++;
		if ((c == 0x0D) || ((c == ' ') && (ln >= linesize))) {
			if (s) {
				s[i++] = '\x0D'; s[i++] = '\x0A';
			} else
				i += 2; ln = 0;
		} else if (c >= 0x20) {
			if (s)
				s[i++] = c;
			else
				i++; ln++;
		}
	}
	if (s)
		s[i] = 0;
	return i;
}


UINT CDecompressTrack::GetRawSongComments(LPSTR s, UINT len, UINT linesize)
	//-------------------------------------------------------------------
{
	LPCSTR p = m_lpszSongComments;
	if (!p)
		return 0;
	UINT i = 0, ln = 0;
	while ((*p) && (i < len - 1)) {
		BYTE c = (BYTE) * p++;
		if ((c == 0x0D) || (c == 0x0A)) {
			if (ln) {
				while (ln < linesize) {
					if (s)
						s[i] = ' '; i++; ln++;
				}
				ln = 0;
			}
		} else if ((c == ' ') && (!ln)) {
			UINT k = 0;
			while ((p[k]) && (p[k] >= ' '))
				k++;
			if (k <= linesize) {
				if (s)
					s[i] = ' ';
				i++;
				ln++;
			}
		} else {
			if (s)
				s[i] = c;
			i++;
			ln++;
			if (ln == linesize)
				ln = 0;
		}
	}
	if (ln) {
		while ((ln < linesize) && (i < len)) {
			if (s)
				s[i] = ' ';
			i++;
			ln++;
		}
	}
	if (s)
		s[i] = 0;
	return i;
}

BOOL CDecompressTrack::SetResamplingMode(UINT nMode)
	//--------------------------------------------
{
	DWORD d = gdwSoundSetup & ~ (SNDMIX_NORESAMPLING |
		SNDMIX_HQRESAMPLER |
		SNDMIX_ULTRAHQSRCMODE);
	switch (nMode) {
	case SRCMODE_NEAREST:
		d |= SNDMIX_NORESAMPLING; break;
	case SRCMODE_LINEAR:
		break;
	case SRCMODE_SPLINE:
		d |= SNDMIX_HQRESAMPLER; break;
	case SRCMODE_POLYPHASE:
		d |= (SNDMIX_HQRESAMPLER | SNDMIX_ULTRAHQSRCMODE); break;
	default:
		return FALSE;
	}
	gdwSoundSetup = d;
	return TRUE;
}


BOOL CDecompressTrack::SetMasterVolume(UINT nVol, BOOL bAdjustAGC)
	//----------------------------------------------------------
{
	if (nVol < 1)
		nVol = 1;
	if (nVol > 0x200)
		nVol = 0x200;	// x4 maximum
	if ((nVol < m_nMasterVolume) &&
		(nVol) &&
		(gdwSoundSetup & SNDMIX_AGC) &&
		(bAdjustAGC)) {
		gnAGC = gnAGC * m_nMasterVolume / nVol;
		if (gnAGC > AGC_UNITY)
			gnAGC = AGC_UNITY;
	}
	m_nMasterVolume = nVol;
	return TRUE;
}


void CDecompressTrack::SetAGC(BOOL b)
	//-----------------------------
{
	if (b) {
		if (!(gdwSoundSetup & SNDMIX_AGC)) {
			gdwSoundSetup |= SNDMIX_AGC;
			gnAGC = AGC_UNITY;
		}
	} else
		gdwSoundSetup &= ~SNDMIX_AGC;
}


UINT CDecompressTrack::GetNumPatterns() const
	//-------------------------------------
{
	UINT i = 0;
	while ((i < MAX_ORDERS) && (Order[i] < 0xFF))
		i++;
	return i;
}


UINT CDecompressTrack::GetNumInstruments() const
	//----------------------------------------
{
	UINT n = 0;
	for (UINT i = 0; i < MAX_INSTRUMENTS; i++)
		if (Ins[i].pSample)
			n++;
	return n;
}


UINT CDecompressTrack::GetMaxPosition() const
	//-------------------------------------
{
	UINT max = 0;
	UINT i = 0;

	while ((i < MAX_ORDERS) && (Order[i] != 0xFF)) {
		if (Order[i] < MAX_PATTERNS)
			max += PatternSize[Order[i]];
		i++;
	}
	return max;
}


UINT CDecompressTrack::GetCurrentPos() const
	//------------------------------------
{
	UINT pos = 0;

	for (UINT i = 0; i < m_nCurrentPattern; i++)
		if (Order[i] < MAX_PATTERNS)
			pos += PatternSize[Order[i]];
	return pos + m_nRow;
}

//-----------------------------------------------------------------------------
// на входе    :
// на выходе   
//-----------------------------------------------------------------------------
void CDecompressTrack::SetCurrentPos(UINT nPos)
	//---------------------------------------
{
	UINT i, nPattern;

	for (i = 0; i < MAX_CHANNELS; i++) {
		Chn[i].nNote = Chn[i].nNewNote = Chn[i].nNewIns = 0;
		Chn[i].pInstrument = NULL;
		Chn[i].pHeader = NULL;
		Chn[i].nPortamentoDest = 0;
		Chn[i].nCommand = 0;
		Chn[i].nPatternLoopCount = 0;
		Chn[i].nPatternLoop = 0;
		Chn[i].nFadeOutVol = 0;
		Chn[i].dwFlags |= CHN_KEYOFF | CHN_NOTEFADE;
		Chn[i].nTremorCount = 0;
	}
	if (!nPos) {
		for (i = 0; i < MAX_CHANNELS; i++) {
			Chn[i].nPeriod = 0;
			Chn[i].nPos = Chn[i].nLength = 0;
			Chn[i].nLoopStart = 0;
			Chn[i].nLoopEnd = 0;
			Chn[i].nROfs = Chn[i].nLOfs = 0;
			Chn[i].pSample = NULL;
			Chn[i].pInstrument = NULL;
			Chn[i].pHeader = NULL;
			Chn[i].nCutOff = 0x7F;
			Chn[i].nResonance = 0;
			Chn[i].nLeftVol = Chn[i].nRightVol = 0;
			Chn[i].nNewLeftVol = Chn[i].nNewRightVol = 0;
			Chn[i].nLeftRamp = Chn[i].nRightRamp = 0;
			Chn[i].nVolume = 256;
			if (i < MAX_BASECHANNELS) {
				Chn[i].dwFlags = ChnSettings[i].dwFlags;
				Chn[i].nPan = ChnSettings[i].nPan;
				Chn[i].nGlobalVol = ChnSettings[i].nVolume;
			} else {
				Chn[i].dwFlags = 0;
				Chn[i].nPan = 128;
				Chn[i].nGlobalVol = 64;
			}
		}
		m_nGlobalVolume = m_nDefaultGlobalVolume;
		m_nMusicSpeed = m_nDefaultSpeed;
		m_nMusicTempo = m_nDefaultTempo;
	}
	m_dwSongFlags &= ~ (SONG_PATTERNLOOP |
		SONG_CPUVERYHIGH |
		SONG_FADINGSONG |
		SONG_ENDREACHED |
		SONG_GLOBALFADE);
	for (nPattern = 0; nPattern < MAX_ORDERS; nPattern++) {
		UINT ord = Order[nPattern];
		if (ord == 0xFE)
			continue;
		if (ord == 0xFF)
			break;
		if (ord < MAX_PATTERNS) {
			if (nPos < (UINT) PatternSize[ord])
				break;
			nPos -= PatternSize[ord];
		}
	}
	// Buggy position ?
	if ((nPattern >= MAX_ORDERS) ||
		(Order[nPattern] >= MAX_PATTERNS) ||
		(nPos >= PatternSize[Order[nPattern]])) {
		nPos = 0;
		nPattern = 0;
	}
	UINT nRow = nPos;
	if ((nRow) && (Order[nPattern] < MAX_PATTERNS)) {
		MODCOMMAND* p = Patterns[Order[nPattern]];
		if ((p) && (nRow < PatternSize[Order[nPattern]])) {
			BOOL bOk = FALSE;
			while ((!bOk) && (nRow > 0)) {
				UINT n = nRow* m_nChannels;
				for (UINT k = 0; k < m_nChannels; k++, n++) {
					if (p[n].note) {
						bOk = TRUE;
						break;
					}
				}
				if (!bOk)
					nRow--;
			}
		}
	}
	m_nNextPattern = nPattern;
	m_nNextRow = nRow;
	m_nTickCount = m_nMusicSpeed;
	m_nBufferCount = 0;
	m_nPatternDelay = 0;
	m_nFrameDelay = 0;
}


// Установка новой позиции в файле, в семплах
/*
void CDecompressTrack::SetCurrentPos(unsigned int Position)
{
	
   UINT i, nPattern, nRow;
   
   unsigned int CurOrder   = 0;
   unsigned int Row 	   = 0;
   unsigned int Speed      = m_nDefaultSpeed;
   unsigned int Tempo      = m_nDefaultTempo;

   while( 1 )
   {
	  if( OrderInfo[CurOrder]._init )
	  {
		 if( Position < OrderInfo[CurOrder]._samples )
		 {
			// Позиция найдена
			nRow = SeekInPattern( CurOrder, Row, Speed, Tempo, Position );
			break;
		 }
		 
		 // Обработка параметров
		 Position -= OrderInfo[CurOrder]._samples;
		 Speed    =  OrderInfo[CurOrder]._speed;
		 Tempo    =  OrderInfo[CurOrder]._tempo;
		 Row	  =  OrderInfo[CurOrder]._row;

		 // Переход на следующий элемент
		 CurOrder += OrderInfo[CurOrder]._next;
	  }
   }
   
   if( m_nRow == nRow && m_nCurrentPattern == Order[CurOrder] )
	  return;
   
   
   

   for (i=0; i<MAX_CHANNELS; i++)
   {
	  Chn[i].nNote = Chn[i].nNewNote = Chn[i].nNewIns = 0;
	  Chn[i].pInstrument = NULL;
	  Chn[i].pHeader = NULL;
	  Chn[i].nPortamentoDest = 0;
	  Chn[i].nCommand = 0;
	  Chn[i].nPatternLoopCount = 0;
	  Chn[i].nPatternLoop = 0;
	  Chn[i].nFadeOutVol = 0;
	  Chn[i].dwFlags |= CHN_KEYOFF|CHN_NOTEFADE;
	  Chn[i].nTremorCount = 0;
	}

	if (!Position)
	{
		for (i=0; i<MAX_CHANNELS; i++)
		{
			Chn[i].nPeriod = 0;
			Chn[i].nPos = Chn[i].nLength = 0;
			Chn[i].nLoopStart = 0;
			Chn[i].nLoopEnd = 0;
			Chn[i].nROfs = Chn[i].nLOfs = 0;
			Chn[i].pSample = NULL;
			Chn[i].pInstrument = NULL;
			Chn[i].pHeader = NULL;
			Chn[i].nCutOff = 0x7F;
			Chn[i].nResonance = 0;
			Chn[i].nLeftVol = Chn[i].nRightVol = 0;
			Chn[i].nNewLeftVol = Chn[i].nNewRightVol = 0;
			Chn[i].nLeftRamp = Chn[i].nRightRamp = 0;
			Chn[i].nVolume = 256;
			if (i < MAX_BASECHANNELS)
			{
				Chn[i].dwFlags = ChnSettings[i].dwFlags;
				Chn[i].nPan = ChnSettings[i].nPan;
				Chn[i].nGlobalVol = ChnSettings[i].nVolume;
			} else
			{
				Chn[i].dwFlags = 0;
				Chn[i].nPan = 128;
				Chn[i].nGlobalVol = 64;
			}
		}
		m_nGlobalVolume = m_nDefaultGlobalVolume;
		m_nMusicSpeed = m_nDefaultSpeed;
		m_nMusicTempo = m_nDefaultTempo;
	}
	
   m_dwSongFlags &= ~(SONG_PATTERNLOOP|SONG_CPUVERYHIGH|SONG_FADINGSONG|SONG_ENDREACHED|SONG_GLOBALFADE);


	for (nPattern = 0; nPattern < MAX_ORDERS; nPattern++)
	{
		UINT ord = Order[nPattern];
		if (ord == 0xFE) continue;
		if (ord == 0xFF) break;
		if (ord < MAX_PATTERNS)
		{
			if (nPos < (UINT)PatternSize[ord]) break;
			nPos -= PatternSize[ord];
		}
	}
	// Buggy position ?
	if ((nPattern >= MAX_ORDERS)
	 || (Order[nPattern] >= MAX_PATTERNS)
	 || (nPos >= PatternSize[Order[nPattern]]))
	{
		nPos = 0;
		nPattern = 0;
	}
//	UINT nRow = nPos;

	if ((nRow) && (Order[CurOrder] < MAX_PATTERNS))
	{
		MODCOMMAND *p = Patterns[Order[CurOrder]];
		if ((p) && (nRow < PatternSize[Order[CurOrder]]))
		{
			BOOL bOk = FALSE;
			while ((!bOk) && (nRow > 0))
			{
				UINT n = nRow * m_nChannels;
				for (UINT k=0; k<m_nChannels; k++, n++)
				{
					if (p[n].note)
					{
						bOk = TRUE;
						break;
					}
				}
				if (!bOk) nRow--;
			}
		}
	}
	m_nNextPattern    = Order[CurOrder];
//	m_nNextRow  	  = nRow;
	m_nTickCount	  = m_nMusicSpeed;
	m_nBufferCount    = 0;
	m_nPatternDelay   = 0;
	m_nFrameDelay     = 0;
}
*/


void CDecompressTrack::SetCurrentOrder(UINT nPos)
	//-----------------------------------------
{
	while ((nPos < MAX_ORDERS) && (Order[nPos] == 0xFE))
		nPos++;
	if ((nPos >= MAX_ORDERS) || (Order[nPos] >= MAX_PATTERNS))
		return;
	for (UINT j = 0; j < MAX_CHANNELS; j++) {
		Chn[j].nPeriod = 0;
		Chn[j].nNote = 0;
		Chn[j].nPortamentoDest = 0;
		Chn[j].nCommand = 0;
		Chn[j].nPatternLoopCount = 0;
		Chn[j].nPatternLoop = 0;
		Chn[j].nTremorCount = 0;
	}
	if (!nPos) {
		SetCurrentPos(0);
	} else {
		m_nNextPattern = nPos;
		m_nRow = m_nNextRow = 0;
		m_nPattern = 0;
		m_nTickCount = m_nMusicSpeed;
		m_nBufferCount = 0;
		m_nTotalCount = 0;
		m_nPatternDelay = 0;
		m_nFrameDelay = 0;
	}
	m_dwSongFlags &= ~ (SONG_PATTERNLOOP |
		SONG_CPUVERYHIGH |
		SONG_FADINGSONG |
		SONG_ENDREACHED |
		SONG_GLOBALFADE);

	CurOrder = nPos;
	/*   CurPatternPos = 0;
	   int i = 0;
	   while( nPos )
	   {
		  if( OrderInfo[i]._init )
		  {
			 CurPatternPos += OrderInfo[i]._samples;
			 nPos--;
		  }
	   }*/

	SamplesInPattern = OrderInfo[CurOrder]._samples;
	PatternSamplePos = 0;

	Read(PatternSampleBuffer, SamplesInPattern * 2 * gnChannels, gnChannels);
}

DWORD CDecompressTrack::GetStereoSamples(void* buffer, DWORD start,
	DWORD length, bool loop)
{
	UINT NeedOrder = 0;
	UINT NeedRead = length;
	UINT CurPos = 0;
	int Block = 0;
	short* Out = (short*) buffer;
	short* In = 0;

	// вычисление номера требуемого патерна
	while (1) {
		if (OrderInfo[NeedOrder]._init) {
			if (start < OrderInfo[NeedOrder]._samples)
				break;
			start -= OrderInfo[NeedOrder]._samples;
			NeedOrder += OrderInfo[NeedOrder]._next;
		}
	}


	CurPos = start;

	while (NeedRead) {
		// если текущая позиция не совпадает с вычисленой
		if (CurOrder != NeedOrder) {
			SetCurrentOrder(NeedOrder);
		}

		if (NeedRead > (SamplesInPattern - CurPos))
			Block = (SamplesInPattern - CurPos);
		else
			Block = NeedRead;

		In = (short *) PatternSampleBuffer + CurPos * gnChannels;
		CurPos += Block;
		NeedRead -= Block;

		// сборка семплов
		if (gnChannels == 1) {
			// моно данные
			do {
				*Out++ = *In;
				*Out++ = *In;
				In++;
			} while (--Block);
		} else {
			// стерео данные
			do {
				*Out++ = *In++;
				*Out++ = *In++;
			} while (--Block);
		}

		if ((SamplesInPattern - CurPos) == 0) {
			NeedOrder++;
			CurPos = 0;
		}
	};

	return (DWORD) Out - (DWORD) buffer;
}


DWORD CDecompressTrack::GetMonoSamples(void* buffer, DWORD start,
	DWORD length, bool loop)
{
	UINT NeedOrder = 0;
	UINT NeedRead = length;
	int Block = 0;
	short* Out = (short*) buffer;
	short* In = 0;
	int s = 0;

	// вычисление номера требуемого патерна
	while (1) {
		if (OrderInfo[NeedOrder]._init) {
			if (start < OrderInfo[NeedOrder]._samples)
				break;
			start -= OrderInfo[NeedOrder]._samples;
			NeedOrder += OrderInfo[NeedOrder]._next;
		}
	}


	while (NeedRead) {
		// если текущая позиция не совпадает с вычисленой
		if (CurOrder != NeedOrder) {
			SetCurrentOrder(NeedOrder);
		}

		if (NeedRead > (SamplesInPattern - PatternSamplePos))
			Block = (SamplesInPattern - PatternSamplePos);
		else
			Block = NeedRead;

		PatternSamplePos += Block;
		NeedRead -= Block;

		// сборка семплов
		if (gnChannels == 1) {
			In = (short *) PatternSampleBuffer + PatternSamplePos;
			// моно данные
			do {
				*Out++ = *In++;
			} while (--Block);
		} else {
			In = (short *) PatternSampleBuffer + PatternSamplePos * 2;
			// стерео данные
			do {
				s = ((int) In[0] + (int) In[1]) >> 1;
				if (s < -32768)
					s = -32768;
				else if (s > 32767)
					s = 32767;
				*Out++ = (short) s;
				In += 2;
			} while (--Block);
		}

		if (SamplesInPattern - PatternSamplePos)
			NeedOrder++;
	}

	return (DWORD) Out - (DWORD) buffer;
}

void CDecompressTrack::ResetChannels()
	//------------------------------
{
	m_dwSongFlags &= ~ (SONG_CPUVERYHIGH |
		SONG_FADINGSONG |
		SONG_ENDREACHED |
		SONG_GLOBALFADE);
	m_nBufferCount = 0;
	for (UINT i = 0; i < MAX_CHANNELS; i++) {
		Chn[i].nROfs = Chn[i].nLOfs = 0;
	}
}


void CDecompressTrack::LoopPattern(int nPat, int nRow)
	//----------------------------------------------
{
	if ((nPat < 0) || (nPat >= MAX_PATTERNS) || (!Patterns[nPat])) {
		m_dwSongFlags &= ~SONG_PATTERNLOOP;
	} else {
		if ((nRow < 0) || (nRow >= PatternSize[nPat]))
			nRow = 0;
		m_nPattern = nPat;
		m_nRow = m_nNextRow = nRow;
		m_nTickCount = m_nMusicSpeed;
		m_nPatternDelay = 0;
		m_nFrameDelay = 0;
		m_nBufferCount = 0;
		m_dwSongFlags |= SONG_PATTERNLOOP;
	}
}
/*

UINT CDecompressTrack::GetBestSaveFormat() const
//----------------------------------------
{
	if ((!m_nSamples) || (!m_nChannels)) return MOD_TYPE_NONE;
	if (!m_nType) return MOD_TYPE_NONE;
	if (m_nType & (MOD_TYPE_MOD|MOD_TYPE_OKT))
		return MOD_TYPE_MOD;
	if (m_nType & (MOD_TYPE_S3M|MOD_TYPE_STM|MOD_TYPE_ULT|MOD_TYPE_FAR|MOD_TYPE_PTM))
		return MOD_TYPE_S3M;
	if (m_nType & (MOD_TYPE_XM|MOD_TYPE_MED|MOD_TYPE_MTM|MOD_TYPE_MT2))
		return MOD_TYPE_XM;
	return MOD_TYPE_IT;
}


UINT CDecompressTrack::GetSaveFormats() const
//-------------------------------------
{
	UINT n = 0;
	if ((!m_nSamples) || (!m_nChannels) || (m_nType == MOD_TYPE_NONE)) return 0;
	switch(m_nType)
	{
	case MOD_TYPE_MOD:	n = MOD_TYPE_MOD;
	case MOD_TYPE_S3M:	n = MOD_TYPE_S3M;
	}
	n |= MOD_TYPE_XM | MOD_TYPE_IT;
	if (!m_nInstruments)
	{
		if (m_nSamples < 32) n |= MOD_TYPE_MOD;
		n |= MOD_TYPE_S3M;
	}
	return n;
}
*/

UINT CDecompressTrack::GetSampleName(UINT nSample, LPSTR s) const
	//--------------------------------------------------------
{
	char sztmp[40] = "";	  // changed from CHAR
	memcpy(sztmp, m_szNames[nSample], 32);
	sztmp[31] = 0;
	if (s)
		strcpy(s, sztmp);
	return strlen(sztmp);
}


UINT CDecompressTrack::GetInstrumentName(UINT nInstr, LPSTR s) const
	//-----------------------------------------------------------
{
	char sztmp[40] = "";  // changed from CHAR
	if ((nInstr >= MAX_INSTRUMENTS) || (!Headers[nInstr])) {
		if (s)
			*s = 0;
		return 0;
	}
	INSTRUMENTHEADER* penv = Headers[nInstr];
	memcpy(sztmp, penv->name, 32);
	sztmp[31] = 0;
	if (s)
		strcpy(s, sztmp);
	return strlen(sztmp);
}

// Flags:
//	0 = signed 8-bit PCM data (default)
//	1 = unsigned 8-bit PCM data
//	2 = 8-bit ADPCM data with linear table
//	3 = 4-bit ADPCM data
//	4 = 16-bit ADPCM data with linear table
//	5 = signed 16-bit PCM data
//	6 = unsigned 16-bit PCM data


UINT CDecompressTrack::ReadSample(MODINSTRUMENT* pIns, UINT nFlags,
	LPCSTR lpMemFile, DWORD dwMemLength)
	//------------------------------------------------------------------------------------------------
{
	//   UINT j;
	UINT len = 0, mem = pIns->nLength + 6;

	if ((!pIns) || (pIns->nLength < 4) || (!lpMemFile))
		return 0;
	if (pIns->nLength > MAX_SAMPLE_LENGTH)
		pIns->nLength = MAX_SAMPLE_LENGTH;
	pIns->uFlags &= ~ (CHN_16BIT | CHN_STEREO);
	if (nFlags & RSF_16BIT) {
		mem *= 2;
		pIns->uFlags |= CHN_16BIT;
	}
	if (nFlags & RSF_STEREO) {
		mem *= 2;
		pIns->uFlags |= CHN_STEREO;
	}
	if ((pIns->pSample = AllocateSample(mem)) == NULL) {
		pIns->nLength = 0;
		return 0;
	}
	switch (nFlags) {
		// 1: 8-bit unsigned PCM data
	case RS_PCM8U:
		 {
			len = pIns->nLength;
			if (len > dwMemLength)
				len = pIns->nLength = dwMemLength;
			signed char * pSample = pIns->pSample;
			for (UINT j = 0; j < len; j++)
				pSample[j] = (signed char) (lpMemFile[j] - 0x80);
		}
		break;

		// 2: 8-bit ADPCM data with linear table
	case RS_PCM8D:
		 {
			len = pIns->nLength;
			if (len > dwMemLength)
				break;
			signed char * pSample = pIns->pSample;
			const signed char* p = (const signed char*) lpMemFile;
			int delta = 0;

			for (UINT j = 0; j < len; j++) {
				delta += p[j];
				*pSample++ = (signed char) delta;
			}
		}
		break;

		// 3: 4-bit ADPCM data
	case RS_ADPCM4:
		 {
			len = (pIns->nLength + 1) / 2;
			if (len > dwMemLength - 16)
				break;
			memcpy(CompressionTable, lpMemFile, 16);
			lpMemFile += 16;
			signed char * pSample = pIns->pSample;
			signed char delta = 0;
			for (UINT j = 0; j < len; j++) {
				BYTE b0 = (BYTE) lpMemFile[j];
				BYTE b1 = (BYTE) (lpMemFile[j] >> 4);
				delta = (signed char) GetDeltaValue((int) delta, b0);
				pSample[0] = delta;
				delta = (signed char) GetDeltaValue((int) delta, b1);
				pSample[1] = delta;
				pSample += 2;
			}
			len += 16;
		}
		break;

		// 4: 16-bit ADPCM data with linear table
	case RS_PCM16D:
		 {
			len = pIns->nLength * 2;
			if (len > dwMemLength)
				break;
			short int* pSample = (short int*) pIns->pSample;
			short int* p = (short int*) lpMemFile;
			int delta16 = 0;
			for (UINT j = 0; j < len; j += 2) {
				delta16 += bswapLE16(*p++);
				*pSample++ = (short int) delta16;
			}
		}
		break;

		// 5: 16-bit signed PCM data
	case RS_PCM16S:
		 {
			len = pIns->nLength * 2;
			if (len <= dwMemLength)
				memcpy(pIns->pSample, lpMemFile, len);
			short int* pSample = (short int*) pIns->pSample;
			for (UINT j = 0; j < len; j += 2) {
				*pSample++ = bswapLE16(*pSample);
			}
		}
		break;

		// 16-bit signed mono PCM motorola byte order
	case RS_PCM16M:
		len = pIns->nLength * 2;
		if (len > dwMemLength)
			len = dwMemLength & ~ 1;
		if (len > 1) {
			signed char * pSample = (signed char *) pIns->pSample;
			signed char * pSrc = (signed char *) lpMemFile;
			for (UINT j = 0; j < len; j += 2) {
				// pSample[j] = pSrc[j+1];
				// pSample[j+1] = pSrc[j];
				*((unsigned short *) (pSample + j)) = bswapBE16(*((unsigned short *)
					(pSrc + j)));
			}
		}
		break;

		// 6: 16-bit unsigned PCM data
	case RS_PCM16U:
		 {
			len = pIns->nLength * 2;
			if (len > dwMemLength)
				break;
			short int* pSample = (short int*) pIns->pSample;
			short int* pSrc = (short int*) lpMemFile;
			for (UINT j = 0; j < len; j += 2)
				*pSample++ = bswapLE16(*(pSrc++)) - 0x8000;
		}
		break;

		// 16-bit signed stereo big endian
	case RS_STPCM16M:
		len = pIns->nLength * 2;
		if (len * 2 <= dwMemLength) {
			signed char * pSample = (signed char *) pIns->pSample;
			signed char * pSrc = (signed char *) lpMemFile;
			for (UINT j = 0; j < len; j += 2) {
				// pSample[j*2] = pSrc[j+1];
				// pSample[j*2+1] = pSrc[j];
				// pSample[j*2+2] = pSrc[j+1+len];
				// pSample[j*2+3] = pSrc[j+len];
				*((unsigned short *) (pSample + j * 2)) = bswapBE16(*((unsigned short *)
					(pSrc + j)));
				*((unsigned short *) (pSample + j * 2 + 2)) = bswapBE16(*((unsigned short *)
					(pSrc + j + len)));
			}
			len *= 2;
		}
		break;

		// 8-bit stereo samples
	case RS_STPCM8S:
	case RS_STPCM8U:
	case RS_STPCM8D:
		 {
			int iadd_l = 0, iadd_r = 0;
			if (nFlags == RS_STPCM8U) {
				iadd_l = iadd_r = -128;
			}
			len = pIns->nLength;
			signed char * psrc = (signed char *) lpMemFile;
			signed char * pSample = (signed char *) pIns->pSample;
			if (len * 2 > dwMemLength)
				break;
			for (UINT j = 0; j < len; j++) {
				pSample[j * 2] = (signed char) (psrc[0] + iadd_l);
				pSample[j * 2 + 1] = (signed char) (psrc[len] + iadd_r);
				psrc++;
				if (nFlags == RS_STPCM8D) {
					iadd_l = pSample[j * 2];
					iadd_r = pSample[j * 2 + 1];
				}
			}
			len *= 2;
		}
		break;

		// 16-bit stereo samples
	case RS_STPCM16S:
	case RS_STPCM16U:
	case RS_STPCM16D:
		 {
			int iadd_l = 0, iadd_r = 0;
			if (nFlags == RS_STPCM16U) {
				iadd_l = iadd_r = -0x8000;
			}
			len = pIns->nLength;
			short int* psrc = (short int*) lpMemFile;
			short int* pSample = (short int*) pIns->pSample;
			if (len * 4 > dwMemLength)
				break;
			for (UINT j = 0; j < len; j++) {
				pSample[j * 2] = (short int) (bswapLE16(psrc[0]) + iadd_l);
				pSample[j * 2 + 1] = (short int)
					(bswapLE16(psrc[len]) + iadd_r);
				psrc++;
				if (nFlags == RS_STPCM16D) {
					iadd_l = pSample[j * 2];
					iadd_r = pSample[j * 2 + 1];
				}
			}
			len *= 4;
		}
		break;

		// IT 2.14 compressed samples
	case RS_IT2148:
	case RS_IT21416:
	case RS_IT2158:
	case RS_IT21516:
		len = dwMemLength;
		if (len < 4)
			break;
		if ((nFlags == RS_IT2148) || (nFlags == RS_IT2158))
			ITUnpack8Bit(pIns->pSample,
				pIns->nLength,
				(LPBYTE) lpMemFile,
				dwMemLength,
				(nFlags == RS_IT2158));
		else
			ITUnpack16Bit(pIns->pSample,
				pIns->nLength,
				(LPBYTE) lpMemFile,
				dwMemLength,
				(nFlags == RS_IT21516));
		break;

		// Default: 8-bit signed PCM data
	default:
		len = pIns->nLength;
		if (len > dwMemLength)
			len = pIns->nLength = dwMemLength;
		memcpy(pIns->pSample, lpMemFile, len);
	}
	if (len > dwMemLength) {
		if (pIns->pSample) {
			pIns->nLength = 0;
			FreeSample(pIns->pSample);
			pIns->pSample = NULL;
		}
		return 0;
	}
	AdjustSampleLoop(pIns);
	return len;
}


void CDecompressTrack::AdjustSampleLoop(MODINSTRUMENT* pIns)
	//----------------------------------------------------
{
	if (!pIns->pSample)
		return;
	if (pIns->nLoopEnd > pIns->nLength)
		pIns->nLoopEnd = pIns->nLength;
	if (pIns->nLoopStart + 2 >= pIns->nLoopEnd) {
		pIns->nLoopStart = pIns->nLoopEnd = 0;
		pIns->uFlags &= ~CHN_LOOP;
	}
	UINT len = pIns->nLength;
	if (pIns->uFlags & CHN_16BIT) {
		short int* pSample = (short int*) pIns->pSample;
		// Adjust end of sample
		if (pIns->uFlags & CHN_STEREO) {
			pSample[len * 2 + 6] = pSample[len * 2 + 4] = pSample[len * 2 + 2] = pSample[len * 2] = pSample[len * 2 - 2];
			pSample[len * 2 + 7] = pSample[len * 2 + 5] = pSample[len * 2 + 3] = pSample[len * 2 + 1] = pSample[len * 2 - 1];
		} else {
			pSample[len + 4] = pSample[len + 3] = pSample[len + 2] = pSample[len + 1] = pSample[len] = pSample[len - 1];
		}
		if ((pIns->uFlags & (CHN_LOOP | CHN_PINGPONGLOOP | CHN_STEREO)) ==
			CHN_LOOP) {
			// Fix bad loops
			if ((pIns->nLoopEnd + 3 >= pIns->nLength) ||
				(m_nType & MOD_TYPE_S3M)) {
				pSample[pIns->nLoopEnd] = pSample[pIns->nLoopStart];
				pSample[pIns->nLoopEnd + 1] = pSample[pIns->nLoopStart + 1];
				pSample[pIns->nLoopEnd + 2] = pSample[pIns->nLoopStart + 2];
				pSample[pIns->nLoopEnd + 3] = pSample[pIns->nLoopStart + 3];
				pSample[pIns->nLoopEnd + 4] = pSample[pIns->nLoopStart + 4];
			}
		}
	} else {
		signed char * pSample = pIns->pSample;
		// Adjust end of sample
		if (pIns->uFlags & CHN_STEREO) {
			pSample[len * 2 + 6] = pSample[len * 2 + 4] = pSample[len * 2 + 2] = pSample[len * 2] = pSample[len * 2 - 2];
			pSample[len * 2 + 7] = pSample[len * 2 + 5] = pSample[len * 2 + 3] = pSample[len * 2 + 1] = pSample[len * 2 - 1];
		} else {
			pSample[len + 4] = pSample[len + 3] = pSample[len + 2] = pSample[len + 1] = pSample[len] = pSample[len - 1];
		}
		if ((pIns->uFlags & (CHN_LOOP | CHN_PINGPONGLOOP | CHN_STEREO)) ==
			CHN_LOOP) {
			if ((pIns->nLoopEnd + 3 >= pIns->nLength) ||
				(m_nType & (MOD_TYPE_MOD | MOD_TYPE_S3M))) {
				pSample[pIns->nLoopEnd] = pSample[pIns->nLoopStart];
				pSample[pIns->nLoopEnd + 1] = pSample[pIns->nLoopStart + 1];
				pSample[pIns->nLoopEnd + 2] = pSample[pIns->nLoopStart + 2];
				pSample[pIns->nLoopEnd + 3] = pSample[pIns->nLoopStart + 3];
				pSample[pIns->nLoopEnd + 4] = pSample[pIns->nLoopStart + 4];
			}
		}
	}
}


/////////////////////////////////////////////////////////////
// Transpose <-> Frequency conversions

// returns 8363*2^((transp*128+ftune)/(12*128))
DWORD CDecompressTrack::TransposeToFrequency(int transp, int ftune)
	//-----------------------------------------------------------
{
	//---GCCFIX:  Removed assembly.
	return (DWORD) (8363 * pow(2.0f, (transp * 128 + ftune) / (1536)));
}


// returns 12*128*log2(freq/8363)
int CDecompressTrack::FrequencyToTranspose(DWORD freq)
	//----------------------------------------------
{
	//---GCCFIX:  Removed assembly.
	return (int) (1536 * (log((double) freq / 8363) / log((double) 2)));
}


void CDecompressTrack::FrequencyToTranspose(MODINSTRUMENT* psmp)
	//--------------------------------------------------------
{
	int f2t = FrequencyToTranspose(psmp->nC4Speed);
	int transp = f2t >> 7;
	int ftune = f2t & 0x7F;
	if (ftune > 80) {
		transp++;
		ftune -= 128;
	}
	if (transp > 127)
		transp = 127;
	if (transp < -127)
		transp = -127;
	psmp->RelativeTone = transp;
	psmp->nFineTune = ftune;
}

/*
void CDecompressTrack::CheckCPUUsage(UINT nCPU)
//---------------------------------------
{
	if (nCPU > 100) nCPU = 100;
	gnCPUUsage = nCPU;
	if (nCPU < 90)
	{
		m_dwSongFlags &= ~SONG_CPUVERYHIGH;
	} else
	if ((m_dwSongFlags & SONG_CPUVERYHIGH) && (nCPU >= 94))
	{
		UINT i=MAX_CHANNELS;
		while (i >= 8)
		{
			i--;
			if (Chn[i].nLength)
			{
				Chn[i].nLength = Chn[i].nPos = 0;
				nCPU -= 2;
				if (nCPU < 94) break;
			}
		}
	} else
	if (nCPU > 90)
	{
		m_dwSongFlags |= SONG_CPUVERYHIGH;
	}
}
*/

BOOL CDecompressTrack::SetPatternName(UINT nPat, LPCSTR lpszName)
	//---------------------------------------------------------
{
	char szName[MAX_PATTERNNAME] = "";   // changed from CHAR
	if (nPat >= MAX_PATTERNS)
		return FALSE;
	if (lpszName)
		lstrcpyn(szName, lpszName, MAX_PATTERNNAME);
	szName[MAX_PATTERNNAME - 1] = 0;
	if (!m_lpszPatternNames)
		m_nPatternNames = 0;
	if (nPat >= m_nPatternNames) {
		if (!lpszName[0])
			return TRUE;
		UINT len = (nPat + 1) * MAX_PATTERNNAME;
		char* p = new char[len];   // changed from CHAR
		if (!p)
			return FALSE;
		memset(p, 0, len);
		if (m_lpszPatternNames) {
			memcpy(p, m_lpszPatternNames, m_nPatternNames * MAX_PATTERNNAME);
			delete m_lpszPatternNames;
			m_lpszPatternNames = NULL;
		}
		m_lpszPatternNames = p;
		m_nPatternNames = nPat + 1;
	}
	memcpy(m_lpszPatternNames + nPat * MAX_PATTERNNAME,
		szName,
		MAX_PATTERNNAME);
	return TRUE;
}


BOOL CDecompressTrack::GetPatternName(UINT nPat, LPSTR lpszName, UINT cbSize) const
	//---------------------------------------------------------------------------
{
	if ((!lpszName) || (!cbSize))
		return FALSE;
	lpszName[0] = 0;
	if (cbSize > MAX_PATTERNNAME)
		cbSize = MAX_PATTERNNAME;
	if ((m_lpszPatternNames) && (nPat < m_nPatternNames)) {
		memcpy(lpszName, m_lpszPatternNames + nPat * MAX_PATTERNNAME, cbSize);
		lpszName[cbSize - 1] = 0;
		return TRUE;
	}
	return FALSE;
}
