#pragma once

#include <endstone/endstone.hpp>

namespace endweave {

/// Endstone plugin entrypoint. The receive -> translate -> emit pipeline is an
/// intentional stub in this MVP; the translation surface lives in
/// endweave/protocol/attribute_layer_sync.h and is exercised by the tests.
class EndweavePlugin : public endstone::Plugin {
public:
    void onEnable() override
    {
        getLogger().info("Endweave enabled (translation pipeline is a stub in this build).");
    }

    void onDisable() override
    {
        getLogger().info("Endweave disabled.");
    }
};

}  // namespace endweave
