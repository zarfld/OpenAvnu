/*
 * Open1722 MSVC Compatibility Header
 * 
 * This header provides compatibility definitions to allow Open1722 
 * to compile with Microsoft Visual C++ compiler.
 * 
 * This file is forcibly included before all Open1722 source files
 * when building with MSVC.
 */

#ifndef OPEN1722_MSVC_COMPAT_H
#define OPEN1722_MSVC_COMPAT_H

#ifdef _MSC_VER

/* Redefine GCC-specific __attribute__ syntax for MSVC.
 * MSVC doesn't support __attribute__, so we remove it entirely.
 * Structure packing is handled via /Zp1 compiler flag instead.
 */
#define __attribute__(x)

/* Provide errno constants if not available */
#ifndef EINVAL
#define EINVAL 22
#endif

#ifndef EPERM
#define EPERM 1
#endif

/* Only define ENOBUFS if it's not already defined in errno.h */
#include <errno.h>
#ifndef ENOBUFS
#define ENOBUFS 105
#endif

/* For flexible array members (array[0] at end of struct).
 * MSVC allows this but issues warnings, which we suppress via compiler flags.
 */

/* Additional compatibility definitions */
#ifdef __cplusplus
extern "C" {
#endif

/* Any additional function declarations or type redefinitions can go here */

#ifdef __cplusplus
}
#endif

#endif /* _MSC_VER */

#endif /* OPEN1722_MSVC_COMPAT_H */
