// readFile.h - Prototype file for readFile.c.
//
// Author: Ron Charlton
// Date:   2019-06-21
// This file is public domain.  Public domain is per CC0 1.0; see
// https://creativecommons.org/publicdomain/zero/1.0/ for information.
//
// "$Id: readFile.h,v 1.5 2023-05-21 15:59:44-04 ron Exp $";

#ifndef READALLFILE_H_
#define READALLFILE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

char *readFile(const char *filename, int modeBinary, int terminate, size_t maxsize,
		size_t *length);

char **readLines(const char *fileName, size_t maxSize, size_t *lineCount);
void freeLines(char **lines);

#ifdef __cplusplus
}
#endif

#endif	// READALLFILE_H_
