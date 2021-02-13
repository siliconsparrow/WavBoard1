/*
 * WavFile.h
 *
 *  Created on: 5 Apr 2020
 *      Author: adam
 */

#ifndef WAVFILE_H_
#define WAVFILE_H_

#include "Filesystem.h"
#include <stdint.h>

class WavFile {
public:
	WavFile();

	bool open(const TCHAR *filename);
	void close();

	unsigned readBlock(uint8_t *dest, unsigned nBytes);

	bool rewind();

	unsigned getByteSize()   const { return _dataSize; }
	unsigned getSampleRate() const { return _sampleRate; }
	unsigned getNumBits()    const { return _bitsPerSample; }
	unsigned getChannels()   const { return _nChannels; }

private:
	File     _f;
	unsigned _sampleRate;
	unsigned _bitsPerSample;
	unsigned _blockAlign;
	unsigned _nChannels;
	unsigned _dataOffset;
	unsigned _dataSize;
};

#endif /* WAVFILE_H_ */
