/*
 * file_trace.c -- Tyrian's files, without a file system. Replaces OpenTyrian's file.c (same file.h).
 *
 * There are two kinds, and every file the game opens goes through dataFileOpen or userFileOpen:
 *   - The game data (levels, sprites, sound), read-only, from the pack the door sent as an asset (tyrtrace.c).
 *   - The player's own files: tyrian.sav (saved games and high scores), tyrian.cfg and opentyrian.cfg (settings).
 *     These live on the BBS, per player. The door sends them at the start; whenever the game writes one, the new
 *     contents go back up a piece at a time (tyrtrace_user_files_pump), so they're safe even if the call drops.
 *
 * File.f is a real stdio stream over memory (fmemopen to read, open_memstream to write), because the game's config
 * reader and writer (config_file.c) use stdio on it directly.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "opentyr.h"

#include "trace_api.h"
#include "tyrtrace.h"

const char *customDataDirPath = NULL;

enum
{
    ERRNUM_EOF = -1,
};

/* ---- the player's files ---- */

#define CHUNK 3000   /* bytes per message to the door, header and all well under TRACE_SEND_MAX; see door/files.c */

typedef struct
{
    const char *name;
    uint8_t    *data;       /* what the game sees */
    size_t      size;
    int         present;    /* the player has this file (the door sent it, or the game wrote it) */

    /* receiving from the door */
    uint8_t    *incoming;
    size_t      incoming_total, incoming_have;

    /* sending to the door: a snapshot, so the game can write again while an older copy is going up */
    uint8_t    *outgoing;
    size_t      outgoing_size, outgoing_sent;
    int         outgoing_busy;
} user_file_t;

/* Only these are kept, and only these may go to the BBS: the door accepts nothing else either */
static user_file_t g_user_files[] =
{
    { "tyrian.sav" },
    { "tyrian.cfg" },
    { "opentyrian.cfg" },
};

static user_file_t *find_user_file(const char *name)
{
    for (size_t i = 0; i < COUNTOF(g_user_files); i++)
        if (strcmp(g_user_files[i].name, name) == 0)
            return &g_user_files[i];
    return NULL;
}

/* A piece of one of the player's files from the door. Pieces come in order; the file is only used once it's whole. */
void tyrtrace_user_file_received(const char *name, size_t off, size_t total, const uint8_t *data, size_t len)
{
    user_file_t *uf = find_user_file(name);

    if (uf == NULL || total > 1024 * 1024)
        return;
    if (off == 0)
    {
        free(uf->incoming);
        uf->incoming = (uint8_t *)malloc(total > 0 ? total : 1);
        uf->incoming_total = total;
        uf->incoming_have = 0;
    }
    if (uf->incoming == NULL || off != uf->incoming_have || off + len > uf->incoming_total)
    {
        tyrtrace_log("tyrian: a piece of %s arrived out of order; ignoring it", name);
        free(uf->incoming);
        uf->incoming = NULL;
        return;
    }
    memcpy(uf->incoming + off, data, len);
    uf->incoming_have += len;
    if (uf->incoming_have == uf->incoming_total)
    {
        free(uf->data);
        uf->data = uf->incoming;
        uf->size = uf->incoming_total;
        uf->present = 1;
        uf->incoming = NULL;
        tyrtrace_log("tyrian: %s from the BBS, %zu bytes", name, uf->size);
    }
}

const uint8_t *tyrtrace_user_file(const char *name, size_t *size)
{
    user_file_t *uf = find_user_file(name);
    if (uf == NULL || !uf->present)
        return NULL;
    *size = uf->size;
    return uf->data;
}

/* The game wrote a file: keep it, and send it to the BBS unless it's what the BBS already has */
void tyrtrace_user_file_written(const char *name, const uint8_t *data, size_t size)
{
    user_file_t *uf = find_user_file(name);
    uint8_t *copy;

    if (uf == NULL)
        return;
    if (uf->present && uf->size == size && (size == 0 || memcmp(uf->data, data, size) == 0))
        return;

    copy = (uint8_t *)malloc(size > 0 ? size : 1);
    if (copy == NULL)
        return;
    memcpy(copy, data, size);
    free(uf->data);
    uf->data = copy;
    uf->size = size;
    uf->present = 1;

    /* The newest copy replaces one still on its way: the door starts again when a piece at offset 0 arrives */
    free(uf->outgoing);
    uf->outgoing = (uint8_t *)malloc(size > 0 ? size : 1);
    if (uf->outgoing == NULL)
    {
        uf->outgoing_busy = 0;
        return;
    }
    memcpy(uf->outgoing, data, size);
    uf->outgoing_size = size;
    uf->outgoing_sent = 0;
    uf->outgoing_busy = 1;
}

/*
 * Sends what the link will take right now:
 *   put name=<n> off=<o> total=<t>\n<bytes>
 * Returns 1 while anything is still waiting to go.
 */
int tyrtrace_user_files_pump(void)
{
    static uint8_t message[CHUNK + 128];
    int waiting = 0;

    for (size_t i = 0; i < COUNTOF(g_user_files); i++)
    {
        user_file_t *uf = &g_user_files[i];

        while (uf->outgoing_busy)
        {
            size_t len = uf->outgoing_size - uf->outgoing_sent;
            int head;

            if (len > CHUNK)
                len = CHUNK;
            head = snprintf((char *)message, 128, "put name=%s off=%zu total=%zu\n",
                            uf->name, uf->outgoing_sent, uf->outgoing_size);
            memcpy(message + head, uf->outgoing + uf->outgoing_sent, len);
            if (trace_send_room() < head + (int)len || trace_send(message, head + (int32_t)len) <= 0)
                break;      /* the link is busy: the rest goes on a later call */

            uf->outgoing_sent += len;
            if (uf->outgoing_sent >= uf->outgoing_size)
            {
                tyrtrace_log("tyrian: %s saved to the BBS, %zu bytes", uf->name, uf->outgoing_size);
                free(uf->outgoing);
                uf->outgoing = NULL;
                uf->outgoing_busy = 0;
            }
        }
        waiting |= uf->outgoing_busy;
    }
    return waiting;
}

/* ---- handles ---- */

/* Files being written: open_memstream keeps the bytes, and they go to the player's file when it's closed */
typedef struct
{
    FILE       *f;
    char       *buffer;
    size_t      size;
    const char *user_name;
} writer_t;

static writer_t g_writers[4];

static File open_read(const uint8_t *data, size_t size)
{
    FILE *f;

    if (data == NULL || size == 0)       /* fmemopen won't take an empty buffer, and an empty file is no use */
        return (File) { NULL, ENOENT, true };
    errno = 0;
    f = fmemopen((void *)data, size, "rb");
    return (File) { f, f == NULL ? (errno ? errno : ENOMEM) : 0, f == NULL };
}

bool findDataFiles(void)
{
    size_t size;
    return tyrtrace_data_file("tyrian1.lvl", &size) != NULL;
}

bool dataFileExists(const char *filename)
{
    size_t size;
    return tyrtrace_data_file(filename, &size) != NULL;
}

bool userFileExists(const char *filename)
{
    size_t size;
    return tyrtrace_user_file(filename, &size) != NULL;
}

File dataFileOpen(const char *filename, const char *mode)
{
    size_t size = 0;
    const uint8_t *data;

    if (strchr(mode, 'w') != NULL || strchr(mode, 'a') != NULL)
        return (File) { NULL, EROFS, true };
    data = tyrtrace_data_file(filename, &size);
    return open_read(data, size);
}

File userFileOpen(const char *filename, const char *mode)
{
    if (strchr(mode, 'w') != NULL)
    {
        user_file_t *uf = find_user_file(filename);
        writer_t *w = NULL;

        if (uf == NULL)
            return (File) { NULL, EACCES, true };   /* e.g. a recorded demo: there's nowhere to keep it */
        for (size_t i = 0; i < COUNTOF(g_writers); i++)
            if (g_writers[i].f == NULL)
            {
                w = &g_writers[i];
                break;
            }
        if (w == NULL)
            return (File) { NULL, EMFILE, true };
        w->buffer = NULL;
        w->size = 0;
        w->f = open_memstream(&w->buffer, &w->size);
        if (w->f == NULL)
            return (File) { NULL, ENOMEM, true };
        w->user_name = uf->name;
        return (File) { w->f, 0, false };
    }
    else
    {
        size_t size = 0;
        const uint8_t *data = tyrtrace_user_file(filename, &size);
        return open_read(data, size);
    }
}

/* ---- the rest is OpenTyrian's own file.c, unchanged apart from fileClose ---- */

void fileSetPosition(File *file, long position)
{
    if (file->error)
        return;

    errno = 0;
    if (fseek(file->f, position, SEEK_SET) == 0)
        return;

    file->errnum = errno;
    file->error = true;
}

long fileGetPosition(File *file)
{
    if (file->error)
        return 0;

    errno = 0;
    long position = ftell(file->f);
    if (position >= 0)
        return position;

    file->errnum = errno;
    file->error = true;

    return 0;
}

long fileGetLength(File *file)
{
    if (file->error)
        return 0;

    errno = 0;
    long position = ftell(file->f);
    if (position >= 0 &&
        fseek(file->f, 0, SEEK_END) == 0)
    {
        long length = ftell(file->f);
        if (length >= 0 &&
            fseek(file->f, position, SEEK_SET) == 0)
        {
            return length;
        }
    }

    file->errnum = errno;
    file->error = true;

    return 0;
}

size_t fileReadAtMost(File *file, void *data, size_t size)
{
    if (file->error)
        return 0;

    errno = 0;
    size_t read = fread(data, 1, size, file->f);
    if (read == size)
        return read;

    file->errnum = errno;
    file->error = ferror(file->f) != 0;

    return read;
}

void fileReadExactly(File *file, void *data, size_t size)
{
    if (file->error)
    {
        memset(data, 0, size);
        return;
    }

    errno = 0;
    size_t read = fread(data, 1, size, file->f);
    if (read == size)
        return;

    file->errnum = errno;
    file->error = true;

    if (file->errnum == 0)
        file->errnum = ERRNUM_EOF;

    memset((uint8_t *)data + read, 0, size - read);
}

void fileWrite(File *file, const void *data, size_t size)
{
    if (file->error)
        return;

    errno = 0;
    size_t written = fwrite(data, 1, size, file->f);
    if (written == size)
        return;

    file->errnum = errno;
    file->error = true;
}

void fileFlush(File *file)
{
    if (file->error)
        return;

    errno = 0;
    if (fflush(file->f) == 0)
        return;

    file->errnum = errno;
    file->error = true;
}

/* Closing a file the game wrote is what sends it to the BBS: only whole, successfully written files go */
void fileClose(File *file)
{
    writer_t *w = NULL;

    if (file->f == NULL)
        return;

    for (size_t i = 0; i < COUNTOF(g_writers); i++)
        if (g_writers[i].f == file->f)
            w = &g_writers[i];

    errno = 0;
    int result = fclose(file->f);
    file->f = NULL;
    if (result != 0)
    {
        file->errnum = errno;
        file->error = true;
    }

    if (w != NULL)
    {
        if (!file->error)
            tyrtrace_user_file_written(w->user_name, (const uint8_t *)w->buffer, w->size);
        free(w->buffer);
        w->buffer = NULL;
        w->f = NULL;
    }
}

const char *fileGetError(File *file)
{
    switch (file->errnum)
    {
    case ERRNUM_EOF:
        return "Unexpected end of file";
    case 0:
        if (file->error)
            return "Unknown error";
        // fall through
    default:
        return strerror(file->errnum);
    }
}
