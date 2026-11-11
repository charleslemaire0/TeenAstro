# Phone finder

The Android app plate-solves with the phone camera. No eFinder, and no `:e` command, is required. The screen is **Phone finder**, from the dashboard or from the alignment screen.

Leave that screen open. The camera is released when you leave it, and **Auto sync** only runs while the screen is open. The Windows build of the app has no camera.

## Field

**Field** is the angle covered by the long side of the picture, from 15° to 90°. The default is 60°. The catalog is stars of magnitude 4.5 and brighter, brought to the current equinox. A field that holds only a few of those stars will not match. Clamp the phone to the tube. An eyepiece view is too narrow.

## Hz cone and Ve cone

The phone does not look along the telescope. **Aim** measures two rotations in the telescope frame. Hz is the horizontal cone and Ve is the vertical cone. The same two names are used on every mount.

Center a bright star in the telescope and sync, then capture. The screen shows **Hz Cone** and **Ve Cone**. **Clear cones** forgets them. The values stay on the phone.

**Solve** reports the telescope axis once the cones are known, and the camera axis while they are still zero. **Solve & Sync** sends that axis with `:Sr`, `:Sd`, and `:CM#`.

## Auto sync

Turn **Auto sync** on. When a goto finishes, the phone solves, syncs the telescope axis, and if that axis is farther from the goto target than **Recenter**, slews back to the target once, then solves and syncs again. The limit runs from 2 to 60 arcminutes and starts at 2. The switch and the limit stay on the phone.

This is the same sequence as eFinder **GoTo++**. The phone does not turn on the main unit's GoTo++, and it does not start an alignment.
