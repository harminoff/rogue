const assert = require("assert");
const catalog = require("../tile_picker/custom_catalog.js");

const result = catalog.buildCustomCatalog(544, 832, 32, 32, undefined, []);
assert.strictEqual(result.atlas.columns, 17);
assert.strictEqual(result.atlas.tileWidth, 32);
assert.strictEqual(result.atlas.tileHeight, 32);
assert.strictEqual(result.tiles.length, 442);
assert.strictEqual(result.tiles[0].name, "tile_0000");
assert.strictEqual(result.tiles[441].index, 441);

const clamped = catalog.buildCustomCatalog(544, 832, 32, 32, 999, []);
assert.strictEqual(clamped.atlas.columns, 17);
assert.strictEqual(clamped.tiles.length, 442);

const image = {complete: true, naturalWidth: 544};
const renderable = catalog.withImage(result, image);
assert.strictEqual(renderable.image, image);
assert.strictEqual(renderable.tiles.length, 442);
assert.strictEqual(renderable.atlas.columns, 17);

assert.strictEqual(catalog.isBlankTilePixels(new Uint8ClampedArray([
  0, 0, 0, 0,
  255, 255, 255, 0,
])), true);
assert.strictEqual(catalog.isBlankTilePixels(new Uint8ClampedArray([
  0, 0, 0, 0,
  255, 255, 255, 255,
])), false);

const filtered = catalog.hideBlankTiles(result, new Set([0, 441]));
assert.strictEqual(filtered.tiles.length, 440);
assert.strictEqual(filtered.tiles[0].index, 1);
assert.strictEqual(filtered.tiles[filtered.tiles.length - 1].index, 440);
assert.strictEqual(filtered.atlas.columns, 17);

const sourceA = {
  id: "source_1",
  fileName: "first.png",
  atlas: {columns: 2, tileWidth: 32, tileHeight: 32},
  tiles: [
    {index: 0, name: "floor"},
    {index: 3, name: "wall"},
  ],
};
const sourceB = {
  id: "source_2",
  fileName: "second.png",
  atlas: {columns: 4, tileWidth: 32, tileHeight: 32},
  tiles: [
    {index: 4, name: "hero"},
  ],
};
const combined = catalog.combineSources([sourceA, sourceB], 4);
assert.strictEqual(combined.atlas.columns, 4);
assert.strictEqual(combined.atlas.tileWidth, 32);
assert.strictEqual(combined.atlas.tileHeight, 32);
assert.strictEqual(combined.tiles.length, 3);
assert.deepStrictEqual(combined.tiles.map(tile => tile.index), [0, 1, 2]);
assert.deepStrictEqual(combined.tiles.map(tile => tile.sourceIndex), [0, 3, 4]);
assert.deepStrictEqual(combined.tiles.map(tile => tile.sourceId), ["source_1", "source_1", "source_2"]);
assert.strictEqual(combined.tiles[2].sourceName, "second.png");

console.log("custom catalog tests passed");
