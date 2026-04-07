#ifndef _WINDS1EDIT_TYPES_H_

#define _WINDS1EDIT_TYPES_H_

typedef unsigned char      UBYTE;

/*
 * WORD: d2-ds1-edit defines as signed short, D2Shared.hpp as unsigned short.
 * In C builds (EditorLib), use the original signed definition.
 * In C++ builds (D2Client), use unsigned short to match D2Shared.hpp.
 */
#ifdef __cplusplus
  #ifndef _D2SHARED_WORD_DEFINED
  #define _D2SHARED_WORD_DEFINED
  typedef unsigned short int WORD;
  #endif
#else
  typedef short int          WORD;
#endif

typedef unsigned short int UWORD;
typedef unsigned long      UDWORD;

// Disable warning message : warning C4996: 'sprintf': This function or variable may be unsafe. Consider using sprintf_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
#pragma warning(disable : 4996)

#endif
