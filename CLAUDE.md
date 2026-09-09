# endweave

Bedrock protocol translation plugin (Endstone). Lets clients on protocol N talk
to servers on protocol M by rewriting packets in flight.

## Sources of truth (in order)

1. **EndstoneMC/protocol-docs** -- one branch per BDS release (`r26_u1`,
   `r26_u2`, ...). Authoritative for wire-format JSON.
2. **Mojang/bedrock-protocol-docs** -- branches `r/26_u1`, `r/26_u2`. Useful
   `.dot` graphs and `json/` schemas; same data, slightly different shape.
3. **gophertunnel** -- handy for cross-checking field names and discovering
   what changed between versions, but **do not trust** for wire formats. It
   is reverse-engineered and has been wrong before (e.g. `ActionFlag`
   uvarint vs uint8 for LocatorBar; chemistry recipe `RecipeUnlockingRequirement`).
4. **CloudburstMC/Protocol** / pmmp -- same caveat as gophertunnel.
5. **bedrock-headers** at `LiteLDev/bedrock-headers` --
   reference only for enums/struct layouts. Never `#include` directly.

## Wire-format conventions

- Game rule int values are `VAR_INT` in every version (not `UVAR_INT`).
- Recipe results: `NETWORK_ITEM_INSTANCE_DESCRIPTOR` (no `HasNetID`), not
  `ITEM_INSTANCE`.
- `ShapedChemistry` and `ShapelessChemistry` recipes do **not** carry
  `RecipeUnlockingRequirement` on the wire in v944. CloudburstMC was right
  here; gophertunnel and pmmp were wrong.
- "Optional[T]" in the JSON docs maps to: bool prefix + (T if true).

## Style

- No double-dashes (`--`) in user-facing prose. They look LLM-generated.
- No `Co-Authored-By: Claude` trailer on commits.
- Changelog entries: state what is broken and what is fixed, not why.

## Useful commands

```sh
uv run python -c "..."          # ad-hoc roundtrip checks (not python3)
gh api repos/.../contents/...   # fetch protocol docs
```
