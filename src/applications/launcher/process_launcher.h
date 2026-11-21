#pragma once

#include <filesystem>
#include <string>

namespace launcher
{
    // Diretório onde o executável atual está localizado.
    std::filesystem::path executableDirectory();

    // Inicia outro executável como um processo independente.
    bool startProcess(const std::filesystem::path &executable,
                      const std::filesystem::path &workingDirectory,
                      std::string &errorMessage);
}
