# Groups.io wiki drafts

Markdown sources and screenshots for the TeenAstro groups.io wiki.

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
python docs/wiki/_capture_uploader_shots.py
```

Republish with `IOGROUP` set:

```text
python docs/wiki/_publish_uploader_multipage.py
```

## Field manuals

Markdown + PDF in [`../manuel/`](../manuel/). Rebuild PDFs with:

```text
python docs/manuel/_build_pdf.py
```
