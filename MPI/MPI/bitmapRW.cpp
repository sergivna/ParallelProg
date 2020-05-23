#include "bitmapRW.h"
#include "omp.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <stddef.h>
#include <string.h>
#include <cassert>
#include "bitmapRW.h"


BitmapImagePtr LoadBitmapFile(const char* filename, BITMAPINFOHEADER* bitmapInfoHeader,
	BITMAPFILEHEADER* bitmapFileHeader)
{
	FILE* filePtr = nullptr; // file pointer

	unsigned char* bitmapImage;  // image data
	auto imageIdx = 0u;  // image index counter
	unsigned char tempRGB = 0;  // swap variable

	//open filename in read binary mode
	auto result = fopen_s(&filePtr, filename, "rb");
	if (filePtr == NULL || result != 0)
		return BitmapImagePtr(nullptr, &free);

	//read the bitmap file header
	fread(bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr);

	//verify that this is a bmp file by check bitmap id
	if (bitmapFileHeader->bfType != 0x4D42)
	{
		fclose(filePtr);
		return BitmapImagePtr(nullptr, &free);
	}

	//read the bitmap info header
	fread(bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr);

	//move file point to the begging of bitmap data
	fseek(filePtr, bitmapFileHeader->bfOffBits, SEEK_SET);

	if (bitmapInfoHeader->biSizeImage == 0)
	{
		auto size = 3 * bitmapInfoHeader->biWidth + bitmapInfoHeader->biWidth % 4;
		size *= bitmapInfoHeader->biHeight;
		bitmapInfoHeader->biSizeImage = size;
		bitmapImage = (unsigned char*)malloc(bitmapInfoHeader->biSizeImage);
	}
	else
	{
		//allocate enough memory for the bitmap image data
		if (bitmapInfoHeader->biBitCount != 32 && (bitmapInfoHeader->biWidth % 4 != 0))
			bitmapImage = (unsigned char*)malloc(bitmapInfoHeader->biSizeImage
				- (4 - (bitmapInfoHeader->biSizeImage % 4)) * bitmapInfoHeader->biHeight);
		else
			bitmapImage = (unsigned char*)malloc(bitmapInfoHeader->biSizeImage);
	}

	//verify memory allocation
	if (!bitmapImage)
	{
		free(bitmapImage);
		fclose(filePtr);
		return BitmapImagePtr(nullptr, &free);
	}

	int bytesPerPixel = (bitmapInfoHeader->biBitCount) / 8;

	//read in the bitmap image data
	int padding = 4 - (bytesPerPixel * bitmapInfoHeader->biWidth) % 4;
	if (bitmapInfoHeader->biBitCount != 32 && (bytesPerPixel * bitmapInfoHeader->biWidth % 4 != 0)) {
		for (auto row = 0u; row < bitmapInfoHeader->biHeight; row++) {
			for (auto col = 0u; col < bytesPerPixel * bitmapInfoHeader->biWidth; col++)
				fread(&bitmapImage[row * bytesPerPixel * bitmapInfoHeader->biWidth + col], 1, 1, filePtr);
			for (int pad = 0; pad < padding; pad++)
				fseek(filePtr, 1, SEEK_CUR);
		}
	}
	else
		fread(bitmapImage, bitmapInfoHeader->biSizeImage, 1, filePtr);

	//make sure bitmap image data was read
	if (bitmapImage == NULL)
	{
		fclose(filePtr);
		return BitmapImagePtr(nullptr, &free);
	}

	//swap the r and b values to get RGB (bitmap is BGR)
	for (imageIdx = 0; imageIdx < bitmapInfoHeader->biWidth * bitmapInfoHeader->biHeight * bytesPerPixel; imageIdx += bytesPerPixel)
	{
		tempRGB = bitmapImage[imageIdx];
		bitmapImage[imageIdx] = bitmapImage[imageIdx + 2];
		bitmapImage[imageIdx + 2] = tempRGB;
	}

	//close file and return bitmap iamge data
	fclose(filePtr);

	

	return BitmapImagePtr(bitmapImage, &free);
}

void WriteBitmapFile(const char* filename, BITMAPINFOHEADER* bitmapInfoHeader,
	BITMAPFILEHEADER* bitmapFileHeader, unsigned char* bitmapImage)
{
	FILE* filePtr = nullptr; // file pointer

	auto imageIdx = 0u;  // image index counter
	unsigned char tempRGB = 0;  // swap variable

	//open filename in write binary mode
	auto result = fopen_s(&filePtr, filename, "wb");
	if (filePtr == NULL || result != 0)
		return;

	//verify that this is a bmp file by check bitmap id
	if (bitmapFileHeader->bfType != 0x4D42)
	{
		fclose(filePtr);
		return;
	}

	//write the bitmap file header
	fwrite(bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr);


	//write the bitmap info header
	fwrite(bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr);

	//move file point to the begging of bitmap data
	fseek(filePtr, bitmapFileHeader->bfOffBits, SEEK_SET);


	//make sure bitmap image data is not empty
	if (bitmapImage == NULL)
	{
		std::cout << "Image empty!" << std::endl;
		fclose(filePtr);
		return;
	}

	int bytesPerPixel = (bitmapInfoHeader->biBitCount) / 8;
	//swap the r and b values to get RGB (bitmap is BGR)
	for (imageIdx = 0; imageIdx < bitmapInfoHeader->biWidth * bitmapInfoHeader->biHeight * bytesPerPixel; imageIdx += bytesPerPixel)
	{
		tempRGB = bitmapImage[imageIdx];
		bitmapImage[imageIdx] = bitmapImage[imageIdx + 2];
		bitmapImage[imageIdx + 2] = tempRGB;
	}

	//write the bitmap image data
	unsigned char null_char = 0;
	int padding = 4 - (bytesPerPixel * bitmapInfoHeader->biWidth) % 4;
	if (bitmapInfoHeader->biBitCount != 32 && (bytesPerPixel * bitmapInfoHeader->biWidth % 4 != 0)) {
		for (auto row = 0u; row < bitmapInfoHeader->biHeight; row++) {
			for (auto col = 0u; col < bytesPerPixel * bitmapInfoHeader->biWidth; col++)
				fwrite(&bitmapImage[row * bytesPerPixel * bitmapInfoHeader->biWidth
					+ col], 1, 1, filePtr);
			for (int pad = 0; pad < padding; pad++)
				fwrite(&null_char, 1, 1, filePtr);
		}
	}
	else
		fwrite(bitmapImage, bitmapInfoHeader->biSizeImage, 1, filePtr);

	//close file and return bitmap iamge data
	fclose(filePtr);
}