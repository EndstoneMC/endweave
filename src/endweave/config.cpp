#include "endweave/config.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <toml++/toml.h>

namespace endweave {
namespace {

constexpr std::string_view kDefaultConfig = R"([debug]
# Enable verbose debug logging for packet translation
enabled = false
# Filter to specific packet IDs (empty = log all packets)
# Example: packets = [11, 193]
packets = []
# Log packets after transformation (pre-transform is always logged when debug is enabled)
log_post_transform = false
)";

/** Writes the default file, so the keys are discoverable without consulting the README. */
void writeDefault(const std::filesystem::path &path, endstone::Logger &logger)
{
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) {
        logger.warning("Could not create {}: {}.", path.parent_path().string(), ec.message());
        return;
    }
    std::ofstream out{path};
    if (!out) {
        logger.warning("Could not write {}; continuing on the defaults.", path.string());
        return;
    }
    out << kDefaultConfig;
}

} // namespace

Config Config::load(const std::filesystem::path &data_folder, endstone::Logger &logger)
{
    Config config;
    const std::filesystem::path path = data_folder / "config.toml";

    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        writeDefault(path, logger);
    }

    const toml::parse_result parsed = toml::parse_file(path.string());
    if (!parsed) {
        logger.warning("{} did not parse ({}); continuing on the defaults.", path.string(),
                       std::string{parsed.error().description()});
        return config;
    }

    const toml::table &table = parsed.table();
    config.debug.enabled = table["debug"]["enabled"].value_or(config.debug.enabled);
    config.debug.log_post_transform = table["debug"]["log_post_transform"].value_or(config.debug.log_post_transform);
    if (const toml::array *const packets = table["debug"]["packets"].as_array()) {
        for (const toml::node &node : *packets) {
            if (const std::optional<std::int64_t> id = node.value<std::int64_t>()) {
                config.debug.packets.insert(static_cast<int>(*id));
            }
            else {
                logger.warning("Ignoring a debug.packets entry that is not a packet id.");
            }
        }
    }
    return config;
}

} // namespace endweave
