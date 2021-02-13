/*
 * WavFile.cpp - Very simplistic and inflexible WAV file decoder.
 *
 *  Created on: 5 Apr 2020
 *      Author: adam
 */

#include "WavFile.h"

// 4CC codes for different chunk types that interest us.
enum WavChunkType {
	kWavHdrRiff = 0x46464952, // "RIFF"
	kWavHdrWave = 0x45564157, // "WAVE"
	kWavHdrData = 0x61746164, // "data"
	kWavHdrFmt  = 0x20746D66, // "FMT "
};

struct __attribute__((packed)) WavChunkHdr
{
	uint32_t chunkType; // Chunk type (one of the WavChunkType values).
	uint32_t size;      // Size of chunk (not including this header).
};

// The RIFF chunk should be at the start of the file.
struct __attribute__((packed)) WavRiffHdr
{
	uint32_t chunkType; // always "RIFF"
	uint32_t size;      // Size of rest of file.
	uint32_t wave;      // Should be "WAVE"
};

// WAV format structure.
struct __attribute__((packed)) WavFormat
{
	uint16_t format;        // Data format (1=PCM)
	uint16_t numChannels;   // 1=Mono, 2=Stereo etc.
	uint32_t sampleRate;    // Samples per second
	uint32_t byteRate;      // Bytes per second
	uint16_t blockAlign;    // Bytes per sample
	uint16_t numBits;       // Bits per sample (8, 16 etc)
};

WavFile::WavFile()
{
	close();
}

void WavFile::close()
{
	_f.close();
	_sampleRate = 0;
	_bitsPerSample = 0;
	_blockAlign = 0;
	_nChannels = 0;
	_dataOffset = 0;
	_dataSize = 0;
}

// Open a WAV file, load the header and find the data section.
bool WavFile::open(const TCHAR *filename)
{
	int r;

	// Dump any existing data.
	close();

	// Open the file.
	if(!_f.open(filename))
		return false;

	// Read the RIFF header.
	WavRiffHdr riffHdr;
	r = _f.read((uint8_t *)&riffHdr, sizeof(riffHdr));
	if(r < sizeof(riffHdr)) {
		return false;
	}

	// First chunk must be RIFF and the chunk content must be WAVE.
	if(riffHdr.chunkType != kWavHdrRiff || riffHdr.wave != kWavHdrWave)
		return false;

	// The RIFF chunk contains the other chunks so iterate through them.
	WavChunkHdr hdr;
	WavFormat   fmt;
	unsigned riffSize = riffHdr.size;
	while(riffSize > 0)
	{
		// Chunks are usually in the order RIFF, FMT, DATA. I'm not sure if it should be a valid
		// WAV file if they are not in this order.

		// Read the next chunk.
		r = _f.read((uint8_t *)&hdr, sizeof(hdr));
		if(r < sizeof(hdr)) {
			return false;
		}

		// Do we care about this chunk?
		switch(hdr.chunkType)
		{
		case kWavHdrData: // Data chunk. Record the location but don't read it yet.
			_dataOffset = _f.tell();
			_dataSize   = hdr.size;
			_f.seek(hdr.size, File::SEEK_CUR);
			break;

		case kWavHdrFmt: // Sample format info. Sample rate, channels etc.
			r = _f.read((uint8_t *)&fmt, sizeof(fmt));
			if(r != sizeof(fmt))
				return false;
			if(fmt.format != 1)
				return false; // Not in PCM sample format.

			// Grab the information we want.
			_nChannels     = fmt.numChannels;
			_sampleRate    = fmt.sampleRate;
			_blockAlign    = fmt.blockAlign;
			_bitsPerSample = fmt.numBits;
			break;

		default: // Unknown chunk. Skip it.
			_f.seek(hdr.size, File::SEEK_CUR);
			break;
		}

		// Do we have all the valid data we need?
		if(_dataOffset != 0 && _sampleRate != 0) {
			rewind();
			return true;
		}

		riffSize -= (sizeof(hdr) + hdr.size);
	}

	// End of file reached, no valid WAV data found.
	return false;
}

unsigned WavFile::readBlock(uint8_t *dest, unsigned nBytes)
{
	return _f.read(dest, nBytes);
}

// Move playback to the beginning of the data.
bool WavFile::rewind()
{
	return _f.seek(_dataOffset);
}


#ifdef NEW

// At the moment this only supports mono 8 or 16-bit PCM WAV files.

#include "audio.h"
#include <string.h>

// Main wave file header.
struct WAVHDR
{
	// First chunk - RIFF
	uint32_t riffSignature; // Should contain "RIFF"
	uint32_t fileSize;      // Size of entire file minus 8
	uint32_t waveSignature; // Should contain "WAVE"
};


// Data chunk header.
struct WAVHDR_DATA
{
	uint32_t dataSignature; // Should contain "data".
	uint32_t dataChunkSize; // Size of data in bytes.
	uint8_t  data[1];       // The actual wave data (may be more than one byte).
};


struct WAVHDR        *wavFile;
struct WAVHDR_FORMAT *wavFormat;
struct WAVHDR_DATA   *wavData;
unsigned              wavPlaybackPosition;
BOOL                  wavLoop;

void wavDataProvider(struct AudioFrame *frame)
{
	unsigned  dataSize = AUDIO_BUFFER_SIZE;
	int       i;
	uint16_t *src, *dst;

	// Handle end of file.
	if(wavPlaybackPosition >= wavData->dataChunkSize)
	{
		if(wavLoop)
			wavPlaybackPosition = 0;
		else
		{
			frame->nSamples = 0;
			return;
		}
	}

	// Find the size of the next frame.
	if(wavPlaybackPosition + dataSize > wavData->dataChunkSize)
		dataSize = wavData->dataChunkSize - wavPlaybackPosition;

	// Retrieve the frame data.
	if(wavFormat->numBits == 16)
	{
		frame->nSamples = dataSize >> 1;

		// Convert from signed 16-bit to unsigned
		src = (uint16_t *)&wavData->data[wavPlaybackPosition];
		dst = (uint16_t *)&frame->data[0];
		for(i = 0; i < frame->nSamples; i++)
			dst[i] = src[i] + 0x8000;
	}
	else
	{
		frame->nSamples = dataSize;
		memcpy(frame->data, &wavData->data[wavPlaybackPosition], dataSize);
	}

	wavPlaybackPosition += dataSize;
}

BOOL wavPlay(uint8_t *wavfile, BOOL loop)
{
	wavFile = (struct WAVHDR *)wavfile;

	// Check the RIFF chunk.
	if(wavFile->riffSignature != WAVE_HEAD_RIFF)
		return FALSE;

	// Check for WAVE signature.
	if(wavFile->waveSignature != WAVE_HEAD_WAVE)
		return FALSE;

	// Find the "fmt " chunk.
	wavfile += sizeof(struct WAVHDR);
	wavFormat = (struct WAVHDR_FORMAT *)wavFindChunk(wavfile, WAVE_HEAD_FMT);
	if(wavFormat == 0)
		return FALSE;

	// Check format is PCM.
	if(wavFormat->format != 0x01)
		return FALSE;

	// We only support mono WAV files.
	if(wavFormat->numChannels != 1)
		return FALSE;

	// We only support 8 or 16 bit data and no funny business with frame padding.
	if(wavFormat->numBits == 8)
	{
		if(wavFormat->blockAlign != 1)
			return FALSE;
	}
	else if(wavFormat->numBits == 16)
	{
		if(wavFormat->blockAlign != 2)
			return FALSE;
	}
	else
		return FALSE;

	// Find the "data" chunk.
	wavfile += sizeof(struct WAVHDR_FORMAT);
	wavData = (struct WAVHDR_DATA *)wavFindChunk(wavfile, WAVE_HEAD_DATA);
	if(wavData == 0)
		return FALSE;

	// Start replay.
	wavLoop = loop;
	wavPlaybackPosition = 0;

	audioPlay(wavFormat->numBits, wavFormat->sampleRate, wavDataProvider);

	return TRUE;
}

}

// Search starting from the given start position, for a chunk with the
// given signature. Return NULL if not found
void *wavFindChunk(uint8_t *start, uint32_t signature)
{
	struct WAVCHUNK *chunk;

	while(start < (uint8_t *)wavFile + wavFile->fileSize + 8)
	{
		chunk = (struct WAVCHUNK *)start;
		if(chunk->signature == signature)
			return start;
		start += 8 + chunk->chunkSize;
	}

	return 0;
}

#endif


#ifdef OLD
// WAV file header.
struct __attribute__((packed)) WAVE_FormatTypeDef {
  uint32_t ChunkID;
  uint32_t FileSize;
  uint32_t FileFormat;

  uint32_t SubChunk1ID;
  uint32_t SubChunk1Size;
  uint16_t AudioFormat;
  uint16_t NumChannels;
  uint32_t SampleRate;
  uint32_t ByteRate;
  uint16_t BlockAlign;
  uint16_t BitsPerSample;

  uint32_t SubChunk2ID;
  uint32_t SubChunk2Size;
};


// Open a file and read the header.
bool WavFile::open(const TCHAR *filename)
{
	if(!_f.open(filename))
		return false;

	WAVE_FormatTypeDef hdr;
	int cb = _f.read((uint8_t *)&hdr, sizeof(hdr));
	if(cb < sizeof(hdr)) {
		return false;
	}

	_sampleRate = hdr.SampleRate;
	_bitsPerSample = hdr.BitsPerSample;
	_nChannels = hdr.NumChannels;

	return true;
}

// Read some data bytes.
unsigned WavFile::readBlock(uint8_t *dest, unsigned nBytes)
{
	return _f.read(dest, nBytes);
}

// Return the size of the entire data section of the file in bytes.
unsigned WavFile::getByteSize() const
{
	return _f.getSize() - sizeof(WAVE_FormatTypeDef);
}

// Move playback to the beginning of the data.
bool WavFile::rewind()
{
	return _f.seek(sizeof(WAVE_FormatTypeDef));
}

#endif // OLD
