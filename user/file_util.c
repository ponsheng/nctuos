#include <inc/syscall.h>
#include <inc/stdio.h>

int touch(int argc, char** argv);
int ls(int argc, char** argv);
int rm(int argc, char** argv);

    
int touch(int argc, char** argv) {
    if ( argc > 1 ) {
        char *path;
        int fd;
        path = argv[1];
        fd = open(path, O_WRONLY | O_CREAT, 0);
        if ( fd >= 0 ) {
            cprintf("Created %s  ret: %d\n", path, fd);
            close(fd);
        } else {
            cprintf("Failed to create %s, err: %d\n", path, fd);
        }
    }
    return 0;
}


int ls(int argc, char** argv) {
    if ( argc > 1 ) {
        char *path;
        int fd;
        char buf[100];
        path = argv[1];
        // TODO
        //fd = open(path, O_RDONLY | O_DIRECTORY, 0);
        readdir(0, path, NULL, NULL);
        /*if ( fd >= 0 ) {
            cprintf("Opened dir %s fd: %d\n", path, fd);

            close(fd);
        } else {
            cprintf("Failed to opendir %s, err: %d\n", path, fd);
        }*/
    }
    return 0;

}
int rm(int argc, char** argv) {
    if ( argc > 1 ) {
        char *path;
        int ret;
        path = argv[1];
        ret = unlink(path);
        if ( ret >= 0 ) {
            cprintf("Delted %s\n", path);
        } else {
            cprintf("Failed to delete %s, err: %d\n", path, ret);
        }
    }
    return 0;
}
