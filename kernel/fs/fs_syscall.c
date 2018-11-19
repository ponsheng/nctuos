/* This file use for NCTU OSDI course */


// It's handel the file system APIs 
#include <inc/stdio.h>
#include <inc/syscall.h>
#include <fs.h>

#include <fat/ff.h>

extern struct fs_fd fd_table[FS_FD_MAX];

/*TODO: Lab7, file I/O system call interface.*/
/*Note: Here you need handle the file system call from user.
 *       1. When user open a new file, you can use the fd_new() to alloc a file object(struct fs_fd)
 *       2. When user R/W or seek the file, use the fd_get() to get file object.
 *       3. After get file object call file_* functions into VFS level
 *       4. Update the file objet's position or size when user R/W or seek the file.(You can find the useful marco in ff.h)
 *       5. Remember to use fd_put() to put file object back after user R/W, seek or close the file.
 *       6. Handle the error code, for example, if user call open() but no fd slot can be use, sys_open should return -STATUS_ENOSPC.
 *
 *  Call flow example:
 *        ┌──────────────┐
 *        │     open     │
 *        └──────────────┘
 *               ↓
 *        ╔══════════════╗
 *   ==>  ║   sys_open   ║  file I/O system call interface
 *        ╚══════════════╝
 *               ↓
 *        ┌──────────────┐
 *        │  file_open   │  VFS level file API
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │   fat_open   │  fat level file operator
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │    f_open    │  FAT File System Module
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │    diskio    │  low level file operator
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │     disk     │  simple ATA disk dirver
 *        └──────────────┘
 */

// Below is POSIX like I/O system call 
int sys_open(const char *file, int flags, int mode)
{
    //We dont care the mode.

    int i, ret;
    int idx = -1;
    struct fs_fd* fd;
/*
    // Search if opened
    for ( i = 0; i < FS_FD_MAX; i++ ) {
        if ( fd_table[i].ref_count) {
            if ( strcmp(fd_table[i].path, file) == 0) {
                idx = i;
            }
        }
    }
  */  
    if ( idx == -1 ) {
        idx = fd_new();
        if ( idx < 0 ) {
            return -STATUS_ENOMEM;
        }

        fd = fd_get(idx);
        fd_put (fd);  // Avoid additional ++
    } else {
        fd = fd_get(idx);
    }
    
    if ( fd ) {
        ret = file_open (fd, file, flags);

        switch ( ret ) {
            case FR_NO_FILE:
                ret = -STATUS_ENOENT;
                break;
            case FR_EXIST:
                ret = -STATUS_EEXIST;
                break;
            case FR_NO_PATH:
                ret = -STATUS_ENOENT;
                break;
            case FR_OK:
                ret = idx;
                break;
            default:
                printk("Undefined ret val %d\n", ret);
        }
        if ( ret < 0 ) {
            fd_put(fd);
        }
        return ret;

    } else {
        fd_put(fd);
        return -STATUS_EINVAL;
    }
}

int sys_close(int fd)
{
    int ret;
    struct fs_fd* f;
    f = fd_get(fd);

    if ( f ) {
        ret = file_close (f);
        fd_put(f);
        fd_put(f);

        return ret;
        
    } else {
        return -STATUS_EINVAL;
    }
}
int sys_read(int fd, void *buf, size_t len)
{
    int ret;
    struct fs_fd* f;
    f = fd_get(fd);

    if (!buf ) {
        return -STATUS_EINVAL;
    }

    if ( f ) {
        ret = file_read(f, buf, len);
        fd_put(f);
        return ret;

    } else {
        return -STATUS_EBADF;
    }
}
int sys_write(int fd, const void *buf, size_t len)
{
    int ret;
    struct fs_fd* f;
    f = fd_get(fd);

    if (!buf ) {
        return -STATUS_EINVAL;
    }

    if ( f ) {
        ret = file_write(f, buf, len);
        fd_put(f);
        return ret;

    } else {
        return -STATUS_EBADF;
    }
}

/* Note: Check the whence parameter and calcuate the new offset value before do file_seek() */
/* whence :
       SEEK_SET
              The file offset is set to offset bytes.

       SEEK_CUR
              The file offset is set to its current location plus offset
              bytes.

       SEEK_END
              The file offset is set to the size of the file plus offset
              bytes. 
*/

off_t sys_lseek(int fd, off_t offset, int whence)
{
    int ret;
    struct fs_fd* f;
    f = fd_get(fd);

    if ( f ) {

        switch ( whence ) {
            case SEEK_SET:
                break;
            case SEEK_CUR:
                offset += f->pos;
                break;
            case SEEK_END:
                offset += f->size;
                break;
        }

        ret = file_lseek(f, offset);
        f->pos = offset;
        fd_put(f);
        return ret;
    } else {
        return -STATUS_EINVAL;
    }
}

int sys_unlink(const char *pathname)
{
    int ret;
    ret = file_unlink(pathname);

    switch ( ret ) {
        case FR_NO_FILE:
            ret = -STATUS_ENOENT;
    }
    return ret;
}

int sys_readdir(int fd,  char *buf ,int *type, unsigned long *size)
{
    int ret;
    //struct fs_fd* f;
    //f = fd_get(fd);

    ret = file_getdents(NULL, buf, 0);
    /*if ( f ) {

        fd_put(f);
        return ret;
    } else {
        return -STATUS_EINVAL;
    }*/
}
