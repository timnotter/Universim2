#include <X11/Xlib.h>

int main() {
  // Open the X11 display and create a window
  Display *display = XOpenDisplay(nullptr);
  int screen_num = DefaultScreen(display);
  Window window = XCreateSimpleWindow(display, RootWindow(display, screen_num),
                                      0, 0, 640, 480, 0, 0, 0);

  // Set the window properties and map it to the screen
  XStoreName(display, window, "My Window");
  XSelectInput(display, window, ExposureMask | KeyPressMask);
  XMapWindow(display, window);

  // Create a graphics context and draw the line
  GC gc = XCreateGC(display, window, 0, nullptr);
  XSetForeground(display, gc, BlackPixel(display, screen_num));

  int win_x, win_y;
  Window child;
  XTranslateCoordinates(display, window, RootWindow(display, screen_num),
                        -50000, -50000, &win_x, &win_y, &child);
  XDrawLine(display, window, gc, win_x, win_y, 100, 100);
  XFlush(display);

  // Event loop
  XEvent event;
  while (true) {
    XNextEvent(display, &event);
    if (event.type == KeyPress) {
      break;
    }
  }

  // Clean up and exit
  XFreeGC(display, gc);
  XDestroyWindow(display, window);
  XCloseDisplay(display);
  return 0;
}
