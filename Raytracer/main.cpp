#include <Windows.h>
#include <iostream>
#include <sstream>

#include <chrono>
#include <thread>

#include "Ray.h"
#include "framebuffer.h"
#include "Shapes.h"

#define VLog(x) MessageBoxA(nullptr, x, "VLog", MB_OK);
#define AlignedRGB(r,g,b)          ((DWORD)(((r)<<16|((g))<<8))|(((b)))) // 32 bits, but packed as 24. Why it works?

Vec3 ray_color(const Ray& r);
void DoSomeRaytracing();

const uint32_t INITIAL_WIDTH = 1600;
const uint32_t INITIAL_HEIGHT = 900;
constexpr auto aspect_ratio = 16.0 / 9.0;

static double offsetX = 0;
static double offsetY = 0;
static double offsetSpeed = 0.03;

FrameBuffer frameBuffer(INITIAL_WIDTH, INITIAL_HEIGHT);

void DrawDIB(HWND hwnd)
{
    PAINTSTRUCT ps;
    RECT r;

    GetClientRect(hwnd, &r);

    int width = r.right - r.left;
    int height = r.bottom - r.top;

    HDC hdc = BeginPaint(hwnd, &ps);

    BITMAPINFOHEADER bitmapinfoheader = {};
    bitmapinfoheader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapinfoheader.biWidth = width;
    bitmapinfoheader.biHeight = height;
    bitmapinfoheader.biPlanes = 1;
    bitmapinfoheader.biBitCount = 32;
    bitmapinfoheader.biCompression = BI_RGB;
    bitmapinfoheader.biSizeImage = 0;
    bitmapinfoheader.biXPelsPerMeter = 0;
    bitmapinfoheader.biYPelsPerMeter = 0;
    bitmapinfoheader.biClrUsed = 0;
    bitmapinfoheader.biClrImportant = 0;

    BITMAPINFO bitmapinfo = {};
    
    bitmapinfo.bmiHeader = bitmapinfoheader;

	auto result = StretchDIBits(hdc, 0, 0, width, height, 0, 0, width, height, frameBuffer.colorbuffer, &bitmapinfo, DIB_RGB_COLORS,  SRCCOPY);

    EndPaint(hwnd, &ps);
}

LRESULT WINAPI AppWindowProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
    switch (Msg)
    {
		case WM_KEYDOWN:
            if (wParam == VK_LEFT)
            {
                offsetX -= offsetSpeed;
            }
            if (wParam == VK_RIGHT)
            {
                offsetX += offsetSpeed;
            }
            if (wParam == VK_UP)
            {
                offsetY -= offsetSpeed;
            }
            if (wParam == VK_DOWN)
            {
                offsetY += offsetSpeed;
            }
            break;
		case WM_SIZE:
            //TODO: Aspect ratio support for window resizing? Predefined resolutions?
            break;
	    case WM_PAINT:
            DrawDIB(hWnd);
	        break;
	    case WM_DESTROY:
	        PostQuitMessage(0);
	        break;
    }
    return DefWindowProc(hWnd, Msg, wParam, lParam);
}


INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
	AllocConsole();
    FILE* fstdin, * fstdout, * fstderr;

    freopen_s(&fstdin, "CONIN$", "r", stdin);
    freopen_s(&fstdout, "CONOUT$", "w", stderr);
    freopen_s(&fstderr, "CONOUT$", "w", stdout);

    const wchar_t CLASS_NAME[] = L"Raytracer";

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = AppWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);

    RegisterClassEx(&wc);

    DWORD windowStyles = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE;

    RECT adjustedRect;
    adjustedRect.left = 0;
    adjustedRect.right = INITIAL_WIDTH;
    adjustedRect.top = 0;
    adjustedRect.bottom = INITIAL_HEIGHT;
    AdjustWindowRectEx(&adjustedRect, windowStyles, NULL, NULL);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Raytracer",
        windowStyles,

        0, 0, adjustedRect.right - adjustedRect.left, adjustedRect.bottom - adjustedRect.top,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (hwnd == NULL)
    {
        VLog("Window null");
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

	MSG msg = { };
    std::ostringstream oss;
    uint64_t framesCount = 0;

	while (GetMessage(&msg, hwnd, 0, 0) > 0)
    {
        auto start = std::chrono::high_resolution_clock::now();

        framesCount++;

        TranslateMessage(&msg);
        DispatchMessage(&msg);

        std::cout << framesCount;

        UpdateWindow(hwnd); // Request to redraw

        DoSomeRaytracing(); //MAIN RAYTRACE FUNCTION

        auto finish = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start);
        oss << "Raytracer:" << ms.count() << " ms" <<  " (" << framesCount << ")";
        auto astring = oss.str();
        std::wstring widestr = std::wstring(astring.begin(), astring.end());
        SetWindowText(hwnd, widestr.c_str());
        oss.str(std::string());

        //std::this_thread::sleep_for(std::chrono::milliseconds(33));

        RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE);
    }

    return 0;
}

//---------------------------------------------------------------------------------------
bool hit_sphere(const Vec3& center, double radius, const Ray& r) {
    Vec3 centerToRayOrigin = r.origin() - center;

    auto a = Vec3::dot(r.direction(), r.direction());
    auto b = 2.0 * Vec3::dot(centerToRayOrigin, r.direction());
    auto c = Vec3::dot(centerToRayOrigin, centerToRayOrigin) - radius * radius;
    auto discriminant = b * b - 4 * a * c;
    return (discriminant > 0);
}

bool hit_sphere(const Sphere& sphere, const Ray& r) {
    Vec3 centerToRayOrigin = r.origin() - sphere.center;

    auto a = Vec3::dot(r.direction(), r.direction());
    auto b = 2.0 * Vec3::dot(centerToRayOrigin, r.direction());
    auto c = Vec3::dot(centerToRayOrigin, centerToRayOrigin) - sphere.radius * sphere.radius;
    auto discriminant = b * b - 4 * a * c;
    return (discriminant > 0);
}

double hit_sphere_with_point(const Sphere& sphere, const Ray& r)
{
    Vec3 centerToRayOrigin = r.origin() - sphere.center;

    auto a = Vec3::dot(r.direction(), r.direction());
    auto b = 2.0 * Vec3::dot(centerToRayOrigin, r.direction());
    auto c = Vec3::dot(centerToRayOrigin, centerToRayOrigin) - sphere.radius * sphere.radius;
    auto discriminant = b * b - 4 * a * c;

	if (discriminant < 0) {
        return -1.0;
    }
    else {
        return (-b + std::sqrt(discriminant)) / (2.0 * a);
    }
}

Vec3 ray_color(const Ray& r)
{
    Sphere sphere(Vec3(offsetX, offsetY, -0.9), 0.5);
    Sphere sphere1(Vec3(0.5, 0, -1), 0.5);
    Sphere sphere2(Vec3(-0.5, 0, -1), 0.5);



    //if (hit_sphere(sphere, r))
    //    return Vec3(1, 0, 0);
    if (hit_sphere(sphere1, r))
        return Vec3(0, 1, 0);
    if (hit_sphere(sphere2, r))
        return Vec3(0, 0, 1);

    double solution = hit_sphere_with_point(sphere, r);
    if (solution > 0.0)
    {
        Vec3 N = unit_vector(r.at(solution) - Vec3(0, 0, -1));
        return 0.5 * Vec3(N.x() + 1, N.y() + 1, N.z() + 1);
    }

    Vec3 unit_direction = unit_vector(r.direction());
    auto t = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - t) * Vec3(1.0, 1.0, 1.0) + t * Vec3(0.5, 0.7, 1.0);
}

void DoSomeRaytracing()
{
    const int image_width = INITIAL_WIDTH;
    const int image_height = static_cast<int>(image_width / aspect_ratio);

    // Camera
    auto viewport_height = 2.0;
    auto viewport_width = aspect_ratio * viewport_height;
    auto focal_length = 1;

    auto origin = Vec3(0, 0, 0);
    auto horizontal = Vec3(viewport_width, 0, 0);
    auto vertical = Vec3(0, viewport_height, 0);
    auto lower_left_corner = origin - horizontal / 2 - vertical / 2 - Vec3(0, 0, focal_length);

    // Render
    Vec3 forwardDirection(0, 0, 1);

    for (int j = 0; j < image_height; ++j) {
        for (int i = 0; i < image_width; ++i) {
            auto u = double(i) / (image_width - 1);
            auto v = double(j) / (image_height - 1);
            //Ray r(origin, lower_left_corner + u * horizontal + v * vertical - origin);
            Ray r(lower_left_corner + u * horizontal + v * vertical, forwardDirection);
        	Vec3 pixel_color = ray_color(r);
            
            frameBuffer.SetPixel(i, j, AlignedRGB((int)(255.999 * pixel_color.x()), (int)(255.999 * pixel_color.y()), (int)(255.999 * pixel_color.z())));
        }
    }
}