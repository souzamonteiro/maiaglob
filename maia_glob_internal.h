/* Internal portable helpers; not a separate public interface. */
#ifndef MAIA_GLOB_INTERNAL_H
#define MAIA_GLOB_INTERNAL_H
#include "maia_glob.h"
#ifdef __cplusplus
extern "C" {
#endif
int maia_glob_utf8_next(const unsigned char *, unsigned int, unsigned int *, unsigned int *);
int maia_glob_limits_valid(const MaiaGlobLimits *);
#ifdef __cplusplus
}
#endif
#endif
