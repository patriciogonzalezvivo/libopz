#pragma once

#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>

namespace T3 {

// debug print
char *printHex(unsigned char *cp, size_t n);
char *printAscii(unsigned char *cp, size_t n);

std::string toStringHex(uint8_t _v);
std::string toStringHex(uint16_t _v);
std::string toStringHex(uint32_t _v);

// memory address to project/pattern values 
uint8_t address2project(uint8_t _address);
uint8_t address2pattern(uint8_t _address);

// encode raw 8bit to 7bits  
size_t encode(const unsigned char* inData, unsigned inLength, unsigned char* outSysEx, bool inFlipHeaderBits = true);

// decode 7bit to raw 8bits
size_t decode(const unsigned char* inData, size_t inLength, unsigned char* outData, bool inFlipHeaderBits = true); 

// Zlib compression
std::vector<unsigned char> compress(const unsigned char* inData, size_t inLength);

// zlib decompression
std::vector<unsigned char> decompress(const unsigned char* inData, size_t inLength);


};