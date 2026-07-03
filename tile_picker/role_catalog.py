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


def variant_monster_roles(mapping: dict) -> list[Role]:
    roles: list[Role] = []
    variants = mapping.get("variantMonsters", {})
    if not isinstance(variants, dict):
        return roles
    for variant_id in sorted(variants):
        monsters = variants.get(variant_id, {})
        if not isinstance(monsters, dict):
            continue
        for glyph in sorted(monsters):
            entry = monsters[glyph]
            if not isinstance(entry, dict):
                continue
            name = str(entry.get("name") or MONSTER_NAMES.get(glyph, "monster"))
            roles.append(monster_role(str(glyph), name, str(variant_id)))
    return roles


def role_by_id() -> dict[str, Role]:
    return {role.role: role for role in all_roles()}
