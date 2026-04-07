"""Handler for MobEffectPacket -- v860 server to v898 client."""

from ....codec import (
    BOOL,
    BYTE,
    UVAR_INT64,
    VAR_INT,
    PacketWrapper,
)


def rewrite_mob_effect(wrapper: PacketWrapper) -> None:
    """Append a missing ambient flag to MobEffect.

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
    wrapper.write(BOOL, False)  # Ambient
