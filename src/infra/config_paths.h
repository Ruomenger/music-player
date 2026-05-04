#pragma once

#include <string>

namespace musicplayer {

class ConfigPaths {
public:
    static std::string dataDir();
    static std::string configDir();
    static std::string cacheDir();
};

}  // namespace musicplayer
