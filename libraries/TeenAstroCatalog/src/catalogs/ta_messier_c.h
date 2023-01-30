#pragma once
// This data is machine generated from the Open NGC database.
// Do NOT edit this data manually. Rather, fix the import programs and rerun.
#define Cat_Messier_Title "Messier"
#define Cat_Messier_Prefix "M"
#define NUM_MESSIER 109

const char *Cat_Messier_Names_EN=
"Crab Nebula;"
"Butterfly Clstr;"
"Ptolemy Clstr;"
"Lagoon Nebula;"
"Wild Duck Clstr;"
"Herc. Globlr Clstr;"
"Great Pegasus Clstr;"
"Eagle Nebula;"
"Omega Nebula;"
"Trifid Nebula;"
"Sagitt. Clstr;"
"Sagitt. Star Cloud;"
"Dumbbell Nebula;"
"Andromeda Gxy;"
"Triangulum Gxy;"
"Pinwheel Clstr;"
"Starfish Clstr;"
"Winnecke 4;"
"Orion Nebula;"
"De Mairan's Nebula;"
"Beehive Clstr;"
"Pleiades;"
"Whirlpool Gxy;"
"Ring Nebula;"
"Sunflower Gxy;"
"Black Eye Gxy;"
"Leo Triplet Gxy;"
"Hamburger Gxy;"
"Little Dumbbell Nbla;"
"Cetus A;"
"Bode's Gxy;"
"Cigar Gxy;"
"South. Pinwheel Gxy;"
"Virgo A;"
"Owl Nebula;"
"Pinwheel Gxy;"
"Sombrero Gxy";

const char *Cat_Messier_Names_FR=
"Nébuleuse du Crabe;"
"Amas du Papillon;"
"Amas de Ptolémée;"
"Nébuleuse de la Lagune;"
"Amas du Canard Sauvage;"
"Grand amas d'Hercule;"
"Nuage de Pégase;"
"Nébuleuse de l'Aigle;"
"Omega - Nébuleuse du Cygne;"
"Nébuleuse Trifide;"
"Amas du Sagittaire;"
"Nuage d'étoiles du Sagittaire;"
"Nébuleuse de l'Haltère;"
"Galaxie d'Andromède;"
"Galaxie du Triangle;"
"Amas du Moulinet;"
"Amas de la Méduse;"
"Winnecke 4;"
"Nébuleuse d'Orion;"
"Nébuleuse de Mairan;"
"Amas de la Ruche;"
"Pléiades;"
"Galaxie du Tourbillon;"
"Nébuleuse de l'Anneau;"
"Galaxie du Tournesol;"
"Galaxie de l'Œil noir;"
"Triplet de Leo;"
"Galaxie du Hamburger5;"
"Nébuleuse du Petit Haltère;"
"Cetus A.;"
"Galaxie de Bode;"
"Galaxie du Cigare;"
"Galaxie Pin-wheel;"
"Virgo A.;"
"Nébuleuse du Hibou;"
"Galaxie du Moulinet;"
"Galaxie du Sombrero;";

const char *Cat_Messier_Names_DE=
"Krabbennebel;"
"Schmetterlingshaufen;"
"Ptolemäus-Haufen";
"Lagunennebel;"
"Wildenten-Haufen;"
"Großer Herkules-Cluster;"
"Pegasus-Wolke;"
"Adlernebel;"
"Omega - Schwanennebel;".
"Trifidnebel;"
"Sternhaufen des Schützen;"
"Sternwolke des Schützen;"
"Hantelnebel;"
"Andromeda-Galaxie;"
"Dreiecksgalaxie;"
"Mühle-Haufen;"
"Quallenhaufen;"
"Winnecke 4;"
"Orionnebel;"
"Mairan-Nebel;"
"Bienenstockhaufen;"
"Plejaden;"
"Wirbelwind-Galaxie;"
"Ringnebel;"
"Sonnenblumen-Galaxie;"
"Galaxie des Schwarzen Auges;"
"Leo-Triplett;"
"Hamburger Galaxie5;"
"Kleiner Hantelnebel;"
"Cetus A.;"
"Bode-Galaxie;"
"Zigarren-Galaxie;"
"Pin-wheel-Galaxie;"
"Virgo A.;"
"Eulennebel;"
"Windrad-Galaxie;"
"Sombrero-Galaxie;"

const char *Cat_Messier_SubId=
" NGC1952;"
" NGC7089;"
" NGC5272;"
" NGC6121;"
" NGC5904;"
" NGC6405;"
" NGC6475;"
" NGC6523;"
" NGC6333;"
" NGC6254;"
" NGC6705;"
" NGC6218;"
" NGC6205;"
" NGC6402;"
" NGC7078;"
" NGC6611;"
" NGC6618;"
" NGC6613;"
" NGC6273;"
" NGC6514;"
" NGC6531;"
" NGC6656;"
" NGC6494;"
" IC4715;"
" IC4725;"
" NGC6694;"
" NGC6853;"
" NGC6626;"
" NGC6913;"
" NGC7099;"
" NGC224;"
" NGC221;"
" NGC598;"
" NGC1039;"
" NGC2168;"
" NGC1960;"
" NGC2099;"
" NGC1912;"
" NGC7092;"
" ;"
" NGC2287;"
" NGC1976;"
" NGC1982;"
" NGC2632;"
" Mel22;"
" NGC2437;"
" NGC2422;"
" NGC2548;"
" NGC4472;"
" NGC2323;"
" NGC5194;"
" NGC7654;"
" NGC5024;"
" NGC6715;"
" NGC6809;"
" NGC6779;"
" NGC6720;"
" NGC4579;"
" NGC4621;"
" NGC4649;"
" NGC4303;"
" NGC6266;"
" NGC5055;"
" NGC4826;"
" NGC3623;"
" NGC3627;"
" NGC2682;"
" NGC4590;"
" NGC6634;"
" NGC6681;"
" NGC6838;"
" NGC6981;"
" NGC6994;"
" NGC628;"
" NGC6864;"
" NGC650;"
" NGC1068;"
" NGC2068;"
" NGC1904;"
" NGC6093;"
" NGC3031;"
" NGC3034;"
" NGC5236;"
" NGC4374;"
" NGC4382;"
" NGC4406;"
" NGC4486;"
" NGC4501;"
" NGC4552;"
" NGC4569;"
" NGC4548;"
" NGC6341;"
" NGC2447;"
" NGC4736;"
" NGC3351;"
" NGC3368;"
" NGC3587;"
" NGC4192;"
" NGC4254;"
" NGC4321;"
" NGC5457;"
" NGC581;"
" NGC4594;"
" NGC3379;"
" NGC4258;"
" NGC6171;"
" NGC3556;"
" NGC3992;"
" NGC205;";

CAT_TYPES Cat_Messier_Type=CAT_DSO_COMP;
const dso_comp_t Cat_Messier[NUM_MESSIER] PROGMEM = {
  {     1, 77, 15,     1,   1,  109, 15225,   8015  },
  {     0,  4,  8,     2,   2,   88, 58866,   -300  },
  {     0, 11,  8,     3,   3,   89, 37419,  10331  },
  {     0, 71,  8,     4,   4,   79, 44764,  -9657  },
  {     0, 73,  8,     5,   5,   84, 41805,    758  },
  {     2, 71,  1,     6,   6,   67, 48257, -11743  },
  {     3, 71,  1,     7,   7,   58, 48872, -12667  },
  {     4, 76, 10,     8,   8,   83, 49320,  -8876  },
  {     0, 58,  8,     9,   9,  109, 47295,  -6741  },
  {     0, 58,  8,    10,  10,   75, 46292,  -1492  },
  {     5, 72,  1,    11,  11,   83, 51478,  -2283  },
  {     0, 58,  8,    12,  12,   86, 45841,   -709  },
  {     6, 39,  8,    13,  13,   83, 45588,  13275  },
  {     0, 58,  8,    14,  14,   82, 48133,  -1182  },
  {     7, 61,  8,    15,  15,   88, 58708,   4430  },
  {     8, 73, 10,    16,  16,   85, 50008,  -5027  },
  {     9, 76, 10,    17,  17,   95, 50098,  -5888  },
  {     0, 76,  1,    18,  18,   94, 50061,  -6226  },
  {     0, 58,  8,    19,  19,   81, 46541,  -9564  },
  {    10, 76, 10,    20,  20,  110, 49275,  -8364  },
  {     0, 76,  1,    21,  21,   84, 49344,  -8188  },
  {    11, 76,  8,    22,  22,   87, 50809,  -8703  },
  {     0, 76,  1,    23,  23,   80, 49019,  -6912  },
  {    12, 76, 13,    24,  24,   70, 49923,  -6741  },
  {     0, 76,  1,    25,  25,   71, 50598,  -6959  },
  {     0, 72,  1,    26,  26,  114, 51214,  -3416  },
  {    13, 87,  9,    27,  27,   99, 54595,   8272  },
  {     0, 76,  8,    28,  28,   94, 50269,  -9055  },
  {     0, 30,  1,    29,  29,   91, 55704,  14020  },
  {     0, 14,  8,    30,  30,   96, 59181,  -8439  },
  {    14,  0,  0,    31,  31,   59,  1945,  15025  },
  {     0,  0,  0,    32,  32,  106,  1943,  14878  },
  {    15, 80,  0,    33,  33,   82,  4271,  11163  },
  {     0, 62,  1,    34,  34,   77,  7379,  15563  },
  {     0, 37,  1,    35,  35,   76, 16797,   8861  },
  {    16,  7,  1,    36,  36,   85, 15305,  12430  },
  {     0,  7,  1,    37,  37,   81, 16034,  11852  },
  {    17,  7,  1,    38,  38,   89, 14960,  13054  },
  {     0, 30,  1,    39,  39,   71, 58792,  17635  },
  {    18, 82,  3,    40,  40,  105, 33781,  21147  },
  {     0,  9,  1,    41,  41,   70, 18478,  -7556  },
  {    19, 59, 12,    42,  42,   65, 15259,  -1962  },
  {    20, 59, 11,    43,  43,  115, 15270,  -1918  },
  {    21, 22,  1,    44,  44,   56, 23683,   7162  },
  {    22, 77,  1,    45,  45,   37, 10353,   8776  },
  {     0, 67,  1,    46,  46,   86, 21016,  -5392  },
  {     0, 67,  1,    47,  47,   69, 20780,  -5273  },
  {     0, 41,  1,    48,  48,   83, 22470,  -2094  },
  {     0, 85,  0,    49,  49,  147, 34123,   2913  },
  {     0, 54,  1,    50,  50,   84, 19236,  -3045  },
  {    23, 11,  0,    51,  51,  109, 36859,  17183  },
  {     0, 16,  1,    52,  52,   94, 63934,  22425  },
  {     0, 24,  8,    53,  53,  103, 36087,   6615  },
  {     0, 76,  8,    54,  54,  102, 51658, -11097  },
  {     0, 76,  8,    55,  55,   90, 53703, -11273  },
  {     0, 51,  8,    56,  56,  109, 52638,  10990  },
  {    24, 51,  9,    57,  57,  113, 51591,  12025  },
  {     0, 85,  0,    58,  58,  122, 34485,   4303  },
  {     0, 85,  0,    59,  59,  121, 34681,   4240  },
  {     0, 85,  0,    60,  60,  113, 34755,   4206  },
  {     0, 85,  0,    61,  61,  121, 33765,   1629  },
  {     0, 58,  8,    62,  62,   99, 46476, -10963  },
  {    25, 11,  0,    63,  63,  111, 36219,  15302  },
  {    26, 24,  0,    64,  64,  110, 35350,   7894  },
  {    27, 46,  0,    65,  65,  118, 30899,   4767  },
  {    28, 46,  0,    66,  66,  114, 30959,   4730  },
  {     0, 22,  1,    67,  67,   94, 24182,   4300  },
  {     0, 41,  8,    68,  68,  105, 34564,  -9737  },
  {     0, 76,  8,    69,  69,  108, 50580, -11777  },
  {     0, 76,  8,    70,  70,  116, 51119, -11757  },
  {     0, 75,  8,    71,  71,   86, 54330,   6837  },
  {     0,  4,  8,    72,  72,  115, 57047,  -4564  },
  {     0,  4,  4,    73,  73,  114, 57295,  -4600  },
  {     0, 66,  0,    74,  74,  120,  4401,   5746  },
  {     0, 76,  8,    75,  75,  108, 54890,  -7981  },
  {    29, 62,  9,    76,  76,  126,  4657,  18777  },
  {    30, 19,  0,    77,  77,  114,  7404,     -5  },
  {     0, 59, 14,    78,  78,  105, 15782,     29  },
  {     0, 47,  8,    79,  79,  107, 14754,  -8929  },
  {     0, 71,  8,    80,  80,   98, 44466,  -8365  },
  {    31, 82,  0,    81,  81,   94, 27104,  25145  },
  {    32, 82,  0,    82,  82,  109, 27119,  25369  },
  {    33, 41,  0,    83,  83,  100, 37183, -10873  },
  {     0, 85,  0,    84,  84,  130, 33909,   4692  },
  {     0, 24,  0,    85,  85,  116, 33924,   6623  },
  {     0, 85,  0,    86,  86,  114, 33960,   4713  },
  {    34, 85,  0,    87,  87,  111, 34171,   4511  },
  {     0, 24,  0,    88,  88,  157, 34224,   5250  },
  {     0, 85,  0,    89,  89,  123, 34391,   4571  },
  {     0, 85,  0,    90,  90,  120, 34444,   4792  },
  {     0, 24,  0,    91,  91,  161, 34381,   5278  },
  {     0, 39,  8,    92,  92,   90, 47201,  15705  },
  {     0, 67,  1,    93,  93,   87, 21139,  -8684  },
  {     0, 11,  0,    94,  94,  107, 35084,  14971  },
  {     0, 46,  0,    95,  95,  122, 29307,   4261  },
  {     0, 46,  0,    96,  96,  118, 29435,   4303  },
  {    35, 82,  9,    97,  97,  124, 30711,  20031  },
  {     0, 24,  0,    98,  98,  126, 33396,   5425  },
  {     0, 24,  0,    99,  99,  124, 33625,   5249  },
  {     0, 24,  0,   100, 100,  119, 33811,   5760  },
  {    36, 82,  0,   101, 101,  104, 38375,  19787  },
  {     0, 16,  1,   102, 103,   99,  4249,  22084  },
  {    37, 85,  0,   103, 104,  105, 34588,  -4232  },
  {     0, 46,  0,   104, 105,  123, 29483,   4581  },
  {     0, 11,  0,   105, 106,  109, 33631,  17222  },
  {     0, 58,  8,   106, 107,  114, 45171,  -4753  },
  {     0, 82,  0,   107, 108,  126, 30561,  20270  },
  {     0, 82,  0,   108, 109,  124, 32659,  19432  },
  {     0,  0,  0,   109, 110,  106,  1837,  15177  },
};
