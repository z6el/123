#ifndef memory_h
#define memory_h

#define WIN32_LEAN_AND_MEAN
#include <memory>
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <winternl.h>
#include <cstdio>

#ifdef __cplusplus
extern "C" {
#endif
    typedef enum _DRIVER_COMMAND {
        CMD_NONE = 0,
        CMD_READ = 1,
        CMD_WRITE = 2,
        CMD_MODULE_BASE = 3,
        CMD_ALLOC = 4,
        CMD_FREE = 5,
        CMD_PROTECT = 6,
        CMD_READ64 = 7,
        CMD_WRITE64 = 8,
        CMD_PING = 99,
        CMD_VERIFY_PTE = 100,
        CMD_VERIFY_SPOOF = 101,
    } DRIVER_COMMAND;


#define REQUEST_MAGIC 0x44524B4E  

    typedef struct _REQUEST_DATA {
        unsigned int    magic;          /* must be REQUEST_MAGIC               */
        unsigned int    command;        /* DRIVER_COMMAND                      */
        unsigned __int64 pid;           /* target process ID                   */
        unsigned __int64 address;       /* virtual address (for R/W)           */
        unsigned __int64 buffer;        /* usermode buffer address             */
        unsigned __int64 size;          /* bytes to R/W                        */
        unsigned __int64 result;        /* output: module base / alloc base    */
        unsigned int    protect;        /* memory protection (alloc/protect)   */
        wchar_t         module_name[64]; /* module name for CMD_MODULE_BASE    */
    } REQUEST_DATA, * PREQUEST_DATA;

#ifdef __cplusplus
}
#endif

typedef NTSTATUS(NTAPI* fn_NtQueryCompositionSurfaceStatistics)(PVOID);

namespace memory_diagnostics {
    inline void win32_error(const char* operation, DWORD error) {
        char message[512] = {};
        const DWORD length = FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, error, 0, message, sizeof(message), nullptr);
        fprintf(stderr, "Memory backend: %s failed (Win32 %lu / 0x%08lX): %s\n",
            operation, error, error, length ? message : "No system error description available.");
    }
}

class DriverComm {
private:
    fn_NtQueryCompositionSurfaceStatistics pNtQuery = nullptr;
    bool connected = false;

    bool SendRequest(REQUEST_DATA* req, NTSTATUS* status = nullptr, DWORD* exception = nullptr) {
        if (!pNtQuery) return false;
        req->magic = REQUEST_MAGIC;
        __try {
            const NTSTATUS returned_status = pNtQuery(req);
            if (status) *status = returned_status;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            if (exception) *exception = GetExceptionCode();
            return false;
        }
        return true;
    }

public:
    bool Init() {
        HMODULE hWin32u = LoadLibraryA("win32u.dll");
        if (!hWin32u) {
            memory_diagnostics::win32_error("loading win32u.dll", GetLastError());
            return false;
        }

        pNtQuery = (fn_NtQueryCompositionSurfaceStatistics)
            GetProcAddress(hWin32u, "NtQueryCompositionSurfaceStatistics");
        if (!pNtQuery) {
            memory_diagnostics::win32_error("resolving the backend entry point", GetLastError());
            return false;
        }

        REQUEST_DATA req = { 0 };
        req.command = CMD_PING;
        req.magic = REQUEST_MAGIC;
        NTSTATUS status = 0;
        DWORD exception = 0;
        if (!SendRequest(&req, &status, &exception)) {
            fprintf(stderr, "Memory backend: initialization request raised exception 0x%08lX.\n", exception);
            return false;
        }

        connected = (req.result == 0x50544548 || req.result == 0x4B524E4C);
        if (!connected) {
            fprintf(stderr,
                "Memory backend: initialization handshake was not recognized "
                "(NTSTATUS 0x%08lX, response 0x%016llX).\n"
                "The entry point was found, but backend availability was not confirmed. "
                "This result alone cannot distinguish an absent backend from an incompatible or unresponsive one.\n",
                static_cast<unsigned long>(status), static_cast<unsigned long long>(req.result));
        }
        return connected;
    }

    bool IsConnected() const { return connected; }

    bool ReadMemory(DWORD pid, uintptr_t address, void* buffer, size_t size) {
        REQUEST_DATA req = { 0 };
        req.command = CMD_READ;
        req.pid = pid;
        req.address = address;
        req.buffer = (unsigned __int64)buffer;
        req.size = size;
        return SendRequest(&req) && req.result;
    }

    bool WriteMemory(DWORD pid, uintptr_t address, void* buffer, size_t size) {
        REQUEST_DATA req = { 0 };
        req.command = CMD_WRITE;
        req.pid = pid;
        req.address = address;
        req.buffer = (unsigned __int64)buffer;
        req.size = size;
        return SendRequest(&req) && req.result;
    }

    uintptr_t GetModuleBase(DWORD pid, const wchar_t* moduleName) {
        REQUEST_DATA req = { 0 };
        req.command = CMD_MODULE_BASE;
        req.pid = pid;
        wcsncpy_s(req.module_name, 64, moduleName, _TRUNCATE);
        SendRequest(&req);
        return (uintptr_t)req.result;
    }

    uintptr_t AllocMemory(DWORD pid, size_t size, DWORD protect) {
        REQUEST_DATA req = { 0 };
        req.command = CMD_ALLOC;
        req.pid = pid;
        req.size = size;
        req.protect = protect;
        SendRequest(&req);
        return (uintptr_t)req.result;
    }

    void FreeMemory(DWORD pid, uintptr_t address) {
        REQUEST_DATA req = { 0 };
        req.command = CMD_FREE;
        req.pid = pid;
        req.result = address;
        SendRequest(&req);
    }

    void ProtectMemory(DWORD pid, uintptr_t address, size_t size, DWORD protect) {
        REQUEST_DATA req = { 0 };
        req.command = CMD_PROTECT;
        req.pid = pid;
        req.address = address;
        req.size = size;
        req.protect = protect;
        SendRequest(&req);
    }


    template<typename T>
    T Read(DWORD pid, uintptr_t address) {
        T value{};
        ReadMemory(pid, address, &value, sizeof(T));
        return value;
    }

    template<typename T>
    bool Write(DWORD pid, uintptr_t address, const T& value) {
        return WriteMemory(pid, address, (void*)&value, sizeof(T));
    }
};


class c_memory
{
private:
    DriverComm driver;
    uint32_t process_pid = 0;

    static DWORD GetPidByName(const wchar_t* process_name, bool report_errors = false) {
        PROCESSENTRY32 entry{ sizeof(PROCESSENTRY32) };
        HANDLE snap_shot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap_shot == INVALID_HANDLE_VALUE) {
            if (report_errors)
                memory_diagnostics::win32_error("creating the process snapshot", GetLastError());
            return 0;
        }

        DWORD pid = 0;
        while (::Process32Next(snap_shot, &entry)) {
            if (!_wcsicmp(process_name, entry.szExeFile)) {
                pid = entry.th32ProcessID;
                break;
            }
        }
        if (!pid && report_errors) {
            const DWORD error = GetLastError();
            if (error != ERROR_NO_MORE_FILES)
                memory_diagnostics::win32_error("enumerating processes", error);
            else
                fprintf(stderr, "Memory backend: process %ls was not found during attachment; it may have exited.\n", process_name);
        }
        if (snap_shot)
            CloseHandle(snap_shot);
        return pid;
    }

public:
    bool attach(const wchar_t* process_name) {
        detach();
        if (!driver.Init())
            return false;
        process_pid = GetPidByName(process_name, true);
        return process_pid != 0;
    }

    bool running(const wchar_t* process_name) {
        return GetPidByName(process_name) != 0;
    }

    void detach() {
        process_pid = 0;
    }

    uint64_t get_module_base(const wchar_t* module_name) {
        if (!process_pid || !driver.IsConnected()) return 0;
        return (uint64_t)driver.GetModuleBase(process_pid, module_name);
    }

    bool read(uint64_t address, void* buffer, size_t size) {
        if (!process_pid || !driver.IsConnected()) return false;
        return driver.ReadMemory(process_pid, (uintptr_t)address, buffer, size);
    }

    template <typename T> T read(uint64_t address) {
        T buffer = { };
        read(address, &buffer, sizeof(T));
        return buffer;
    }

    bool write(uint64_t address, void* buffer, size_t size) {
        if (!process_pid || !driver.IsConnected()) return false;
        return driver.WriteMemory(process_pid, (uintptr_t)address, buffer, size);
    }

    template <typename T> bool write(uint64_t address, T buffer) {
        return write(address, &buffer, sizeof(T));
    }

    std::string read_string(uint64_t address) {
        char buffer[256] = { 0 };
        if (!read(address, buffer, sizeof(buffer)))
            return "";
        return std::string(buffer);
    }

    std::string read_string(uint64_t address, uint32_t size) {
        if (size == 0 || size > 4096)
            return "";
        std::vector<char> buffer(size, 0);
        if (!read(address, buffer.data(), size))
            return "";
        return std::string(buffer.data());
    }

    std::wstring read_wstring(uint64_t address) {
        wchar_t buffer[256] = { 0 };
        if (!read(address, buffer, sizeof(buffer)))
            return L"";
        return std::wstring(buffer);
    }

    std::wstring read_wstring(uint64_t address, uint32_t size) {
        if (size == 0 || size > 4096)
            return L"";
        std::vector<wchar_t> buffer(size, 0);
        if (!read(address, buffer.data(), size * sizeof(wchar_t)))
            return L"";
        return std::wstring(buffer.data());
    }
};

inline c_memory m;

#endif
