"""ServerboundDiagnostics handler for v975 server <- v1001 client.

v1001 appended a Whisker Scopes list after the three lists present in v975
(Memory Category Values, Entity Diagnostics, System Diagnostics). Strip the
Whisker Scopes tail so the v975 server only sees the three lists it expects.
"""

from endstone_endweave.codec import (
    BYTE,
    FLOAT_LE,
    INT64_LE,
    REMAINING_BYTES,
    STRING,
    UVAR_INT,
    PacketWrapper,
)


def rewrite_diagnostics(wrapper: PacketWrapper) -> None:
    for _ in range(9):
        wrapper.passthrough(FLOAT_LE)  # AvgFps .. AvgUnaccountedTimePercent

    # Memory::MemoryCategoryCounter[]
    mem_count = wrapper.passthrough(UVAR_INT)
    for _ in range(mem_count):
        wrapper.passthrough(BYTE)       # Category (uint8)
        wrapper.passthrough(INT64_LE)   # Current Bytes (uint64)

    # ECS::Profiling::Diagnostics::EntityDiagnosticTimingInfo[]
    entity_count = wrapper.passthrough(UVAR_INT)
    for _ in range(entity_count):
        wrapper.passthrough(STRING)     # Display Name
        wrapper.passthrough(STRING)     # Entity
        wrapper.passthrough(INT64_LE)   # Time in NS (uint64)
        wrapper.passthrough(BYTE)       # Percent of Total (uint8)

    # ECS::Profiling::Diagnostics::SystemDiagnosticTimingInfo[]
    system_count = wrapper.passthrough(UVAR_INT)
    for _ in range(system_count):
        wrapper.passthrough(STRING)     # Display Name
        wrapper.passthrough(INT64_LE)   # System Index (uint64)
        wrapper.passthrough(INT64_LE)   # Time in NS (uint64)
        wrapper.passthrough(BYTE)       # Percent of Total (uint8)

    wrapper.read(REMAINING_BYTES)  # discard Whisker Scopes (v1001-only)
