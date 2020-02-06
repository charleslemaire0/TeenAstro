#pragma once

#define ENGLISH 0
#define FRENCH 1
#define GERMAN 2
#define LANGUAGE ENGLISH


#if LANGUAGE == ENGLISH
#include "SHC_text_English.h"
#elif LANGUAGE == FRENCH
#include "SHC_text_French.h"
#elif LANGUAGE == GERMAN
#include "SHC_text_German.h"
#else
#include "SHC_text_English.h"
#endif