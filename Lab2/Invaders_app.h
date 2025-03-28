#pragma once
#include <windows.h>
#include <commdlg.h>
#include "board.h"
#include "Enemy.h"
#include <string>
#include <list>
#include <dwmapi.h>
#include <stdexcept>
#include <iostream>
#include "resource.h"
#include <commdlg.h>
#include <stdio.h>
#include <algorithm>
#include "resource.h"
class Invaders_app
{
private:
	bool register_class();
	static std::wstring const s_class_name;
	static LRESULT CALLBACK window_proc_static(
		HWND window, UINT message,
		WPARAM wparam, LPARAM lparam);
	LRESULT window_proc(
		HWND window, UINT message,
		WPARAM wparam, LPARAM lparam);
	HWND create_window(DWORD style, DWORD ex_style = 0);
	HINSTANCE m_instance;
	DWORD main_style;
	DWORD ex_style; 

	HWND m_main, m_player;// , m_enemy;
	std::vector<Enemy> enemies;

	board m_board;
	POINT m_screen_size;
	HBRUSH m_enemy_brush;
	HBRUSH m_player_brush;
	HBRUSH m_bullet_brush;

	HBITMAP player_bitmap;
	HBITMAP enemy_bitmap;

	bool backgroundIsBitmap = false;
	HBITMAP background_bitmap;

	HDC memDC; 

	enum bgMode
	{
		fill,
		tile,
		center,
		fit
	};
	bgMode bg_mode = center; 

	enum wSize
	{
		Small,
		Medium,
		Large
	};
	wSize window_size = Medium; 

	const int wHeight[3] = {420,600,780};
	const int wWidth[3] = {560,800,1040};

	const int enemyGridHeight[3] = { 1,3,5};
	const int enemyGridWidth[3] = { 5,7,11 };
	const int space = 10;  // Space between enemies

	const int enemyWidth = 50;   // Width of each enemy window
	const int enemyHeight = 40;  // Height of each enemy window

	//movement 
	int m_enemy_direction; // 1 for right, -1 for left
	int m_enemy_start_x;
	void OnArrows(WPARAM wparam);
	void OnSpace();
	std::vector<HWND> m_bullets; // List to store bullet  
	void startNewGame(HWND window);

	bool onArr = false;
	WPARAM arrWparam;

	bool playerMoove = false;
	int moveAmount = 10;
	//timer
	static constexpr UINT_PTR TIMER_ID = 1;
	static constexpr UINT TIMER_INTERVAL = 50;

	void on_timer();

	// painting
	void update_transparency(int a);
	void playerSprite(HDC hdc);
	void enemySPrite(HDC hdc);
	void DisplayScore(HWND hwnd, HDC memDC);
	int enemyBitmapOffsetIterator = 0;
	int playerBitmapOffsetIterator = 0;

	int currentBackgroundOption;
	std::vector<int> backgroundOptions = { ID_BACKGROUND_SOLID, ID_BACKGROUND_IMAGE, ID_BACKGROUND_TILE, ID_BACKGROUND_CENTER, ID_BACKGROUND_FILL,ID_BACKGROUND_FIT };
	int currentSizeOption;
	std::vector<int> sizeOptions = { ID_OPTIONS_SMALL, ID_OPTIONS_MEDIUM, ID_OPTIONS_LARGE };

	void UpdateMenuCheck(HMENU hMenu, int checkedItemID, const std::vector<int>& MenuItems);
	void ApplyBackgroundBitmap(HWND window, HDC hdc);

	COLORREF bgColor = RGB(255, 255, 255);


	// logic 
	bool oldGame = false;


	void loadConfig();
	void updateConfig(); 
	wchar_t bitmapFilePath[MAX_PATH];

	void sortScores();

	std::wstring fullPath;
public:
	Invaders_app(HINSTANCE instance);
	int run(int show_command);

	int gameScore = 0;
	int lastPlayerScore; 
	WCHAR lastPlayerName[100];
	int scores[3];  
	wchar_t userNames[3][100]; 
};