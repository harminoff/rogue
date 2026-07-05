"""Rogue semantic tile roles used by the picker and generated C mapping."""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class Role:
    role: str
    group: str
    key: str
    glyph: str
    layer: str
    c_layer: str
    label: str
    name: str


ROLES: tuple[Role, ...] = (
    Role("terrain.empty", "terrain", "empty", "' '", "terrain", "ROGUE_TILE_EMPTY", "Empty", "empty"),
    Role("terrain.floor", "terrain", "floor", "FLOOR", "terrain", "ROGUE_TILE_TERRAIN", "Floor", "floor"),
    Role("terrain.passage", "terrain", "passage", "PASSAGE", "terrain", "ROGUE_TILE_TERRAIN", "Passage", "passage"),
    Role("terrain.door", "terrain", "door", "DOOR", "terrain", "ROGUE_TILE_TERRAIN", "Door", "door"),
    Role("terrain.vertical_wall", "terrain", "vertical_wall", "'|'", "terrain", "ROGUE_TILE_TERRAIN", "Vertical wall", "vertical wall"),
    Role("terrain.horizontal_wall", "terrain", "horizontal_wall", "'-'", "terrain", "ROGUE_TILE_TERRAIN", "Horizontal wall", "horizontal wall"),
    Role("terrain.stairs_down", "terrain", "stairs_down", "STAIRS", "terrain", "ROGUE_TILE_TERRAIN", "Stairs down", "stairs down"),
    Role("terrain.known_trap", "terrain", "known_trap", "TRAP", "terrain", "ROGUE_TILE_TERRAIN", "Known trap", "known trap"),
    Role("object.gold", "objects", "gold", "GOLD", "object", "ROGUE_TILE_OBJECT", "Gold", "gold"),
    Role("object.potion", "objects", "potion", "POTION", "object", "ROGUE_TILE_OBJECT", "Potion", "potion"),
    Role("object.scroll", "objects", "scroll", "SCROLL", "object", "ROGUE_TILE_OBJECT", "Scroll", "scroll"),
    Role("object.magic", "objects", "magic", "MAGIC", "object", "ROGUE_TILE_OBJECT", "Magic detection marker", "magic"),
    Role("object.food", "objects", "food", "FOOD", "object", "ROGUE_TILE_OBJECT", "Food", "food"),
    Role("object.weapon", "objects", "weapon", "WEAPON", "object", "ROGUE_TILE_OBJECT", "Weapon", "weapon"),
    Role("object.armor", "objects", "armor", "ARMOR", "object", "ROGUE_TILE_OBJECT", "Armor", "armor"),
    Role("object.amulet", "objects", "amulet", "AMULET", "object", "ROGUE_TILE_OBJECT", "Amulet", "amulet of Yendor"),
    Role("object.ring", "objects", "ring", "RING", "object", "ROGUE_TILE_OBJECT", "Ring", "ring"),
    Role("object.stick", "objects", "stick", "STICK", "object", "ROGUE_TILE_OBJECT", "Wand or staff", "wand or staff"),
    Role("actor.player", "actors", "player", "PLAYER", "actor", "ROGUE_TILE_ACTOR", "Player", "player"),
)

TRAP_ROLES: tuple[Role, ...] = (
    Role("trap.trapdoor", "traps", "trapdoor", ">", "terrain", "ROGUE_TILE_TERRAIN", "Trapdoor", "trapdoor"),
    Role("trap.arrow", "traps", "arrow", "{", "terrain", "ROGUE_TILE_TERRAIN", "Arrow trap", "arrow trap"),
    Role("trap.sleeping_gas", "traps", "sleeping_gas", "$", "terrain", "ROGUE_TILE_TERRAIN", "Sleeping gas trap", "sleeping gas trap"),
    Role("trap.bear", "traps", "bear", "}", "terrain", "ROGUE_TILE_TERRAIN", "Bear trap", "bear trap"),
    Role("trap.teleport", "traps", "teleport", "~", "terrain", "ROGUE_TILE_TERRAIN", "Teleport trap", "teleport trap"),
    Role("trap.poison_dart", "traps", "poison_dart", "`", "terrain", "ROGUE_TILE_TERRAIN", "Poison dart trap", "poison dart trap"),
)


MONSTER_NAMES: dict[str, str] = {
    "A": "aquator",
    "B": "bat",
    "C": "centaur",
    "D": "dragon",
    "E": "emu",
    "F": "venus flytrap",
    "G": "griffin",
    "H": "hobgoblin",
    "I": "ice monster",
    "J": "jabberwock",
    "K": "kestrel",
    "L": "leprechaun",
    "M": "medusa",
    "N": "nymph",
    "O": "orc",
    "P": "phantom",
    "Q": "quagga",
    "R": "rattlesnake",
    "S": "snake",
    "T": "troll",
    "U": "black unicorn",
    "V": "vampire",
    "W": "wraith",
    "X": "xeroc",
    "Y": "yeti",
    "Z": "zombie",
}


def monster_role(glyph: str, name: str, variant_id: str | None = None) -> Role:
    if variant_id:
        role_id = f"monster.{variant_id}.{glyph}"
        group = "monsters"
        key = f"{variant_id}.{glyph}"
        label = f"{variant_id} {glyph}: {name.title()}"
    else:
        role_id = f"monster.{glyph}"
        group = "monsters"
        key = glyph
        label = name.title()
    return Role(role_id, group, key, f"'{glyph}'", "actor", "ROGUE_TILE_ACTOR", label, name)


def all_roles() -> list[Role]:
    roles = list(ROLES)
    for glyph, name in MONSTER_NAMES.items():
        roles.append(monster_role(glyph, name))
    return roles


def trap_roles() -> list[Role]:
    return list(TRAP_ROLES)


def variant_monster_entries(mapping: dict, variant_id: str) -> list[tuple[str, dict]]:
    variants = mapping.get("variantMonsters", {})
    if not isinstance(variants, dict):
        return []
    monsters = variants.get(variant_id, {})
    if isinstance(monsters, dict):
        return [
            (str(glyph), entry)
            for glyph, entry in monsters.items()
            if isinstance(entry, dict)
        ]
    if isinstance(monsters, list):
        entries: list[tuple[str, dict]] = []
        for entry in monsters:
            if not isinstance(entry, dict):
                continue
            glyph = entry.get("glyph")
            if glyph is None:
                continue
            entries.append((str(glyph), entry))
        return entries
    return []


def variant_monster_entry(mapping: dict, variant_id: str, glyph: str) -> dict:
    for candidate_glyph, entry in variant_monster_entries(mapping, variant_id):
        if candidate_glyph == glyph:
            return entry
    return {}


def set_variant_monster_atlas(
    mapping: dict, variant_id: str, glyph: str, atlas_name: str | None
) -> None:
    variants = mapping.setdefault("variantMonsters", {})
    monsters = variants.setdefault(variant_id, {})
    if isinstance(monsters, list):
        for entry in monsters:
            if isinstance(entry, dict) and str(entry.get("glyph")) == glyph:
                entry["atlas"] = atlas_name
                return
        monsters.append({"glyph": glyph, "atlas": atlas_name})
        return
    if not isinstance(monsters, dict):
        monsters = {}
        variants[variant_id] = monsters
    entry = monsters.setdefault(glyph, {})
    if isinstance(entry, dict):
        entry["atlas"] = atlas_name


def variant_monster_roles(mapping: dict) -> list[Role]:
    roles: list[Role] = []
    variants = mapping.get("variantMonsters", {})
    if not isinstance(variants, dict):
        return roles
    for variant_id in sorted(variants):
        for glyph, entry in sorted(variant_monster_entries(mapping, str(variant_id))):
            name = str(entry.get("name") or MONSTER_NAMES.get(glyph, "monster"))
            roles.append(monster_role(str(glyph), name, str(variant_id)))
    return roles


def role_by_id(mapping: dict | None = None) -> dict[str, Role]:
    roles = all_roles() + trap_roles()
    if mapping is not None:
        roles += variant_monster_roles(mapping)
    return {role.role: role for role in roles}
