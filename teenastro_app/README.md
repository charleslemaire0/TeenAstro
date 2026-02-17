# TeenAstro Controller

Flutter Android app that replaces the SHC (Smart Hand Controller) for TeenAstro telescope mounts.

## Setup

1. Install [Flutter SDK](https://flutter.dev/docs/get-started/install)
2. Run inside this directory:
   ```bash
   flutter create --project-name teenastro_app .
   flutter pub get
   flutter run
   ```

## Architecture

- **Transport**: TCP socket to port 9999 (LX200 protocol)
- **State**: Riverpod for reactive state management
- **Navigation**: GoRouter with bottom navigation bar
- **Theme**: Dark astronomical theme (red accent, night-vision friendly)

## Features (V1)

- Mount status dashboard (RA/Dec, Alt/Az, time, status icons)
- Manual slew control (N/S/E/W hold-to-move)
- Goto from catalogs (Messier, NGC, stars, etc.)
- Tracking control (sidereal/lunar/solar/user)
- 2-star alignment
- Park / Home / Unpark
