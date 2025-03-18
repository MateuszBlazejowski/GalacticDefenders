#include <windows.h>
#include "Invaders_app.h"


int WINAPI wWinMain(HINSTANCE instance,
	HINSTANCE,
	LPWSTR,
	int show_command)
{
	Invaders_app app{ instance };
	return app.run(show_command);
}
