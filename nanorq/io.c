#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io.h"

struct fileioctx {
  struct ioctx io;
  FILE *fp;
};

static size_t fileio_read(struct ioctx *io, uint8_t *buf, size_t len) {
  struct fileioctx *_io = (struct fileioctx *)io;
  return fread(buf, 1, len, _io->fp);
}

static size_t fileio_write(struct ioctx *io, const uint8_t *buf, size_t len) {
  struct fileioctx *_io = (struct fileioctx *)io;
  return fwrite(buf, 1, len, _io->fp);
}

static bool fileio_seek(struct ioctx *io, const size_t offset) {
  struct fileioctx *_io = (struct fileioctx *)io;
#ifdef _MSC_VER // Microsoft Visual Studio
  return (_fseeki64(_io->fp, offset, SEEK_SET) == 0);
#else // gcc
  return (fseek(_io->fp, offset, SEEK_SET) == 0);
#endif
}

static size_t fileio_tell(struct ioctx *io) {
  struct fileioctx *_io = (struct fileioctx *)io;
#ifdef _MSC_VER // Microsoft Visual Studio
  return _ftelli64(_io->fp);
#else // gcc
  return ftell(_io->fp);
#endif
}

static void fileio_destroy(struct ioctx *io) {
  struct fileioctx *_io = (struct fileioctx *)io;
  fclose(_io->fp);
  free(_io);
  return;
}

static size_t fileio_size(struct ioctx *io) {
  struct fileioctx *_io = (struct fileioctx *)io;
#ifdef _MSC_VER // Microsoft Visual Studio
  size_t ret = 0;
  __int64 pos = _ftelli64(_io->fp);
  _fseeki64(_io->fp, 0, SEEK_END);
  ret = _ftelli64(_io->fp);
  _fseeki64(_io->fp, pos, SEEK_SET);
#else // gcc
  long ret = 0;
  long pos = ftell(_io->fp);
  fseek(_io->fp, 0, SEEK_END);
  ret = ftell(_io->fp);
  fseek(_io->fp, pos, SEEK_SET);
#endif
  return ret;
}

struct ioctx *ioctx_from_file(const char *fn, int t) {
  struct fileioctx *_io = NULL;
  FILE *fp;

#ifdef _MSC_VER // Microsoft Visual Studio
  errno_t err;
  if (t) {
    err = fopen_s(&fp, fn, "rb");
  } else {
    err = fopen_s(&fp, fn, "w+b"); // create decoder
  }
  if (err != 0)
    return NULL;
#else // gcc
  if (t) {
    fp = fopen(fn, "r");
  } else {
    fp = fopen(fn, "w+"); // create decoder
  }
#endif

  if (!fp)
    return NULL;

  _io = calloc(1, sizeof(struct fileioctx));
  _io->fp = fp;

  _io->io.read = fileio_read;
  _io->io.write = fileio_write;
  _io->io.seek = fileio_seek;
  _io->io.size = fileio_size;
  _io->io.tell = fileio_tell;
  _io->io.destroy = fileio_destroy;
  _io->io.seekable = true;
  _io->io.writable = (t == 0);

  return (struct ioctx *)_io;
}

struct memioctx {
  struct ioctx io;
  uint8_t *ptr;
  size_t pos;
  size_t size;
};

static size_t memio_read(struct ioctx *io, uint8_t *buf, size_t len) {
  struct memioctx *_io = (struct memioctx *)io;
  if (_io->pos + len > _io->size) {
    size_t diff = _io->size - _io->pos;
    memcpy(buf, _io->ptr + _io->pos, diff);
    _io->pos = _io->size;
    return diff;
  }
  memcpy(buf, _io->ptr + _io->pos, len);
  _io->pos += len;
  return len;
}

static size_t memio_write(struct ioctx *io, const uint8_t *buf, size_t len) {
  struct memioctx *_io = (struct memioctx *)io;
  if (_io->pos + len > _io->size) {
    size_t diff = _io->size - _io->pos;
    memcpy(_io->ptr + _io->pos, buf, diff);
    _io->pos = _io->size;
    return diff;
  }
  memcpy(_io->ptr + _io->pos, buf, len);
  _io->pos += len;
  return len;
}

static bool memio_seek(struct ioctx *io, const size_t offset) {
  struct memioctx *_io = (struct memioctx *)io;
  if (offset >= _io->size)
    return false;
  _io->pos = offset;
  return true;
}

static size_t memio_tell(struct ioctx *io) {
  struct memioctx *_io = (struct memioctx *)io;
  return _io->pos;
}

static void memio_destroy(struct ioctx *io) {
  struct memioctx *_io = (struct memioctx *)io;
  free(_io);
  return;
}

static size_t memio_size(struct ioctx *io) {
  struct memioctx *_io = (struct memioctx *)io;
  return _io->size;
}

struct ioctx *ioctx_from_mem(const uint8_t *ptr, size_t sz) {
  struct memioctx *_io = NULL;

  _io = calloc(1, sizeof(struct memioctx));
  _io->ptr = (uint8_t *)ptr;
  _io->pos = 0;
  _io->size = sz;

  _io->io.read = memio_read;
  _io->io.write = memio_write;
  _io->io.seek = memio_seek;
  _io->io.size = memio_size;
  _io->io.tell = memio_tell;
  _io->io.destroy = memio_destroy;
  _io->io.seekable = true;
  _io->io.writable = true;

  return (struct ioctx *)_io;
}
