#include "SmartController.h"
#include "SHC_text.h"

static const char* const kHorizonLines[8] = {
  T_ABOVE " " T_HORIZON,
  T_ABOVE " 10 " T_DEG,
  T_ABOVE " 20 " T_DEG,
  T_ABOVE " 30 " T_DEG,
  T_ABOVE " 40 " T_DEG,
  T_ABOVE " 50 " T_DEG,
  T_ABOVE " 60 " T_DEG,
  T_ABOVE " 70 " T_DEG
};

static const char* const kZenithLines[6] = {
  T_BELOW_ZENITH,
  T_BELOW " 85 " T_DEG,
  T_BELOW " 80 " T_DEG,
  T_BELOW " 75 " T_DEG,
  T_BELOW " 70 " T_DEG,
  T_BELOW " 65 " T_DEG
};

static const char* const kMagLines[8] = {
  T_ALL, "10", "11", "12", "13", "14", "15", "16"
};

static const char* const kDblMinLines[11] = {
  "Off", "Min 0.2\"", "Min 0.5\"", "Min 1.0\"", "Min 1.5\"", "Min 2.0\"",
  "Min 3.0\"", "Min 5.0\"", "Min 10\"", "Min 20\"", "Min 50\""
};

static const char* const kDblMaxLines[11] = {
  "Off", "Max 0.5\"", "Max 1.0\"", "Max 1.5\"", "Max 2.0\"", "Max 3.0\"",
  "Max 5.0\"", "Max 10\"", "Max 20\"", "Max 50\"", "Max 100\""
};

static const char* const kVarLines[9] = {
  "Off", "Max 0.5 days", "Max 1.0 days", "Max 2.0 days", "Max 5.0 days",
  "Max 10 days", "Max 20 days", "Max 50 days", "Max 100 days"
};

enum {
  kActReset = 1,
  kActHorizon,
  kActZenith,
  kActStars,
  kActDeep,
  kActCon,
  kActType,
  kActMag,
  kActVar,
  kActDblMin,
  kActDblMax
};

static void appendMarked(char* buf, int& n, const char* text, bool on)
{
  if (n > 0) strcat(buf, "\n");
  if (on) strcat(buf, "+");
  strcat(buf, text);
  if (on) strcat(buf, "+");
  n++;
}

// Above Horizon steps, selection 1..8: 0° then 10° through 70°.
static int horizonLimitDeg(int selection) {
  if (selection < 1) return 0;
  if (selection > 8) return 70;
  return (selection - 1) * 10;
}

// Below Zenith steps, selection 1..6: Off, then 85° down to 65°.
static int zenithLimitDeg(int selection) {
  static const int deg[6] = { 90, 85, 80, 75, 70, 65 };
  if (selection < 1) return 90;
  if (selection > 6) return 65;
  return deg[selection - 1];
}

int SmartHandController::runMappedList(const char* title, int currentOrig, const char* list, const int* map, int n)
{
  if (n <= 0) return 0;
  int cursor = 1;
  for (int i = 0; i < n; i++) {
    if (map[i] == currentOrig) cursor = i + 1;
  }
  int sel = display->UserInterfaceSelectionList(&buttonPad, title, cursor, list);
  if (sel <= 0 || sel > n) return 0;
  return map[sel - 1];
}

SmartHandController::MENU_RESULT SmartHandController::menuFilters()
{
  while (true)
  {
    const bool starOn = current_selection_filter_byMag > 1
      || current_selection_filter_dblmin > 1
      || current_selection_filter_dblmax > 1
      || current_selection_filter_varmax > 1;
    char string_list_Filters[240];
    int n = 0;
    int act[8];
    string_list_Filters[0] = '\0';
    act[n] = kActReset;
    appendMarked(string_list_Filters, n, T_RESET " " T_FILTERS, false);
    act[n] = kActHorizon;
    appendMarked(string_list_Filters, n, "Above Horizon", current_selection_filter_horizon != 0);
    act[n] = kActZenith;
    appendMarked(string_list_Filters, n, "Below Zenith", current_selection_filter_zenith != 0);
    act[n] = kActStars;
    appendMarked(string_list_Filters, n, T_STARS, starOn);
    act[n] = kActDeep;
    appendMarked(string_list_Filters, n, T_DEEPSKY,
      current_selection_filter_dsoMag > 1 || current_selection_filter_type > 1);
    act[n] = kActCon;
    appendMarked(string_list_Filters, n, "Constellation", current_selection_filter_con > 1);
    int current_selection = display->UserInterfaceSelectionList(
      &buttonPad, T_FILTERS T_ALLOW, current_selection_filter, string_list_Filters);
    if (current_selection == 0) return MR_CANCEL;
    current_selection_filter = (uint8_t)current_selection;
    switch (act[current_selection - 1])
    {
    case kActReset:
      current_selection_filter_con = 1;
      current_selection_filter_horizon = 1;
      current_selection_filter_zenith = 1;
      current_selection_filter_type = 1;
      current_selection_filter_byMag = 1;
      current_selection_filter_dsoMag = 1;
      current_selection_filter_nearby = 1;
      current_selection_filter_dblmin = 1;
      current_selection_filter_dblmax = 1;
      current_selection_filter_varmax = 1;
      DisplayMessage(T_FILTERS, T_RESET, 1000);
      break;
    case kActHorizon:
      menuFilterHorizon();
      break;
    case kActZenith:
      menuFilterZenith();
      break;
    case kActStars:
      menuFilterStars();
      break;
    case kActDeep:
      menuFilterDeepSky();
      break;
    case kActCon:
      menuFilterCon();
      break;
    default:
      break;
    }
  }
}

SmartHandController::MENU_RESULT SmartHandController::menuFilterStars()
{
  while (true)
  {
    char list[160];
    int n = 0;
    int act[6];
    list[0] = '\0';
    act[n] = kActMag;
    appendMarked(list, n, "Magnitude", current_selection_filter_byMag > 1);
    if (cat_mgr.hasDblStarCatalog())
    {
      act[n] = kActDblMin;
      appendMarked(list, n, "Dbl* Min Sep.", current_selection_filter_dblmin > 1);
      act[n] = kActDblMax;
      appendMarked(list, n, "Dbl* Max Sep.", current_selection_filter_dblmax > 1);
    }
    if (cat_mgr.hasVarStarCatalog())
    {
      act[n] = kActVar;
      appendMarked(list, n, "Var* Max Per.", current_selection_filter_varmax > 1);
    }
    int current_selection = display->UserInterfaceSelectionList(
      &buttonPad, T_STARS, current_selection_filter_stars, list);
    if (current_selection == 0) return MR_CANCEL;
    current_selection_filter_stars = (uint8_t)current_selection;
    switch (act[current_selection - 1])
    {
    case kActMag: menuFilterByMag(); break;
    case kActDblMin: menuFilterDblMinSep(); break;
    case kActDblMax: menuFilterDblMaxSep(); break;
    case kActVar: menuFilterVarMaxPer(); break;
    default: break;
    }
  }
}

SmartHandController::MENU_RESULT SmartHandController::menuFilterDeepSky()
{
  while (true)
  {
    char list[80];
    int n = 0;
    int act[4];
    list[0] = '\0';
    act[n] = kActMag;
    appendMarked(list, n, "Magnitude", current_selection_filter_dsoMag > 1);
    act[n] = kActType;
    appendMarked(list, n, "Type", current_selection_filter_type > 1);
    int current_selection = display->UserInterfaceSelectionList(
      &buttonPad, T_DEEPSKY, current_selection_filter_deep, list);
    if (current_selection == 0) return MR_CANCEL;
    current_selection_filter_deep = (uint8_t)current_selection;
    switch (act[current_selection - 1])
    {
    case kActMag: menuFilterMag(current_selection_filter_dsoMag); break;
    case kActType: menuFilterType(); break;
    default: break;
    }
  }
}

void SmartHandController::addStoredFilters(bool withCon)
{
  if (current_selection_filter_horizon > 0)
    cat_mgr.filterAdd(FM_ABOVE_HORIZON, current_selection_filter_horizon - 1);
  if (current_selection_filter_zenith > 1)
    cat_mgr.filterAdd(FM_BELOW_ZENITH, current_selection_filter_zenith - 2);
  if (withCon && current_selection_filter_con > 1)
    cat_mgr.filterAdd(FM_CONSTELLATION, current_selection_filter_con - 2);
  if (cat_mgr.isDsoCatalog())
  {
    if (current_selection_filter_type > 1)
      cat_mgr.filterAdd(FM_OBJ_TYPE, current_selection_filter_type - 2);
    if (current_selection_filter_dsoMag > 1)
      cat_mgr.filterAdd(FM_BY_MAG, current_selection_filter_dsoMag - 2);
  }
  else if (cat_mgr.isStarCatalog())
  {
    if (current_selection_filter_byMag > 1)
      cat_mgr.filterAdd(FM_BY_MAG, current_selection_filter_byMag - 2);
    if (cat_mgr.isDblStarCatalog())
    {
      if (current_selection_filter_dblmin > 1)
        cat_mgr.filterAdd(FM_DBL_MIN_SEP, current_selection_filter_dblmin - 2);
      if (current_selection_filter_dblmax > 1)
        cat_mgr.filterAdd(FM_DBL_MAX_SEP, current_selection_filter_dblmax - 2);
    }
    if (cat_mgr.isVarStarCatalog() && current_selection_filter_varmax > 1)
      cat_mgr.filterAdd(FM_VAR_MAX_PER, current_selection_filter_varmax - 2);
  }
}

void SmartHandController::setCatMgrFilters()
{
  cat_mgr.filtersClear();
  addStoredFilters(true);
}

SmartHandController::MENU_RESULT SmartHandController::menuFilterCon()
{
  // Abbreviations (~450 B) instead of long names (~850 B); keep BSS small for ESP8266 heap/OTA.
  static char string_list_fCon[512];
  bool seen[88];
  for (int i = 0; i < 88; i++) seen[i] = m_filterCatalog < 0;
  if (m_filterCatalog >= 0)
  {
    cat_mgr.select(m_filterCatalog);
    const long saved = cat_mgr.getIndex();
    cat_mgr.filtersClear();
    addStoredFilters(false);
    if (cat_mgr.setIndex(0))
    {
      const long start = cat_mgr.getIndex();
      for (;;)
      {
        const int c = (int)cat_mgr.constellation();
        if (c >= 0 && c < 88) seen[c] = true;
        if (!cat_mgr.incIndex()) break;
        if (cat_mgr.getIndex() == start) break;
      }
    }
    cat_mgr.setIndexRaw(saved);
  }

  if (current_selection_filter_con > 1)
  {
    const int code = current_selection_filter_con - 2;
    if (code < 0 || code >= 88 || !seen[code]) current_selection_filter_con = 1;
  }

  int map[89];
  int n = 0;
  string_list_fCon[0] = '\0';
  map[n++] = 1;
  strcat(string_list_fCon, T_ALL);
  for (int code = 0; code < 88; code++)
  {
    if (!seen[code]) continue;
    strcat(string_list_fCon, "\n");
    strcat(string_list_fCon, cat_mgr.constellationCodeToStr(code));
    map[n++] = code + 2;
  }
  const int last = current_selection_filter_con;
  const int picked = runMappedList(T_FILTERBYCON, last, string_list_fCon, map, n);
  if (picked == 0)
  {
    current_selection_filter_con = (uint8_t)last;
    return MR_CANCEL;
  }
  current_selection_filter_con = (uint8_t)picked;
  return MR_OK;
}

static void addMappedLine(char* list, int* map, int& n, const char* text, int orig)
{
  if (n > 0) strcat(list, "\n");
  strcat(list, text);
  map[n++] = orig;
}

SmartHandController::MENU_RESULT SmartHandController::menuFilterType()
{
  char list[500];
  int map[22];
  int n = 0;
  list[0] = '\0';
  for (int l = 0; l < 22; l++)
    addMappedLine(list, map, n, l == 0 ? T_ALL : cat_mgr.objectTypeCodeToStr(l - 1), l + 1);
  const int last = current_selection_filter_type;
  const int picked = runMappedList(T_FILTERBYTYPE, last, list, map, n);
  if (picked == 0) return MR_CANCEL;
  current_selection_filter_type = (uint8_t)picked;
  return MR_OK;
}

SmartHandController::MENU_RESULT SmartHandController::menuFilterHorizon()
{
  uint8_t& sel = current_selection_filter_horizon;
  const uint8_t zen = current_selection_filter_zenith;
  char list[160];
  int map[8];
  int n = 0;
  list[0] = '\0';
  for (int s = 1; s <= 8; s++)
  {
    if (zen > 1 && horizonLimitDeg(s) >= zenithLimitDeg(zen)) continue;
    addMappedLine(list, map, n, kHorizonLines[s - 1], s);
  }
  const int last = sel;
  const int picked = runMappedList(T_FILTER " " T_HORIZON, last, list, map, n);
  if (picked == 0) return MR_CANCEL;
  sel = (uint8_t)picked;
  return MR_OK;
}

SmartHandController::MENU_RESULT SmartHandController::menuFilterZenith()
{
  uint8_t& sel = current_selection_filter_zenith;
  const uint8_t hor = current_selection_filter_horizon;
  const int floor = 1;
  char list[160];
  int map[6];
  int n = 0;
  list[0] = '\0';
  for (int s = 1; s <= 6; s++)
  {
    if (s < floor) continue;
    if (s > 1 && zenithLimitDeg(s) <= horizonLimitDeg(hor)) continue;
    addMappedLine(list, map, n, kZenithLines[s - 1], s);
  }
  const int last = sel < 1 ? 1 : sel;
  const int picked = runMappedList(T_FILTER " " T_ZENITH, last, list, map, n);
  if (picked == 0) return MR_CANCEL;
  sel = (uint8_t)picked;
  return MR_OK;
}

SmartHandController::MENU_RESULT SmartHandController::menuFilterMag(uint8_t& sel)
{
  char list[80];
  int map[8];
  int n = 0;
  list[0] = '\0';
  for (int s = 1; s <= 8; s++)
    addMappedLine(list, map, n, kMagLines[s - 1], s);
  const int last = sel;
  const int picked = runMappedList(T_FILTER " Magnitude", last, list, map, n);
  if (picked == 0) return MR_CANCEL;
  sel = (uint8_t)picked;
  return MR_OK;
}

SmartHandController::MENU_RESULT SmartHandController::menuFilterByMag()
{
  return menuFilterMag(current_selection_filter_byMag);
}

SmartHandController::MENU_RESULT SmartHandController::menuFilterNearby()
{
  const char* string_list_fNearby = "Off\nWithin  1\xb0\nWithin  5\xb0\nWithin 10\xb0\nWithin 15\xb0";
  int last_selection_filter_nearby = current_selection_filter_nearby;
  current_selection_filter_nearby = display->UserInterfaceSelectionList(&buttonPad, "Filter Nearby", current_selection_filter_nearby, string_list_fNearby);
  if (current_selection_filter_nearby == 0)
  {
    current_selection_filter_nearby = last_selection_filter_nearby; return MR_CANCEL;
  }
  return MR_OK;
}

SmartHandController::MENU_RESULT SmartHandController::menuFilterDblMinSep()
{
  const uint8_t mx = current_selection_filter_dblmax;
  char list[180];
  int map[11];
  int n = 0;
  list[0] = '\0';
  for (int s = 1; s <= 11; s++)
  {
    if (mx > 1 && s > 1 && s > mx) continue;
    addMappedLine(list, map, n, kDblMinLines[s - 1], s);
  }
  const int last = current_selection_filter_dblmin;
  const int picked = runMappedList("Filter Dbl* Sep.", last, list, map, n);
  if (picked == 0) return MR_CANCEL;
  current_selection_filter_dblmin = (uint8_t)picked;
  return MR_OK;
}

SmartHandController::MENU_RESULT SmartHandController::menuFilterDblMaxSep()
{
  const uint8_t mn = current_selection_filter_dblmin;
  char list[180];
  int map[11];
  int n = 0;
  list[0] = '\0';
  for (int s = 1; s <= 11; s++)
  {
    if (mn > 1 && s > 1 && s < mn) continue;
    addMappedLine(list, map, n, kDblMaxLines[s - 1], s);
  }
  const int last = current_selection_filter_dblmax;
  const int picked = runMappedList("Filter Dbl* Sep.", last, list, map, n);
  if (picked == 0) return MR_CANCEL;
  current_selection_filter_dblmax = (uint8_t)picked;
  return MR_OK;
}

SmartHandController::MENU_RESULT SmartHandController::menuFilterVarMaxPer()
{
  char list[160];
  int map[9];
  int n = 0;
  list[0] = '\0';
  for (int s = 1; s <= 9; s++)
    addMappedLine(list, map, n, kVarLines[s - 1], s);
  const int last = current_selection_filter_varmax;
  const int picked = runMappedList("Filter Var* Period", last, list, map, n);
  if (picked == 0) return MR_CANCEL;
  current_selection_filter_varmax = (uint8_t)picked;
  return MR_OK;
}