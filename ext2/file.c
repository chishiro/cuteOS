/*
 * Standard Unix system calls for the file system
 *
 * Copyright (C) 2012 Ahmed S. Darwish <darwish.07@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2.
 *
 * We don't repeat what's written in the POSIX spec here: check the sta-
 * ndard to make sense of all the syscalls parameters and return values.
 */

#include <kernel.h>
#include <percpu.h>
#include <errno.h>
#include <ext2.h>
#include <file.h>
#include <fcntl.h>
#include <tests.h>
#include <unrolled_list.h>
#include <spinlock.h>
#include <kmalloc.h>

/*
 * File Table Entry
 *
 * Each call to open() results in a _new_ allocation of the file
 * structure below.  Each instance mainly contains a byte offset
 * field where the kernel expects the next read() or write() op-
 * eration to begin.
 *
 * Classical Unix allocated a table of below structure at system
 * boot, calling it the system 'file table'.  We don't need such
 * table anymore: we can allocate each entry dynamically!
 *
 * Ken Thompson rightfully notes  that below elements could have
 * been embedded in each entry of the file descriptor table, but
 * a separate structure  is used to  allow sharing of the offset
 * pointer between several user FDs, mainly for fork() and dup().
 */
struct file {
	uint64_t inum;		/* Inode# of the open()-ed file */
	int flags;		/* Flags passed  to open() call */
	spinlock_t lock;	/* ONLY FOR offset and refcount */
	uint64_t offset;	/* MAIN FIELD: File byte offset */
	int refcount;		/* Reference count; fork,dup,.. */
};

static void file_init(struct file *file, uint64_t inum, int flags)
{
	file->inum = inum;
	file->flags = flags;
	spin_init(&file->lock);
	file->offset = 0;
	file->refcount = 1;
}

int sys_chdir(const char *path)
{
	int64_t inum;

	inum = name_i(path);
	if (inum < 0)
		return inum;
	if (!is_dir(inum))
		return -ENOTDIR;

	assert(inum != 0);
	current->working_dir = inum;
	return 0;
}

int sys_open(const char *path, int flags, __unused mode_t mode)
{
	int64_t inum;
	struct file *file;

	if ((flags & O_ACCMODE) == 0)
		return -EINVAL;

	if (flags & O_WRONLY || flags & O_CREAT ||
	    flags & O_EXCL   || flags & O_TRUNC ||
	    flags & O_APPEND)
		return -EINVAL;		/* No write support, yet! */

	inum = name_i(path);
	if (inum < 0)
		return inum;		/* -ENOENT, -ENOTDIR, -ENAMETOOLONG */

	/* All UNIX kernels keep the write permission
	 * to directories exclusively for themselves! */
	if (is_dir(inum) && flags & O_WRONLY)
		return -EISDIR;

	file = kmalloc(sizeof(*file));
	file_init(file, inum, flags);
	return unrolled_insert(&current->fdtable, file);
}

int64_t sys_read(int fd, void *buf, uint64_t count)
{
	struct file *file;
	struct inode *inode;
	int64_t read_len;

	file = unrolled_lookup(&current->fdtable, fd);
	if (file == NULL)
		return -EBADF;
	if ((file->flags & O_WRONLY) && !(file->flags & O_RDONLY))
		return -EBADF;

	assert(file->inum > 0);
	if (is_dir(file->inum))
		return -EISDIR;
	if (!is_regular_file(file->inum))
		return -EBADF;
	inode = inode_get(file->inum);

	spin_lock(&file->lock);
	read_len = file_read(inode, buf, file->offset, count);
	assert(file->offset + read_len <= inode->size_low);
	file->offset += read_len;
	spin_unlock(&file->lock);

	return read_len;
}
