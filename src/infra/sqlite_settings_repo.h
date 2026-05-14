#pragma once

#include <optional>
#include <string>

namespace musicplayer {

// Key-value persistence for user settings. value_type is informational only —
// the storage column is always TEXT, callers pick the typed accessor that
// matches what they wrote. seedDefaults() inserts the Phase 2 preset list
// (volume / play_mode / language / ...) only when missing, so users can
// override and we won't clobber on next launch.
class SqliteSettingsRepo {
public:
    explicit SqliteSettingsRepo(std::string connectionName = "musicplayer");

    bool setString(const std::string& key, const std::string& value);
    bool setInt(const std::string& key, int value);
    bool setDouble(const std::string& key, double value);
    bool setBool(const std::string& key, bool value);

    std::optional<std::string> getString(const std::string& key);
    std::optional<int> getInt(const std::string& key);
    std::optional<double> getDouble(const std::string& key);
    std::optional<bool> getBool(const std::string& key);

    bool remove(const std::string& key);

    // Inserts predefined defaults (volume=0.8, play_mode=sequential, ...) for
    // any key not already present. Returns false on SQL failure.
    bool seedDefaults();

private:
    bool setRaw(const std::string& key, const std::string& value, const char* valueType);

    std::string connectionName_;
};

}  // namespace musicplayer
