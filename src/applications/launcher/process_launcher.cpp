#include "process_launcher.h"

#include <system_error>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__linux__)
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace launcher
{
    std::filesystem::path executableDirectory()
    {
#ifdef _WIN32
        std::vector<wchar_t> buffer(32768, L'\0');
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(),
                                                static_cast<DWORD>(buffer.size()));
        if (length == 0 || length >= buffer.size())
            return std::filesystem::current_path();

        return std::filesystem::path(std::wstring(buffer.data(), length)).parent_path();
#elif defined(__linux__)
        char buffer[PATH_MAX] = {};
        const ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
        if (length <= 0)
            return std::filesystem::current_path();

        buffer[length] = '\0';
        return std::filesystem::path(buffer).parent_path();
#else
        return std::filesystem::current_path();
#endif
    }

    bool startProcess(const std::filesystem::path &executable,
                      const std::filesystem::path &workingDirectory,
                      std::string &errorMessage)
    {
        errorMessage.clear();

        std::error_code fileError;
        if (!std::filesystem::is_regular_file(executable, fileError) || fileError)
        {
            errorMessage = "Executável não encontrado: " + executable.filename().string();
            return false;
        }

#ifdef _WIN32
        STARTUPINFOW startupInfo{};
        startupInfo.cb = sizeof(startupInfo);

        PROCESS_INFORMATION processInfo{};
        const std::wstring executablePath = executable.wstring();
        std::wstring commandLine = L"\"" + executablePath + L"\"";

        // CreateProcessW pode modificar o buffer da linha de comando.
        std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
        mutableCommandLine.push_back(L'\0');

        const std::wstring currentDirectory = workingDirectory.wstring();
        const BOOL created = CreateProcessW(
            executablePath.c_str(),
            mutableCommandLine.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            currentDirectory.empty() ? nullptr : currentDirectory.c_str(),
            &startupInfo,
            &processInfo);

        if (!created)
        {
            errorMessage = "Não foi possível iniciar o aplicativo (erro do Windows " +
                           std::to_string(GetLastError()) + ").";
            return false;
        }

        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
        return true;
#elif defined(__linux__)
        int errorPipe[2]{};
        if (pipe(errorPipe) != 0)
        {
            errorMessage = std::string("Falha ao preparar o processo: ") + std::strerror(errno);
            return false;
        }

        // O fechamento automático no exec permite ao processo pai saber se o exec teve êxito.
        if (fcntl(errorPipe[1], F_SETFD, FD_CLOEXEC) == -1)
        {
            const int error = errno;
            close(errorPipe[0]);
            close(errorPipe[1]);
            errorMessage = std::string("Falha ao preparar o processo: ") + std::strerror(error);
            return false;
        }

        const pid_t child = fork();
        if (child < 0)
        {
            const int error = errno;
            close(errorPipe[0]);
            close(errorPipe[1]);
            errorMessage = std::string("Falha ao criar o processo: ") + std::strerror(error);
            return false;
        }

        if (child == 0)
        {
            close(errorPipe[0]);
            if (!workingDirectory.empty() && chdir(workingDirectory.c_str()) != 0)
            {
                const int error = errno;
                (void)write(errorPipe[1], &error, sizeof(error));
                _exit(126);
            }

            const std::string executableString = executable.string();
            const std::string executableName = executable.filename().string();
            execl(executableString.c_str(), executableName.c_str(),
                  static_cast<char *>(nullptr));

            const int error = errno;
            (void)write(errorPipe[1], &error, sizeof(error));
            _exit(127);
        }

        close(errorPipe[1]);
        int childError = 0;
        const ssize_t bytesRead = read(errorPipe[0], &childError, sizeof(childError));
        close(errorPipe[0]);

        if (bytesRead > 0)
        {
            int status = 0;
            (void)waitpid(child, &status, 0);
            errorMessage = std::string("Não foi possível iniciar o aplicativo: ") +
                           std::strerror(childError);
            return false;
        }

        return true;
#else
        (void)workingDirectory;
        errorMessage = "Inicialização de processos ainda não está disponível nesta plataforma.";
        return false;
#endif
    }
}
