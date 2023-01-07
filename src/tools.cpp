#include "libopz/tools.h"

#include <zlib.h>
#include <stdlib.h>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace T3 {

// print array of unsigend chars as HEX pairs
char *printHex(unsigned char *cp, size_t n) {
    char *s = (char*)malloc(3*n );

    if (s == NULL)
        return s;

    for (size_t k = 0; k < n; ++k)
        sprintf(s + 3*(k), "%02X ", cp[k]);

    s[3*n] = '\0';
    return s;
}

char *printAscii(unsigned char *cp, size_t n) {
    char *s = (char*)malloc(n);

    if (s == NULL)
        return s;

    for (size_t k = 0; k < n; ++k)
        sprintf(s + k, "%c", cp[k]);

    s[n] = '\0';
    return s;
}

std::string toString(uint32_t _value) {
    std::ostringstream out;
    out << std::fixed << _value;
    return out.str();
}

std::string toStringHex(uint8_t _v) {
    std::ostringstream strStream;
    strStream << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (0xFF & _v);
    return strStream.str();
}

std::string toStringHex(uint16_t _v) {
    std::ostringstream strStream;
    strStream << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << (0xFFFF & _v);
    return strStream.str();
}

std::string toStringHex(uint32_t _v) {
    std::ostringstream strStream;
    strStream << std::setfill('0') << std::setw(8) << std::uppercase << std::hex << (0xFFFFFFFF & _v);
    return strStream.str();
}

std::vector<std::string> note_letter = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

std::string toStringNote(uint8_t _v) {
    std::ostringstream out;
    out << std::fixed << (_v / 12) << note_letter[_v % 12];
    return out.str();
}

uint8_t address2project(uint8_t _address) { return _address / 16; }
uint8_t address2pattern(uint8_t _address) { return _address % 16; }

size_t encode(const unsigned char* inData, unsigned inLength, unsigned char* outSysEx, bool inFlipHeaderBits) {
    size_t outLength    = 0;     // Num bytes in output array.
    unsigned char count = 0;     // Num 7bytes in a block.
    outSysEx[0]         = 0;

    for (unsigned i = 0; i < inLength; ++i) {
        const unsigned char data = inData[i];
        const unsigned char msb  = data >> 7;
        const unsigned char body = data & 0x7f;

        outSysEx[0] |= (msb << (inFlipHeaderBits ? count : (6 - count)));
        outSysEx[1 + count] = body;

        if (count++ == 6) {
            outSysEx   += 8;
            outLength  += 8;
            outSysEx[0] = 0;
            count       = 0;
        }
    }
    return outLength + count + (count != 0 ? 1 : 0);
}

size_t decode(const unsigned char* inData, size_t inLength, unsigned char* outData, bool inFlipHeaderBits) {
    size_t count  = 0;
    unsigned char msbStorage = 0;
    unsigned char byteIndex  = 0;

    for (size_t i = 0; i < inLength; ++i) {
        if ((i % 8) == 0) {
            msbStorage = inData[i];
            byteIndex  = 6;
        }
        else {
            const unsigned char body     = inData[i];
            const unsigned char shift    = inFlipHeaderBits ? 6 - byteIndex : byteIndex;
            const unsigned char msb      = uint8_t(((msbStorage >> shift) & 1) << 7);
            byteIndex--;
            outData[count++] = msb | body;
        }
    }
    return count;
}

const unsigned CHUNK_SIZE = 4096;
std::vector<unsigned char> compress(const unsigned char* inData, size_t inLength) {
    std::vector<unsigned char> output;

	z_stream stream;
	stream.zalloc = 0;
	stream.zfree = 0;
	stream.opaque = 0;

	stream.avail_in = inLength - 1;
    stream.next_in = const_cast<Byte *>(&inData[0]);

    int res = deflateInit(&stream, 9);
	if (res != Z_OK)
		return output;

    unsigned char ChunkOut[CHUNK_SIZE];
	do
	{
		stream.avail_out = sizeof(ChunkOut);
		stream.next_out = ChunkOut;
		res = deflate(&stream, Z_FINISH);
		unsigned compressed = sizeof(ChunkOut) - stream.avail_out;
		unsigned oldsize = output.size();
		output.resize(oldsize + compressed);
		memcpy(&output[oldsize], ChunkOut, compressed);
	}
	while (stream.avail_out == 0);
	deflateEnd(&stream);

	return output;
}

std::vector<unsigned char> decompress(const unsigned char* inData, size_t inLength) {
    std::vector<unsigned char> output;

    z_stream stream;
    stream.zalloc = 0;
    stream.zfree = 0;
    stream.opaque = 0;

    stream.avail_in = inLength - 1;
    stream.next_in = const_cast<Byte*>(&inData[0]);
    
    int res = inflateInit(&stream);
    if (res != Z_OK)
        return output;

    unsigned char ChunkOut[CHUNK_SIZE];
    do {
        stream.avail_out = sizeof(ChunkOut);
        stream.next_out = ChunkOut;
        res = inflate(&stream, Z_FINISH);
        unsigned compressed = sizeof(ChunkOut) - stream.avail_out;
        unsigned oldsize = output.size();
        output.resize(oldsize + compressed);
        memcpy(&output[oldsize], ChunkOut, compressed);
    }
    while (stream.avail_out == 0);
    inflateEnd(&stream);

    return output;
}

};