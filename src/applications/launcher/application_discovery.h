#pragma once

#include <filesystem>
#include <string>
#include <vector>

struct DiscoveredApplication
{
    std::string id;
    std::string name;
    std::string description;
    std::filesystem::path executablePath;
};

namespace launcher
{
    // Versão principal do contrato de integração aceito por esta versão do Launcher.
    constexpr int supportedRotorApiVersion = 1;

    // Descobre aplicações declaradas por manifestos .rotor-app.json ao lado do Launcher.
    // Aplicações incompatíveis, sem executável ou com manifesto inválido são ignoradas.
    std::vector<DiscoveredApplication> discoverCompatibleApplications(
        const std::filesystem::path &directory);
}
