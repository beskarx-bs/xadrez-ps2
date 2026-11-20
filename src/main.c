
#include <gsKit.h>
#include <dmaKit.h>
#include <libpad.h>
#include <tamtypes.h>

static unsigned char padBuffer[256] __attribute__((aligned(64)));

char tabuleiro[8][8] = {
    {'r','n','b','q','k','b','n','r'},
    {'p','p','p','p','p','p','p','p'},
    {' ',' ',' ',' ',' ',' ',' ',' '},
    {' ',' ',' ',' ',' ',' ',' ',' '},
    {' ',' ',' ',' ',' ',' ',' ',' '},
    {' ',' ',' ',' ',' ',' ',' ',' '},
    {'P','P','P','P','P','P','P','P'},
    {'R','N','B','Q','K','B','N','R'}
};

int cursorX = 0, cursorY = 0;
int selecionado = 0;
int origemX, origemY;
int vezBrancas = 1;

int movimentos[8][8];
int capturas[8][8];

int dentro(int x, int y)
{
    return x >= 0 && x < 8 && y >= 0 && y < 8;
}

int ehBranca(char p)
{
    return p >= 'A' && p <= 'Z';
}

int ehPreta(char p)
{
    return p >= 'a' && p <= 'z';
}

int inimiga(char p, int branca)
{
    return branca ? ehPreta(p) : ehBranca(p);
}

void bloco(GSGLOBAL *gs, int x, int y, int w, int h,
           u64 contorno, u64 cor)
{
    gsKit_prim_sprite(gs, x, y, x+w, y+h, 2, contorno);
    gsKit_prim_sprite(gs, x+2, y+2, x+w-2, y+h-2, 2, cor);
}

void desenharPeca(GSGLOBAL *gs, int x, int y,
                  char tipo, int branca)
{
    u64 contorno = branca
        ? GS_SETREG_RGBAQ(35,45,60,0x80,0)
        : GS_SETREG_RGBAQ(210,220,235,0x80,0);

    u64 cor = branca
        ? GS_SETREG_RGBAQ(240,240,225,0x80,0)
        : GS_SETREG_RGBAQ(20,30,45,0x80,0);

    bloco(gs,x+7,y+37,34,7,contorno,cor);
    bloco(gs,x+13,y+28,22,10,contorno,cor);

    switch(tipo)
    {
        case 'P':
            bloco(gs,x+17,y+15,14,14,contorno,cor);
            break;
        case 'R':
            bloco(gs,x+13,y+11,22,18,contorno,cor);
            bloco(gs,x+13,y+7,6,7,contorno,cor);
            bloco(gs,x+21,y+7,6,7,contorno,cor);
            bloco(gs,x+29,y+7,6,7,contorno,cor);
            break;
        case 'N':
            bloco(gs,x+15,y+14,18,15,contorno,cor);
            bloco(gs,x+19,y+8,15,12,contorno,cor);
            bloco(gs,x+27,y+7,7,7,contorno,cor);
            break;
        case 'B':
            bloco(gs,x+17,y+13,14,16,contorno,cor);
            bloco(gs,x+19,y+8,10,7,contorno,cor);
            break;
        case 'Q':
            bloco(gs,x+13,y+15,22,14,contorno,cor);
            bloco(gs,x+11,y+9,7,8,contorno,cor);
            bloco(gs,x+21,y+5,7,10,contorno,cor);
            bloco(gs,x+31,y+9,7,8,contorno,cor);
            break;
        case 'K':
            bloco(gs,x+14,y+14,20,15,contorno,cor);
            bloco(gs,x+21,y+5,6,19,contorno,cor);
            bloco(gs,x+16,y+10,16,6,contorno,cor);
            break;
    }
}

void limparMovimentos(void)
{
    int x,y;
    for(y=0;y<8;y++)
        for(x=0;x<8;x++)
        {
            movimentos[y][x]=0;
            capturas[y][x]=0;
        }
}

void adicionar(int x, int y, int branca)
{
    if(!dentro(x,y)) return;

    char p=tabuleiro[y][x];

    if(p==' ')
        movimentos[y][x]=1;
    else if(inimiga(p,branca))
    {
        movimentos[y][x]=1;
        capturas[y][x]=1;
    }
}

void gerarMovimentos(void)
{
    limparMovimentos();

    char p=tabuleiro[origemY][origemX];
    int branca=ehBranca(p);
    char tipo=branca ? p : p-'a'+'A';

    int x,y,i;
    int dx[8]={1,-1,0,0,1,1,-1,-1};
    int dy[8]={0,0,1,-1,1,-1,1,-1};

    if(tipo=='P')
    {
        int direcao=branca ? -1 : 1;
        int inicio=branca ? 6 : 1;

        y=origemY+direcao;

        if(dentro(origemX,y) && tabuleiro[y][origemX]==' ')
        {
            adicionar(origemX,y,branca);

            if(origemY==inicio &&
               tabuleiro[y+direcao][origemX]==' ')
                adicionar(origemX,y+direcao,branca);
        }

        for(i=-1;i<=1;i+=2)
        {
            x=origemX+i;
            y=origemY+direcao;

            if(dentro(x,y) &&
               inimiga(tabuleiro[y][x],branca))
            {
                adicionar(x,y,branca);
            }
        }
    }

    else if(tipo=='N')
    {
        int nx[8]={1,2,2,1,-1,-2,-2,-1};
        int ny[8]={-2,-1,1,2,2,1,-1,-2};

        for(i=0;i<8;i++)
            adicionar(origemX+nx[i],origemY+ny[i],branca);
    }

    else if(tipo=='K')
    {
        for(y=-1;y<=1;y++)
            for(x=-1;x<=1;x++)
                if(x!=0 || y!=0)
                    adicionar(origemX+x,origemY+y,branca);
    }

    else
    {
        int direcoes=tipo=='R' ? 4 :
                     tipo=='B' ? 4 : 8;

        int inicio=tipo=='B' ? 4 : 0;

        for(i=inicio;i<inicio+direcoes;i++)
        {
            x=origemX+dx[i];
            y=origemY+dy[i];

            while(dentro(x,y))
            {
                char destino=tabuleiro[y][x];

                if(destino==' ')
                    adicionar(x,y,branca);
                else
                {
                    if(inimiga(destino,branca))
                        adicionar(x,y,branca);
                    break;
                }

                x+=dx[i];
                y+=dy[i];
            }
        }
    }
}

void desenharBorda(GSGLOBAL *gs,int x,int y,int t,u64 cor)
{
    int b=4;

    gsKit_prim_sprite(gs,x,y,x+t,y+b,4,cor);
    gsKit_prim_sprite(gs,x,y+t-b,x+t,y+t,4,cor);
    gsKit_prim_sprite(gs,x,y,x+b,y+t,4,cor);
    gsKit_prim_sprite(gs,x+t-b,y,x+t,y+t,4,cor);
}

void desenharBolinha(GSGLOBAL *gs,int cx,int cy,u64 cor)
{
    gsKit_prim_sprite(gs,cx-2,cy-5,cx+2,cy-2,3,cor);
    gsKit_prim_sprite(gs,cx-5,cy-2,cx+5,cy+2,3,cor);
    gsKit_prim_sprite(gs,cx-2,cy+2,cx+2,cy+5,3,cor);
}

void processarControle(int botoes)
{
    if(botoes&PAD_LEFT && cursorX>0) cursorX--;
    if(botoes&PAD_RIGHT && cursorX<7) cursorX++;
    if(botoes&PAD_UP && cursorY>0) cursorY--;
    if(botoes&PAD_DOWN && cursorY<7) cursorY++;

    if(botoes&PAD_CIRCLE)
    {
        selecionado=0;
        limparMovimentos();
    }

    if(botoes&PAD_CROSS)
    {
        char p=tabuleiro[cursorY][cursorX];

        if(!selecionado)
        {
            if(p!=' ' &&
              ((vezBrancas && ehBranca(p)) ||
               (!vezBrancas && ehPreta(p))))
            {
                origemX=cursorX;
                origemY=cursorY;
                selecionado=1;
                gerarMovimentos();
            }
        }
        else
        {
            if(movimentos[cursorY][cursorX])
            {
                tabuleiro[cursorY][cursorX]=
                    tabuleiro[origemY][origemX];

                tabuleiro[origemY][origemX]=' ';

                selecionado=0;
                vezBrancas=!vezBrancas;
                limparMovimentos();
            }
            else if(p!=' ' &&
              ((vezBrancas && ehBranca(p)) ||
               (!vezBrancas && ehPreta(p))))
            {
                origemX=cursorX;
                origemY=cursorY;
                gerarMovimentos();
            }
        }
    }
}

int main(int argc,char *argv[])
{
    GSGLOBAL *gsGlobal=gsKit_init_global();

    dmaKit_init(
        D_CTRL_RELE_OFF,
        D_CTRL_MFD_OFF,
        D_CTRL_STS_UNSPEC,
        D_CTRL_STD_OFF,
        D_CTRL_RCYC_8,
        1<<DMA_CHANNEL_GIF
    );

    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_init_screen(gsGlobal);
    gsKit_mode_switch(gsGlobal,GS_PERSISTENT);

    padInit(0);
    padPortOpen(0,0,padBuffer);

    struct padButtonStatus pad;
    u16 botoesAnteriores=0;

    int tamanho=48;
    int inicioX=(gsGlobal->Width-8*tamanho)/2;
    int inicioY=(gsGlobal->Height-8*tamanho)/2;

    while(1)
    {
        if(padGetState(0,0)==PAD_STATE_STABLE)
        {
            if(padRead(0,0,&pad))
            {
                u16 atuais=(u16)(~pad.btns);
                u16 pressionados=atuais & (u16)(~botoesAnteriores);

                processarControle(pressionados);
                botoesAnteriores=atuais;
            }
        }

        gsKit_clear(gsGlobal,
            GS_SETREG_RGBAQ(8,20,45,0x80,0));

        for(int linha=0;linha<8;linha++)
        {
            for(int coluna=0;coluna<8;coluna++)
            {
                int x=inicioX+coluna*tamanho;
                int y=inicioY+linha*tamanho;

                u64 cor=(linha+coluna)%2==0
                    ? GS_SETREG_RGBAQ(190,205,220,0x80,0)
                    : GS_SETREG_RGBAQ(55,85,115,0x80,0);

                gsKit_prim_sprite(gsGlobal,
                    x,y,x+tamanho,y+tamanho,1,cor);

                char p=tabuleiro[linha][coluna];

                if(p!=' ')
                {
                    int branca=ehBranca(p);
                    char tipo=branca ? p : p-'a'+'A';

                    desenharPeca(gsGlobal,x,y,tipo,branca);
                }
            }
        }

        if(selecionado)
        {
            for(int linha=0;linha<8;linha++)
            {
                for(int coluna=0;coluna<8;coluna++)
                {
                    if(movimentos[linha][coluna])
                    {
                        int cx=inicioX+coluna*tamanho+tamanho/2;
                        int cy=inicioY+linha*tamanho+tamanho/2;

                        u64 cor=capturas[linha][coluna]
                            ? GS_SETREG_RGBAQ(50,230,100,0x80,0)
                            : GS_SETREG_RGBAQ(35,150,255,0x80,0);

                        desenharBolinha(gsGlobal,cx,cy,cor);
                    }
                }
            }

            desenharBorda(gsGlobal,
                inicioX+origemX*tamanho,
                inicioY+origemY*tamanho,
                tamanho,
                GS_SETREG_RGBAQ(255,190,40,0x80,0));
        }

        desenharBorda(gsGlobal,
            inicioX+cursorX*tamanho,
            inicioY+cursorY*tamanho,
            tamanho,
            GS_SETREG_RGBAQ(30,150,255,0x80,0));

        gsKit_queue_exec(gsGlobal);
        gsKit_sync_flip(gsGlobal);
        gsKit_queue_reset(gsGlobal->Per_Queue);
    }

    return 0;
}
