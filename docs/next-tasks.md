# RogueTiles Next Tasks

These are the remaining follow-up tasks after the Rogue 5.2.1 variant and variant-aware tile mapping work.

## Rogue 5.2.1 GUI Coverage

- [x] Audit `?` command help and `? *` full command help in Rogue 5.2.1 tile mode.
- [x] Audit `/` identify-symbol prompts in Rogue 5.2.1 tile mode.
- [x] Audit `i`, `I`, and all item selection prompts: wield, wear, quaff, read, eat, throw, zap, call, and drop.
- [x] Surface `D` discovered-item lists in the GUI with readable scrolling.
- [x] Surface `o` options in the GUI instead of the terminal.
- [x] Surface special prompts such as genocide monster selection in the GUI.
- [x] Surface save, restore, quit confirmation, death, score, and win screens in the GUI.
- [x] Audit long text/list windows for clipping, wrapping, and page scrolling.

## Variant Readiness

- [x] Confirm custom tile packs can override base roles such as `monster.M`.
- [x] Confirm custom tile packs can override variant roles such as `monster.rogue52.M`.
- [ ] Run a manual Rogue 5.2.1 tile-mode session through movement, combat, stairs, inventory, and at least one prompt-driven item action.
- [x] Package and smoke test `dist\RogueTiles` after each GUI coverage slice.
