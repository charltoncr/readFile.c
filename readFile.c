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
		"$Id: readFile.c,v 2.76 2024-02-26 16:45:11-05 ron Exp $";

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

#ifdef FALSE
    #undef FALSE
#endif
#define FALSE 0

/*
readFile reads the entire contents of the file named "fileName" into a
newly-malloc'ed buffer and returns a pointer to the buffer.  Include
readFile.h before calling readFile.
ARGUMENTS
---------
  Inputs:
	fileName    the name of the file to read
	textMode    Read in text mode if non-zero, or in binary mode if zero.
                If textMode is non-zero then "\r\n" is replaced by "\n" and
                terminate is forced to TRUE.
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
NOTE: An address is hidden before the first entry in lines.  Don't mess with
it or compensate in any way for it.
*/
char *
readFile(const char *fileName, int textMode, int terminate, size_t maxSize,
		size_t *length)
{
	FILE *in;			// input file stream pointer
	size_t fsize;		// file size
	size_t n = 0;		// length in chars of the data as read from the file
	char *buf = NULL;	// points to retrieved data
    size_t t;           // space for terminating '\0'
    int err = FALSE;

	if (!fileName) {
		errno = EINVAL;
        err = TRUE;
	} else {
        if (textMode)
            terminate = TRUE;
		t = !!terminate;	// allowance for terminator if required
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
                        errno = 0;  // per man page for rewind
    					rewind(in);
                        if (!errno) {
                            n = fread(buf, 1, fsize, in);
                            if (!ferror(in)) {
                                if (terminate)
                                    buf[n] = '\0';  // for strchr below
                                if (textMode) {
                                    char *s = strchr(buf, '\r');
                                    if (s) {
                                        char *d = s, *end = buf + n;
                                        for (; s < end; ++s)
                                            if (*s != '\r' || s[1] != '\n')
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
                                        errno = 0; // let original buf be returned
                                }
                            } else
                                err = TRUE; // note fread error
                        } else
                            err = TRUE;
					}
				} else {
					errno = EFBIG;
                    err = TRUE;
                }
			}
			fclose(in);
		}
	}

	if (!buf || err) {
		n = 0;
		free(buf);
		buf = NULL;
    }

	if (length)
		*length = n;

	return buf;
}


/*
readLines reads the entire contents of the text file named fileName into a
newly-malloc'ed buffer and returns an argv-like array of pointers to the
lines found in the buffer.  Carriage returns and linefeeds are excluded
from the lines.  Each line is terminated with '\0'.  Include readFile.h
before calling readLines or freeLines.  The caller should call freeLines
when the lines are no longer needed, even if *lineCount is 0.
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
    char *buf;                  // pointer to text returned by readFile
    size_t length;              // length  of text returned by readFile
    char *p;                    // scratch pointer
    size_t lnCnt;               // local line count
    size_t linesSize;           // space required for line pointers
    char **ln, **lines = NULL;  // argv-like array of lines found in fileName
    
    // Store the address of readFile's buf variable as a hidden line at lines'
    // beginning.
    buf = readFile(fileName, TRUE, TRUE, maxSize, &length);
    if (buf) {
        lnCnt = length && buf[length-1] != '\n'; // line with no '\n' at EOF
        for (p = buf; (p = strchr(p, '\n')); ++p)
            ++lnCnt;
        linesSize = (lnCnt + 1+1) * sizeof(*lines); // 1+1 NULL, buf pointer
        if (!maxSize || (linesSize+length+1) <= maxSize) {
            if ( (lines = malloc(linesSize)) ) {
                *lines++ = buf;    // save readFile buffer location
                ln = lines;
                if (length)
                    for (*ln++ = p = buf; (p = strchr(p, '\n'));) {
                        *p++ = '\0';
                        if (*p)
                            *ln++ = p;
                    }
                *ln = NULL;
            }
        } else {
            errno = EFBIG;  // lines will be NULL so buf will be freed below
        }
	}

	if (!buf || !lines || !length) {
        lnCnt = 0;
		free(buf);
		buf = NULL;
	}

	if (lineCount)
		*lineCount = lnCnt;

	return lines;
}

/*
freeLines frees memory allocated by readLines.
ARGUMENTS
---------
  Inputs:
    lines   a pointer to lines allocated by readLines.  A NULL argument is
            acceptable and has no effect.
*/
void
freeLines(char **lines)
{
    --lines;
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
Also write to stderr the number of lines in the file.
Processing an 8 GB pi_8e9.txt file takes 1.75 seconds (writing to /dev/null)
on a 2023 3.504 GHz M2 Max Mac Studio w/32 GB RAM, and reading from the
internal SSD.  The runtime is 4.52 seconds if pi_8e9.txt is preceeded by "\r\n".
pi_8e9.txt is pure digits plus one period.
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
        puts(*p++);
    
    fprintf(stderr, "lineCount: %ld\n", (long)lineCount);

    freeLines(lines);

    return 0;
}

#ifdef __cplusplus
}
#endif

#endif
