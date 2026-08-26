// COMP1521 Assignment 2: stdio
//
// Name: [z5687204]
// Date: [10th April 2026]
//

#include "cs1521_stdio.h"

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

struct file {
    int fd;
    int eof;
    int err;
};

static struct file stdin_file = { .fd = 0, .eof = 0, .err = 0 };
static struct file stdout_file = { .fd = 1, .eof = 0, .err = 0 };
static struct file stderr_file = { .fd = 2, .eof = 0, .err = 0 };

cs1521_FILE *cs1521_stdin = &stdin_file;
cs1521_FILE *cs1521_stdout = &stdout_file;
cs1521_FILE *cs1521_stderr = &stderr_file;

//////////////
// SUBSET 0 //
//////////////

cs1521_FILE *cs1521_fopen(char *pathname, char *mode) {
    int flags;
    bool append_mode = false;   // track append mode

    if (strcmp(mode, "r") == 0) {
        flags = O_RDONLY;
    } else if (strcmp(mode, "w") == 0) {
        flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if (strcmp(mode, "a") == 0) {
        flags = O_WRONLY | O_CREAT | O_APPEND;
        append_mode = true;
    } else if (strcmp(mode, "r+") == 0) {
        flags = O_RDWR;
    } else if (strcmp(mode, "w+") == 0) {
        flags = O_RDWR | O_CREAT | O_TRUNC;
    } else if (strcmp(mode, "a+") == 0) {
        flags = O_RDWR | O_CREAT | O_APPEND;
        append_mode = true;
    } else {
        errno = EINVAL;         // invalid mode
        return NULL;
    }

    int fd = open(pathname, flags, 0644);
    if (fd < 0) {
        return NULL;
    }

    if (append_mode) {
        if (lseek(fd, 0, SEEK_END) < 0) {   // start at end
            close(fd);
            return NULL;
        }
    }

    cs1521_FILE *f = malloc(sizeof(struct file));
    if (f == NULL) {
        close(fd);              // avoid leaking fd
        return NULL;
    }

    f->fd = fd;
    f->eof = 0;
    f->err = 0;

    return f;
}

int cs1521_fclose(cs1521_FILE *stream) {
    if (stream == NULL) {
        errno = EINVAL;
        return -1;
    }

    int result = close(stream->fd);

    if (stream != cs1521_stdin &&
        stream != cs1521_stdout &&
        stream != cs1521_stderr) {
        free(stream);           // only free fopen streams
    }

    return result;
}

int cs1521_fileno(cs1521_FILE *stream) {
    if (stream == NULL) {
        errno = EINVAL;
        return -1;
    }

    return stream->fd;          // return underlying fd
}

int cs1521_fgetc(cs1521_FILE *stream) {
    if (stream == NULL) {
        errno = EINVAL;
        return cs1521_EOF;
    }

    if (stream->eof) {
        return cs1521_EOF;      // already at EOF
    }

    unsigned char c;
    ssize_t result = read(stream->fd, &c, 1);

    if (result == 0) {
        stream->eof = 1;        // reached EOF
        return cs1521_EOF;
    }

    if (result < 0) {
        stream->err = 1;        // read error
        return cs1521_EOF;
    }

    return c;
}

int cs1521_fputc(int c, cs1521_FILE *stream) {
    if (stream == NULL) {
        errno = EINVAL;
        return cs1521_EOF;
    }

    unsigned char ch = (unsigned char)c;

    ssize_t result = write(stream->fd, &ch, 1);

    if (result != 1) {
        stream->err = 1;        // write failed
        return cs1521_EOF;
    }

    return ch;
}

//////////////
// SUBSET 1 //
//////////////

size_t cs1521_fread(
    void *ptr,
    size_t size,
    size_t nitems,
    cs1521_FILE *stream
) {
    if (stream == NULL) {
        errno = EINVAL;
        return 0;
    }

    if (size == 0 || nitems == 0) {
        return 0;               // nothing to read
    }

    if (stream->eof) {
        return 0;               // already at EOF
    }

    char *buf = ptr;
    size_t total = size * nitems;
    size_t read_bytes = 0;

    while (read_bytes < total) {
        ssize_t result = read(
            stream->fd,
            buf + read_bytes,
            total - read_bytes
        );

        if (result == 0) {
            stream->eof = 1;    // reached EOF
            break;
        }

        if (result < 0) {
            stream->err = 1;    // read error
            return 0;
        }

        read_bytes += result;
    }

    return read_bytes / size;   // number of full items
}

size_t cs1521_fwrite(
    void *ptr,
    size_t size,
    size_t nitems,
    cs1521_FILE *stream
) {
    if (stream == NULL) {
        errno = EINVAL;
        return 0;
    }

    if (size == 0 || nitems == 0) {
        return 0;               // nothing to write
    }

    size_t total = size * nitems;
    ssize_t result = write(stream->fd, ptr, total);

    if (result < 0 || (size_t)result != total) {
        stream->err = 1;        // failed or partial write
        return 0;
    }

    return nitems;
}

char *cs1521_fgets(char *ptr, size_t size, cs1521_FILE *stream) {
    if (stream == NULL) {
        errno = EINVAL;
        return NULL;
    }

    if (size == 0) {
        errno = EINVAL;
        stream->err = 1;
        return NULL;
    }

    if (stream->eof) {
        return NULL;            // already at EOF
    }

    size_t i = 0;

    while (i < size - 1) {
        char c;
        ssize_t result = read(stream->fd, &c, 1);

        if (result == 0) {
            stream->eof = 1;    // reached EOF
            break;
        }

        if (result < 0) {
            stream->err = 1;    // read error
            return NULL;
        }

        ptr[i++] = c;

        if (c == '\n') {
            break;              // stop at newline
        }
    }

    if (i == 0) {
        return NULL;
    }

    ptr[i] = '\0';              // terminate string
    return ptr;
}

int cs1521_fputs(char *s, cs1521_FILE *stream) {
    if (stream == NULL) {
        errno = EINVAL;
        return cs1521_EOF;
    }

    size_t len = 0;
    while (s[len] != '\0') {
        len++;                  // find string length
    }

    ssize_t result = write(stream->fd, s, len);

    if (result < 0 || (size_t)result != len) {
        stream->err = 1;        // write failed
        return cs1521_EOF;
    }

    return 0;
}

//////////////
// SUBSET 2 //
//////////////

int cs1521_fseek(cs1521_FILE *stream, long offset, int whence) {
    if (stream == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (whence != SEEK_SET &&
        whence != SEEK_CUR &&
        whence != SEEK_END) {
        errno = EINVAL;         // invalid whence
        return -1;
    }

    off_t result = lseek(stream->fd, offset, whence);

    if (result < 0) {
        stream->err = 1;        // seek failed
        return -1;
    }

    stream->eof = 0;            // seek clears EOF
    return 0;
}

long cs1521_ftell(cs1521_FILE *stream) {
    if (stream == NULL) {
        errno = EINVAL;
        return -1;
    }

    off_t pos = lseek(stream->fd, 0, SEEK_CUR);   // current position

    if (pos < 0) {
        stream->err = 1;
        return -1;
    }

    return pos;
}

void cs1521_perror(char *msg) {
    const char *err_str = strerror(errno);   // errno as text

    if (msg == NULL || msg[0] == '\0') {
        cs1521_fputs((char *)err_str, cs1521_stderr);
        cs1521_fputc('\n', cs1521_stderr);
    } else {
        cs1521_fputs(msg, cs1521_stderr);
        cs1521_fputs(": ", cs1521_stderr);
        cs1521_fputs((char *)err_str, cs1521_stderr);
        cs1521_fputc('\n', cs1521_stderr);
    }
}

int cs1521_feof(cs1521_FILE *stream) {
    if (stream == NULL) {
        return 0;
    }

    return stream->eof;         // return EOF flag
}

int cs1521_ferror(cs1521_FILE *stream) {
    if (stream == NULL) {
        return 0;
    }

    return stream->err;         // return error flag
}

void cs1521_clearerr(cs1521_FILE *stream) {
    if (stream == NULL) {
        return;
    }

    stream->eof = 0;            // reset flags
    stream->err = 0;
}

//////////////
// SUBSET 3 //
//////////////

cs1521_wchar_t cs1521_fgetwc(cs1521_FILE *stream) {
    if (stream == NULL) {
        errno = EINVAL;
        return cs1521_WEOF;
    }

    int c = cs1521_fgetc(stream);

    if (c == cs1521_EOF) {
        return cs1521_WEOF;     // pass on EOF/error
    }

    if ((c & 0x80) == 0) {
        return c;               // one-byte ASCII
    }

    int num_bytes;
    cs1521_wchar_t codepoint;

    if ((c & 0xE0) == 0xC0) {
        num_bytes = 2;
        codepoint = c & 0x1F;   // remove UTF-8 prefix
    } else if ((c & 0xF0) == 0xE0) {
        num_bytes = 3;
        codepoint = c & 0x0F;
    } else if ((c & 0xF8) == 0xF0) {
        num_bytes = 4;
        codepoint = c & 0x07;
    } else {
        errno = EILSEQ;
        stream->err = 1;
        return cs1521_WEOF;
    }

    for (int i = 1; i < num_bytes; i++) {
        int next = cs1521_fgetc(stream);

        if (next == cs1521_EOF) {
            errno = EILSEQ;
            stream->err = 1;
            return cs1521_WEOF;
        }

        if ((next & 0xC0) != 0x80) {
            errno = EILSEQ;
            stream->err = 1;
            return cs1521_WEOF; // invalid continuation byte
        }

        codepoint = (codepoint << 6) | (next & 0x3F);
    }

    return codepoint;
}

cs1521_wchar_t cs1521_fputwc(
    cs1521_wchar_t wc,
    cs1521_FILE *stream
) {
    if (stream == NULL) {
        errno = EINVAL;
        return cs1521_WEOF;
    }

    if (wc > 0x10FFFF ||
        (wc >= 0xD800 && wc <= 0xDFFF)) {
        errno = EILSEQ;
        stream->err = 1;
        return cs1521_WEOF;     // invalid Unicode value
    }

    unsigned char bytes[4];
    int count;

    if (wc <= 0x7F) {
        bytes[0] = wc;
        count = 1;
    } else if (wc <= 0x7FF) {
        bytes[0] = 0xC0 | (wc >> 6);
        bytes[1] = 0x80 | (wc & 0x3F);
        count = 2;
    } else if (wc <= 0xFFFF) {
        bytes[0] = 0xE0 | (wc >> 12);
        bytes[1] = 0x80 | ((wc >> 6) & 0x3F);
        bytes[2] = 0x80 | (wc & 0x3F);
        count = 3;
    } else {
        bytes[0] = 0xF0 | (wc >> 18);
        bytes[1] = 0x80 | ((wc >> 12) & 0x3F);
        bytes[2] = 0x80 | ((wc >> 6) & 0x3F);
        bytes[3] = 0x80 | (wc & 0x3F);
        count = 4;
    }

    for (int i = 0; i < count; i++) {
        if (cs1521_fputc(bytes[i], stream) == cs1521_EOF) {
            return cs1521_WEOF; // write failed
        }
    }

    return wc;
}

extern char **environ;

int cs1521_posix_spawnp(
    pid_t *pid,
    char *file,
    cs1521_posix_spawn_file_actions_t *file_actions,
    cs1521_posix_spawnattr_t *attrp,
    char *argv[],
    char *envp[]
) {
    (void)file_actions;
    (void)attrp;

    if (file == NULL || pid == NULL) {
        errno = EINVAL;
        return -1;
    }

    char *path = getenv("PATH");
    if (path == NULL) {
        errno = ENOENT;
        return -1;
    }

    char *path_copy = malloc(strlen(path) + 1);
    if (path_copy == NULL) {
        return -1;
    }

    strcpy(path_copy, path);     // strtok modifies its input

    char *dir = strtok(path_copy, ":");

    while (dir != NULL) {
        char fullpath[1024];
        size_t len = 0;

        for (size_t i = 0; dir[i] != '\0'; i++) {
            fullpath[len++] = dir[i];
        }

        fullpath[len++] = '/';

        for (size_t i = 0; file[i] != '\0'; i++) {
            fullpath[len++] = file[i];
        }

        fullpath[len] = '\0';    // build dir/file path

        if (access(fullpath, F_OK) == 0) {
            pid_t child = fork();

            if (child < 0) {
                free(path_copy);
                return -1;
            }

            if (child == 0) {
                if (envp == NULL) {
                    execve(fullpath, argv, environ);
                } else {
                    execve(fullpath, argv, envp);
                }

                _exit(127);      // exec failed
            }

            *pid = child;
            free(path_copy);
            return 0;
        }

        dir = strtok(NULL, ":"); // check next PATH directory
    }

    free(path_copy);
    errno = ENOENT;
    return -1;
}