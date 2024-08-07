#include <iostream>
#include <set>
#include <filesystem>

#include <Windows.h>
#include <psapi.h>

static std::set<HWND> found_hwnds;

static BOOL handle_window(HWND, LPARAM);
static BOOL handle_child(HWND, LPARAM);

int main() {
    std::cout << "HWND monitoring started." << std::endl;

    while (true) {
        EnumWindows(handle_window, NULL);
        Sleep(100);
    }
}

BOOL handle_window(HWND hwnd, LPARAM lparam) {
    if (found_hwnds.contains(hwnd))
        return TRUE; // continue iteration; this HWND was already logged previously

    found_hwnds.insert(hwnd);

    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);

    auto print_plain = [&]() {
        std::cout << "0x" << std::hex << hwnd << " (owned by PID " << std::dec << pid << ")" << std::endl;
    };

    [&]() {
        auto hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (hProcess == 0) {
            // couldn't open a handle to the process (are we missing permissions...?)
            print_plain();
            return;
        }

        TCHAR processPathRaw[MAX_PATH];
        DWORD processPathLen = GetProcessImageFileNameW(hProcess, processPathRaw, MAX_PATH);

        if (processPathLen == 0) {
            // no characters copied; this indicates an error
            print_plain();
            return;
        }

        std::filesystem::path processPath(processPathRaw);
        auto processName = processPath.filename().wstring();

        std::wcout << "0x" << std::hex << hwnd << " (owned by " << processName << ", PID " << std::dec << pid << ")" << std::endl;
    }();

    // enumerate child HWNDs as well
    EnumChildWindows(hwnd, handle_child, 1); // last param is the nest level
    return TRUE; // continue enumerating
}

BOOL handle_child(HWND hwnd, LPARAM nestLevel) {
    char className[64];
    if (GetClassNameA(hwnd, className, 64) == 0) {
        strcpy_s(className, "<unknown class>");
    }

    std::cout << std::string(4 * nestLevel, ' ') << "child " << className << ": 0x" << std::hex << hwnd << std::endl;
    EnumChildWindows(hwnd, handle_child, nestLevel + 1);
    return TRUE; // continue enumerating
}
