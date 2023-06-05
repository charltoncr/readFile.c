/* readFile.c - Read the entire contents of a file into a malloc'ed buffer.
 *
 * Author: Ron Charlton
 * Date:   2019-06-21
 * This file is public domain.  Public domain is per CC0 1.0; see
 * <https://creativecommons.org/publicdomain/zero/1.0/> for information.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
	// Microsoft C (Windows)
	#include <malloc.h>		// Microsoft help says realloc requires it.
#endif

#include "readFile.h"

#ifdef __cplusplus
extern "C" {
#endif

char rcsid_readFile[] =
		"$Id: readFile.c,v 2.38 2023-06-05 08:21:09-04 ron Exp $";

#ifdef _MSC_VER
	#define fseek _fseeki64
	#define ftell _ftelli64
#else
	#define errno_t int
#endif

#ifdef TRUE
    #undef TRUE
#endif
#define TRUE 1


static inline char *Strchr(const char *s, int c);

/* faster than macOS's clang v14.0.3 strchr on a 3.2 GHz M1 Mac mini */
static inline char *
Strchr(const char *s, int c)
{
	const char c1 = c;

    // loop unrolled for speed
	for (; *s != c1; ++s) {
		if (!*s) return NULL;
        if (*++s == c) break;
        if (!*s) return NULL;
        if (*++s == c) break;
        if (!*s) return NULL;
    }

	return (char *)s;
}


/*
readFile reads the entire contents of the file named "fileName" into a
newly-malloc'ed buffer and returns a pointer to the buffer.  Include
readFile.h before calling readFile.  The maximum size of the file is
SIZE_MAX as defined in limits.h.
ARGUMENTS
---------
  Inputs:
	fileName    the name of the file to read
	textMode    Read in text mode if non-zero, or in binary mode if zero.
                If textMode is non-zero then carriage returns are removed
                and terminate is forced to TRUE.
	terminate   If terminate is non-zero then a terminating '\0' is added after
				the end of the file's contents in the buffer.
	maxSize		If maxSize is non-zero its value sets a limit on the 
				returned buffer size, measured in chars.  If maxSize is
				non-zero and inadequate then errno is set to EFBIG and NULL
				is returned.  If maxSize is zero then there is no limit.
  Outputs:	
	length      if length is non-NULL then the length of the data in chars
				(excluding any added terminating '\0') is stored in *length.
RETURN VALUE
------------
readFile returns a pointer to a newly malloc'ed data buffer or, if an error
occurs, it sets errno and returns NULL.  The caller should free the returned
buffer when it is no longer needed, even if *length is 0. 
*/
char *
readFile(const char *fileName, int textMode, int terminate, size_t maxSize,
		size_t *length)
{
	FILE *in;			// input file stream pointer
	size_t fsize;		// file size
	size_t n = 0;		// length in chars of the data as read from the file
	char *buf = NULL;	// points to retrieved data
	errno_t e = errno;	// remember errno to restore it if no error
    size_t t;           // space for terminating '\0'

	if (!fileName) {
		errno = EINVAL;
	} else {
        if (textMode)
            terminate = TRUE;
		t = !!terminate;	// allowance for terminator if required
		errno = 0;
		#ifdef _MSC_VER
		  if (!fopen_s(&in, fileName, "rb") && in) {
		#else
		  if ((in = fopen(fileName, "rb"))) {
		#endif
			if (!fseek(in, 0, SEEK_END)) {
				fsize = ftell(in);
				if (!maxSize || (fsize + t) <= maxSize) {
					buf = malloc(fsize + t);
					if (buf) {
    					rewind(in);
						n = fread(buf, 1, fsize, in);
						if (!ferror(in)) {
                            if (terminate)
								buf[n] = '\0';  // for Strchr below
                            if (textMode) {
                                char *s = Strchr(buf, '\r');
                                if (s) {
                                    char *d = s, *end = buf + n;
                                    for (; s < end; ++s)
                                        if (*s != '\r')
                                            *d++ = *s;
                                    n = d - buf;
                                    if (terminate)
                                        buf[n] = '\0';
                                }
                            }
							if (n < fsize) {
								char *b = realloc(buf, n + t);
								if (b)
									buf = b;
								else
									errno = 0;	// let original buf be returned
							}
						}
					}
				} else
					errno = EFBIG;
			}
			fclose(in);
		}
	}

	if (errno) {
		n = 0;
		free(buf);
		buf = NULL;
	} else
		errno = e;

	if (length)
		*length = n;

	return buf;
}


/*
readLines reads the entire contents of the file named fileName into a
newly-malloc'ed buffer and returns an argv-like array of pointers to the
lines found in the buffer.  Carriage returns and linefeeds are excluded
from the lines.  Each line is terminated with '\0'.  Include readFile.h
before calling readLines or freeLines.  The caller should call freeLines
when the lines are no longer needed, even if *lineCount is 0.  The maximum
size of the file is SIZE_MAX as defined in limits.h.
ARGUMENTS
---------
  Inputs:
	fileName    the name of the text file to read
	maxSize		If maxSize is non-zero its value sets a limit on the 
				total memory allocated by readLines, measured in chars.
                If maxSize is non-zero and inadequate then errno is set to
                EFBIG and NULL is returned.  If maxSize is zero then there
                is no limit.
  Outputs:	
	lineCount   if lineCount is non-NULL then the number of lines is stored
                in *lineCount.
RETURN VALUE
------------
readLines returns a pointer to an argv-like array of pointers to the lines or,
if an error occurs, it sets errno and returns NULL.
The returned line pointers are followed by a NULL pointer, similar to argv.
*/
char **
readLines(const char *fileName, size_t maxSize, size_t *lineCount)
{
    char *buf;              // pointer to text returned by readFile
    size_t length;          // length  of text returned by readFile
    char *p;                // scratch pointer
    size_t lnCnt = 0;       // local line count
    size_t linesSize;       // space required for line pointers
    char **lines = NULL;    // argv-like array of lines found in fileName
	errno_t e = errno;	    // remember errno to restore it if no error

    errno = 0;
    buf = readFile(fileName, 1, 1, maxSize, &length);
    if (buf) {
        lnCnt += length && buf[length-1] != '\n'; // for file without final '\n'
        for (p = buf; (p = Strchr(p, '\n')); ++p)
            ++lnCnt;
        linesSize = (lnCnt + 1) * sizeof(*lines);
        if (!maxSize || (linesSize+length+1) <= maxSize) {
            lines = malloc(linesSize);
            if (lines) {
                char **ln = lines;
                if (length) {
                    *ln++ = buf; // first line
                    for (p = buf; (p = Strchr(p, '\n'));) {
                        *p++ = '\0';
                        if (*p)
                            *ln++ = p;
                    }
                } else {
                    free(buf);
                    buf = NULL;
                }
                *ln = NULL;
            }
        } else {
            errno = EFBIG;
        }
	}

	if (errno) {
        lnCnt = 0;
		free(buf);
		buf = NULL;
	} else
		errno = e;

	if (lineCount)
		*lineCount = lnCnt;

	return lines;
}

/*
freeLines frees memory allocated by readLines.
ARGUMENTS
---------
  Inputs:
    lines   a pointer to lines allocated by readLines.  A NULL value is
            acceptable and has no effect.
*/
void
freeLines(char **lines)
{
    if (lines)
        free(*lines);
    free(lines);
}

#ifdef __cplusplus
}
#endif



#ifdef READFILE_TEST

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
Read a file specified on the command line and write its contents to stdout.
Also write the number of lines in the file to stderr.
Processing an 8 GB pi_8e9.txt file takes 11.7 seconds (writing to /dev/null)
on a 2020 3.2 GHz M1 Mac mini w/16 GB RAM, and reading from the internal SSD.
The runtime is unchanged if pi_8e9.txt is preceeded by "\r\n".  pi_8e9.txt is
pure digits plus one period.
*/
int
main(int argc, char *argv[])
{
	char *filename;
    char **lines, **p;
    size_t lineCount;
    char *progName = argv[0];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s filename\n", progName);
        return 1;
    }
    filename = argv[1];
    lines = readLines(filename, 0, &lineCount);
	if (!lines) {
        fprintf(stderr, "%s: reading file \"%s\": ", progName, filename);
        perror(NULL);
        return 1;
    }

    p = lines;
    while (*p)
        printf("%s\n", *p++);
    
    fprintf(stderr, "lineCount: %ld\n", (long)lineCount);

    freeLines(lines);

    return 0;
}


#ifdef __cplusplus
}
#endif

#endif
