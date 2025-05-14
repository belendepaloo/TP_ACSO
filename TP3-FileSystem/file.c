#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "file.h"
#include "inode.h"
#include "diskimg.h"

int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    struct inode inode_data;
    if (inode_iget(fs, inumber, &inode_data) < 0) {
        return -1;
    }

    int block = inode_indexlookup(fs, &inode_data, blockNum);
    if (block == -1) {
        return -1;
    }

    int bytes = diskimg_readsector(fs->dfd, block, buf);
    if (bytes == -1) {
        return -1;
    }

    int filesize = inode_getsize(&inode_data);
    int max_bytes = filesize - (blockNum * DISKIMG_SECTOR_SIZE);

    if (max_bytes >= DISKIMG_SECTOR_SIZE) {
        return DISKIMG_SECTOR_SIZE;
    } else {
        return max_bytes > 0 ? max_bytes : 0;
    }
}


