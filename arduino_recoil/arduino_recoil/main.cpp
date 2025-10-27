#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#pragma comment(lib, "setupapi.lib")

bool toggle_key()
{
    static bool previous_state = false;
    static bool toggle = false;

    bool current_state = (GetAsyncKeyState(VK_INSERT) & 0x8000);

    if (current_state && !previous_state)
    {
        toggle = !toggle;
        std::cout << "INSERT TOGGLE: " << (toggle ? "ON" : "OFF") << "\n";
    }

    previous_state = current_state;
    return toggle;
}

bool left_pressed() 
{
    return (GetAsyncKeyState(VK_LBUTTON) & 0x8000);
}

bool right_pressed() 
{
    return (GetAsyncKeyState(VK_RBUTTON) & 0x8000);
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
    std::cout << "  [-] Press [INSERT] to toggle active mode\n";
    std::cout << "  [-] While active, hold [Left + Right Click] to send recoil\n\n";

    bool recoil_active = false;

    while (true) 
    {
        bool is_toggled = toggle_key();

        if (is_toggled && left_pressed() && right_pressed())
        {
            if (!recoil_active)
            {
                char cmd = 's';
                DWORD bytes_written;
                WriteFile(serial_handle, &cmd, 1, &bytes_written, nullptr);
                std::cout << "RECOIL: ACTIVE\n";
                recoil_active = true;
            }
        }
        else 
        {
            if (recoil_active)
            {
                char cmd = 'x';
                DWORD bytes_written;
                WriteFile(serial_handle, &cmd, 1, &bytes_written, nullptr);
                std::cout << "RECOIL: STOPPED\n";
                recoil_active = false;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    CloseHandle(serial_handle);
    return 0;
}
