#pragma once

#include <protocol/skin.h>

namespace bp = bedrock::protocol;

namespace endweave {

/* BDS writes SerializedSkinRef two ways at one protocol version: cerealised for
 * PlayerSkinPacket, and through SerializedSkinImpl::write for the entries of a
 * PlayerListPacket. That is not a version hop, so it is not a Transformer -- both
 * sides are 1001, and Transformer<bp::SerializedSkinRef_<2168>>::downgrade already
 * means the cerealised 1001 skin. These convert to and from the cerealised form of
 * the same era, leaving the version hop to the Transformer that already exists. */
bp::SerializedSkinRef_<1001> cerealize(bp::legacy::SerializedSkinRef &&from, bool trusted);
bp::legacy::SerializedSkinRef decerealize(bp::SerializedSkinRef_<1001> &&from);

/** The trailing bool a pre-cereal PlayerListPacket writes per entry. */
bool isTrusted(const bp::SerializedSkinRef_<1001> &skin);

} // namespace endweave
