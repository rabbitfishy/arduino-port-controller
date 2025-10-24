#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#pragma comment(lib, "setupapi.lib")

//#define USB_ENDPOINTS 7 // AtMegaxxU4

bool isToggleKeyOn() 
{
    static bool previousState = false;
    static bool toggle = false;

    bool currentState = (GetAsyncKeyState(VK_INSERT) & 0x8000);

    if (currentState && !previousState) 
    {
        toggle = !toggle;
        std::cout << "INSERT TOGGLE: " << (toggle ? "ON" : "OFF") << "\n";
    }

    previousState = currentState;
    return toggle;
}

bool isLeftPressed() 
{
    return (GetAsyncKeyState(VK_LBUTTON) & 0x8000);
}

bool isRightPressed() 
{
    return (GetAsyncKeyState(VK_RBUTTON) & 0x8000);
}

std::string find_arduino_port()
{
    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, nullptr, nullptr, DIGCF_PRESENT);
    if (deviceInfoSet == INVALID_HANDLE_VALUE)
        return std::string();

    SP_DEVINFO_DATA deviceInfoData;
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    char buffer[256];
    for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); ++i)
    {
        if (SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &deviceInfoData, SPDRP_FRIENDLYNAME, nullptr, (PBYTE)buffer, sizeof(buffer), nullptr))
        {
            std::string name = buffer;

            if (name.find("Arduino") != std::string::npos || name.find("USB Serial Device") != std::string::npos)
            {
                size_t begin = name.find("COM");
                size_t end = name.find(")", begin);

                if (begin != std::string::npos && end != std::string::npos)
                {
                    std::string port = name.substr(begin, end - begin);
                    SetupDiDestroyDeviceInfoList(deviceInfoSet);
                    return "\\\\.\\" + port;
                }
            }
        }
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return std::string();
}

int main() 
{
    std::string port = find_arduino_port();

    if (port.empty()) 
    {
        std::cerr << "Could not auto-detect Arduino!\n";
        return 1;
    }

    std::cout << "Found Arduino on " << port << "\n";

    HANDLE hSerial = CreateFileA(port.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hSerial == INVALID_HANDLE_VALUE) 
    {
        std::cerr << "Failed to open serial port!\n";
        return 1;
    }

    // Serial config
    DCB dcb = { 0 };
    dcb.DCBlength = sizeof(DCB);
    GetCommState(hSerial, &dcb);
    dcb.BaudRate = CBR_9600;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = NOPARITY;
    SetCommState(hSerial, &dcb);

    std::cout << "Connected to " << port << "\n";
    std::cout << "Controls:\n";
    std::cout << "  - Press [INSERT] to toggle active mode\n";
    std::cout << "  - While active, hold [Left + Right Click] to send recoil\n\n";

    bool isRecoilActive = false;

    while (true) 
    {
        bool isModeOn = isToggleKeyOn();

        if (isModeOn && isLeftPressed() && isRightPressed()) 
        {
            if (!isRecoilActive) 
            {
                char cmd = 's';
                DWORD bytesWritten;
                WriteFile(hSerial, &cmd, 1, &bytesWritten, nullptr);
                std::cout << "RECOIL: ACTIVE\n";
                isRecoilActive = true;
            }
        }
        else 
        {
            if (isRecoilActive) 
            {
                char cmd = 'x';
                DWORD bytesWritten;
                WriteFile(hSerial, &cmd, 1, &bytesWritten, nullptr);
                std::cout << "RECOIL: STOPPED\n";
                isRecoilActive = false;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    CloseHandle(hSerial);
    return 0;
}