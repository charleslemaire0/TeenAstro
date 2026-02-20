# Fix Flutter / Dart (broken SDK)

**Script:** Use `scripts\fix_flutter_and_build.ps1` to reinstall Flutter (optional) and build the app for **Android** and **Windows**:

```powershell
cd "c:\Users\charl\source\repos\charleslemaire0\TeenAstro\teenastro_app\scripts"
.\fix_flutter_and_build.ps1 -FixFlutter    # Fix Flutter then build APK + Windows
.\fix_flutter_and_build.ps1                # Build only (Flutter on PATH)
.\fix_flutter_and_build.ps1 -SkipAndroid   # Build Windows only
.\fix_flutter_and_build.ps1 -SkipWindows   # Build APK only
```

Windows output: `build\windows\x64\runner\Release\teenastro_app.exe` (and DLLs in that folder).

---

Your Flutter SDK at `C:\Users\charl\OneDrive\Dokumente\develop\flutter` is in a **broken state**: the folder is a git repo with **no commits** (all files untracked). Flutter uses the git commit to know which Dart SDK to download; without it you get:

- `dart --version` → "dart" not found (Dart SDK was never downloaded)
- Flutter tries to download Dart from an invalid URL → **NoSuchKey** error

## Fix: Reinstall Flutter with a proper clone

In PowerShell (run as yourself, not necessarily in this folder):

```powershell
cd "C:\Users\charl\OneDrive\Dokumente\develop"

# Backup broken install
Rename-Item flutter flutter_broken -ErrorAction SilentlyContinue

# Proper clone (this has real git history so Flutter can resolve the Dart SDK)
git clone https://github.com/flutter/flutter.git -b stable

# Add to PATH if not already (User PATH already has this folder)
# [Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\Users\charl\OneDrive\Dokumente\develop\flutter\bin", "User")

# First run will download Dart SDK and other artifacts (can take a few minutes)
cd flutter
.\bin\flutter.bat doctor
.\bin\dart.bat --version
```

After this, `dart` and `flutter` should work in **new** terminals. If you added `flutter\bin` to User PATH, open a **new** Cursor window or restart Cursor so the terminal picks up the updated PATH.

## Why `dart` wasn’t found in Cursor

Your **User** PATH already includes `C:\Users\charl\OneDrive\Dokumente\develop\flutter\bin`. Terminals opened **before** you added it (or that don’t load User env) won’t see it. After fixing the SDK:

1. Close all terminals in Cursor.
2. Open a new terminal, or restart Cursor.

Then run:

```powershell
dart --version
flutter --version
```

## Build the Android app

From the repo root:

```powershell
cd "c:\Users\charl\source\repos\charleslemaire0\TeenAstro\teenastro_app"
flutter pub get
flutter build apk
```

APK output: `build\app\outputs\flutter-apk\app-release.apk`.
