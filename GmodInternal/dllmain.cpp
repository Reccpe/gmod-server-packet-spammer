#pragma comment(lib, "C:\\libs\\libMinHook.x86.lib")
#include "pch.h"
#include "MinHook.h"
#include <iostream>
#include <windows.h>
#include <cstdio>

typedef bool(__fastcall* SendNetMsg_t)(void* ecx, void* edx, void* pMsg, bool bForceReliable, bool voice);
SendNetMsg_t oSendNetMsg = nullptr;
bool g_PacketSpam = false;

bool __fastcall hkSendNetMsg(void* ecx, void* edx, void* pMsg, bool bForceReliable, bool voice) {

	std::cout << "Kanca calisiyor" << std::endl;

    if (!pMsg)
        return oSendNetMsg(ecx, edx, pMsg, bForceReliable, voice);

    std::cout << "Paket yakalandi! Mesaj: " << pMsg << std::endl;

    if (g_PacketSpam) {
        for (int i = 0; i < 10; ++i) {
            oSendNetMsg(ecx, edx, pMsg, bForceReliable, voice);
        }
    }
    return oSendNetMsg(ecx, edx, pMsg, bForceReliable, voice);
}


void MainThread(HMODULE hModule) {
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);

    std::cout << "GMOD CRASHER BASLATILDI" << std::endl;

    if (MH_Initialize() != MH_OK) return;

    while (!GetModuleHandleA("engine.dll")) Sleep(500);

    typedef void* (__cdecl* GetEngineClient_t)();
    auto GetEngineClient = (GetEngineClient_t)GetProcAddress(GetModuleHandleA("engine.dll"), "GetEngineClient");

    if (GetEngineClient) {

        void* pEngine = GetEngineClient();


        typedef void* (__thiscall* GetNetChannelInfo_t)(void*);
        void* pNetChannel = ((GetNetChannelInfo_t)(*(void***)pEngine)[78])(pEngine);

        if (pNetChannel) {
            void** vTable = *(void***)pNetChannel;
            uintptr_t targetAddr = (uintptr_t)vTable[41]; 

            std::cout << "Hedef Adres: " << std::hex << targetAddr << std::dec << std::endl;

            MH_STATUS status = MH_CreateHook((LPVOID)targetAddr, &hkSendNetMsg, (LPVOID*)&oSendNetMsg);

            if (status == MH_OK) {
                MH_EnableHook((LPVOID)targetAddr);
                std::cout << "Hook basarili!" << std::endl;
            }
            else {
                std::cout << "Hook BASARISIZ! Hata kodu: " << status << std::endl;
            }
        }

        void** vTable = *(void***)pNetChannel;
        std::cout << "Kanca aranıyor..." << std::endl;

        for (int i = 30; i < 60; i++) {
            uintptr_t targetAddr = (uintptr_t)vTable[i];

            if (targetAddr > 0x10000000 && targetAddr < 0x80000000) {
                if (MH_CreateHook((LPVOID)targetAddr, &hkSendNetMsg, (LPVOID*)&oSendNetMsg) == MH_OK) {
                    MH_EnableHook((LPVOID)targetAddr);
                    std::cout << "!!! KANCA TUTTU! İNDEKS: " << i << " ADRES: " << std::hex << targetAddr << std::dec << std::endl;
                    break;
                }
            }
        }
    }


    while (!(GetAsyncKeyState(VK_END) & 1)) {
        if (GetAsyncKeyState(0x4D) & 1) {
            g_PacketSpam = !g_PacketSpam;
            std::cout << "Packet Spammer: " << (g_PacketSpam ? "ON" : "OFF") << std::endl;
            Sleep(300);
        }
        Sleep(10);
    }

    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    fclose(f);
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr));
    return TRUE;
}