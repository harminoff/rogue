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

console.log("custom catalog tests passed");
