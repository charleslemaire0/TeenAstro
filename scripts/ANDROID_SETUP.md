# Get Android builds working (TeenAstro app)

Use **Flutter’s own checks** – no custom setup.

## 1. Run Flutter doctor

From the repo root (or from `teenastro_app`):

```powershell
flutter doctor -v
```

Read the output. It will tell you exactly what’s wrong (e.g. Android toolchain, licenses, ANDROID_HOME).

## 2. Fix what doctor reports

- **Android toolchain / ANDROID_HOME**  
  Install [Android Studio](https://developer.android.com/studio), run it once so it installs the SDK.  
  Set **ANDROID_HOME** (User env) to the SDK path (e.g. `C:\Users\<You>\AppData\Local\Android\Sdk`).  
  Or run: `.\scripts\setup_android_sdk.ps1 -SdkPath "C:\path\to\android\sdk"` to set it and update `local.properties`.

- **Android license status**  
  Run:
  ```powershell
  flutter doctor --android-licenses
  ```
  Accept all when prompted (`y`).

- **Flutter / Dart**  
  If doctor says Flutter is broken, run: `.\scripts\build_app.ps1 -FixFlutter`

## 3. Build

When `flutter doctor -v` shows the Android toolchain as OK (green check):

```powershell
.\scripts\build_app.ps1
```

The build script runs `flutter doctor -v` before building the APK so you can see any remaining issues.

---

**Summary:** Run `flutter doctor -v` → fix what it says (install Android Studio, set ANDROID_HOME, run `flutter doctor --android-licenses`) → then `.\scripts\build_app.ps1`.
