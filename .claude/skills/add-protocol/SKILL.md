---
name: add-protocol
description: Follow bedrock-protocol to a new network version - rename the era directory onto the new number, renumber every Transformer key, and carry the update's new wire changes across the hop below it in both directions. Use when the schema has rolled forward or should ("add 2192 support", "follow 1.26.50.x to protocol N", "support the new preview's protocol", "bump the supported version").
---

# Follow bedrock-protocol to a new protocol version

The preview channel renumbers the network version repeatedly inside one update:
r26_u5 went 2171 → 2177 → 2181 → 2187 → 2192 before the release shipped.

**One protocol per update line.** A newer preview *replaces* the older number in
`SUPPORTED_VERSIONS`; it never becomes a second era beside it. Nobody runs a
superseded preview, and translating between two previews of one update buys
nothing. So a roll here is a directory rename plus the transforms the update's
new wire changes demand.

## 0. The schema comes first

endweave owns translation semantics, not the wire format. Before touching
anything here, the sibling `../bedrock-protocol` checkout must already model the
new version.

- If it does not, run **bedrock-protocol's `bump-protocol` skill**
  first - invoke it if this session lists it, otherwise read it directly at
  `../bedrock-protocol/.claude/skills/bump-protocol/SKILL.md` and
  follow it there. It covers establishing the number (check it exists - a
  requested number may not), diffing the protocol-docs dumps, renumbering the
  superseded gates and modelling the delta.
- If it does, take the wire delta from that repo's last commit message, and
  confirm it against the same dump diff rather than trusting the summary.

Either way you arrive here knowing exactly which packets changed shape.

## 1. Rename the era

```shell
git mv src/endweave/protocols/v<old> src/endweave/protocols/v<new>
sed -i "s/\b<old>\b/<new>/g" $(grep -rl "<old>" src/)          # Transformer keys, ENDWEAVE notes
sed -i "s|protocols/v<old>/|protocols/v<new>/|g" $(grep -rl "protocols/v<old>" src/)
sed -i "s/\b<old>\b/<new>/g; s/v<old>/v<new>/g" CLAUDE.md README.md
```

`\b<old>\b` does not match inside `v<old>` - no word boundary after a letter -
which is why the include paths need their own pass. Finish with
`grep -rn "<old>" src/ CLAUDE.md README.md`, which must come back empty.

Then follow through by hand:

- `src/endweave/protocol/version.h` - the enumerator keeps its update-level name
  (`v26_50 = <new>`). It is the same era, so `SUPPORTED_VERSIONS` gains no entry
  and every other number in that file stays put.
- Root `CMakeLists.txt` `add_subdirectory`, and the `transform.h` include in
  `src/endweave/protocol/handler.h`.
- `README.md`'s supported-versions table, and `CLAUDE.md` - it names the version
  line, the outermost eras modelled, and several per-packet claims by number.

## 2. Carry the delta across the hop below

Each wire change the update added needs a `Transformer` in **both** directions
between the new era and the one below it.

- **One file per source version.** `protocols/v<new>/<module>.{h,cpp}` holds
  `<new> → <below>`; `protocols/v<below>/<module>.{h,cpp}` holds the way up.
  The module name matches bedrock-protocol's (`boss.py` → `boss.{h,cpp}`).
- Declare the specialization in the header, define the body in the sibling
  `.cpp`, then list the `.cpp` in that era's `CMakeLists.txt` and the header in
  its `transform.h` - both alphabetical.
- Assign every field explicitly, in declaration order, through
  `auto &to = ctx.out();`. Move anything that owns storage; delegate a changed
  field's arithmetic to that field's own `Transformer` through `ew::transform`.
- **Every field dropped, invented or refused earns one `// ENDWEAVE:` line**
  saying what the far side will actually see - "player_id is invented as the
  null actor; a 2168 client that keys the bar on the id is handed one no actor
  holds". Nothing else gets a comment. Weigh `ctx.cancel()` against inventing:
  invent only where the destination can live with the lie.
- A **variant where only some alternatives changed** needs
  `if constexpr (std::is_same_v<...>)` around the one that did, moving the rest
  straight through - there is no `Transformer<T, T>` and adding one collides
  with the container specializations.
- Expect more types than the diff named: versioning propagates transitively, so
  one new field pulled `TransactionData`, `InventoryTransactionPacket`,
  `PackedItemUseLegacyInventoryTransaction` and `PlayerAuthInputPacket` in with
  it. Let the build enumerate them rather than predicting the list.

## 3. Build - that is the gate

```shell
cmake --build build -j8
clang-format --dry-run -Werror <every file you touched>
```

`listener.cpp` reaches `handler.h` through `connection.h`, so building
instantiates every handler table: a reshaped packet with no `Transformer` stops
the build, and green means every packet that needs work between the supported
versions has it. There is no test suite, and a missing-`Transformer` error is
the answer to "can we ship yet".

After the rename, clear the stale build tree so old objects cannot linger:

```shell
rm -rf build/CMakeFiles/endweave.dir/src/endweave/protocols/v<old> \
       build/src/endweave/protocols/v<old>
```

## 4. Land it

Commit bedrock-protocol first - a rename there is a compile error here. `git
fetch` and check `git rev-list --left-right --count origin/develop...HEAD`
before pushing; other agents share this branch. Stage your own paths, never
`git add -A`. Author is `Vincent <magicdroidx@gmail.com>`; no `Co-Authored-By`
line.

Message shape - what the update did, not what you edited:

```
feat(protocol): follow <build> to protocol <new>

bedrock-protocol renumbered the <release> era to the protocol its current
preview ships, so protocols/v<old> becomes v<new> and every Transformer key,
the version enum and the docs follow.

<per packet: what changed, and what each direction invents or drops>
```
