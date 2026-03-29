"""MobEffect packet handler for v898 server to v860 client translation."""

from endstone_endweave.codec import (
    BOOL,
    BYTE,
    UVAR_INT64,
    VAR_INT,
    PacketWrapper,
)


def rewrite_mob_effect(wrapper: PacketWrapper) -> None:
    """Drop the ambient flag from MobEffect.

    Args:
        wrapper: Packet wrapper for MobEffect.
    """
    wrapper.passthrough(UVAR_INT64)  # Target Runtime ID
    wrapper.passthrough(BYTE)  # Event ID
    wrapper.passthrough(VAR_INT)  # Effect ID
    wrapper.passthrough(VAR_INT)  # Effect Amplifier
    wrapper.passthrough(BOOL)  # Show Particles
    wrapper.passthrough(VAR_INT)  # Effect Duration Ticks
    wrapper.passthrough(UVAR_INT64)  # Tick
    wrapper.read(BOOL)  # Ambient
