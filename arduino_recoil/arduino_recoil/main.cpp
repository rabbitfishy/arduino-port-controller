#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#pragma comment(lib, "setupapi.lib")

bool color_bot_key()
{
    return GetAsyncKeyState(VK_XBUTTON1); // VK_XBUTTON1 = mouse 5
}

bool bhop_toggle_key()
{
    static bool previous_state = false;
    static bool toggle = false;

    bool current_state = GetAsyncKeyState(VK_PRIOR); // VK_PRIOR = page up key

    if (current_state && !previous_state)
    {
        toggle = !toggle;
        std::cout << "BHOP: " << (toggle ? "ON" : "OFF") << "\n";
    }

    previous_state = current_state;
    return toggle;
}

bool space_pressed()
{
    return GetAsyncKeyState(VK_SPACE);
}

COLORREF get_pixel_color(int x, int y)
{
    HDC hdc = GetDC(NULL);
    COLORREF color = GetPixel(hdc, x, y);
    ReleaseDC(NULL, hdc);
    return color;
}

bool is_color_different(COLORREF color1, COLORREF color2, int threshold)
{
    int red1 = GetRValue(color1), green1 = GetGValue(color1), blue1 = GetBValue(color1);
    int red2 = GetRValue(color2), green2 = GetGValue(color2), blue2 = GetBValue(color2);
    return (std::abs(red1 - red2) > threshold || std::abs(green1 - green2) > threshold || std::abs(blue1 - blue2) > threshold);
}

std::string find_arduino_port()
{
    HDEVINFO device_info_set = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, nullptr, nullptr, DIGCF_PRESENT);
    if (device_info_set == INVALID_HANDLE_VALUE)
        return std::string();

    SP_DEVINFO_DATA device_info_data;
    device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

    char buffer[256];
    for (DWORD i = 0; SetupDiEnumDeviceInfo(device_info_set, i, &device_info_data); ++i) 
    {
        if (SetupDiGetDeviceRegistryPropertyA(device_info_set, &device_info_data, SPDRP_FRIENDLYNAME, nullptr, (PBYTE)buffer, sizeof(buffer), nullptr)) 
        {
            std::string name = buffer;

            if (name.find("Arduino") != std::string::npos || name.find("USB Serial Device") != std::string::npos)
            {
                size_t begin = name.find("COM");
                size_t end = name.find(")", begin);

                if (begin != std::string::npos && end != std::string::npos) 
                {
                    std::string port = name.substr(begin, end - begin);
                    SetupDiDestroyDeviceInfoList(device_info_set);
                    return "\\\\.\\" + port;
                }
            }
        }
    }

    SetupDiDestroyDeviceInfoList(device_info_set);
    return std::string();
}

int main()
{
    std::string port = find_arduino_port();

    if (port.empty())
    {
        std::cerr << "[!] Could not auto-detect Arduino!\n";
        return 1;
    }

    std::cout << "[+] Found Arduino on " << port << "\n";
    std::this_thread::sleep_for(std::chrono::seconds(2)); // wait after reset.

    HANDLE serial_handle = CreateFileA(port.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (serial_handle == INVALID_HANDLE_VALUE)
    {
        std::cerr << "[!] Failed to open serial port!\n";
        return 1;
    }

    // serial config.
    DCB dcb = { 0 };
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(serial_handle, &dcb))
    {
        std::cerr << "[!] GetCommState failed!\n";
        CloseHandle(serial_handle);
        return 1;
    }

    dcb.BaudRate = CBR_9600;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = NOPARITY;

    if (!SetCommState(serial_handle, &dcb))
    {
        std::cerr << "[!] SetCommState failed!\n";
        CloseHandle(serial_handle);
        return 1;
    }

    std::cout << "Controls:\n";
    std::cout << "  [-] Press [PAGE UP] -> bhop\n";
    std::cout << "  [-] Hold [Mouse 5] -> triggerbot\n";
    std::cout << "  [-] Hold [Space Bar] -> auto bhop\n\n";

    bool bhop_active = false;
    bool trigger_active = false;

    while (true)
    {
        // auto bhop.
        if (bhop_toggle_key() && space_pressed())
        {
            if (!bhop_active)
            {
                char cmd = 'j';
                DWORD bytes_written;
                WriteFile(serial_handle, &cmd, 1, &bytes_written, nullptr);
                bhop_active = true;
            }
        }
        else
        {
            if (bhop_active)
            {
                char cmd = 'a';
                DWORD bytes_written;
                WriteFile(serial_handle, &cmd, 1, &bytes_written, nullptr);
                bhop_active = false;
            }
        }

        // trigger control.
        POINT cursor;
        if (!GetCursorPos(&cursor))
        {
            std::cerr << "[!] Could not find mouse cursor!\n";
            return 1;
        }

        COLORREF initial = get_pixel_color(cursor.x + 2, cursor.y + 2);

        if (color_bot_key())
        {
            GetCursorPos(&cursor);
            COLORREF current = get_pixel_color(cursor.x + 2, cursor.y + 2);

            // get color change threshold.
            int color_threshold = 20;

            if (is_color_different(initial, current, color_threshold))
            {
                if (!trigger_active)
                {
                    char cmd = 's';
                    DWORD bytes_written;
                    WriteFile(serial_handle, &cmd, 1, &bytes_written, nullptr);
                    trigger_active = true;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        else
        {
            if (trigger_active)
            {
                char cmd = 'x';
                DWORD bytes_written;
                WriteFile(serial_handle, &cmd, 1, &bytes_written, nullptr);
                trigger_active = false;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    CloseHandle(serial_handle);
    return 0;
}
