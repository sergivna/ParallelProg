#pragma once

#include <cstdint>
#include <functional>
#include <memory>


#pragma pack(push, 1)

struct BITMAPFILEHEADER
{
	std::uint16_t bfType = 0;  //specifies the file type
	std::uint32_t bfSize = 0;  //specifies the size in bytes of the bitmap file
	std::uint16_t bfReserved1 = 0;  //reserved; must be 0
	std::uint16_t bfReserved2 = 0;  //reserved; must be 0
	std::uint32_t bfOffBits = 0;  //species the offset in bytes from the bitmapfileheader to the bitmap bits
};

#pragma pack(pop)

#pragma pack(push, 1)

struct BITMAPINFOHEADER
{
	std::uint32_t biSize = 0;  //specifies the number of bytes required by the struct
	std::uint32_t biWidth = 0;  //specifies width in pixels
	std::uint32_t biHeight = 0;  //species height in pixels
	std::uint16_t biPlanes = 0; //specifies the number of color planes, must be 1
	std::uint16_t biBitCount = 0; //specifies the number of bit per pixel
	std::uint32_t biCompression = 0;//spcifies the type of compression
	std::uint32_t biSizeImage = 0;  //size of image in bytes
	std::uint32_t biXPelsPerMeter = 0;  //number of pixels per meter in x axis
	std::uint32_t biYPelsPerMeter = 0;  //number of pixels per meter in y axis
	std::uint32_t biClrUsed = 0;  //number of colors used by th ebitmap
	std::uint32_t biClrImportant = 0;  //number of colors that are important
};

#pragma pack(pop)

using BitmapImagePtr = std::unique_ptr<unsigned char[], std::function<void(unsigned char*)>>;
BitmapImagePtr LoadBitmapFile(const char* filename, BITMAPINFOHEADER* bitmapInfoHeader, BITMAPFILEHEADER* bitmapFileHeader);

void WriteBitmapFile(const char* filename, BITMAPINFOHEADER* bitmapInfoHeader,
	BITMAPFILEHEADER* bitmapFileHeader, unsigned char* bitmapImage);