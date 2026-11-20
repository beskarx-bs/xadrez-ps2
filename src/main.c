
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

    int tamanho = 48;
    int inicioX = (gsGlobal->Width - 8 * tamanho) / 2;
    int inicioY = (gsGlobal->Height - 8 * tamanho) / 2;

    while (1)
    {
        gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(8, 20, 45, 0x80, 0));

        for (int linha = 0; linha < 8; linha++)
        {
            for (int coluna = 0; coluna < 8; coluna++)
            {
                int x1 = inicioX + coluna * tamanho;
                int y1 = inicioY + linha * tamanho;
                int x2 = x1 + tamanho;
                int y2 = y1 + tamanho;

                u64 cor;

                if ((linha + coluna) % 2 == 0)
                    cor = GS_SETREG_RGBAQ(190, 205, 220, 0x80, 0);
                else
                    cor = GS_SETREG_RGBAQ(55, 85, 115, 0x80, 0);

                gsKit_prim_sprite(
                    gsGlobal,
                    x1, y1, x2, y2,
                    1,
                    cor
                );
            }
        }

        gsKit_queue_exec(gsGlobal);
        gsKit_sync_flip(gsGlobal);
        gsKit_queue_reset(gsGlobal->Per_Queue);
    }

    return 0;
}
