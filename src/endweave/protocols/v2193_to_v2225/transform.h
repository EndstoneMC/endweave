#pragma once

#include "actor.h"
#include "attributes.h"
#include "chunk.h"
#include "connection_data.h"
#include "dimension.h"
#include "endweave/protocol/transform.h"
#include "game.h"
#include "inventory.h"
#include "item_stack.h"
#include "mappings.h"
#include "player.h"

#include <bedrock/protocol/sound.h>
#include <type_traits>

namespace bp = bedrock::protocol;

// ENDWEAVE: 2193 writes ClientboundUpdateSoundData's one event into all seven of its slots, as BDS
// does, and bedrock-protocol reads back the last of them, so a 2193 server that wrote seven different
// events would reach 2225 with the last alone. The codec carries the packet with no transform.
static_assert(std::is_same_v<bp::ClientboundUpdateSoundDataPacket_<2193>, bp::ClientboundUpdateSoundDataPacket_<2225>>);
