# Block data for 26.50 translation

The generated `block_mappings.inc` and `block_connections.inc` derive from
[CloudburstMC/Data](https://github.com/CloudburstMC/Data), licensed under Apache-2.0
(the same license as this repository; see `LICENSE`). Source commits, filenames,
and SHA-256 checksums are pinned in `generate_block_mappings.py`.

With the maintainer dependency `amulet-nbt` installed, download the two palettes
and geometry file listed in that script, then run:

```sh
python tools/generate_block_mappings.py palette-2168.nbt palette-2193.nbt blocks.json
```

No data download or extra dependency is needed when running the plugin. The
tables cover all 17,499 states in the older palette, changing 593 hashes and
mapping 3,768 new corner/connection variants back to their old states.

World packets additionally resolve stair corners and horizontal connections
from the server's current neighboring blocks. Palette storage grows when one
old state needs several new shapes within a subchunk. Neighbor updates refresh
shapes after placing or removing blocks. The world lookup cache lasts only for
one packet and is keyed by dimension and position.

The block blob cache is disabled when translating between different palettes:
the server's blob hashes describe bytes that Endweave must change. Clients on
matching palettes retain caching. World gameplay versions are preserved.

These mappings translate existing vanilla server blocks; they do not add block
types absent from the older server. Custom block IDs are preserved. Connection
geometry comes from the pinned vanilla data; custom connection rules require
their own definitions.
