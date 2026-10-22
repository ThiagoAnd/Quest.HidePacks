# HidePacks

Quest mod for Beat Saber 1.40.8 that hides official packs in the Solo menu.

## In the game

A new eye tab appears in the Solo category bar. It lists every official pack with its cover and a switch (on means visible).

- **All** and **Only DLC** only filter the list that is shown, they do not change the switches.
- **Reset** turns every switch on.
- **Cancel** goes back to what is saved.
- **Apply** saves and takes effect right away. It stays disabled while there is nothing to apply.
- At least one pack has to stay visible.

Custom packs and playlists are never touched.

## Config

Hidden pack ids are stored in `/sdcard/ModData/com.beatgames.beatsaber/Configs/HidePacks.json`:

```json
{"hidden": ["OstVol1", "Camellia"]}
```

The file can also be edited by hand. Pack ids for 1.40.8 are `OstVol1` to `OstVol7`, `Extras`, `Camellia` and the DLC ones such as `Monstercat`, `LinkinPark`, `LinkinPark2` or `ShockDrop`.

## Compatibility

The tab bar shows four tabs per page with Back and Next, so it works with or without RecentlyPlayed.
