#include "libopz/tools.h"

#include <zlib.h>
#include <stdlib.h>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace opz {

std::string printHex(unsigned char *cp, size_t n) {
    std::string s;
    s.reserve(3 * n);
    char buf[4];
    for (size_t k = 0; k < n; ++k) {
        snprintf(buf, sizeof(buf), "%02X ", cp[k]);
        s += buf;
    }
    return s;
}

std::string printAscii(unsigned char *cp, size_t n) {
    return std::string(reinterpret_cast<char*>(cp), n);
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

std::vector<std::string> note_letter = { "C",  "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
std::vector<bool> note_sharp = { false, true, false, true, false, false, true, false, true, false, true, false };
bool isSharpNote(uint8_t _v) { return note_sharp[_v % 12]; }
std::string toStringNote(uint8_t _v) {
    std::ostringstream out;
    out << note_letter[_v % 12] << std::fixed << (_v / 12);
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

    if (inLength == 0)
        return output;

	z_stream stream;
	stream.zalloc = 0;
	stream.zfree = 0;
	stream.opaque = 0;

	stream.avail_in = inLength;
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
		memcpy(output.data() + oldsize, ChunkOut, compressed);
	}
	while (stream.avail_out == 0);
	deflateEnd(&stream);

	return output;
}

std::vector<unsigned char> decompress(const unsigned char* inData, size_t inLength, bool* _complete) {
    std::vector<unsigned char> output;

    if (_complete)
        *_complete = false;

    if (inLength == 0)
        return output;

    z_stream stream;
    stream.zalloc = 0;
    stream.zfree = 0;
    stream.opaque = 0;

    stream.avail_in = inLength;
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
        memcpy(output.data() + oldsize, ChunkOut, compressed);

        // Per zlib's documented contract, Z_BUF_ERROR with avail_out == 0 just means
        // our chunk buffer was too small to hold everything in this round when
        // Z_FINISH is used - that's expected/non-fatal, so keep looping with a
        // fresh buffer. Z_BUF_ERROR with avail_out > 0 means input truly ran out
        // (a real truncation), which - like any other unexpected code - is fatal.
        bool buffer_too_small = (res == Z_BUF_ERROR && stream.avail_out == 0);
        if (res != Z_OK && res != Z_STREAM_END && !buffer_too_small)
            break;
    }
    while (stream.avail_out == 0 && res != Z_STREAM_END);
    inflateEnd(&stream);

    // A truncated/corrupt input (e.g. a dropped packet upstream) makes inflate()
    // stop before Z_STREAM_END; the caller can check _complete to tell a full
    // decode apart from this best-effort partial one.
    if (_complete)
        *_complete = (res == Z_STREAM_END);

    return output;
}

};