#include <iostream>
#include "graphicInterface/window.hpp"
#include "graphicInterface/renderer.hpp"
#include "graphicInterface/point2d.hpp"
#include "helpers/constants.hpp"
#ifdef _WIN32
// #include <Windows.h>
// #include <gdiplus.h>
using namespace Gdiplus;

MyWindow::MyWindow() : windowWidth(SCREEN_WIDTH), windowHeight(SCREEN_HEIGHT), hdc(nullptr), hBitmap(nullptr), graphics(nullptr), hwnd(nullptr) {
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"MyWindowClass";
    RegisterClass(&wc);

    hwnd = CreateWindowEx(0, L"MyWindowClass", L"My Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight, nullptr, nullptr, GetModuleHandle(nullptr), this);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    hdc = GetDC(hwnd);

    hBitmap = CreateCompatibleBitmap(hdc, windowWidth, windowHeight);
    memDC = CreateCompatibleDC(hdc);
    SelectObject(memDC, hBitmap);

    graphics = Graphics::FromHDC(memDC);
    graphics->Clear(Color::Black);
}

MyWindow::~MyWindow() {
    delete graphics;
    DeleteDC(memDC);
    DeleteObject(hBitmap);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
    GdiplusShutdown(gdiplusToken);
}

void MyWindow::drawBackground(int colour) {
    graphics->Clear(Color(colour));
}

void MyWindow::endDrawing() {
    BitBlt(hdc, 0, 0, windowWidth, windowHeight, memDC, 0, 0, SRCCOPY);
}

int MyWindow::drawPoint(unsigned int col, int x, int y) {
    SolidBrush brush(Color(col));
    graphics->FillRectangle(&brush, x, y, 1, 1);
    return 0;
}

int MyWindow::drawLine(unsigned int col, int x1, int y1, int x2, int y2) {
    Pen pen(Color(col));
    graphics->DrawLine(&pen, x1, y1, x2, y2);
    return 0;
}

int MyWindow::drawRect(unsigned int col, int x, int y, int width, int height) {
    SolidBrush brush(Color(col));
    graphics->FillRectangle(&brush, x, y, width, height);
    return 0;
}

int MyWindow::drawCircle(unsigned int col, int x, int y, int diam) {
    Pen pen(Color(col));
    graphics->DrawEllipse(&pen, x - diam / 2, y - diam / 2, diam, diam);
    SolidBrush brush(Color(col));
    graphics->FillEllipse(&brush, x - diam / 2, y - diam / 2, diam, diam);
    return 0;
}

int MyWindow::drawString(unsigned int col, int x, int y, const char* stringToBe) {
    FontFamily fontFamily(L"Arial");
    Font font(&fontFamily, 12, FontStyleRegular, UnitPixel);
    SolidBrush brush(Color(col));
    graphics->DrawString(std::wstring(stringToBe, stringToBe + strlen(stringToBe)).c_str(), -1, &font, PointF(static_cast<REAL>(x), static_cast<REAL>(y)), &brush);
    return 0;
}

int MyWindow::drawTriangle(unsigned int col, int x1, int y1, int x2, int y2, int x3, int y3) {
    Point points[] = { Point(x1, y1), Point(x2, y2), Point(x3, y3) };
    SolidBrush brush(Color(col));
    graphics->FillPolygon(&brush, points, 3);
    return 0;
}

int MyWindow::drawPolygon(unsigned int col, short count, Point2d* points) {
    std::vector<Point> gdiPoints(count);
    for (int i = 0; i < count; ++i) {
        gdiPoints[i] = Point(points[i].getX(), points[i].getY());
    }
    SolidBrush brush(Color(col));
    graphics->FillPolygon(&brush, gdiPoints.data(), count);
    return 0;
}

int MyWindow::getWindowWidth() {
    return windowWidth;
}

int MyWindow::getWindowHeight() {
    return windowHeight;
}

int MyWindow::getNumberOfPendingEvents() {
    MSG msg;
    return PeekMessage(&msg, hwnd, 0, 0, PM_NOREMOVE);
}

int MyWindow::translateWindowsEventType(int windowsEventType) {
    switch (windowsEventType) {
        case WM_PAINT: return Expose;
        case WM_KEYDOWN: return KeyPress;
        // Add translations for other Windows event types as needed
        default: return windowsEventType; // Return the original event type if no translation is found
    }
}

unsigned int MyWindow::translateWindowsKeycode(unsigned int windowsKeycode) {
    switch (windowsKeycode) {
        case VK_ESCAPE: return KEY_ESCAPE;
        case VK_SPACE: return KEY_SPACE;
        case VK_Q: return KEY_Q;
        case VK_W: return KEY_W;
        case VK_E: return KEY_E;
        case VK_R: return KEY_R;
        case VK_S: return KEY_S;
        case VK_D: return KEY_D;
        case VK_F: return KEY_F;
        case VK_J: return KEY_J;
        case VK_K: return KEY_K;
        case VK_L: return KEY_L;
        case VK_OEM_4: return KEY_OE;
        case VK_OEM_6: return KEY_AE;
        case VK_LEFT: return KEY_LEFT;
        case VK_UP: return KEY_UP;
        case VK_RIGHT: return KEY_RIGHT;
        case VK_DOWN: return KEY_DOWN;
        case VK_PRIOR: return KEY_PG_UP;
        case VK_NEXT: return KEY_PG_DOWN;
        case VK_1: return KEY_1;
        case VK_2: return KEY_2;
        case VK_3: return KEY_3;
        case VK_4: return KEY_4;
        case VK_5: return KEY_5;
        case VK_6: return KEY_6;
        case VK_9: return KEY_9;
        default: return windowsKeycode; // If no translation, return the original keycode
    }
}

void MyWindow::getPendingEvents(int* eventTypes, u_int* parameters, int numberOfEvents) {
    MSG msg;
    for (int i = 0; i < numberOfEvents; ++i) {
        if (PeekMessage(&msg, hwnd, 0, 0, PM_NOREMOVE)) {
            GetMessage(&msg, hwnd, 0, 0);
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            eventTypes[i] = msg.message;
            parameters[i] = msg.wParam;
        } else {
            eventTypes[i] = -1;
            return;
        }
    }
}

LRESULT CALLBACK MyWindow::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        MyWindow* window = reinterpret_cast<MyWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    MyWindow* window = reinterpret_cast<MyWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (window) {
        switch (message) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
        }
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}
#endif

// #ifdef __unix
#ifndef _WIN32
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xdbe.h>

MyWindow::MyWindow(){
    display = XOpenDisplay(NULL);
    if (display==NULL){
        throw std::runtime_error("Unable to open display");
    }
    screen = DefaultScreen(display);
    // Simple window with no alpha value
    window = new Window;
    Window windowPointer = 
    *(static_cast<Window *>(window)) = XCreateSimpleWindow((static_cast<Display *>(display)), RootWindow((static_cast<Display *>(display)), screen), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 1, BlackPixel((static_cast<Display *>(display)), screen), BlackPixel((static_cast<Display *>(display)), screen));

    event = new XEvent;
    gc = new GC;

    int major_version_return, minor_version_return;
    if(!XdbeQueryExtension((static_cast<Display *>(display)), &major_version_return, &minor_version_return)){
        printf("DBE Problem\n");
    }
    

    XMapWindow((static_cast<Display *>(display)), *(static_cast<Window *>(window)));
    backBuffer = new XdbeBackBuffer;
    *(static_cast<XdbeBackBuffer *>(backBuffer)) = XdbeAllocateBackBufferName((static_cast<Display *>(display)),*(static_cast<Window *>(window)), XdbeBackground);
    XSelectInput((static_cast<Display *>(display)),*(static_cast<Window *>(window)), KeyPressMask | ExposureMask);
    *(static_cast<GC *>(gc)) = XCreateGC((static_cast<Display *>(display)),*(static_cast<Window *>(window)), 0, 0);

    rootWindow = new Window;
}

MyWindow::~MyWindow(){
    XFreeGC((static_cast<Display *>(display)), *(static_cast<GC *>(gc)));
    XDestroyWindow((static_cast<Display *>(display)),*(static_cast<Window *>(window)));
    XCloseDisplay((static_cast<Display *>(display)));
    delete (static_cast<Window *>(window));
    delete (static_cast<XEvent *>(event));
    delete (static_cast<GC *>(gc));
    delete (static_cast<XdbeBackBuffer *>(backBuffer));
    delete (static_cast<Window *>(rootWindow));
    // delete (static_cast<Display *>(display));                    // Possible memory leak, but throws error if enabled
    // delete event;
    // delete gc;
    // delete backBuffer;
    // delete rootWindow;
    // delete display;
}

void MyWindow::drawBackground(int colour){
    XGetGeometry((static_cast<Display *>(display)),*(static_cast<Window *>(window)), (static_cast<Window *>(rootWindow)), &tempX, &tempY, &windowWidth, &windowHeight, &borderWidth, &depth);
    XSetWindowBackground((static_cast<Display *>(display)),*(static_cast<Window *>(window)), 0x111111);
}

void MyWindow::endDrawing(){
    XdbeSwapInfo swap_info = {*(static_cast<Window *>(window)), 1};
    XdbeSwapBuffers((static_cast<Display *>(display)), &swap_info, 1);
}

int MyWindow::drawPoint(unsigned int col, int x, int y){
    XSetForeground((static_cast<Display *>(display)), *(static_cast<GC *>(gc)), col);
    XDrawPoint((static_cast<Display *>(display)), *(static_cast<XdbeBackBuffer *>(backBuffer)), *(static_cast<GC *>(gc)), x, y);
    return 0;
}
int MyWindow::drawLine(unsigned int col, int x1, int y1, int x2, int y2){
    XSetForeground((static_cast<Display *>(display)), *(static_cast<GC *>(gc)), col);
    XDrawLine((static_cast<Display *>(display)), *(static_cast<XdbeBackBuffer *>(backBuffer)), *(static_cast<GC *>(gc)), x1, y1, x2, y2);
    return 0;
}

int MyWindow::drawRect(unsigned int col, int x, int y, int width, int height){
    XSetForeground((static_cast<Display *>(display)), *(static_cast<GC *>(gc)), col);
    XFillRectangle((static_cast<Display *>(display)), *(static_cast<XdbeBackBuffer *>(backBuffer)), *(static_cast<GC *>(gc)), x, y, width, height);
    return 0;
}

int MyWindow::drawCircle(unsigned int col, int x, int y, int diam){
    XSetForeground((static_cast<Display *>(display)), *(static_cast<GC *>(gc)), col);
    XDrawArc((static_cast<Display *>(display)), *(static_cast<XdbeBackBuffer *>(backBuffer)), *(static_cast<GC *>(gc)), x-diam/2, y-diam/2, diam, diam, 0, 360 * 64);
    XFillArc((static_cast<Display *>(display)), *(static_cast<XdbeBackBuffer *>(backBuffer)), *(static_cast<GC *>(gc)), x-diam/2, y-diam/2, diam, diam, 0, 360 * 64);
    return 0;
}

int MyWindow::drawString(unsigned int col, int x, int y, const char *stringToBe){
    XSetForeground((static_cast<Display *>(display)), *(static_cast<GC *>(gc)), col);
    XDrawString((static_cast<Display *>(display)), *(static_cast<XdbeBackBuffer *>(backBuffer)), *(static_cast<GC *>(gc)), x, y, stringToBe, strlen(stringToBe));
    return 0;
}

int MyWindow::drawTriangle(unsigned int col, int x1, int y1, int x2, int y2, int x3, int y3){
    // printf("Drawing triangle with points: (%d, %d), (%d, %d), (%d, %d)\n\n", x1, y1, x2, y2, x3, y3);
    XPoint xPoints[3];
    xPoints[0].x = x1;
    xPoints[0].y = y1;
    xPoints[1].x = x2;
    xPoints[1].y = y2;
    xPoints[2].x = x3;
    xPoints[2].y = y3;
    XSetForeground((static_cast<Display *>(display)), *(static_cast<GC *>(gc)), col);
    XFillPolygon((static_cast<Display *>(display)), *(static_cast<XdbeBackBuffer *>(backBuffer)), *(static_cast<GC *>(gc)), xPoints, 3, Convex, CoordModeOrigin);
    return 0;
}

int MyWindow::drawPolygon(unsigned int col, short count, Point2d *points){
    // if(count < 3) printf("Tried to draw polygin with %d points", count);
    // printf("Drawing polygon with points: ");
    // for(int i=0;i<count;i++){
    //     printf("(%d, %d), ", points[i].getX(), points[i].getY());
    // }
    // printf("\n\n");
    XPoint xPoints[count];
    for(int i=0;i<count;i++){
        xPoints[i].x = points[i].getX();
        xPoints[i].y = points[i].getY();
    }
    XSetForeground((static_cast<Display *>(display)), *(static_cast<GC *>(gc)), col);
    XFillPolygon((static_cast<Display *>(display)), *(static_cast<XdbeBackBuffer *>(backBuffer)), *(static_cast<GC *>(gc)), xPoints, count, Convex, CoordModeOrigin);
    // for(int i=0;i<count;i++){
    //     // printf("Trying to draw line from (%d, %d) to (%d, %d)\n", points[i].x, points[i].y, points[(i+1)%count].x, points[(i+1)%count].y);
    //     drawLine(RED_COL, points[i].getX(), points[i].getY(), points[(i+1)%count].getX(), points[(i+1)%count].getY());
    // }
    return 0;
}

int MyWindow::getNumberOfPendingEvents(){
    return XEventsQueued((static_cast<Display *>(display)), QueuedAlready);
}

void MyWindow::getPendingEvents(int *eventTypes, u_int *parameters, int numberOfEvents){
    if (XEventsQueued((static_cast<Display *>(display)), QueuedAlready) < numberOfEvents){
        eventTypes[0] = -1;
        return;
    }

    for(int i=0;i<numberOfEvents;i++){
        XNextEvent((static_cast<Display *>(display)), static_cast<XEvent *>(event));
        eventTypes[i] = static_cast<XEvent *>(event)->type;
        if (eventTypes[i] == KeyPress){
            parameters[i] = static_cast<XEvent *>(event)->xkey.keycode;
        }
    }
}
#endif

int MyWindow::getWindowWidth(){
    return windowWidth;
}

int MyWindow::getWindowHeight(){
    return windowHeight;
}
