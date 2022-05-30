#pragma once
#include <cstdint>
#include <Windows.h>

class FrameBuffer
{
public:
	FrameBuffer();
	FrameBuffer(int width, int height);
	~FrameBuffer();
	DWORD* colorbuffer;
	static DWORD CompressColor(int r, int g, int b);
	void SetPixel(int x, int y, DWORD color);
	void Resize(uint32_t width, uint32_t height);

	int width;
	int height;
};

