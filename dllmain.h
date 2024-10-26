#pragma once

HWND WaitForMainWindow();

unsigned long MainLoop(LPVOID arg);

void WaitUntilWindowIsClosed(HWND hwnd);

void SetAffinity();
