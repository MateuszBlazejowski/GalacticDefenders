#pragma once
#include <windows.h>
#include "board.h"
#include <string>
#include <list>
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

	HWND m_main, m_player;// , m_enemy;
	std::vector<HWND> enemies; 

	board m_board;
	POINT m_screen_size;
	HBRUSH m_enemy_brush;
	HBRUSH m_player_brush;
	HBRUSH m_bullet_brush;

	HBITMAP player_bitmap;
	HBITMAP enemy_bitmap;

	//movement 
	int m_enemy_direction; // 1 for right, -1 for left
	int m_enemy_start_x;
	void OnArrows(WPARAM wparam);
	void OnSpace();

	std::list<HWND> m_bullets; // List to store bullet  

	//timer
	static constexpr UINT_PTR TIMER_ID = 1;
	static constexpr UINT TIMER_INTERVAL = 50;
	void on_timer();

	// painting
	void update_transparency(int a);
	void playerSprite(HDC hdc);
	void enemySPrite(HDC hdc, HWND m_enemy);
	int enemyBitmapOffsetIterator = 0;
	int playerBitmapOffsetIterator = 0;

public:
	Invaders_app(HINSTANCE instance);
	int run(int show_command);
};