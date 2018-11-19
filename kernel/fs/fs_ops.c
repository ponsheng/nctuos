/* This file use for NCTU OSDI course */
/* It's contants fat file system operators */

#include <inc/stdio.h>
#include <fs.h>
#include <fat/ff.h>
#include <diskio.h>

#include <kernel/mem.h>

extern struct fs_dev fat_fs;

/* Lab7, fat level file operator.
 *       Implement below functions to support basic file system operators by using the elmfat's API(f_xxx).
 *       Reference: http://elm-chan.org/fsw/ff/00index_e.html (or under doc directory (doc/00index_e.html))
 *
 *  Call flow example:
 *        ┌──────────────┐
 *        │     open     │
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │   sys_open   │  file I/O system call interface
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │  file_open   │  VFS level file API
 *        └──────────────┘
 *               ↓
 *        ╔══════════════╗
 *   ==>  ║   fat_open   ║  fat level file operator
 *        ╚══════════════╝
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

/* Note: 1. Get FATFS object from fs->data
*        2. Check fs->path parameter then call f_mount.  TODO ??
*/
int fat_mount(struct fs_dev *fs, const void* data)
{
    FATFS* fatfs;
    fatfs = fs->data;
    return f_mount(fatfs, data, 1);
}

/* Note: Just call f_mkfs at root path '/' */
int fat_mkfs(const char* device_name)
{
    return f_mkfs("/", 0, 0);
}

/* Note: Convert the POSIX's open flag to elmfat's flag.
*        Example: if file->flags == O_RDONLY then open_mode = FA_READ
*                 if file->flags & O_APPEND then f_seek the file to end after f_open
*/
int fat_open(struct fs_fd* file)
{
    BYTE flag = 0;
    FIL* fp;
    int ret;
    int last2bit = file->flags & 3;
    if ( last2bit == O_RDONLY ) {
        flag |= FA_READ;
    } else if ( last2bit == O_WRONLY ) {
        flag |= FA_WRITE;
    } else if ( last2bit == O_RDWR ) {
        flag |= FA_WRITE | FA_READ;
    } else {
        printk("fat_open unknown flag\n");
    }

    if ( file->flags & O_CREAT ) {
        if ( file->flags & O_EXCL ) {
            //flag |= FA_CREATE_NEW;
        }
        flag |= FA_CREATE_NEW;
    }

    if ( file->flags & O_TRUNC ) {
        if ( flag & FA_WRITE ) {
            flag = FA_CREATE_ALWAYS | ( flag & 3 );
        } else {
            printk("fat_open access denied\n");
        }
    }

    if ( file->flags & O_DIRECTORY ) {
        /*DIR* dir;
        dir = (DIR*) page_alloc(ALLOC_ZERO); // FIXME
        ret = f_opendir (dir, file->path);
        file->data = dir;*/
    } else {
        fp = file->data;

        ret = f_open(fp, file->path, flag);
    }

    file->size = f_size((FIL*)file->data);  // update size

    if ( file->flags & O_APPEND ) {
        ret = f_lseek(fp, file->size);
        //fs_fd->pos = size;
        file->pos = f_tell((FIL*)file->data);
    }

    // FRESULT process
    if ( ret == FR_EXIST ) {
        //return EEXIST; FIXME POSIX ERRPR not defined
    }
    return ret;
}


int fat_close(struct fs_fd* file)
{
    int ret;
    ret = f_close(file->data);
    return ret;
}

int fat_read(struct fs_fd* file, void* buf, size_t count)
{
    UINT bw;
    f_read(file->data, buf, count, &bw);
    return bw;
}
int fat_write(struct fs_fd* file, const void* buf, size_t count)
{
    UINT bw;
    f_write(file->data, buf, count, &bw);
    file->size = f_size((FIL*)file->data);  // update size
    return bw;
}
int fat_lseek(struct fs_fd* file, off_t offset)
{
    int ret;
    ret = f_lseek(file->data, offset);
    file->pos = f_tell((FIL*)file->data);
    return ret;
}
int fat_unlink(struct fs_fd* file, const char *pathname)
{
    return f_unlink(pathname);
}



FRESULT scan_files (
    char* path        /* Start node to be scanned (***also used as work area***) */
)
{
    FRESULT res;
    DIR dir;
    UINT i;
    static FILINFO fno;


    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) {
                //printk("End of dir\n");
                break;  /* Break on error or end of dir */
            }

            if ( path[strlen(path)-1] == '/') {
                printk("%s", path);
            } else {
                printk("%s/", path);
            }
            printk("%-10s", fno.fname);

            if (fno.fattrib & AM_DIR) {
                printk(" type:DIR ");
            } else{
                printk(" type:FILE");
            }

            printk(" size:%d\n", fno.fsize);


            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
                i = strlen(path);
                //printk(&path[i], "%s", fno.fname);
                strncpy( &path[i], fno.fname, strlen(fno.fname));
                res = scan_files(path);                    /* Enter the directory */
                if (res != FR_OK) break;
                path[i] = 0;
            } else {                                       /* It is a file. */
                   // printk("%s/%s\n", path, fno.fname);
            }
        }
        f_closedir(&dir);
    }

    return res;
}

int fat_getdents (struct fs_fd* fd, struct dirent* dirp, uint32_t count)
{
    //ret f_readdir (fd->data, FILINFO* fno);
    //f_mkdir("dir" );
    return scan_files(dirp);
}


struct fs_ops elmfat_ops = {
    .dev_name = "elmfat",
    .mount = fat_mount,
    .mkfs = fat_mkfs,
    .open = fat_open,
    .close = fat_close,
    .read = fat_read,
    .write = fat_write,
    .lseek = fat_lseek,
    .unlink = fat_unlink,
    .getdents = fat_getdents
};
