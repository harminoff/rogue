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


VARIANT_TERRAIN_ROLES: tuple[Role, ...] = (
    Role("terrain.srogue90.magic_pool", "variantTerrain", "srogue90.magic_pool", "'\"'", "terrain", "ROGUE_TILE_TERRAIN", "Super-Rogue Magic Pool", "magic pool"),
    Role("terrain.srogue90.trading_post", "variantTerrain", "srogue90.trading_post", "'^'", "terrain", "ROGUE_TILE_TERRAIN", "Super-Rogue Trading Post", "trading post"),
    Role("terrain.srogue90.secret_door", "variantTerrain", "srogue90.secret_door", "'&'", "terrain", "ROGUE_TILE_TERRAIN", "Super-Rogue Revealed Secret Door", "revealed secret door"),
)


VARIANT_TRAP_ROLES: tuple[Role, ...] = (
    Role("trap.srogue90.maze", "variantTraps", "srogue90.maze", "'\\\\'", "terrain", "ROGUE_TILE_TERRAIN", "Super-Rogue Maze Trap", "maze trap"),
    Role("trap.srogue90.trapdoor", "variantTraps", "srogue90.trapdoor", "'>'", "terrain", "ROGUE_TILE_TERRAIN", "Super-Rogue Trapdoor", "trapdoor"),
    Role("trap.srogue90.arrow", "variantTraps", "srogue90.arrow", "'{'", "terrain", "ROGUE_TILE_TERRAIN", "Super-Rogue Arrow Trap", "arrow trap"),
    Role("trap.srogue90.sleeping_gas", "variantTraps", "srogue90.sleeping_gas", "'$'", "terrain", "ROGUE_TILE_TERRAIN", "Super-Rogue Sleeping Gas Trap", "sleeping gas trap"),
    Role("trap.srogue90.bear", "variantTraps", "srogue90.bear", "'}'", "terrain", "ROGUE_TILE_TERRAIN", "Super-Rogue Bear Trap", "bear trap"),
    Role("trap.srogue90.teleport", "variantTraps", "srogue90.teleport", "'~'", "terrain", "ROGUE_TILE_TERRAIN", "Super-Rogue Teleport Trap", "teleport trap"),
    Role("trap.srogue90.poison_dart", "variantTraps", "srogue90.poison_dart", "'`'", "terrain", "ROGUE_TILE_TERRAIN", "Super-Rogue Poison Dart Trap", "poison dart trap"),
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


def c_glyph_display(c_expr: str) -> str:
    if len(c_expr) >= 2 and c_expr[0] == "'" and c_expr[-1] == "'":
        glyph = c_expr[1:-1]
    else:
        glyph = c_expr
    return glyph.replace("\\\\", "\\").replace("\\'", "'").replace('\\"', '"')


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


def variant_terrain_roles(mapping: dict | None = None) -> list[Role]:
    if mapping is None:
        return list(VARIANT_TERRAIN_ROLES)
    variants = mapping.get("variantTerrain", {})
    if not isinstance(variants, dict):
        return []
    configured = {(role.role, role.key): role for role in VARIANT_TERRAIN_ROLES}
    roles: list[Role] = []
    for variant_id in sorted(variants):
        entries = variants.get(variant_id, {})
        if not isinstance(entries, dict):
            continue
        for key, entry in sorted(entries.items()):
            role_key = f"{variant_id}.{key}"
            role_id = f"terrain.{role_key}"
            role = configured.get((role_id, role_key))
            if role is not None:
                roles.append(role)
                continue
            if not isinstance(entry, dict):
                entry = {}
            glyph = str(entry.get("glyph") or "?")
            name = str(entry.get("name") or str(key).replace("_", " "))
            label = f"{variant_id} {name.title()}"
            roles.append(Role(role_id, "variantTerrain", role_key, f"'{glyph}'", "terrain", "ROGUE_TILE_TERRAIN", label, name))
    return roles


def variant_trap_roles(mapping: dict | None = None) -> list[Role]:
    if mapping is None:
        return list(VARIANT_TRAP_ROLES)
    variants = mapping.get("variantTraps", {})
    if not isinstance(variants, dict):
        return []
    configured = {(role.role, role.key): role for role in VARIANT_TRAP_ROLES}
    roles: list[Role] = []
    for variant_id in sorted(variants):
        entries = variants.get(variant_id, {})
        if not isinstance(entries, dict):
            continue
        for key, entry in sorted(entries.items()):
            role_key = f"{variant_id}.{key}"
            role_id = f"trap.{role_key}"
            role = configured.get((role_id, role_key))
            if role is not None:
                roles.append(role)
                continue
            if not isinstance(entry, dict):
                entry = {}
            glyph = str(entry.get("glyph") or "?")
            name = str(entry.get("name") or str(key).replace("_", " trap"))
            label = f"{variant_id} {name.title()}"
            roles.append(Role(role_id, "variantTraps", role_key, f"'{glyph}'", "terrain", "ROGUE_TILE_TERRAIN", label, name))
    return roles


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


def variant_entry(mapping: dict, section: str, variant_id: str, key: str) -> dict:
    variants = mapping.get(section, {})
    if not isinstance(variants, dict):
        return {}
    entries = variants.get(variant_id, {})
    if not isinstance(entries, dict):
        return {}
    entry = entries.get(key, {})
    return entry if isinstance(entry, dict) else {}


def set_variant_atlas(
    mapping: dict, section: str, variant_id: str, key: str, atlas_name: str | None
) -> None:
    variants = mapping.setdefault(section, {})
    entries = variants.setdefault(variant_id, {})
    if not isinstance(entries, dict):
        entries = {}
        variants[variant_id] = entries
    entry = entries.setdefault(key, {})
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
        roles += variant_terrain_roles(mapping)
        roles += variant_trap_roles(mapping)
        roles += variant_monster_roles(mapping)
    return {role.role: role for role in roles}
