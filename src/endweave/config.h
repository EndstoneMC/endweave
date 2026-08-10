#pragma once

#include <endstone/endstone.hpp>
#include <filesystem>
#include <set>

namespace endweave {

/** The plugin's `config.toml`, read once at enable. The layout is the one the Python
 * releases used, so a server carrying a config from those keeps its settings. */
struct Config {
    struct Debug {
        /** Logs every packet either side of its transform, so a connection that dies can
         * be read back packet by packet. Off by default: the lines are voluminous.
         * @see ViaVersion DebugHandler. */
        bool enabled = false;
        /** The ids worth seeing. Empty logs every packet. */
        std::set<int> packets;
        /** Whether to log a packet a second time once translated. */
        bool log_post_transform = false;
    };

    Debug debug;

    /** Reads the config from the data folder, writing the commented default first where
     * no file exists yet. A file that does not parse logs and yields the defaults, so a
     * typo costs the settings rather than the server. */
    static Config load(const std::filesystem::path &data_folder, endstone::Logger &logger);
};

} // namespace endweave
