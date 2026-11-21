#include "application_discovery.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>
#include <system_error>

#include <nlohmann/json.hpp>

namespace
{
    using Json = nlohmann::json;

    bool hasManifestSuffix(const std::filesystem::path &path)
    {
        const std::string filename = path.filename().string();
        constexpr char suffix[] = ".rotor-app.json";
        constexpr std::size_t suffixLength = sizeof(suffix) - 1;

        return filename.size() >= suffixLength &&
               filename.compare(filename.size() - suffixLength, suffixLength, suffix) == 0;
    }

    bool readRequiredString(const Json &json, const char *key, std::string &value)
    {
        const auto it = json.find(key);
        if (it == json.end() || !it->is_string())
            return false;

        value = it->get<std::string>();
        return !value.empty();
    }

    bool isSafeRelativeExecutablePath(const std::filesystem::path &path)
    {
        if (path.empty() || path.is_absolute() || path.has_root_name() || path.has_root_directory())
            return false;

        for (const auto &part : path)
        {
            if (part == "..")
                return false;
        }

        return true;
    }

    bool loadManifest(const std::filesystem::path &manifestPath,
                      DiscoveredApplication &application)
    {
        std::ifstream file(manifestPath);
        if (!file)
            return false;

        Json manifest;
        try
        {
            file >> manifest;
        }
        catch (const Json::exception &)
        {
            return false;
        }

        if (!manifest.is_object())
            return false;

        const auto manifestVersion = manifest.find("manifest_version");
        const auto rotorApiVersion = manifest.find("rotor_api_version");
        if (manifestVersion == manifest.end() || !manifestVersion->is_number_integer() ||
            manifestVersion->get<int>() != 1 ||
            rotorApiVersion == manifest.end() || !rotorApiVersion->is_number_integer() ||
            rotorApiVersion->get<int>() != launcher::supportedRotorApiVersion)
        {
            return false;
        }

        std::string executable;
        if (!readRequiredString(manifest, "id", application.id) ||
            !readRequiredString(manifest, "name", application.name) ||
            !readRequiredString(manifest, "executable", executable))
        {
            return false;
        }

        const auto description = manifest.find("description");
        if (description != manifest.end() && description->is_string())
            application.description = description->get<std::string>();

        const std::filesystem::path relativeExecutable(executable);
        if (!isSafeRelativeExecutablePath(relativeExecutable))
            return false;

        application.executablePath = (manifestPath.parent_path() / relativeExecutable).lexically_normal();

        std::error_code error;
        return std::filesystem::is_regular_file(application.executablePath, error) && !error;
    }

    std::string lowercase(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char character)
                       { return static_cast<char>(std::tolower(character)); });
        return value;
    }
}

namespace launcher
{
    std::vector<DiscoveredApplication> discoverCompatibleApplications(
        const std::filesystem::path &directory)
    {
        std::vector<DiscoveredApplication> applications;
        std::set<std::string> discoveredIds;
        std::error_code directoryError;

        if (!std::filesystem::is_directory(directory, directoryError) || directoryError)
            return applications;

        std::filesystem::directory_iterator iterator(
            directory, std::filesystem::directory_options::skip_permission_denied, directoryError);
        const std::filesystem::directory_iterator end;

        while (!directoryError && iterator != end)
        {
            const std::filesystem::directory_entry entry = *iterator;
            std::error_code entryError;

            if (entry.is_regular_file(entryError) && !entryError && hasManifestSuffix(entry.path()))
            {
                DiscoveredApplication application;
                if (loadManifest(entry.path(), application) && discoveredIds.insert(application.id).second)
                    applications.push_back(std::move(application));
            }

            iterator.increment(directoryError);
        }

        std::sort(applications.begin(), applications.end(),
                  [](const DiscoveredApplication &left, const DiscoveredApplication &right)
                  {
                      const std::string leftName = lowercase(left.name);
                      const std::string rightName = lowercase(right.name);
                      if (leftName != rightName)
                          return leftName < rightName;
                      return left.id < right.id;
                  });

        return applications;
    }
}
