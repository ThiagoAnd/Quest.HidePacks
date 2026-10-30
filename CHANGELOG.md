# Changelog

All notable changes to this mod are listed here. The format follows Keep a Changelog.

## [0.1.1] - 2026-10-30

### Added
- Cover image shown in the mod list.

### Changed
- Author name is now S4lezardv.
- Installs CategoryTabs 0.1.1 when it is missing.

## [0.1.0] - 2026-10-23

### Added
- Eye tab in the Solo category bar with every official pack (cover, name and a switch).
- All and Only DLC filters, Reset, Cancel and Apply buttons, and a counter of hidden packs.
- Hidden packs are saved to `HidePacks.json` and kept between sessions.
- Apply takes effect right away and shows "Applied: N hidden".
- Needs CategoryTabs, which a mod manager installs together with this mod. Without it no pack is hidden and the log says so, so nothing stays hidden with no way to undo it.
- Log file with the mod and game version, and a clear error line to send when reporting a problem.

### Fixed
- Covers not showing in the pack list.
- Cancel not putting the switches back.
- Song details staying on screen when switching to the eye tab.
