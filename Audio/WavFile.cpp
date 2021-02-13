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

// Get a chunk of wave data.
unsigned WavFile::readBlock(uint8_t *dest, unsigned nBytes)
{
	return _f.read(dest, nBytes);
}

// Move playback to the beginning of the data.
bool WavFile::rewind()
{
	return _f.seek(_dataOffset);
}
