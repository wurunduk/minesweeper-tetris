#include "MemoryManager.h"
#include <iostream>
#include <Windows.h>
#include "Vector2.h"

/*
side symbol is '\x10'
free check is '@'
normal check is '\xf'
bomb is 'è'
1 is 'A' = blue
2 is 'B' = green
3 is 'C' = red
4 is 'D' = dark blue
5 E dark red
6 F cyan
7 G black
8 H grey
I question mark
J bomb not exploded
K exploded bomb
L the fuck bomb?
M question on standart field
N flag on field
O standart field
etc??
*/

MemoryManager memoryManager;

const DWORD m_dwFieldSizeXOfsset = 0x5334;		//there is maximum of 30 playable fields + 2 for sides
const DWORD m_dwFieldSizeYOffset = 0x5338;		//maximum is 24 and + 2 fields too

const DWORD m_dwTime = 0x579C;		//currentTime

const DWORD m_dwClickPositionX = 0x5118;		
const DWORD m_dwClickPositionY = 0x511C;		

const DWORD m_dwClick = 0x5140;		

const DWORD m_dwFieldOffset = 0x5340;

const Vector2 maxFieldSize(32, 26);

//size of one box in pixels
const int checkerSize = 16;

const Vector2 windowPixelOffset(8, 90);

unsigned long int cycle = 0;

HWND consoleWindow, sweeperWindow;
RECT windowRect;

int score = 0;

bool fastMode = false;

bool running = false;

//left clicks mouse button
void LeftClick()
{
	INPUT    Input = { };

	Input.type = INPUT_MOUSE;
	Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

	::SendInput(1, &Input, sizeof(INPUT));


	::ZeroMemory(&Input, sizeof(INPUT));

	Input.type = INPUT_MOUSE;
	Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;

	::SendInput(1, &Input, sizeof(INPUT));
}

//presses left and right mouse buttons
void Update()
{
	INPUT Input[2] = { };

	Input[0].type = INPUT_MOUSE;
	Input[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;

	Input[1].type = INPUT_MOUSE;
	Input[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

	::SendInput(2, Input, sizeof(INPUT));
		
	::ZeroMemory(&Input, sizeof(INPUT));

	Input[1].type = INPUT_MOUSE;
	Input[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;

	Input[0].type = INPUT_MOUSE;
	Input[0].mi.dwFlags = MOUSEEVENTF_LEFTUP;

	::SendInput(2, Input, sizeof(INPUT));
}

//Function to check if the mouse pointer is inside the minesweeper's field to avoid clicking on the desktop/other apps to avoid total destruction
void SetCursorPosSafe(const int x, const int y)
{
	if(!(x < windowRect.left || x > windowRect.right || y < windowRect.bottom || y > windowRect.top))
		SetCursorPos(x, y);
}

//Object object represents one of tetris'es main shapes
//All of them can be put in a 3x3 square so we use 9 char length array
struct Object
{
	char objectField[9];

	//rotates the object around the center(Yes, it's not how the original tetris rotation works)
	void rotate()
	{
		char oldObjectField[9];
		oldObjectField[0] = objectField[0];
		oldObjectField[1] = objectField[1];
		oldObjectField[2] = objectField[2];
		oldObjectField[3] = objectField[3];
		oldObjectField[4] = objectField[4];
		oldObjectField[5] = objectField[5];
		oldObjectField[6] = objectField[6];
		oldObjectField[7] = objectField[7];
		oldObjectField[8] = objectField[8];

		objectField[0] = oldObjectField[6];
		objectField[1] = oldObjectField[3];
		objectField[2] = oldObjectField[0];
		objectField[3] = oldObjectField[7];
		objectField[4] = oldObjectField[4];
		objectField[5] = oldObjectField[1];
		objectField[6] = oldObjectField[8];
		objectField[7] = oldObjectField[5];
		objectField[8] = oldObjectField[2];

	}

	//fills the object's data corresponding to one of the preset patterns
	void SetObject(const int type)
	{
		//o represents a filled cell, @ - empty
		switch (type)
		{
		case 0:
			objectField[0] = 'O';
			objectField[1] = 'O';
			objectField[2] = '@';
			objectField[3] = 'O';
			objectField[4] = 'O';
			objectField[5] = '@';
			objectField[6] = '@';
			objectField[7] = '@';
			objectField[8] = '@';
			break;
		case 1:
			objectField[0] = 'O';
			objectField[1] = 'O';
			objectField[2] = 'O';
			objectField[3] = 'O';
			objectField[4] = '@';
			objectField[5] = '@';
			objectField[6] = '@';
			objectField[7] = '@';
			objectField[8] = '@';
			break;
		case 2:
			objectField[0] = 'O';
			objectField[1] = 'O';
			objectField[2] = 'O';
			objectField[3] = '@';
			objectField[4] = 'O';
			objectField[5] = '@';
			objectField[6] = '@';
			objectField[7] = '@';
			objectField[8] = '@';
			break;
		case 3:
			objectField[0] = 'O';
			objectField[1] = 'O';
			objectField[2] = '@';
			objectField[3] = '@';
			objectField[4] = 'O';
			objectField[5] = 'O';
			objectField[6] = '@';
			objectField[7] = '@';
			objectField[8] = '@';
			break;
		case 4:
			objectField[0] = '@';
			objectField[1] = 'O';
			objectField[2] = 'O';
			objectField[3] = 'O';
			objectField[4] = 'O';
			objectField[5] = '@';
			objectField[6] = '@';
			objectField[7] = '@';
			objectField[8] = '@';
			break;
		case 5:
			objectField[0] = 'O';
			objectField[1] = 'O';
			objectField[2] = 'O';
			objectField[3] = '@';
			objectField[4] = '@';
			objectField[5] = '@';
			objectField[6] = '@';
			objectField[7] = '@';
			objectField[8] = '@';
			break;
		default:
			break;
		}
		
	}
};

//filed object represents the whole game field
//maximum possible size of the game is (30+2)*(24+2) = 832
//+2 are comming because game's borders are also stored in this array
struct Field
{
	char field[832];

	char* operator[](const int i)
	{
		return &field[i];
	}
};


struct FieldInfo
{
	//we are putting a field object and not a char array here so we could read memory easier(i was just lazy to fix a bug)
	Field field;
	//instance of the current object falling
	Object currentObject;
	Vector2 objectPosition;


	void Move(int right = 1)
	{
		if(right != 1) right = -1;

		const Vector2 newPos(objectPosition.x + right, objectPosition.y);
		for(auto i = 0; i < 9; i++)
		{
			if(currentObject.objectField[i] != '@')
			{
				if(*GetObjectCollision(newPos, i) != '@')
				{
					return;
				}
			}	
		}

		POINT oldCursorPosition;
		GetCursorPos(&oldCursorPosition);

		SetCursorPos(windowRect.left + windowPixelOffset.x + objectPosition.x*checkerSize, windowRect.top + windowPixelOffset.y + objectPosition.y*checkerSize);
		Update();	
			
		objectPosition = newPos;

		SetCursorPos(windowRect.left + windowPixelOffset.x + objectPosition.x*checkerSize, windowRect.top + windowPixelOffset.y + objectPosition.y*checkerSize);
		Update();

		SetCursorPos(oldCursorPosition.x, oldCursorPosition.y);
	}

	char* GetObjectCollision(const Vector2 pos, const int i)
	{
		return field[TwoDimToLinear(pos.x - 1 + i%3, pos.y - 1 + i/3)];
	}

	char* GetObjectCollision(const int i)
	{
		return field[TwoDimToLinear(objectPosition.x - 1 + i%3, objectPosition.y - 1 + i/3)];
	}

	char GetSymbol(const int x, const int y)
	{
		const auto ch = field.field[TwoDimToLinear(x, y)];
		switch (ch)
		{
		case '\x10':
			 return '+';
		default:
			return ch;
		}
	}

	void SetSymbol(const int x, const int y, const char ch)
	{
		field.field[TwoDimToLinear(x, y)] = ch;
	}

	static int TwoDimToLinear(const int x, const int y)
	{
		return y*(maxFieldSize.x) + x;
	}
};

FieldInfo currentField;

DWORD WINAPI Run(LPVOID params)
{
	do
	{
		sweeperWindow = FindWindow(nullptr, "Minesweeper");
		static auto loading = true;
		if(loading)
		{
			loading = false;
			std::cout << "Waiting for minesweeper \n";
		}
	}
	while(sweeperWindow == nullptr);

	memoryManager.AttachToProcess("winmine.exe");
	GetWindowRect(sweeperWindow, &windowRect);

	const auto baseAddress = memoryManager.GetModuleBaseAddress("winmine.exe");

	const auto fieldSizeX = memoryManager.Read<int>(baseAddress + m_dwFieldSizeXOfsset);
	const auto fieldSizeY = memoryManager.Read<int>(baseAddress + m_dwFieldSizeYOffset);
	std::cout << "field size x : y " << fieldSizeX << " : " << fieldSizeY << std::endl;

	currentField.currentObject.SetObject(5);

	currentField.objectPosition.Set(fieldSizeX/2, 2);

	currentField.field = memoryManager.Read<Field>(baseAddress + m_dwFieldOffset);

	auto newObjectId = 0;
	
	SetActiveWindow(sweeperWindow);
	SetWindowPos(sweeperWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	for (auto y = 1; y <= fieldSizeY; y++)
	{
		for (auto x = 1; x <= fieldSizeX; x++)
		{
			currentField.SetSymbol(x, y, '@');
		}
	}

	memoryManager.Write<Field>(baseAddress + m_dwFieldOffset, currentField.field);

	for (auto y = 1; y <= fieldSizeY; y++)
	{
		for (auto x = 1; x <= fieldSizeX; x++)
		{
			if ((x - 2) % 3 == 0 && (y - 2) % 3 == 0)
			{
				SetCursorPos(windowRect.left + windowPixelOffset.x + x*checkerSize, windowRect.top + windowPixelOffset.y + y*checkerSize);
				Update();
			}
		}
	}

	while(running)
	{
		memoryManager.Write<int>(baseAddress + m_dwTime, score);

		GetWindowRect(sweeperWindow, &windowRect);

		if(fastMode)
			Sleep(1);
		else
			Sleep(300);

		char backup[9];

		for(auto i = 0; i < 9; i++)
		{
			backup[i] = *currentField.GetObjectCollision(i);
			if(currentField.currentObject.objectField[i] != '@')
				*currentField.GetObjectCollision(i) = currentField.currentObject.objectField[i];
		}

		memoryManager.Write<Field>(baseAddress + m_dwFieldOffset, currentField.field);

		for(auto i = 0; i < 9; i++)
		{
			*currentField.GetObjectCollision(i) = backup[i];
		}

		POINT oldCursorPosition;
		GetCursorPos(&oldCursorPosition);

		for (auto y = 1; y <= fieldSizeY; y++)
		{
			for (auto x = 1; x <= fieldSizeX; x++)
			{
				if ((x - 2) % 3 == 0 && (y - 2) % 3 == 0)
				{
					SetCursorPos(windowRect.left + windowPixelOffset.x + x*checkerSize, windowRect.top + windowPixelOffset.y + y*checkerSize);
					Update();
				}

				if(fieldSizeX%3==1 && x == fieldSizeX)
				{
					SetCursorPos(windowRect.left + windowPixelOffset.x + x*checkerSize, windowRect.top + windowPixelOffset.y + y*checkerSize);
					Update();
				}

				if(fieldSizeY%3==1 && y == fieldSizeY)
				{
					SetCursorPos(windowRect.left + windowPixelOffset.x + x*checkerSize, windowRect.top + windowPixelOffset.y + y*checkerSize);
					Update();
				}

			}
		}

		SetCursorPos(oldCursorPosition.x, oldCursorPosition.y);

		currentField.objectPosition.y += 1;
		
		for (auto i = 0; i < 9; i++)
		{
			if (currentField.currentObject.objectField[i] != '@')
			{
				if (*currentField.GetObjectCollision(i) != '@')
				{
					currentField.objectPosition.y -= 1;

					if(currentField.objectPosition.y == 2)
					{
						std::cout << "YOU LOST. YOUR SCORE: " << score << "\n";
						std::cout << "Press INSERT to start a new game";
						running = false;
						score = 0;
						break;
						
					}

					for(auto n = 0; n < 9; n++)
					{
						if(currentField.currentObject.objectField[n] != '@')
							*currentField.GetObjectCollision(n) = currentField.currentObject.objectField[n];
					}


					for (auto y = 1; y <= fieldSizeY; y++)
					{
						for (auto x = 1; x <= fieldSizeX; x++)
						{
							if (*currentField.field[FieldInfo::TwoDimToLinear(x, y)] == '@')
							{
								break;
							}

							if(x == fieldSizeX)
							{
								score += 1;
								for(auto y2 = y; y2 > 0; y2--)
								{
									for(auto x3 = 1; x3 <= fieldSizeX; x3++)
									{
										if(y2 == 1)
											*currentField.field[FieldInfo::TwoDimToLinear(x3, y2)] = '@';
										else
											*currentField.field[FieldInfo::TwoDimToLinear(x3, y2)] = *currentField.field[FieldInfo::TwoDimToLinear(x3, y2-1)];

										if ((x + 1) % 3 == 0 && (y + 1) % 3 == 0)
										{
											SetCursorPos(windowRect.left + windowPixelOffset.x + x*checkerSize, windowRect.top + windowPixelOffset.y + y*checkerSize);
											Update();
										}
									}
								}
							}
						}
					}

					currentField.objectPosition.Set(fieldSizeX/2, 2);
					newObjectId += 1;
					newObjectId %= 6;
					currentField.currentObject.SetObject(newObjectId);

					break;
				}
			}
		}
	}

	running = false;
	
	return 0;
}

int main()
{
	std::cout << "INSERT to start, CTRL to stop , DELETE to exit\n";
	consoleWindow = GetConsoleWindow();
	while (true)
	{
		const auto temp = GetForegroundWindow();
		if(temp == consoleWindow || (sweeperWindow != nullptr && temp == sweeperWindow))
		{
			if (GetAsyncKeyState(VK_INSERT) & 1 && !running)
			{
				std::cout << "Started game\n";
				running = true;
				CreateThread(nullptr, 0, &Run, nullptr, 0, nullptr);
			}
			else if (GetAsyncKeyState(VK_CONTROL) & 1)
			{
				running = false;
				std::cout << "Stopped game\n";
			}

			if (GetAsyncKeyState(VK_LEFT) & 1)
			{
				currentField.Move(-1);
				Sleep(100);
			}
			else if (GetAsyncKeyState(VK_RIGHT) & 1)
			{
				currentField.Move(1);
				Sleep(100);
			}

			if (GetAsyncKeyState(VK_UP) & 1 || GetAsyncKeyState(VK_SPACE) & 1)
			{
				currentField.currentObject.rotate();
				Sleep(100);
			}

			fastMode = GetAsyncKeyState(VK_LSHIFT) || GetAsyncKeyState(VK_DOWN);

			if(GetAsyncKeyState(VK_DELETE) & 1 || GetAsyncKeyState(VK_ESCAPE) & 1)
			{
				running = false;
				break;
			}
		}
		Sleep(1);
	}
	
	return 0;
}

