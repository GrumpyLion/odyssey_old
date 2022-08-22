#pragma once

#define PRAGMA_DEOPTIMIZE __pragma(optimize( "", off ))
#define PRAGMA_REOPTIMIZE __pragma(optimize( "", on ))
