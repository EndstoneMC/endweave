#include "game.h"

#include <utility>

namespace ew = endweave;

namespace endweave {
void Transformer<bp::LevelSettings_<2193>, bp::LevelSettings_<2225>>::transform(Context<bp::LevelSettings_<2225>> &ctx,
                                                                                bp::LevelSettings_<2193> &&from)
{
    auto &to = ctx.out();
    ew::transform_members(ctx, std::move(from), to);
    // ENDWEAVE: 2193 records no editor migration, so the level reads as one that never went through any.
    to.editor_level_migration_version = bp::EditorLevelMigrationVersion::Legacy;
}

} // namespace endweave
