// --------------------------------------------------------------------------------
// Data for different catalogs, each is a collection of certain celestial objects
// ** In this file is everything you need touch to add a new "TeenAstroCatalog.h"         **
// ** file and use it within the SHC.                                            **

// Simply include the catalog you want here and add a line for it in the section below, up to 64 catalogs are supported.
// Each catalog type has the possiblity of several different combinations depending on the _select, _c, and _vc suffixes available.
// For example:
// ngc.h          // for the full ngc catalog at the highest accuracy.
// ngc_vc.h       // for a nearly complete ngc catalog at somewhat reduced accuracy, but much smaller size (50% of above.)
// ngc_select_c.h // for a selection of the brighter objects from the ngc catalog at somewhat reduced accuracy.

// Note: You can navigate to and open the SmartHandController's catalogs directory in the Arduino IDE to see the available catalogs.

  #include "catalogs/ta_stars_vc.h"        // Catalog of 408 bright stars
  #include "catalogs/ta_stf_select_c.h"    // Struve STF catalog, limited to 595 double stars brighter than Magnitude 8.5
  #include "catalogs/ta_stt_select_c.h"    // Struve STT catalog, limited to 114 double stars brighter than Magnitude 8.5
  #include "catalogs/ta_gcvs_select_c.h"   // General Catalog of Variable Stars, limited to 621 stars brighter than Magnitude 10 w/ a difference in magnitude of >1
////#include "catalogs/carbon.h"          // Carbon Variable Stars, S&T list of 101 stars
  #include "catalogs/ta_messier_c.h"       // Charles Messier's famous catalog of 109 DSO's
  #include "catalogs/ta_caldwell_c.h"      // The Caldwell (supplement) catalog of 109 DSO's
  #include "catalogs/ta_herschel_c.h"      // Herschel's "400 best of the NGC" catalog
//  #include "catalogs/ta_collinder_vc.h"    // The Collinder catalog of 471 open clusters
  #include "catalogs/ta_ngc_select_c.h"    // The New General Catalog of 2367 DSO's, limited to Magnitude 16 AND with surface brightness >= 22.5m/arc-sec^2
  #include "catalogs/ta_ic_select_c.h"     // The Index Catalog (supplement) of 473 DSO's, limited to Magnitude 16 AND with surface brightness >= 22.5m/arc-sec^2


// Note: There should be a matching line below for every catalog #included above (catalogs appear in the menus in the order the appear below):
//catalog_t catalog[] = {
//// Note: Alignment always uses the first catalog!
//// Note: Sub Menu items should be grouped together in this list!
//// Sub Menu     Title               Prefix               Num records   Catalog data  Catalog name string  Catalog subId string  Type                Epoch
//  {"Stars>"     Cat_Stars_Title,    Cat_Stars_Prefix,    NUM_STARS,    Cat_Stars,    Cat_Stars_Names,     Cat_Stars_SubId,      Cat_Stars_Type,     2000, 0},
//  {"Stars>"     Cat_STF_Title,      Cat_STF_Prefix,      NUM_STF,      Cat_STF,      Cat_STF_Names,       Cat_STF_SubId,        Cat_STF_Type,       2000, 0},
//  {"Stars>"     Cat_STT_Title,      Cat_STT_Prefix,      NUM_STT,      Cat_STT,      Cat_STT_Names,       Cat_STT_SubId,        Cat_STT_Type,       2000, 0},
//  {"Stars>"     Cat_GCVS_Title,     Cat_GCVS_Prefix,     NUM_GCVS,     Cat_GCVS,     Cat_GCVS_Names,      Cat_GCVS_SubId,       Cat_GCVS_Type,      2000, 0},
////{"Stars>"     Cat_Carbon_Title,   Cat_Carbon_Prefix,   NUM_CARBON,   Cat_Carbon,   Cat_Carbon_Names,    Cat_Carbon_SubId,     Cat_Carbon_Type,    2000, 0},
//  {"Deep Sky>"  Cat_Messier_Title,  Cat_Messier_Prefix,  NUM_MESSIER,  Cat_Messier,  Cat_Messier_Names,   Cat_Messier_SubId,    Cat_Messier_Type,   2000, 0},
//  {"Deep Sky>"  Cat_Caldwell_Title, Cat_Caldwell_Prefix, NUM_CALDWELL, Cat_Caldwell, Cat_Caldwell_Names,  Cat_Caldwell_SubId,   Cat_Caldwell_Type,  2000, 0},
//  {"Deep Sky>"  Cat_Herschel_Title, Cat_Herschel_Prefix, NUM_HERSCHEL, Cat_Herschel, Cat_Herschel_Names,  Cat_Herschel_SubId,   Cat_Herschel_Type,  2000, 0},
//  {"Deep Sky>"  Cat_Collinder_Title,Cat_Collinder_Prefix,NUM_COLLINDER,Cat_Collinder,Cat_Collinder_Names, Cat_Collinder_SubId,  Cat_Collinder_Type, 2000, 0},
//  {"Deep Sky>"  Cat_NGC_Title,      Cat_NGC_Prefix,      NUM_NGC,      Cat_NGC,      Cat_NGC_Names,       Cat_NGC_SubId,        Cat_NGC_Type,       2000, 0},
//  {"Deep Sky>"  Cat_IC_Title,       Cat_IC_Prefix,       NUM_IC,       Cat_IC,       Cat_IC_Names,        Cat_IC_SubId,         Cat_IC_Type,        2000, 0},
//  {             "",                 "",                  0,            NULL,         NULL,                NULL,                 CAT_NONE,           0,    0}
//};

catalog_t catalog[] = {
// Note: Alignment always uses the first catalog!
// Note: Sub Menu items should be grouped together in this list!
// Sub Menu     Title               Prefix               Num records   Catalog data  Catalog name string  Catalog subId string  Type                Epoch
  {"Stars>"     Cat_Stars_Title,    Cat_Stars_Prefix,    NUM_STARS,    Cat_Stars,    Cat_Stars_Names,     Cat_Stars_SubId,      Cat_Stars_Type,     2000, 0},
  {"Stars>"     Cat_STF_Title,      Cat_STF_Prefix,      NUM_STF,      Cat_STF,      Cat_STF_Names,       Cat_STF_SubId,        Cat_STF_Type,       2000, 0},
  {"Stars>"     Cat_STT_Title,      Cat_STT_Prefix,      NUM_STT,      Cat_STT,      Cat_STT_Names,       Cat_STT_SubId,        Cat_STT_Type,       2000, 0},
  {"Stars>"     Cat_GCVS_Title,     Cat_GCVS_Prefix,     NUM_GCVS,     Cat_GCVS,     Cat_GCVS_Names,      Cat_GCVS_SubId,       Cat_GCVS_Type,      2000, 0},
////{"Stars>"     Cat_Carbon_Title,   Cat_Carbon_Prefix,   NUM_CARBON,   Cat_Carbon,   Cat_Carbon_Names,    Cat_Carbon_SubId,     Cat_Carbon_Type,    2000, 0},
  {"Deep Sky>"  Cat_Messier_Title,  Cat_Messier_Prefix,  NUM_MESSIER,  Cat_Messier,  Cat_Messier_Names,   Cat_Messier_SubId,    Cat_Messier_Type,   2000, 0},
  {"Deep Sky>"  Cat_Caldwell_Title, Cat_Caldwell_Prefix, NUM_CALDWELL, Cat_Caldwell, Cat_Caldwell_Names,  Cat_Caldwell_SubId,   Cat_Caldwell_Type,  2000, 0},
  {"Deep Sky>"  Cat_Herschel_Title, Cat_Herschel_Prefix, NUM_HERSCHEL, Cat_Herschel, Cat_Herschel_Names,  Cat_Herschel_SubId,   Cat_Herschel_Type,  2000, 0},
//  {"Deep Sky>"  Cat_Collinder_Title,Cat_Collinder_Prefix,NUM_COLLINDER,Cat_Collinder,Cat_Collinder_Names, Cat_Collinder_SubId,  Cat_Collinder_Type, 2000, 0},
  {"Deep Sky>"  Cat_NGC_Title,      Cat_NGC_Prefix,      NUM_NGC,      Cat_NGC,      Cat_NGC_Names,       Cat_NGC_SubId,        Cat_NGC_Type,       2000, 0},
  {"Deep Sky>"  Cat_IC_Title,       Cat_IC_Prefix,       NUM_IC,       Cat_IC,       Cat_IC_Names,        Cat_IC_SubId,         Cat_IC_Type,        2000, 0},
  {             "",                 "",                  0,            NULL,         NULL,                NULL,                 CAT_NONE,           0,    0}
};