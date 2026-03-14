import 'package:flutter/material.dart';

// TeenAstro dark astronomical color palette (matches web UI CSS variables)
class TAColors {
  static const background = Color(0xFF0D1117);
  static const surface = Color(0xFF161B22);
  static const surfaceVariant = Color(0xFF1C2128);
  static const border = Color(0xFF30363D);
  static const accent = Color(0xFFC9453A);
  static const accentHover = Color(0xFFE05544);
  static const accentBg = Color(0x1FC9453A);
  static const text = Color(0xFFC9D1D9);
  static const textSecondary = Color(0xFF8B949E);
  static const textHigh = Color(0xFFF0F6FC);
  static const nav = Color(0xFF21262D);
  static const card = Color(0xFF161B22);
  static const cardBorder = Color(0xFF30363D);
  static const error = Color(0xFFE05544);
  static const warning = Color(0xFFF0C040);
  static const success = Color(0xFF3FB950);
}

final teenAstroTheme = ThemeData(
  brightness: Brightness.dark,
  scaffoldBackgroundColor: TAColors.background,
  colorScheme: const ColorScheme.dark(
    primary: TAColors.accent,
    secondary: TAColors.accentHover,
    surface: TAColors.surface,
    error: TAColors.error,
    onPrimary: Colors.white,
    onSurface: TAColors.text,
    onError: Colors.white,
  ),
  cardTheme: CardThemeData(
    color: TAColors.card,
    shape: RoundedRectangleBorder(
      borderRadius: BorderRadius.circular(8),
      side: const BorderSide(color: TAColors.cardBorder),
    ),
    elevation: 2,
  ),
  navigationBarTheme: NavigationBarThemeData(
    backgroundColor: TAColors.nav,
    indicatorColor: TAColors.accent,
    labelTextStyle: WidgetStatePropertyAll(
      TextStyle(fontSize: 12, color: TAColors.text),
    ),
  ),
  elevatedButtonTheme: ElevatedButtonThemeData(
    style: ElevatedButton.styleFrom(
      backgroundColor: TAColors.accent,
      foregroundColor: Colors.white,
      minimumSize: const Size(44, 44),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(8)),
    ),
  ),
  inputDecorationTheme: InputDecorationTheme(
    filled: true,
    fillColor: TAColors.surfaceVariant,
    border: OutlineInputBorder(
      borderRadius: BorderRadius.circular(8),
      borderSide: const BorderSide(color: TAColors.border),
    ),
    enabledBorder: OutlineInputBorder(
      borderRadius: BorderRadius.circular(8),
      borderSide: const BorderSide(color: TAColors.border),
    ),
    focusedBorder: OutlineInputBorder(
      borderRadius: BorderRadius.circular(8),
      borderSide: const BorderSide(color: TAColors.accent),
    ),
    labelStyle: const TextStyle(color: TAColors.textSecondary),
    hintStyle: const TextStyle(color: TAColors.textSecondary),
  ),
  textTheme: const TextTheme(
    headlineLarge: TextStyle(color: TAColors.textHigh, fontWeight: FontWeight.w700),
    headlineMedium: TextStyle(color: TAColors.textHigh, fontWeight: FontWeight.w600),
    titleLarge: TextStyle(color: TAColors.textHigh, fontWeight: FontWeight.w600),
    titleMedium: TextStyle(color: TAColors.text),
    bodyLarge: TextStyle(color: TAColors.text),
    bodyMedium: TextStyle(color: TAColors.text),
    bodySmall: TextStyle(color: TAColors.textSecondary),
    labelLarge: TextStyle(color: TAColors.text, fontWeight: FontWeight.w600),
  ),
);

// Night vision: red-dominant theme to preserve dark adaptation
class NightViewColors {
  static const background = Color(0xFF1A0808);
  static const surface = Color(0xFF2A1010);
  static const surfaceVariant = Color(0xFF351515);
  static const border = Color(0xFF552222);
  static const accent = Color(0xFFCC4444);
  static const accentHover = Color(0xFFDD6666);
  static const text = Color(0xFFCC6666);
  static const textSecondary = Color(0xFFAA5555);
  static const textHigh = Color(0xFFDD8888);
  static const nav = Color(0xFF251010);
  static const card = Color(0xFF2A1010);
  static const cardBorder = Color(0xFF552222);
  static const error = Color(0xFFDD6666);
  static const warning = Color(0xFFCC8844);
  static const success = Color(0xFFAA6644);
}

final teenAstroNightTheme = ThemeData(
  brightness: Brightness.dark,
  scaffoldBackgroundColor: NightViewColors.background,
  colorScheme: const ColorScheme.dark(
    primary: NightViewColors.accent,
    secondary: NightViewColors.accentHover,
    surface: NightViewColors.surface,
    error: NightViewColors.error,
    onPrimary: Color(0xFF1A0808),
    onSurface: NightViewColors.text,
    onError: Color(0xFF1A0808),
  ),
  cardTheme: CardThemeData(
    color: NightViewColors.card,
    shape: RoundedRectangleBorder(
      borderRadius: BorderRadius.circular(8),
      side: const BorderSide(color: NightViewColors.cardBorder),
    ),
    elevation: 2,
  ),
  navigationBarTheme: NavigationBarThemeData(
    backgroundColor: NightViewColors.nav,
    indicatorColor: NightViewColors.accent,
    labelTextStyle: WidgetStatePropertyAll(
      TextStyle(fontSize: 12, color: NightViewColors.text),
    ),
  ),
  elevatedButtonTheme: ElevatedButtonThemeData(
    style: ElevatedButton.styleFrom(
      backgroundColor: NightViewColors.accent,
      foregroundColor: NightViewColors.background,
      minimumSize: const Size(44, 44),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(8)),
    ),
  ),
  inputDecorationTheme: InputDecorationTheme(
    filled: true,
    fillColor: NightViewColors.surfaceVariant,
    border: OutlineInputBorder(
      borderRadius: BorderRadius.circular(8),
      borderSide: const BorderSide(color: NightViewColors.border),
    ),
    enabledBorder: OutlineInputBorder(
      borderRadius: BorderRadius.circular(8),
      borderSide: const BorderSide(color: NightViewColors.border),
    ),
    focusedBorder: OutlineInputBorder(
      borderRadius: BorderRadius.circular(8),
      borderSide: const BorderSide(color: NightViewColors.accent),
    ),
    labelStyle: const TextStyle(color: NightViewColors.textSecondary),
    hintStyle: const TextStyle(color: NightViewColors.textSecondary),
  ),
  textTheme: const TextTheme(
    headlineLarge: TextStyle(color: NightViewColors.textHigh, fontWeight: FontWeight.w700),
    headlineMedium: TextStyle(color: NightViewColors.textHigh, fontWeight: FontWeight.w600),
    titleLarge: TextStyle(color: NightViewColors.textHigh, fontWeight: FontWeight.w600),
    titleMedium: TextStyle(color: NightViewColors.text),
    bodyLarge: TextStyle(color: NightViewColors.text),
    bodyMedium: TextStyle(color: NightViewColors.text),
    bodySmall: TextStyle(color: NightViewColors.textSecondary),
    labelLarge: TextStyle(color: NightViewColors.text, fontWeight: FontWeight.w600),
  ),
);
