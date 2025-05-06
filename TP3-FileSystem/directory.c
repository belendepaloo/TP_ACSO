#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/**
 * TODO
 */
int directory_findname(struct unixfilesystem *fs, const char *name,
                       int dirinumber, struct direntv6 *dirEnt) {
    struct inode in;
    if (inode_iget(fs, dirinumber, &in) < 0) {
        return -1;
    }

    if (!(in.i_mode & IALLOC) || ((in.i_mode & IFMT) != IFDIR)) {
        // No está asignado o no es un directorio
        return -1;
    }

    int filesize = inode_getsize(&in);
    int num_blocks = (filesize + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;

    for (int bno = 0; bno < num_blocks; bno++) {
        char buf[DISKIMG_SECTOR_SIZE];
        int bytes = file_getblock(fs, dirinumber, bno, buf);
        if (bytes < 0) return -1;

        int nentries = bytes / sizeof(struct direntv6);
        struct direntv6 *entries = (struct direntv6 *) buf;

        for (int i = 0; i < nentries; i++) {
            if (strncmp(name, entries[i].d_name, 14) == 0) {
                *dirEnt = entries[i];
                return 0;
            }
        }
    }

    return -1;  // No se encontró
}

