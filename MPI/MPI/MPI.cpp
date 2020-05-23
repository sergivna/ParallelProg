#include <iostream>
#include <fstream>
#include <ctime>
#include <mpi.h>
#include "bitmapRW.h"

using namespace std;

int ProcNum;
int ProcRank;

BITMAPFILEHEADER file_header;
BITMAPINFOHEADER info_header;

BitmapImagePtr p_bitmap;

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

void Initialization(int argc, char** argv, int*& pixels, int*& pixelResult, int& width, int& height,
	int*& procPixelRows, int*& procPixelResult, int& rowNum) {

	if (ProcRank == 0) {
		cout << "Initialization" << endl;
		if (argc == 3) {
			p_bitmap = LoadBitmapFile(argv[1], &info_header, &file_header);
			width = info_header.biWidth;
			height = info_header.biHeight;
		}
		else {
			cout << "Usage: mpiexec -n ProcNum ColorInverseMPI.exe input.bmp output.bmp" << endl;
		}
	}

	ScopedPixels pixels_(info_header.biHeight, info_header.biWidth);
	Bitmap2RGBPixels(info_header, p_bitmap.get(), pixels_);

	MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);

	int RestRows = height;
	for (int i = 0; i < ProcRank; i++)
		RestRows = RestRows - RestRows / (ProcNum - i);

	rowNum = RestRows / (ProcNum - ProcRank);

	pixelResult = new int[3 * width * height];
	procPixelRows = new int[3 * width * rowNum];
	procPixelResult = new int[3 * width * rowNum];

	if (ProcRank == 0) {
		pixels = new int[3 * width * height];
		for (int i = 0; i < height; i++)
			for (int j = 0; j < width; j++) {
				pixels[i * 3 * width + 3 * j] = pixels_[i][j].blue;
				pixels[i * 3 * width + 3 * j + 1] = pixels_[i][j].green;
				pixels[i * 3 * width + 3 * j + 2] = pixels_[i][j].red;
			}
	}
}

void DataDistribution(int* pixels, int* procPixelRows, int rowNum, int width, int height) {
	int* pSendNum = NULL;
	int* pSendInd = NULL;
	int restRows = height;

	pSendInd = new int[ProcNum];
	pSendNum = new int[ProcNum];

	rowNum = (height / ProcNum);
	pSendNum[0] = 3 * width * rowNum;
	pSendInd[0] = 0;

	for (int i = 1; i < ProcNum; i++) {
		restRows -= rowNum;
		rowNum = restRows / (ProcNum - i);
		pSendNum[i] = 3 * width * rowNum;
		pSendInd[i] = pSendInd[i - 1] + pSendNum[i - 1];
	}

	MPI_Scatterv(pixels, pSendNum, pSendInd, MPI_INT, procPixelRows, pSendNum[ProcRank], MPI_INT, 0, MPI_COMM_WORLD);

	delete[] pSendInd;
	delete[] pSendNum;
}

void ParallelColorInversion(int* procPixelRows, int* procPixelResult, int width, int rowNum) {
	for (int i = 0; i < 3 * width * rowNum; i++)
	{
		procPixelResult[i] = 255 - procPixelRows[i];
	}

	
}

void ResultReplication(int* procPixelResult, int* pixelResult, int rowNum, int width, int height) {
	int* pReceiveNum = NULL;
	int* pReceiveInd = NULL;
	int restRows = height;

	pReceiveNum = new int[ProcNum];
	pReceiveInd = new int[ProcNum];

	rowNum = (height / ProcNum);
	pReceiveNum[0] = 3 * width * rowNum;
	pReceiveInd[0] = 0;

	for (int i = 1; i < ProcNum; i++) {
		restRows -= rowNum;
		rowNum = restRows / (ProcNum - i);
		pReceiveNum[i] = 3 * width * rowNum;
		pReceiveInd[i] = pReceiveInd[i - 1] + pReceiveNum[i - 1];
	}

	MPI_Gatherv(procPixelResult, pReceiveNum[ProcRank], MPI_INT, pixelResult, pReceiveNum, pReceiveInd, MPI_INT, 0, MPI_COMM_WORLD);

	delete[] pReceiveInd;
	delete[] pReceiveNum;
}

void ProcessTerminator(int argc, char** argv, int* pixels, int* inversed, int* procPixelRows, int* procPixelResult, int width, int height) {
	if (ProcRank == 0) {
		if (argc == 3) {

			ScopedPixels scopedPixel(info_header.biHeight, info_header.biWidth);

			for (int i = 0; i < height; i++) {
				for (int j = 0; j < width; j++) {
					scopedPixel[i][j].blue = inversed[3 * i * width + 3 * j];
					scopedPixel[i][j].green = inversed[3 * i * width + 3 * j + 1];
					scopedPixel[i][j].red = inversed[3 * i * width + 3 * j + 2];
				}
			}

			RGBPixels2Bitmap(scopedPixel, info_header, p_bitmap.get());
			WriteBitmapFile(argv[2], &info_header, &file_header, p_bitmap.get());

		}
		delete[] pixels;
		delete[] inversed;
	}

	delete[] procPixelResult;
	delete[] procPixelRows;
}

void TestInitialization(int* pixels, int width, int height) {
	if (ProcRank == 0) {
		cout << "Width = " << width << " Height = " << height << endl;
		cout << sizeof(pixels) << " " << pixels << endl;
		if (pixels == NULL)cout << "ERROR: pixels == NULL" << endl;
		else cout << "pixels - ok" << endl;
	}
}

int main(int argc, char** argv)
{
	int* pixels = NULL;
	int* inversed = NULL;
	int* procPixelRows = NULL;
	int* procPixelResult = NULL;

	int width = 0;
	int height = 0;
	int rowNum;
	double start, end;
	start = clock();
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);
	MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);

	

	Initialization(argc, argv, pixels, inversed, width, height, procPixelRows, procPixelResult, rowNum);

	DataDistribution(pixels, procPixelRows, rowNum, width, height);

	ParallelColorInversion(procPixelRows, procPixelResult, width, rowNum);

	ResultReplication(procPixelResult, inversed, rowNum, width, height);

	ProcessTerminator(argc, argv, pixels, inversed, procPixelRows, procPixelResult, width, height);

	end = clock();

	if (ProcRank == 0) {
		auto processTime = (end - start) / CLOCKS_PER_SEC;
		cout << "Processing time - " << processTime << endl;
	}

	MPI_Finalize();

	return 0;
}

