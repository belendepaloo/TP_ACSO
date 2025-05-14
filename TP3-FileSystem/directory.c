#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int directory_findname(struct unixfilesystem *file_system, const char *name, int directory_inumber, struct direntv6 *directory_entry) {
    struct inode inode_data;
    if (inode_iget(file_system, directory_inumber, &inode_data) < 0) {
        return -1;
    }

    if (!(inode_data.i_mode & IALLOC) || ((inode_data.i_mode & IFMT) != IFDIR)) {
        return -1;
    }

    int filesize = inode_getsize(&inode_data);
    int num_blocks = (filesize + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;
    int block = 0; 
    for (block; block < num_blocks; block++) {
        char buffer[DISKIMG_SECTOR_SIZE];
        int bytes = file_getblock(file_system, directory_inumber, block, buffer);
        if (bytes < 0){ 
            return -1;
        }

        int num_entries = bytes / sizeof(struct direntv6);
        struct direntv6 *entries = (struct direntv6 *) buffer;

        int i = 0;
        for (i; i < num_entries; i++) {
            if (strncmp(name, entries[i].d_name, 14) == 0) {
                *directory_entry = entries[i];
                return 0;
            }
        }
    }

    return -1;
}

