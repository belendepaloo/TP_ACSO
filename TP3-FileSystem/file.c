#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "file.h"
#include "inode.h"
#include "diskimg.h"

int file_getblock(struct unixfilesystem *file_system, int inumber, int blockNum, void *buffer) {
    struct inode inode_data;
    if (inode_iget(file_system, inumber, &inode_data) < 0) {
        return -1;
    }

    int block = inode_indexlookup(file_system, &inode_data, blockNum);
    if (block == -1) {
        return -1;
    }

    int bytes = diskimg_readsector(file_system->dfd, block, buffer);
    if (bytes == -1) {
        return -1;
    }

    int file_size = inode_getsize(&inode_data);
    int remaining_bytes = file_size - (blockNum * DISKIMG_SECTOR_SIZE);

    if (remaining_bytes <= 0) {
        return 0;
    } else if (remaining_bytes >= DISKIMG_SECTOR_SIZE) {
        return DISKIMG_SECTOR_SIZE;
    } else {
        return remaining_bytes;
    }
}


