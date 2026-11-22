# Phone finder

The Android app plate-solves with the phone camera. No eFinder, and no `:e` command, is required. The screen is **Phone finder**, on the **Finder** tab, or from the card on the dashboard.

Leave that screen open. The camera is released when you leave it, and **Auto sync** only runs while the screen is open. The Windows build of the app has no camera.

## Without a TeenAstro

On the connection screen, **Use as finder** starts a session with no mount. The phone, clamped to the tube, is the finder. The mount controller's tabs are not shown: the session has only two tabs, **Sky** and **Camera**.

**Camera** shows the camera, or the last photo with a ring on each extracted star. **Solve** sets where the telescope points. The tune icon opens the parameters: solver, field, Live duration, site, the SkySafari address, and the telescope cones.

**Sky** is the planetarium. Its crosshair is the last solve, drawn with the phone's clock and its GPS site. The site is kept on the phone for the next session. **Locate**, in the parameters, reads it again.

**Live** solves again and again, about one second apart, so the crosshair follows the tube while you move it. It runs for the time set in the parameters, from 30 seconds to 3 minutes (1 minute at first), then stops by itself. **Stop live** stops it sooner. Live keeps the camera and the processor busy, so the battery drains faster.

The preview and every still are focused at infinity. Autofocus is off.

## Push-to

Tap an object on **Sky** and choose **Set target**. A card then shows, on both tabs, how far to move the tube from the last solve, for example `3.4°  right 2.9°  up 1.8°`. Right is toward larger azimuth (clockwise seen from above), up is higher altitude. Without a site, the card gives the change in RA and Dec instead. A line joins the crosshair and the target on the chart. Run **Live** and push until the numbers are near zero. The card also says how old the last solve is. Its close button, or **Clear target** on the object, removes the target.

## SkySafari

The app also answers SkySafari. On a tablet on the same Wi-Fi, choose a Meade LX200 Classic telescope over Wi-Fi, with the address and port shown in the Camera header (port 4030). SkySafari reads the last solve. Its GoTo and Sync do not move anything, because there are no motors; the object SkySafari sends becomes the push-to target, named "SkySafari target".

**Exit** (the icon in the Camera header, or top left on Sky) ends the session.

## Field

**Field** is the angle covered by the long side of the picture, from 15° to 90°. The default is 60°. The catalog is stars of magnitude 4.5 and brighter, brought to the current equinox. A field that holds only a few of those stars will not match. Clamp the phone to the tube. An eyepiece view is too narrow.

## Hz cone and Ve cone

The phone does not look along the telescope. **Aim** measures two rotations in the telescope frame. Hz is the horizontal cone and Ve is the vertical cone. The same two names are used on every mount.

Center a bright star in the telescope and sync, then capture. The screen shows **Hz Cone** and **Ve Cone**. **Clear cones** forgets them. The values stay on the phone.

**Solve** reports the telescope axis once the cones are known, and the camera axis while they are still zero. **Solve & Sync** sends that axis with `:Sr`, `:Sd`, and `:CM#`.

## Auto sync

Turn **Auto sync** on. When a goto finishes, the phone solves, syncs the telescope axis, and if that axis is farther from the goto target than **Recenter**, slews back to the target once, then solves and syncs again. The limit runs from 2 to 60 arcminutes and starts at 2. The switch and the limit stay on the phone.

This is the same sequence as eFinder **GoTo++**. The phone does not turn on the main unit's GoTo++.

## Align

**Align** offers **2 Stars** and **4 Stars** on every mount, and **3+3 Stars** on a German equatorial mount. The telescope must already be at home. The phone then slews and plate-solves; nothing is centered by hand. **2 Stars** takes two places on the home pier. **4 Stars** takes four on that pier, or four places spread around the sky on an alt-azimuth mount. **3+3 Stars** takes three on the home pier, flips, then takes three on the other pier. The solved telescope axis is what is recorded. The model is saved when the solves are accepted. **Stop** cancels the session. The hardware eFinder commands are not used.
