#include "fonts.h"

// Include font data headers - these define const data with internal linkage
#include "firasans.h"
#include "firasans_lg.h"
#include "firasans_sm.h"

// Export via pointers (external linkage)
const GFXfont* FONT_LG = &firasans_lg;
const GFXfont* FONT_MD = &FiraSans;
const GFXfont* FONT_SM = &firasans_sm;
