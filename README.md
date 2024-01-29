<!-- title: readFile.c Read Me -->
<!-- $Id: README.md,v 1.4 2024-01-29 13:08:10-05 ron Exp $ -->

# readFile.c

readFile.c contains three public domain C language functions: 
readFile, readLines and freeLines.

## `readFile` function

```c
char *readFile(const char *fileName, int textMode,
    int terminate, size_t maxSize, size_t *length)
```

`readFile` reads the entire contents of a file into a newly-malloc'ed buffer
and returns a pointer to it.  It will read the file in text mode, if requested,
by removing any carriage returns found. It will terminate the contents with
'\0' if requested. `maxSize` may specify a maximum size for the buffer.  The
content's byte count may be obtained through `length`.

## `readLines` function

```c
char **readLines(const char *fileName, size_t maxSize, 
    size_t *lineCount)
```

`readLines` reads the entire contents of a text file into a newly-malloc'ed
buffer and returns an argv-like array of pointers to the lines of text in
the buffer.  Carriage returns and linefeeds are excluded from the lines.
Each line is terminated with `'\0'`.  `maxSize` may specify a maximum amount of
memory to use.  The number of lines found may be obtained through `lineCount`.

## `freeLines` function

```c
void freeLines(char **lines)
```

`freeLines` frees all memory allocated by readLines.

---

If `-DREADFILE_TEST` is given when readFile.c is compiled a simple test
`main` will be included.  `main` writes a file named on the command line to
stdout, and displays on stderr the number of lines read.

Ron Charlton
