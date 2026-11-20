
#include <gsKit.h>
#include <dmaKit.h>

int main(int argc, char *argv[])
{
    GSGLOBAL *gsGlobal = gsKit_init_global();

    dmaKit_init(
        D_CTRL_RELE_OFF,
        D_CTRL_MFD_OFF,
        D_CTRL_STS_UNSPEC,
        D_CTRL_STD_OFF,
        D_CTRL_RCYC_8,
        1 << DMA_CHANNEL_GIF
    );

    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_init_screen(gsGlobal);
    gsKit_mode_switch(gsGlobal, GS_PERSISTENT);

    while (1)
    {
        gsKit_clear(
            gsGlobal,
            GS_SETREG_RGBAQ(8, 20, 45, 0x80, 0)
        );

        gsKit_queue_exec(gsGlobal);
        gsKit_sync_flip(gsGlobal);
        gsKit_queue_reset(gsGlobal->Per_Queue);
    }

    return 0;
}
