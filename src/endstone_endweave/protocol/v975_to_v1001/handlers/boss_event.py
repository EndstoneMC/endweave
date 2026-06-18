"""Clientbound BossEvent(74) handler for the v975 -> v1001 delta.

Protocol 1001 (MC 1.26.30) completely restructured BossEvent's wire format
(confirmed gophertunnel v1.56.2 vs v1.57.0 boss_event.go). v944 boss_event.go is
byte-for-byte identical to v975, so a packet reaching this handler is in the
SWITCH-based v975 shape and must be flattened to the v1001 shape a 1.26.30 client
decodes.

v975 (== v944; what the older server produces)::
    Varint64   BossEntityUniqueID
    Varuint32  EventType                 (a SWITCH -- the fields below depend on it)
    switch EventType:
      Show(0):                 String Title, String FilteredTitle, Float32 Health,
                               Uint16 ScreenDarkening, Varuint32 Colour, Varuint32 Overlay
      RegisterPlayer(1) /
        UnregisterPlayer(3) /
        Request(8):            Varint64 PlayerUniqueID
      Hide(2):                 (no payload)
      HealthPercentage(4):     Float32 Health
      Title(5):                String Title, String FilteredTitle
      AppearanceProperties(6): Uint16 ScreenDarkening, Varuint32 Colour, Varuint32 Overlay
      Texture(7):              Varuint32 Colour, Varuint32 Overlay

v1001 (what a 1.26.30 client expects -- FLAT, all fields ALWAYS written)::
    Varint64   BossEntityUniqueID
    Varint64   PlayerUniqueID            (moved up; no longer conditional)
    Uint8      EventType                 (was Varuint32; now a plain byte, no switch)
    String     BossBarTitle              (always)
    String     FilteredBossBarTitle      (always)
    Float32    HealthPercentage          (always)
    Uint8      Colour                    (was Varuint32; ScreenDarkening field removed)
    Uint8      Overlay                   (was Varuint32)

So v1001 dropped the per-event-type switch entirely, dropped ScreenDarkening,
narrowed EventType/Colour/Overlay from Varuint32 to a single byte each, and hoisted
PlayerUniqueID to a fixed position. An untranslated v975-shaped BossEvent makes a
1.26.30 client read the Varuint32 EventType's continuation bytes as the next fields,
desync, and disconnect cleanly with no server-side packet violation -- the same
signature as the LevelSoundEvent join-drop. This is content-CONDITIONAL (only sent
when a server actually shows a boss bar), which is why the owner's bar-less server
never triggered it but a content-heavier ~944 server does.

Colour remap (cosmetic, cannot cause a drop -- the client clamps out-of-range):
v975 colours are Grey=0,Blue=1,Red=2,Green=3,Yellow=4,Purple=5,White=6; v1001 are
Pink=0,Blue=1,Red=2,Green=3,Yellow=4,Purple=5,RebeccaPurple=6,White=7. Values 1-5
match by meaning and are preserved; v975 White(6) maps to v1001 White(7); v975
Grey(0) has no v1001 equivalent and is left as 0 (Pink), a valid v1001 colour.
Any value that does not fit a byte is masked to 8 bits (no real Colour/Overlay
exceeds 7, so this never loses information).
"""

from endstone_endweave.codec import (
    BYTE,
    FLOAT_LE,
    STRING,
    USHORT_LE,
    UVAR_INT,
    VAR_INT64,
    PacketWrapper,
)

# BossEvent EventType constants (identical numbering v975<->v1001).
_SHOW = 0
_REGISTER_PLAYER = 1
_HIDE = 2
_UNREGISTER_PLAYER = 3
_HEALTH_PERCENTAGE = 4
_TITLE = 5
_APPEARANCE_PROPERTIES = 6
_TEXTURE = 7
_REQUEST = 8

# v975 colour index -> v1001 colour index. Only the endpoints shifted; 1-5 are
# meaning-stable. Missing keys (e.g. an out-of-table value) fall back to identity.
_COLOUR_V975_TO_V1001 = {6: 7}  # White moved 6 -> 7; Grey(0)->0 (Pink), 1-5 identity.


def _colour_to_v1001(colour_v975: int) -> int:
    """Map a v975 boss-bar colour index to its v1001 index, masked to a byte."""
    return _COLOUR_V975_TO_V1001.get(colour_v975, colour_v975) & 0xFF


def rewrite_boss_event(wrapper: PacketWrapper) -> None:
    """BossEvent (74): flatten the v975 switch-based wire into the v1001 layout.

    Reads the v975 (== v944) shape and re-emits every field in the fixed v1001
    order so a 1.26.30 client parses it. Fields absent for a given event type in
    v975 are written as their v1001 zero defaults (empty string / 0.0 / 0).

    Args:
        wrapper: Packet wrapper for a clientbound BossEventPacket.

    Raises:
        ValueError: if the v975 decode leaves trailing bytes (model misaligned) --
            surfaces a loud failure + payload dump instead of a malformed packet.
    """
    w = wrapper.writer

    # --- read the v975 switch-based shape ---
    boss_unique_id = wrapper.read(VAR_INT64)
    event_type = wrapper.read(UVAR_INT)

    # v1001 always-written field defaults (used for event types that omit them in v975).
    player_unique_id = 0
    title = ""
    filtered_title = ""
    health = 0.0
    colour = 0
    overlay = 0

    if event_type == _SHOW:
        title = wrapper.read(STRING)
        filtered_title = wrapper.read(STRING)
        health = wrapper.read(FLOAT_LE)
        wrapper.read(USHORT_LE)  # ScreenDarkening -- removed at v1001, drop it
        colour = wrapper.read(UVAR_INT)
        overlay = wrapper.read(UVAR_INT)
    elif event_type in (_REGISTER_PLAYER, _UNREGISTER_PLAYER, _REQUEST):
        player_unique_id = wrapper.read(VAR_INT64)
    elif event_type == _HIDE:
        pass  # no payload
    elif event_type == _HEALTH_PERCENTAGE:
        health = wrapper.read(FLOAT_LE)
    elif event_type == _TITLE:
        title = wrapper.read(STRING)
        filtered_title = wrapper.read(STRING)
    elif event_type == _APPEARANCE_PROPERTIES:
        wrapper.read(USHORT_LE)  # ScreenDarkening -- removed at v1001, drop it
        colour = wrapper.read(UVAR_INT)
        overlay = wrapper.read(UVAR_INT)
    elif event_type == _TEXTURE:
        colour = wrapper.read(UVAR_INT)
        overlay = wrapper.read(UVAR_INT)
    else:
        # Unknown event type: nothing more to read in v975 (it would have raised an
        # UnknownEnumOption on the wire). Pass the id through; the flat v1001 body of
        # all-zero defaults is still a structurally valid packet.
        pass

    if wrapper.has_remaining:
        raise ValueError(
            f"BossEvent v975->v1001: input not fully consumed for event_type={event_type} "
            "(decode model misaligned)"
        )

    # --- write the v1001 flat shape ---
    w.write_varint64(boss_unique_id)
    w.write_varint64(player_unique_id)
    BYTE.write(w, event_type & 0xFF)  # EventType narrowed Varuint32 -> Uint8
    STRING.write(w, title)
    STRING.write(w, filtered_title)
    FLOAT_LE.write(w, health)
    BYTE.write(w, _colour_to_v1001(colour))  # Colour narrowed Varuint32 -> Uint8
    BYTE.write(w, overlay & 0xFF)  # Overlay narrowed Varuint32 -> Uint8


# v1001 colour index -> v975 colour index (inverse of _COLOUR_V975_TO_V1001).
_COLOUR_V1001_TO_V975 = {7: 6}  # White moved back 7 -> 6; 0-5 identity (v1001 RebeccaPurple(6) has no v975 match).


def _colour_to_v975(colour_v1001: int) -> int:
    """Map a v1001 boss-bar colour index back to its v975 index, masked to a byte."""
    return _COLOUR_V1001_TO_V975.get(colour_v1001, colour_v1001) & 0xFF


def rewrite_boss_event_serverbound(wrapper: PacketWrapper) -> None:
    """BossEvent (74) SERVERBOUND: convert the v1001 flat wire back to the v975 switch.

    BossEvent is bidirectional: a 1.26.30 client SENDS it (e.g. RegisterPlayer/
    UnregisterPlayer/Request when it sees a boss entity) in the v1001 FLAT format,
    but the v944/v975 server expects the v975 SWITCH format. Untranslated, the server
    reads the flat bytes as the switch shape, fails ("readNoHeader failed! packetId:
    74"), reports PACKET_MALFORMED and terminates the connection. This is the exact
    inverse of rewrite_boss_event: read the always-present v1001 flat fields and
    re-emit only the fields the v975 switch expects for that event type.

    Args:
        wrapper: Packet wrapper for a serverbound BossEventPacket.

    Raises:
        ValueError: if the v1001 decode leaves trailing bytes (model misaligned).
    """
    # --- read the v1001 flat shape (all fields always present) ---
    boss_unique_id = wrapper.read(VAR_INT64)
    player_unique_id = wrapper.read(VAR_INT64)
    event_type = wrapper.read(BYTE)
    title = wrapper.read(STRING)
    filtered_title = wrapper.read(STRING)
    health = wrapper.read(FLOAT_LE)
    colour = wrapper.read(BYTE)
    overlay = wrapper.read(BYTE)

    if wrapper.has_remaining:
        raise ValueError(
            f"BossEvent v1001->v975 (serverbound): input not fully consumed for "
            f"event_type={event_type} (decode model misaligned)"
        )

    # --- write the v975 switch shape (only the fields this event type carries) ---
    w = wrapper.writer
    w.write_varint64(boss_unique_id)
    UVAR_INT.write(w, event_type)  # EventType Uint8 -> Varuint32
    if event_type == _SHOW:
        STRING.write(w, title)
        STRING.write(w, filtered_title)
        FLOAT_LE.write(w, health)
        USHORT_LE.write(w, 0)  # ScreenDarkening -- present at v975, dropped at v1001
        UVAR_INT.write(w, _colour_to_v975(colour))
        UVAR_INT.write(w, overlay)
    elif event_type in (_REGISTER_PLAYER, _UNREGISTER_PLAYER, _REQUEST):
        w.write_varint64(player_unique_id)
    elif event_type == _HIDE:
        pass  # no payload
    elif event_type == _HEALTH_PERCENTAGE:
        FLOAT_LE.write(w, health)
    elif event_type == _TITLE:
        STRING.write(w, title)
        STRING.write(w, filtered_title)
    elif event_type == _APPEARANCE_PROPERTIES:
        USHORT_LE.write(w, 0)  # ScreenDarkening
        UVAR_INT.write(w, _colour_to_v975(colour))
        UVAR_INT.write(w, overlay)
    elif event_type == _TEXTURE:
        UVAR_INT.write(w, _colour_to_v975(colour))
        UVAR_INT.write(w, overlay)
    else:
        pass  # unknown event type: id written, no extra fields (mirrors clientbound)
