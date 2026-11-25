#pragma once

#include "actor.h"
#include "attributes.h"
#include "chunk.h"
#include "endweave/protocol/transform.h"
#include "inventory.h"
#include "item_stack.h"
#include "mappings.h"
#include "player.h"
#include "skin.h"

#include <bedrock/protocol/sound.h>
#include <type_traits>

namespace bp = bedrock::protocol;

static_assert(std::is_same_v<bp::ClientboundUpdateSoundDataPacket_<2225>, bp::ClientboundUpdateSoundDataPacket_<2193>>);
