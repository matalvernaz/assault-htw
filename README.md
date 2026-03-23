# Assault: High Tech War

A real-time strategy MUD originally developed by Amnon Kruvi (PA_MUD / PA2), based on ACK!MUD 4.3 by Stephen Zepp, Merc 2.2, and Diku. Running live and actively developed.

**Connect:** `assault.thealvernaz.space` port `6660`

## What is it?

Assault: HTW is a MUD where players build bases, gather resources, fight other players, and compete for map control. It's a text-based real-time strategy game — think base building and warfare in a classic MUD engine.

Recent additions include:
- AI factions — autonomous factions that own buildings, fight each other, and can be managed with the `aifaction` admin command
- `base` command — dashboard of all your owned buildings with HP, shields, and status
- `salvage` command — harvest resources from destroyed terrain
- `bounty` command — place QP bounties on other players
- Exploration achievements
- Starter kit for new players
- Security and stability fixes, full `-Wall -Werror` clean build

## Setup

```
git clone <repo>
cd src
make
```

Requires: `libgd`, `libz`, `libcrypt`, `libm`

## Credits

**Diku MUD** — Katja Nyboe, Tom Madsen, Michael Seifert, Sebastian Hammer, Hans Henrik Staerfeldt
**Merc 2.2** — Hatchet, Furey, Altrag, Ramias
**ACK!MUD 4.3** — Stimpy, Thalen, Zenithar, Spectrum
**PA_MUD / PA2** — Amnon Kruvi
Original Assault: HTW — Demortes (assault.demortes.com, archived December 2021)
Current maintainer — Fucker
