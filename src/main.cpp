#include <windows.h>
#include "SevenZipHelper.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    std::wstring z7FM = SevenZipHelper::Get7zFMPath();
    std::wstring args = pCmdLine ? pCmdLine : L"";
    SevenZipHelper::LaunchProcess(z7FM, args);
    return 0;
}
