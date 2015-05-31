#include <cstdio>
#include <cstdint>
#include <string>
#include <memory>
#include <new>

#include "sPcie.h"

uint32_t roundup(uint32_t num, uint32_t blk){
    return ((num + blk - 1) / blk) * blk;
}

int main()
{
    char ip_mode;
    ip_mode = get_pcie_cfg_mode();
    printf(">configure mode:x%dgen%d\t", (ip_mode>>4)&0x0F, (ip_mode&0x0F));
    ip_mode = get_pcie_cur_mode();
    printf(" current mode:x%dgen%d\n", (ip_mode>>4)&0x0F, (ip_mode&0x0F));

    struct AES_block
    {
        uint32_t data[4];
    };

    AES_block aes_key = {{0x00000000, 0x00000000, 0x00000000, 0x00000000, }};

    AES_block aes_data[] = {{{0x00000000, 0x00000000, 0x00000000, 0x00000000, }},
                            {{0x00000000, 0x00000000, 0x00000000, 0x00000001, }},
                            {{0x00000000, 0x00000000, 0x00000000, 0x00000002, }},};
    write_usr_reg(0, &aes_key.data[0]);
    write_usr_reg(1, &aes_key.data[1]);
    write_usr_reg(2, &aes_key.data[2]);
    write_usr_reg(3, &aes_key.data[3]);
    write_usr_reg(4, 0); // start init key

    const uint32_t dma_step_bytes = 1024;
    const uint32_t dma_step_symbols = 8;

    const uint32_t dma_length_bytes = roundup(sizeof(aes_data), dma_step_bytes);
    const uint32_t dma_length_symbols = dma_length_bytes / 4;

    uint8_t * dma_source = new uint8_t[dma_length_bytes];
    uint8_t * dma_drain = new uint8_t[dma_length_bytes];

    for (int i = 0; i < sizeof(aes_data) / sizeof(AES_block); ++i)
    {
        AES_block & block = aes_data[i];
        for (int j = 0; j < 4; ++j)
        {
            for (int k = 0; k < 4; ++k)
            {
                dma_source[i * 16 + j * 4 + k] = block.data[j] >> (8 * (4 - k));
            }
        }
    }

    for (int i = 0; i < dma_length_symbols; i += dma_step_symbols)
    {
        if (dma_host2board(i+1, dma_source) < 0)
        {
            fprintf(stderr, "dma_host2board error\n");
            return -1;
        }
        if (dma_board2host(i+1, dma_drain) < 0)
        {
            fprintf(stderr, "dma_board2host error\n");
            return -1;
        }
    }

    for (int i = 0; i < sizeof(aes_data) / sizeof(AES_block); ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            for (int k = 0; k < 4; ++k)
            {
                printf("%02x", dma_drain[i * 16 + j * 4 + k]);
            }
        }
        printf("\n");
    }

    return 0;
}
