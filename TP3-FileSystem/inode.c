#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "inode.h"
#include "diskimg.h"

#define INODE_SIZE sizeof(struct inode)


int inode_iget(struct unixfilesystem *file_system, int inumber, struct inode *inode_pointer) {
    if (inumber < 1) {
        return -1;
    }

    int num_inodes_per_sector = DISKIMG_SECTOR_SIZE / sizeof(struct inode);
    int zero_based_inumber = inumber - 1;
    int inode_sector_number = INODE_START_SECTOR + (zero_based_inumber / num_inodes_per_sector);
    int inode_offset_within_sector = zero_based_inumber % num_inodes_per_sector;

    struct inode inode_sector_buffer[num_inodes_per_sector];

    int read_result = diskimg_readsector(file_system->dfd, inode_sector_number, &inode_sector_buffer);
    if (read_result == -1) {
        return -1;
    }

    *inode_pointer = inode_sector_buffer[inode_offset_within_sector];
    return 0;
}


int inode_indexlookup(struct unixfilesystem *fs, struct inode *inode_pointer, int fBlockNum) {
    if (!(inode_pointer->i_mode & IALLOC)) {
        return -1;
    }

    if (!(inode_pointer->i_mode & ILARG)) {
        if (fBlockNum < 0 || fBlockNum >= 8) {
            return -1;
        }
        return inode_pointer->i_addr[fBlockNum];
    } else {
        const int BLOCKS_PER_INDIRECT = DISKIMG_SECTOR_SIZE / sizeof(uint16_t);

        if (fBlockNum < 7 * BLOCKS_PER_INDIRECT) {
            int indirect_block_index = fBlockNum / BLOCKS_PER_INDIRECT;
            int indirect_offset = fBlockNum % BLOCKS_PER_INDIRECT;

            uint16_t indirect_block[BLOCKS_PER_INDIRECT];
            int error_caution = diskimg_readsector(fs->dfd, inode_pointer->i_addr[indirect_block_index], &indirect_block);
            if (error_caution == -1) {
                return -1;
            }

            return indirect_block[indirect_offset];

        } else {
            int remaining = fBlockNum - 7 * BLOCKS_PER_INDIRECT;

            uint16_t double_indirect_block[BLOCKS_PER_INDIRECT];
            int error_caution = diskimg_readsector(fs->dfd, inode_pointer->i_addr[7], &double_indirect_block);
            if (error_caution == -1) {
                return -1;
            }

            int indirect_block_index = remaining / BLOCKS_PER_INDIRECT;
            int indirect_offset = remaining % BLOCKS_PER_INDIRECT;

            if (indirect_block_index >= BLOCKS_PER_INDIRECT) {
                return -1;
            }

            uint16_t indirect_block[BLOCKS_PER_INDIRECT];
            error_caution = diskimg_readsector(fs->dfd, double_indirect_block[indirect_block_index], &indirect_block);
            if (error_caution == -1) {
                return -1;
            }

            return indirect_block[indirect_offset];
        }
    }
}


int inode_getsize(struct inode *inode_pointer) {
  return ((inode_pointer->i_size0 << 16) | inode_pointer->i_size1); 
}
