#pragma once

#include <windows.h>
#include <vector>
#include <iostream>


struct Enemy {
    HWND hwnd;  // Window handle for the enemy
    int startX, startY;   // starting position of the enemy (in pixels)
};
