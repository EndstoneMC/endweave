// Prints the version graph and the paths through it, so the graph can be inspected without a
// server. Build target: endweave_paths.
#include "endweave/protocol/manager.h"

#include <cstdio>
#include <string>

namespace {

std::string describe(const endweave::ProtocolPath &path)
{
    if (path.empty()) {
        return "(same version)";
    }
    std::string out;
    for (const endweave::ProtocolPathEntry &entry : path) {
        out += out.empty() ? "" : " -> ";
        out += entry.protocol->getName();
        out += entry.step == endweave::Step::Upgrade ? "/upgrade" : "/downgrade";
        out += " => " + std::to_string(entry.output_protocol_version);
    }
    return out;
}

void printPath(endweave::ProtocolManager &manager, int from, int to)
{
    const auto path = manager.getProtocolPath(from, to);
    std::printf("%d -> %d : %s\n", from, to, path ? describe(*path).c_str() : "(unreachable)");
}

} // namespace

int main()
{
    endweave::ProtocolManager manager;
    manager.registerProtocols();

    for (const int server : {975, 1001}) {
        manager.refreshVersions(server);
        std::printf("server %d supports:", server);
        for (const int version : manager.getSupportedVersions()) {
            std::printf(" %d", version);
        }
        std::printf("\n");
    }

    printPath(manager, 975, 1001);
    printPath(manager, 1001, 975);
    printPath(manager, 975, 975);
    printPath(manager, 975, 2168);
    return 0;
}
