#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "inode.h"
#include "diskimg.h"


/**
 * TODO
 */
int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    if (inumber < 1) {
        return -1;
    }

    int inodes_per_sector = DISKIMG_SECTOR_SIZE / sizeof(struct inode);
    int sector = INODE_START_SECTOR + (inumber - 1) / inodes_per_sector;
    int offset = (inumber - 1) % inodes_per_sector;

    struct inode inodes[inodes_per_sector];
    int err = diskimg_readsector(fs->dfd, sector, &inodes);
    if (err == -1) {
        return -1;
    }

    *inp = inodes[offset];
    return 0;
}


/**
 * TODO
 */
int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
    if (!(inp->i_mode & IALLOC)) {
        return -1; // inodo no asignado
    }

    if (!(inp->i_mode & ILARG)) {
        // archivo pequeño: i_addr[] contiene directamente los bloques de datos
        if (blockNum < 0 || blockNum >= 8) {
            return -1;
        }
        return inp->i_addr[blockNum];
    } else {
        // archivo grande: i_addr[0..6] → bloques indirectos, i_addr[7] → doble indirecto
        const int BLOCKS_PER_INDIRECT = DISKIMG_SECTOR_SIZE / sizeof(uint16_t); // 512 / 2 = 256

        if (blockNum < 7 * BLOCKS_PER_INDIRECT) {
            // primer bloque indirecto
            int indirect_block_index = blockNum / BLOCKS_PER_INDIRECT;
            int indirect_offset = blockNum % BLOCKS_PER_INDIRECT;

            uint16_t indirect_block[BLOCKS_PER_INDIRECT];
            int err = diskimg_readsector(fs->dfd, inp->i_addr[indirect_block_index], &indirect_block);
            if (err == -1) return -1;

            return indirect_block[indirect_offset];

        } else {
            // bloque doblemente indirecto
            int remaining = blockNum - 7 * BLOCKS_PER_INDIRECT;

            uint16_t double_indirect_block[BLOCKS_PER_INDIRECT];
            int err = diskimg_readsector(fs->dfd, inp->i_addr[7], &double_indirect_block);
            if (err == -1) return -1;

            int indirect_block_index = remaining / BLOCKS_PER_INDIRECT;
            int indirect_offset = remaining % BLOCKS_PER_INDIRECT;

            if (indirect_block_index >= BLOCKS_PER_INDIRECT) return -1;

            uint16_t indirect_block[BLOCKS_PER_INDIRECT];
            err = diskimg_readsector(fs->dfd, double_indirect_block[indirect_block_index], &indirect_block);
            if (err == -1) return -1;

            return indirect_block[indirect_offset];
        }
    }
}


int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
