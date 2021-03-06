#include "stdafx.h"
#include <Setupapi.h>
#include <codecvt>
#include <locale>
#include <windows.h>
#include <iostream>
extern "C" {
#include <hidclass.h>
#include <hidsdi.h>
}

extern "C" {
	bool isInitialized = false;
	HANDLE devHandle;

	//Use this method to scan for devices that uses vid
	__declspec(dllexport) int LightFXInitialize(int vid) {
		int pid = -1;
		GUID guid;
		bool flag = false;

		HidD_GetHidGuid(&guid);
		HDEVINFO hDevInfo = SetupDiGetClassDevsA(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (hDevInfo == INVALID_HANDLE_VALUE) {
			//std::cout << "Couldn't get guid";
			return false;
		}
		unsigned int dw = 0;
		SP_DEVICE_INTERFACE_DATA deviceInterfaceData;

		unsigned int lastError = 0;
		while (!flag) {
			deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
			if (!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guid, dw, &deviceInterfaceData)) {
				lastError = GetLastError();
				return pid;
			}
			dw++;
			DWORD dwRequiredSize = 0;
			if (SetupDiGetDeviceInterfaceDetailW(hDevInfo, &deviceInterfaceData, NULL, 0, &dwRequiredSize, NULL)) {
				//std::cout << "Getting the needed buffer size failed";
				return pid;
			}
			//std::cout << "Required size is " << dwRequiredSize << std::endl;
			if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
				//std::cout << "Last error is not ERROR_INSUFFICIENT_BUFFER";
				return pid;
			}
			std::unique_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA> deviceInterfaceDetailData((SP_DEVICE_INTERFACE_DETAIL_DATA*)new char[dwRequiredSize]);
			deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			if (SetupDiGetDeviceInterfaceDetailW(hDevInfo, &deviceInterfaceData, deviceInterfaceDetailData.get(), dwRequiredSize, NULL, NULL)) {
				std::wstring devicePath = deviceInterfaceDetailData->DevicePath;
				//OutputDebugString(devicePath.c_str());
				devHandle = CreateFile(devicePath.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

				if (devHandle != INVALID_HANDLE_VALUE) {
					std::unique_ptr<HIDD_ATTRIBUTES> attributes(new HIDD_ATTRIBUTES);
					attributes->Size = sizeof(HIDD_ATTRIBUTES);
					if (HidD_GetAttributes(devHandle, attributes.get())) {

						if (attributes->VendorID == vid) {
							pid = attributes->Size;
							flag = true;
						}
					}
					//CloseHandle(hDevice);
					//devHandle = CreateFile(devicePath.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

				}
			}
		}
		//OutputDebugString(flag);
		return pid;
	}

	//Use this method for general devices
	__declspec(dllexport) bool HIDInitialize(int vid, int pid) {
		GUID guid;
		bool flag = false;

		HidD_GetHidGuid(&guid);
		HDEVINFO hDevInfo = SetupDiGetClassDevsA(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (hDevInfo == INVALID_HANDLE_VALUE) {
			//std::cout << "Couldn't get guid";
			return false;
		}
		unsigned int dw = 0;
		SP_DEVICE_INTERFACE_DATA deviceInterfaceData;

		unsigned int lastError = 0;
		while (!flag) {
			deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
			if (!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guid, dw, &deviceInterfaceData)) {
				lastError = GetLastError();
				return false;
			}
			dw++;
			DWORD dwRequiredSize = 0;
			if (SetupDiGetDeviceInterfaceDetailW(hDevInfo, &deviceInterfaceData, NULL, 0, &dwRequiredSize, NULL)) {
				//std::cout << "Getting the needed buffer size failed";
				return false;
			}
			//std::cout << "Required size is " << dwRequiredSize << std::endl;
			if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
				//std::cout << "Last error is not ERROR_INSUFFICIENT_BUFFER";
				return false;
			}
			std::unique_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA> deviceInterfaceDetailData((SP_DEVICE_INTERFACE_DETAIL_DATA*)new char[dwRequiredSize]);
			deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			if (SetupDiGetDeviceInterfaceDetailW(hDevInfo, &deviceInterfaceData, deviceInterfaceDetailData.get(), dwRequiredSize, NULL, NULL)) {
				std::wstring devicePath = deviceInterfaceDetailData->DevicePath;
				//OutputDebugString(devicePath.c_str());
				devHandle = CreateFile(devicePath.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

				if (devHandle != INVALID_HANDLE_VALUE) {
					std::unique_ptr<HIDD_ATTRIBUTES> attributes(new HIDD_ATTRIBUTES);
					attributes->Size = sizeof(HIDD_ATTRIBUTES);
					if (HidD_GetAttributes(devHandle, attributes.get())) {

						if (((attributes->VendorID == vid) && (attributes->ProductID == pid))) {

							flag = true;
						}
					}
					//CloseHandle(hDevice);
					//devHandle = CreateFile(devicePath.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

				}
			}
		}
		//OutputDebugString(flag);
		return flag;
	}
	bool readStatus;
	__declspec(dllexport) bool getReadStatus() {
		return readStatus;
	}
	__declspec(dllexport) int HIDRead(byte Buffer[], int length) {
		size_t BytesWritten;
		readStatus = DeviceIoControl(devHandle, IOCTL_HID_GET_INPUT_REPORT, NULL, 0, Buffer, length, (DWORD*) &BytesWritten, NULL);
		return Buffer[0];
	}

	__declspec(dllexport) bool HIDWrite(byte Buffer[], int length) {
		size_t BytesWritten;
		bool val = DeviceIoControl(devHandle, IOCTL_HID_SET_OUTPUT_REPORT, Buffer, length, NULL, 0, (DWORD*) &BytesWritten, NULL);
		if (!val) {
			int GetError();
		}
		return val;
	}

	__declspec(dllexport) bool HIDClose() {
		bool result;
		if (devHandle != NULL) {
			result = CloseHandle(devHandle);
		}
		return result;
	}

	__declspec(dllexport) int GetError() {
		return GetLastError();
	}
}