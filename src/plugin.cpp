#include "endweave/plugin.h"
#include "endweave/version.h"

ENDSTONE_PLUGIN(/*name=*/"endweave", /*version=*/ENDWEAVE_VERSION, /*main_class=*/endweave::EndweavePlugin)
{
    prefix = "Endweave";
    description = "Bedrock protocol translation plugin for Endstone.";
    website = "https://github.com/EndstoneMC/endweave";
    authors = {"Vincent <magicdroidx@gmail.com>"};
}
