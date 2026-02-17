#!/usr/bin/env python3
"""
extract_catalogs.py - Extract TeenAstro C PROGMEM catalogs to JSON

Parses the C header files in libraries/TeenAstroCatalog/src/catalogs/
and produces JSON files for the Flutter app.

Usage:
    python tools/extract_catalogs.py
"""

import json
import os
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
CATALOG_DIR = REPO_ROOT / "libraries" / "TeenAstroCatalog" / "src" / "catalogs"
OUTPUT_DIR = REPO_ROOT / "teenastro_app" / "assets" / "catalogs"

# Coordinate decoding constants
RA_DIVISOR  = 2730.6666666666666
DEC_DIVISOR = 364.07777777777777

def decode_mag_compact(val):
    """Decode compact magnitude: (val / 10.0) - 2.5; 255 = unknown"""
    if val == 255:
        return None
    return round((val / 10.0) - 2.5, 2)

def decode_ra(val):
    """Decode compact RA to hours"""
    return round(val / RA_DIVISOR, 6)

def decode_dec(val):
    """Decode compact Dec to degrees (signed)"""
    return round(val / DEC_DIVISOR, 6)

def decode_period(val):
    """Decode variable star period"""
    if val >= 32767:
        return None  # unknown
    if val >= 32766:
        return -1  # irregular
    if val >= 1000:
        return round((val - 900) * 0.1, 1)
    return round(val / 100.0, 2)

def extract_names(text, var_name):
    """Extract semicolon-separated name string from PROGMEM.
    Looks for the ENGLISH section."""
    # Find the English section
    eng_start = text.find('#if LANGUAGE == ENGLISH')
    if eng_start < 0:
        eng_start = 0
    eng_end = text.find('#elif', eng_start + 1)
    if eng_end < 0:
        eng_end = len(text)
    section = text[eng_start:eng_end]

    # Find the names variable
    pattern = rf'const\s+char\s+{re.escape(var_name)}\[\]\s+PROGMEM\s*=\s*'
    m = re.search(pattern, section)
    if m:
        # Offset within section -> offset within text
        start = eng_start + m.end()
    else:
        # Fallback: search in full text
        m = re.search(pattern, text)
        if not m:
            return []
        start = m.end()

    # Collect all string literals until semicolon terminator
    result = ''
    i = start
    while i < len(text):
        if text[i] == '"':
            j = i + 1
            while j < len(text) and text[j] != '"':
                if text[j] == '\\':
                    j += 1
                j += 1
            result += text[i+1:j]
            i = j + 1
        elif text[i] == ';':
            break
        else:
            i += 1

    if not result:
        return []
    # Split by semicolons, removing empty trailing entries
    names = result.split(';')
    while names and names[-1] == '':
        names.pop()
    return names

def extract_array(text, var_name):
    """Extract array of struct initializers from PROGMEM."""
    pattern = rf'const\s+\w+\s+{re.escape(var_name)}\[[\w\s]*\]\s+PROGMEM\s*=\s*\{{'
    m = re.search(pattern, text)
    if not m:
        return []

    start = m.end()
    # Find matching closing brace
    depth = 1
    i = start
    while i < len(text) and depth > 0:
        if text[i] == '{':
            depth += 1
        elif text[i] == '}':
            depth -= 1
        i += 1
    content = text[start:i-1]

    # Extract individual entries: { val, val, ... }
    entries = []
    for em in re.finditer(r'\{([^}]+)\}', content):
        vals = [v.strip() for v in em.group(1).split(',')]
        entries.append(vals)
    return entries

def parse_int(s):
    """Parse an integer, handling negative values"""
    s = s.strip()
    if s.startswith('-'):
        return -int(s[1:])
    return int(s)

def process_dso(header_file, output_name, cat_title, prefix, array_var, names_var, subid_var=None):
    """Process a DSO compact catalog (dso_comp_t)"""
    text = header_file.read_text(encoding='utf-8', errors='replace')
    names = extract_names(text, names_var)
    subids = extract_names(text, subid_var) if subid_var else []
    entries = extract_array(text, array_var)

    objects = []
    for vals in entries:
        if len(vals) < 8:
            continue
        it_name = int(vals[0])
        cons = int(vals[1])
        obj_type = int(vals[2])
        it_subid = int(vals[3])
        obj_id = int(vals[4])
        mag_raw = int(vals[5])
        ra_raw = int(vals[6])
        de_raw = parse_int(vals[7])

        obj = {
            'id': obj_id,
            'name': names[it_name - 1] if 0 < it_name <= len(names) else '',
            'ra': decode_ra(ra_raw),
            'dec': decode_dec(de_raw),
            'mag': decode_mag_compact(mag_raw),
            'type_idx': obj_type,
            'cons': cons,
        }
        if subids and 0 < it_subid <= len(subids):
            obj['subId'] = subids[it_subid - 1].strip()
        objects.append(obj)

    return {
        'catalog': cat_title,
        'prefix': prefix,
        'epoch': 2000,
        'type': 'dso',
        'objects': objects,
    }

def process_stars(header_file, output_name, cat_title, prefix, array_var, names_var, subid_var=None):
    """Process a star catalog (gen_star_vcomp_t)"""
    text = header_file.read_text(encoding='utf-8', errors='replace')
    names = extract_names(text, names_var)
    subids = extract_names(text, subid_var) if subid_var else []
    entries = extract_array(text, array_var)

    objects = []
    for i, vals in enumerate(entries):
        if len(vals) < 7:
            continue
        it_name = int(vals[0])
        cons = int(vals[1])
        bayer = int(vals[2])
        it_subid = int(vals[3])
        mag_raw = int(vals[4])
        ra_raw = int(vals[5])
        de_raw = parse_int(vals[6])

        obj = {
            'id': i + 1,
            'name': names[it_name - 1] if 0 < it_name <= len(names) else '',
            'ra': decode_ra(ra_raw),
            'dec': decode_dec(de_raw),
            'mag': decode_mag_compact(mag_raw),
            'cons': cons,
        }
        if subids and 0 < it_subid <= len(subids):
            obj['subId'] = subids[it_subid - 1].strip()
        objects.append(obj)

    return {
        'catalog': cat_title,
        'prefix': prefix,
        'epoch': 2000,
        'type': 'star',
        'objects': objects,
    }

def process_double(header_file, output_name, cat_title, prefix, array_var, names_var, subid_var=None):
    """Process a double star catalog (dbl_star_comp_t)"""
    text = header_file.read_text(encoding='utf-8', errors='replace')
    names = extract_names(text, names_var)
    subids = extract_names(text, subid_var) if subid_var else []
    entries = extract_array(text, array_var)

    objects = []
    for vals in entries:
        if len(vals) < 11:
            continue
        it_name = int(vals[0])
        cons = int(vals[1])
        bayer = int(vals[2])
        it_subid = int(vals[3])
        obj_id = int(vals[4])
        sep_raw = int(vals[5])
        pa_raw = int(vals[6])
        mag2_raw = int(vals[7])
        mag_raw = int(vals[8])
        ra_raw = int(vals[9])
        de_raw = parse_int(vals[10])

        obj = {
            'id': obj_id,
            'name': names[it_name - 1] if 0 < it_name <= len(names) else '',
            'ra': decode_ra(ra_raw),
            'dec': decode_dec(de_raw),
            'mag': decode_mag_compact(mag_raw),
            'cons': cons,
            'sep': round(sep_raw / 10.0, 1) if sep_raw < 9999 else None,
            'pa': pa_raw if pa_raw <= 360 else None,
            'mag2': decode_mag_compact(mag2_raw),
        }
        if subids and 0 < it_subid <= len(subids):
            obj['subId'] = subids[it_subid - 1].strip()
        objects.append(obj)

    return {
        'catalog': cat_title,
        'prefix': prefix,
        'epoch': 2000,
        'type': 'double',
        'objects': objects,
    }

def process_variable(header_file, output_name, cat_title, prefix, array_var, names_var, subid_var=None):
    """Process a variable star catalog (var_star_comp_t)"""
    text = header_file.read_text(encoding='utf-8', errors='replace')
    names = extract_names(text, names_var)
    subids = extract_names(text, subid_var) if subid_var else []
    entries = extract_array(text, array_var)

    objects = []
    for vals in entries:
        if len(vals) < 10:
            continue
        it_name = int(vals[0])
        cons = int(vals[1])
        bayer = int(vals[2])
        it_subid = int(vals[3])
        obj_id = int(vals[4])
        period_raw = int(vals[5])
        mag2_raw = int(vals[6])
        mag_raw = int(vals[7])
        ra_raw = int(vals[8])
        de_raw = parse_int(vals[9])

        obj = {
            'id': obj_id,
            'name': names[it_name - 1] if 0 < it_name <= len(names) else '',
            'ra': decode_ra(ra_raw),
            'dec': decode_dec(de_raw),
            'mag': decode_mag_compact(mag_raw),
            'cons': cons,
            'period': decode_period(period_raw),
            'magMin': decode_mag_compact(mag2_raw),
        }
        if subids and 0 < it_subid <= len(subids):
            obj['subId'] = subids[it_subid - 1].strip()
        objects.append(obj)

    return {
        'catalog': cat_title,
        'prefix': prefix,
        'epoch': 2000,
        'type': 'variable',
        'objects': objects,
    }


# Catalog definitions: (header_file, output_name, title, prefix, array_var, names_var, subid_var, processor)
CATALOGS = [
    ('ta_messier_c.h',      'messier',  'Messier',      'M',     'Cat_Messier',      'Cat_Messier_Names',  'Cat_Messier_SubId',  process_dso),
    ('ta_caldwell_c.h',     'caldwell', 'Caldwell',     'C',     'Cat_Caldwell',     'Cat_Caldwell_Names', 'Cat_Caldwell_SubId', process_dso),
    ('ta_herschel_c.h',     'herschel', 'Herschel 400', 'H',     'Cat_Herschel',     'Cat_Herschel_Names', 'Cat_Herschel_SubId', process_dso),
    ('ta_ngc_select_c.h',   'ngc',      'NGC',          'NGC',   'Cat_NGC',          'Cat_NGC_Names',      'Cat_NGC_SubId',      process_dso),
    ('ta_ic_select_c.h',    'ic',       'IC',           'IC',    'Cat_IC',           'Cat_IC_Names',       'Cat_IC_SubId',       process_dso),
    ('ta_stars_vc.h',       'stars',    'Bright Stars', 'Star ', 'Cat_Stars',        'Cat_Stars_Names',    'Cat_Stars_SubId',    process_stars),
    ('ta_stf_select_c.h',   'stf',      'STF Doubles',  'STF',   'Cat_STF',          'Cat_STF_Names',      'Cat_STF_SubId',      process_double),
    ('ta_stt_select_c.h',   'stt',      'STT Doubles',  'STT',   'Cat_STT',          'Cat_STT_Names',      'Cat_STT_SubId',      process_double),
    ('ta_gcvs_select_c.h',  'gcvs',     'GCVS Variables','GCVS', 'Cat_GCVS',         'Cat_GCVS_Names',     'Cat_GCVS_SubId',     process_variable),
]


def main():
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    total = 0
    for header_name, out_name, title, prefix, arr_var, names_var, subid_var, processor in CATALOGS:
        header_path = CATALOG_DIR / header_name
        if not header_path.exists():
            print(f"  SKIP: {header_name} not found")
            continue

        print(f"  Processing {header_name} -> {out_name}.json ...", end=' ')
        try:
            data = processor(header_path, out_name, title, prefix, arr_var, names_var, subid_var)
            count = len(data['objects'])
            total += count

            out_path = OUTPUT_DIR / f"{out_name}.json"
            with open(out_path, 'w', encoding='utf-8') as f:
                json.dump(data, f, indent=1, ensure_ascii=False)
            print(f"{count} objects")
        except Exception as e:
            print(f"ERROR: {e}")

    print(f"\nDone. {total} total objects extracted to {OUTPUT_DIR}")


if __name__ == '__main__':
    main()
