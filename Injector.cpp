#include <Windows.h>
#include <tlhelp32.h>
#include <string>

using namespace std;

// fun��o para obter o Id do processo por nome:

DWORD GetProcessIdByName(const wstring& processName) {

    DWORD processId = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 processEntry = { sizeof(PROCESSENTRY32) };
    if (Process32First(snapshot, &processEntry)) {
        do {
            if (processName == processEntry.szExeFile) {
                processId = processEntry.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &processEntry));
    }
    CloseHandle(snapshot);
    return processId;
}

// fun��o para estarmos consequindo injetar uma DLL em um processo:

int InjectDll(DWORD processId, const wstring& dllPath) {
    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId); // abre o processo alvo
    if (processHandle == NULL) {
        return -1; // retorna caso a abertura falhar
    }

    LPVOID loadLibraryAddress = (LPVOID)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryW"); // permite que o executavel carregue a dll
    if (loadLibraryAddress == NULL) {
        CloseHandle(processHandle);
        return -2;
    }

    size_t pathLength = (dllPath.size() + 1) * sizeof(wchar_t); // calcula o tamanho da dll
    LPVOID remoteDllPath = VirtualAllocEx(processHandle, NULL, pathLength, MEM_COMMIT, PAGE_READWRITE); // aloca memoria no processo alvo
    if (remoteDllPath == NULL) {
        CloseHandle(processHandle);
        return -3; // retorna caso a aloca��o falhar
    }

    if (!WriteProcessMemory(processHandle, remoteDllPath, dllPath.c_str(), pathLength, NULL)) {
        VirtualFreeEx(processHandle, remoteDllPath, 0, MEM_RELEASE);
        CloseHandle(processHandle);
        return -4; // retorna caso a grava��o na mem�ria do processo falhar
    }

    HANDLE threadHandle = CreateRemoteThread(processHandle, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddress, remoteDllPath, 0, NULL); // cria uma thread remota para executar LoadLibraryW
    if (threadHandle == NULL) {
        VirtualFreeEx(processHandle, remoteDllPath, 0, MEM_RELEASE);
        CloseHandle(processHandle);
        return -5;
    }

    WaitForSingleObject(threadHandle, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeThread(threadHandle, &exitCode);

    CloseHandle(threadHandle);
    VirtualFreeEx(processHandle, remoteDllPath, 0, MEM_RELEASE); // libera a mem�ria alocada
    CloseHandle(processHandle);

    return exitCode;
}

// Fun��o principal respons�vel por ocultar o console, encontrar processos alvo e injetar uma DLL neles.

int main() {
    // oculta o console
    FreeConsole();

    // parte para armazenar o diret�rio do executavel
    wchar_t currentDir[MAX_PATH];
    GetModuleFileName(NULL, currentDir, MAX_PATH);
    wstring currentDirStr(currentDir);
    wstring::size_type pos = currentDirStr.find_last_of(L"\\/");
    if (pos == wstring::npos) {
        return -1;
    }
    wstring exeDir = currentDirStr.substr(0, pos + 1);

    // nome dos processos a serem injetados
    wstring exeNames[] = { L"ProcessHacker.exe", L"", L"", L"", L"" };
    const int numExes = sizeof(exeNames) / sizeof(wstring);

    // dll a ser injetada
    wstring dllName = exeDir + L"msgbox.dll";

    while (true) {
        PROCESSENTRY32 processEntry = { sizeof(PROCESSENTRY32) };

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return -1;
        }
        if (!Process32First(snapshot, &processEntry)) {
            CloseHandle(snapshot);
            return -1;
        }
        do {
            for (int i = 0; i < numExes; i++) {
                if (_wcsicmp(processEntry.szExeFile, exeNames[i].c_str()) == 0) {
                    DWORD processId = processEntry.th32ProcessID;
                    int result = InjectDll(processId, dllName); // tenta injetar a dll
                    if (result < 0) {

                    }
                    else {

                    }
                }
            }
        } while (Process32Next(snapshot, &processEntry));
        CloseHandle(snapshot);

        Sleep(5000); // aguarda 5 segundos

    }
}