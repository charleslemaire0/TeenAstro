# Groups.io wiki drafts

Markdown sources and screenshots for the TeenAstro groups.io wiki.

## Webserver / Wi‑Fi / Alpaca

| Draft | Wiki |
|-------|------|
| [Webserver.md](Webserver.md) | https://groups.io/g/TeenAstro/wiki/8105 |
| [WiFi_Interface.md](WiFi_Interface.md) | https://groups.io/g/TeenAstro/wiki/14592 |
| [ASCOM_Alpaca.md](ASCOM_Alpaca.md) | https://groups.io/g/TeenAstro/wiki/43151 |
| [Upgrade_SHC_to_ESP32S3.md](Upgrade_SHC_to_ESP32S3.md) | https://groups.io/g/TeenAstro/wiki/43152 |

Republish Alpaca + S3 upgrade + Home / Tools visibility with `IOGROUP` set:

```text
py -3 docs/wiki/_publish_alpaca_and_s3.py
```

Home diagram + Tools sidebar only (no page rewrite):

```text
py -3 docs/wiki/_patch_home_diagram_alpaca.py
py -3 docs/wiki/_publish_alpaca_visibility.py
```

Capture from a live unit (default `http://192.168.1.18`, Wi‑Fi page password `password`):

```text
py -3 docs/wiki/_capture_webserver_shots.py
```

Republish with `IOGROUP` set:

```text
py -3 docs/wiki/_publish_webserver_pages.py
```

## TeenAstroUploader (Windows)

| Draft | Wiki |
|-------|------|
| [TeenAstroUploader_for_Windows.md](TeenAstroUploader_for_Windows.md) | https://groups.io/g/TeenAstro/wiki/8837 |
| [TeenAstroUploader_Telescope.md](TeenAstroUploader_Telescope.md) | https://groups.io/g/TeenAstro/wiki/43070 |
| [TeenAstroUploader_Focuser.md](TeenAstroUploader_Focuser.md) | https://groups.io/g/TeenAstro/wiki/43071 |
| [TeenAstroUploader_Hand_controller.md](TeenAstroUploader_Hand_controller.md) | https://groups.io/g/TeenAstro/wiki/43072 |
| [TeenAstroUploader_Auto_EEPROM.md](TeenAstroUploader_Auto_EEPROM.md) | https://groups.io/g/TeenAstro/wiki/10404 (was TeenAstroConfig) |

Screenshots live in [`screenshots/`](screenshots/). Recapture:

```text
py -3 docs/wiki/_capture_uploader_shots.py
```

Republish with `IOGROUP` set:

```text
py -3 docs/wiki/_publish_uploader_multipage.py
```

## Android app

Eight pages. Screenshots are captured with `flutter run -d windows -t tool/wiki_shooter.dart` from `teenastro_app` (the window has to be tall enough for an 844-pixel phone, then restored to 1280×720). The status image is cropped above the temporary debug panel.

| Draft | Wiki |
|-------|------|
| [Android_App.md](Android_App.md) | https://groups.io/g/TeenAstro/wiki/43283 |
| [Android_App_Status.md](Android_App_Status.md) | https://groups.io/g/TeenAstro/wiki/43284 |
| [Android_App_Control.md](Android_App_Control.md) | https://groups.io/g/TeenAstro/wiki/43285 |
| [Android_App_Goto.md](Android_App_Goto.md) | https://groups.io/g/TeenAstro/wiki/43286 |
| [Android_App_Planetarium.md](Android_App_Planetarium.md) | https://groups.io/g/TeenAstro/wiki/43287 |
| [Android_App_Tracking.md](Android_App_Tracking.md) | https://groups.io/g/TeenAstro/wiki/43288 |
| [Android_App_Alignment.md](Android_App_Alignment.md) | https://groups.io/g/TeenAstro/wiki/43289 |
| [Android_App_Phone_finder.md](Android_App_Phone_finder.md) | https://groups.io/g/TeenAstro/wiki/43290 |

Republish with `IOGROUP` set:

```text
py -3 docs/wiki/_publish_android_app.py
```

## Field manuals

Markdown + PDF in [`../manuel/`](../manuel/). Rebuild PDFs with:

```text
py -3 docs/manuel/_build_pdf.py
```
