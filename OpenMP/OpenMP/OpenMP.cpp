#include <iostream>
#include <fstream>
#include <ctime>
#include "bitmapRW.h"
#include <omp.h>
#include <thread>


using namespace std;

struct RGBPixel {
	unsigned char blue;
	unsigned char green;
	unsigned char red;
};

void Bitmap2RGBPixels(const BITMAPINFOHEADER& i_info, const unsigned char* ip_bitmap, RGBPixel** op_rgb_pixels)
{
	const auto bytes_per_pixel = (i_info.biBitCount) / 8;
	size_t pos = 0;
	for (size_t i = 0; i < i_info.biWidth; ++i)
	{
		for (size_t j = 0; j < i_info.biHeight; ++j)
		{
			op_rgb_pixels[j][i].blue = ip_bitmap[pos + 2];
			op_rgb_pixels[j][i].green = ip_bitmap[pos + 1];
			op_rgb_pixels[j][i].red = ip_bitmap[pos];
			pos += bytes_per_pixel;
		}
	}
}

void RGBPixels2Bitmap(RGBPixel** ip_rgb_pixels, const BITMAPINFOHEADER& i_info, unsigned char* op_bitmap)
{
	const auto bytes_per_pixel = (i_info.biBitCount) / 8;
	size_t pos = 0;
	for (size_t i = 0; i < i_info.biWidth; ++i)
	{
		for (size_t j = 0; j < i_info.biHeight; ++j)
		{
			op_bitmap[pos + 2] = ip_rgb_pixels[j][i].blue;
			op_bitmap[pos + 1] = ip_rgb_pixels[j][i].green;
			op_bitmap[pos] = ip_rgb_pixels[j][i].red;
			pos += bytes_per_pixel;
		}
	}
}

class ScopedPixels
{
public:
	ScopedPixels(std::uint32_t i_h, std::uint32_t i_w)
		: m_h(i_h)
		, m_w(i_w)
	{
		mp_pixels = new RGBPixel * [m_h];
		for (size_t i = 0; i < m_h; ++i)
			mp_pixels[i] = new RGBPixel[i_w];
	}

	~ScopedPixels()
	{
		for (size_t i = 0; i < m_h; ++i)
			delete[] mp_pixels[i];
		delete[] mp_pixels;
	}

	operator RGBPixel** () { return mp_pixels; }
	RGBPixel** Get() { return mp_pixels; }

private:
	std::uint32_t m_h = 0, m_w = 0;
	RGBPixel** mp_pixels = nullptr;
};


void inverseColors(RGBPixel** pixels, int w, int h) {

	unsigned int NumberOfThreads = std::thread::hardware_concurrency();
	omp_set_dynamic(true);
	#pragma omp parallel num_threads(NumberOfThreads)
	{
		#pragma omp for
		for (int i = 0; i < h; i++) {
			for (int j = 0; j < w; j++) {
				pixels[i][j].blue = 255 - pixels[i][j].blue;
				pixels[i][j].green = 255 - pixels[i][j].green;
				pixels[i][j].red = 255 - pixels[i][j].red;
			}
		}
	}
}

int main(int argc, char** argv) {
	cout << fixed;
	cout.precision(6);

	if (argc == 3)
	{
		double start, end, readTime, writeTime, processTime;

		BITMAPFILEHEADER file_header;
		BITMAPINFOHEADER info_header;

		start = clock();
		auto p_bitmap = LoadBitmapFile(argv[1], &info_header, &file_header);

		ScopedPixels pixels(info_header.biHeight, info_header.biWidth);
		Bitmap2RGBPixels(info_header, p_bitmap.get(), pixels);

		inverseColors(pixels, info_header.biWidth, info_header.biHeight);

		RGBPixels2Bitmap(pixels, info_header, p_bitmap.get());

		WriteBitmapFile(argv[2], &info_header, &file_header, p_bitmap.get());
		end = clock();

		processTime = (end - start) / CLOCKS_PER_SEC;
		cout << "Processing time - " << processTime << endl;
	}
	else {
		cout << "Usage: ColorInverseSerial.exe input.bmp output.bmp" << endl;
	}
	return 0;
}
