#include <tamtypes.h>
#include <kernel.h>
#include <debug.h>

int main(void)
{
    init_scr();
    scr_printf("XADREZ PS2\n");
    scr_printf("Nosso jogo esta funcionando!");

    SleepThread();

    return 0;
}
