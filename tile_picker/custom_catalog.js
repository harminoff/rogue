(function (root) {
  function positiveInt(value, fallback) {
    const parsed = Number(value);
    if (!Number.isFinite(parsed) || parsed <= 0) return fallback;
    return Math.floor(parsed);
  }

  function inferColumns(imageWidth, tileWidth) {
    const width = positiveInt(imageWidth, 0);
    const tw = positiveInt(tileWidth, 32);
    return Math.max(1, Math.floor(width / tw));
  }

  function buildCustomCatalog(imageWidth, imageHeight, tileWidth, tileHeight, columns, names) {
    const tw = positiveInt(tileWidth, 32);
    const th = positiveInt(tileHeight, tw);
    const inferredColumns = inferColumns(imageWidth, tw);
    const cols = positiveInt(columns, inferredColumns);
    const effectiveColumns = Math.min(cols, inferredColumns);
    const rows = Math.max(0, Math.floor(positiveInt(imageHeight, 0) / th));
    const count = rows * effectiveColumns;
    const labels = Array.isArray(names) ? names : [];

    return {
      atlas: {
        columns: effectiveColumns,
        tileWidth: tw,
        tileHeight: th,
      },
      tiles: Array.from({length: count}, (_, index) => ({
        index,
        name: labels[index] || `tile_${String(index).padStart(4, "0")}`,
      })),
    };
  }

  function withImage(source, image) {
    return {
      ...source,
      image,
    };
  }

  function isBlankTilePixels(data) {
    if (!data || !data.length) return true;
    for (let index = 3; index < data.length; index += 4) {
      if (data[index] !== 0) return false;
    }
    return true;
  }

  function hideBlankTiles(source, blankIndexes) {
    const blanks = blankIndexes instanceof Set ? blankIndexes : new Set(blankIndexes || []);
    return {
      ...source,
      tiles: source.tiles.filter(tile => !blanks.has(tile.index)),
    };
  }

  root.RogueCustomCatalog = {
    inferColumns,
    buildCustomCatalog,
    withImage,
    isBlankTilePixels,
    hideBlankTiles,
  };

  if (typeof module !== "undefined") {
    module.exports = root.RogueCustomCatalog;
  }
})(typeof window !== "undefined" ? window : globalThis);
