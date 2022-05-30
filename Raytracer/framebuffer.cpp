#include "FrameBuffer.h"

FrameBuffer::FrameBuffer()
{
}

FrameBuffer::FrameBuffer(int width, int height)
{
	Resize(width, height);
}

FrameBuffer::~FrameBuffer()
{
	delete[] colorbuffer;
}

DWORD FrameBuffer::CompressColor(int r, int g, int b)
{
	return (DWORD(((BYTE)(r) | ((WORD)((BYTE)(g)) << 8)) | (((DWORD)(BYTE)(b)) << 16)));
}

void FrameBuffer::SetPixel(int x, int y, DWORD color)
{
	colorbuffer[x + y * width] = color;
}

void FrameBuffer::Resize(uint32_t width, uint32_t height)
{
	if (this->width == width && this->height == height)
		return;

	delete[] colorbuffer;
	this->width = width;
	this->height = height;

	colorbuffer = new DWORD[width * height];
}
