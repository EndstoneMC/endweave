"""Clientbound LevelSoundEvent(123) handler for the v975 -> v1001 delta.

At protocol 1001 the first field of LevelSoundEvent changed wire encoding
(confirmed gophertunnel v1.56.2 vs v1.57.0 level_sound_event.go):

  v975 (what the v944/v975 layer produces)::
      Varuint32   SoundType   (numeric enum id)
      Vec3        Position
      Varint32    ExtraData
      String      EntityType
      Bool        BabyMob
      Bool        DisableRelativeVolume
      Int64 LE    EntityUniqueID
      Optional[Vec3] FireAtPosition

  v1001 (what a 1.26.30 client expects)::
      String      SoundType   (lowercase dotted sound-name string)
      ...fields 2-8 BYTE-IDENTICAL to v975...

Only the SoundType field changed (Varuint32 numeric id -> String name); the
remaining seven fields are byte-for-byte identical at both versions, so they are
copied through verbatim with passthrough_all().

This is the crash fix: a Bedrock melee hit is broadcast as clientbound
LevelSoundEvent(123) with the numeric attack-family SoundType (id 1 = "hit").
Untranslated, a 1.26.30 client reads the numeric id as a length-prefixed String,
desyncs, and disconnects cleanly with no server-side packet-violation -- exactly
the reported "crash on attack".

Chain note: a v1001 client's clientbound chain runs v944_to_v975 FIRST (whose
rewrite_level_sound_event remaps the numeric id via MAPPINGS.sound.shift_up and
re-writes it as Varuint32), THEN this handler. So the SoundType reaching here is
already the v975-NUMBERED id, which is exactly what SOUND_EVENT_NAMES keys on.

Unmapped ids (numeric gaps, or anything outside the table) fall back to the empty
string, which is a valid v1001 sound name (SoundEventUndefined-equivalent) and
never crashes the parse.
"""

from endstone_endweave.codec import STRING, UVAR_INT, PacketWrapper

from .sound_event_map import SOUND_EVENT_NAMES


def rewrite_level_sound_event(wrapper: PacketWrapper) -> None:
    """LevelSoundEvent (123): rewrite numeric SoundType to its v1001 wire string.

    Args:
        wrapper: Packet wrapper for a clientbound LevelSoundEventPacket.
    """
    sound_id = wrapper.read(UVAR_INT)  # v975 numeric SoundType (enum id)
    # v1001 SoundType is the lowercase dotted sound-name string; "" on miss is a
    # valid v1001 sound name and keeps an unmapped id from crashing the client.
    wrapper.write(STRING, SOUND_EVENT_NAMES.get(sound_id, ""))
    # Fields 2-8 (Vec3 Position, Varint32 ExtraData, String EntityType, Bool BabyMob,
    # Bool DisableRelativeVolume, Int64 EntityUniqueID, Optional[Vec3] FireAtPosition)
    # are byte-identical v975<->v1001 -- copy them through verbatim.
    wrapper.passthrough_all()
