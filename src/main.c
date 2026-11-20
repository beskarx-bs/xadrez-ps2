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

int cursorX=0, cursorY=0;
int selecionado=0;
int origemX, origemY;
int vezBrancas=1;

int movimentos[8][8];
int capturas[8][8];

int emXeque=0;
int fimDeJogo=0;
int vencedor=0;

int dentro(int x,int y)
{
    return x>=0 && x<8 && y>=0 && y<8;
}

int absoluto(int n)
{
    return n<0 ? -n : n;
}

int ehBranca(char p)
{
    return p>='A' && p<='Z';
}

int ehPreta(char p)
{
    return p>='a' && p<='z';
}

int inimiga(char p,int branca)
{
    return branca ? ehPreta(p) : ehBranca(p);
}

int ehRei(char p)
{
    return p=='K' || p=='k';
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

void adicionarPseudo(int lista[8][8],
                     int caps[8][8],
                     int x,int y,int branca)
{
    if(!dentro(x,y)) return;

    char p=tabuleiro[y][x];

    if(p==' ')
        lista[y][x]=1;
    else if(inimiga(p,branca) && !ehRei(p))
    {
        lista[y][x]=1;
        caps[y][x]=1;
    }
}

void gerarPseudo(int ox,int oy,
                 int lista[8][8],
                 int caps[8][8])
{
    int x,y,i;

    for(y=0;y<8;y++)
        for(x=0;x<8;x++)
        {
            lista[y][x]=0;
            caps[y][x]=0;
        }

    char p=tabuleiro[oy][ox];
    int branca=ehBranca(p);
    char tipo=branca ? p : p-'a'+'A';

    int dx[8]={1,-1,0,0,1,1,-1,-1};
    int dy[8]={0,0,1,-1,1,-1,1,-1};

    if(tipo=='P')
    {
        int direcao=branca ? -1 : 1;
        int inicio=branca ? 6 : 1;

        y=oy+direcao;

        if(dentro(ox,y) && tabuleiro[y][ox]==' ')
        {
            adicionarPseudo(lista,caps,ox,y,branca);

            if(oy==inicio &&
               dentro(ox,y+direcao) &&
               tabuleiro[y+direcao][ox]==' ')
                adicionarPseudo(lista,caps,ox,y+direcao,branca);
        }

        for(i=-1;i<=1;i+=2)
        {
            x=ox+i;
            y=oy+direcao;

            if(dentro(x,y) &&
               inimiga(tabuleiro[y][x],branca) &&
               !ehRei(tabuleiro[y][x]))
            {
                adicionarPseudo(lista,caps,x,y,branca);
            }
        }
    }
    else if(tipo=='N')
    {
        int nx[8]={1,2,2,1,-1,-2,-2,-1};
        int ny[8]={-2,-1,1,2,2,1,-1,-2};

        for(i=0;i<8;i++)
            adicionarPseudo(lista,caps,
                            ox+nx[i],oy+ny[i],branca);
    }
    else if(tipo=='K')
    {
        for(y=-1;y<=1;y++)
            for(x=-1;x<=1;x++)
                if(x!=0 || y!=0)
                    adicionarPseudo(lista,caps,
                                    ox+x,oy+y,branca);
    }
    else
    {
        int inicio=tipo=='B' ? 4 : 0;
        int quantidade=tipo=='R' ? 4 :
                       tipo=='B' ? 4 : 8;

        for(i=inicio;i<inicio+quantidade;i++)
        {
            x=ox+dx[i];
            y=oy+dy[i];

            while(dentro(x,y))
            {
                char destino=tabuleiro[y][x];

                if(destino==' ')
                    adicionarPseudo(lista,caps,x,y,branca);
                else
                {
                    adicionarPseudo(lista,caps,x,y,branca);
                    break;
                }

                x+=dx[i];
                y+=dy[i];
            }
        }
    }
}

int casaAtacada(int tx,int ty,int porBrancas)
{
    int x,y,dx,dy;

    int nx[8]={1,2,2,1,-1,-2,-2,-1};
    int ny[8]={-2,-1,1,2,2,1,-1,-2};

    for(y=0;y<8;y++)
    {
        for(x=0;x<8;x++)
        {
            char p=tabuleiro[y][x];

            if(p==' ') continue;

            if(porBrancas && !ehBranca(p)) continue;
            if(!porBrancas && !ehPreta(p)) continue;

            char tipo=ehBranca(p) ? p : p-'a'+'A';

            dx=tx-x;
            dy=ty-y;

            if(tipo=='P')
            {
                int direcao=porBrancas ? -1 : 1;

                if(absoluto(dx)==1 && dy==direcao)
                    return 1;
            }
            else if(tipo=='N')
            {
                for(int i=0;i<8;i++)
                    if(dx==nx[i] && dy==ny[i])
                        return 1;
            }
            else if(tipo=='K')
            {
                if(absoluto(dx)<=1 &&
                   absoluto(dy)<=1 &&
                   (dx!=0 || dy!=0))
                    return 1;
            }
            else
            {
                int diagonal=(absoluto(dx)==absoluto(dy));
                int reta=(dx==0 || dy==0);

                if(tipo=='R' && !reta) continue;
                if(tipo=='B' && !diagonal) continue;
                if(tipo=='Q' && !reta && !diagonal) continue;

                int passoX=(dx>0) ? 1 : (dx<0 ? -1 : 0);
                int passoY=(dy>0) ? 1 : (dy<0 ? -1 : 0);

                int cx=x+passoX;
                int cy=y+passoY;
                int bloqueado=0;

                while(cx!=tx || cy!=ty)
                {
                    if(tabuleiro[cy][cx]!=' ')
                    {
                        bloqueado=1;
                        break;
                    }

                    cx+=passoX;
                    cy+=passoY;
                }

                if(!bloqueado)
                    return 1;
            }
        }
    }

    return 0;
}

int reiEmXeque(int branca)
{
    char rei=branca ? 'K' : 'k';

    for(int y=0;y<8;y++)
    {
        for(int x=0;x<8;x++)
        {
            if(tabuleiro[y][x]==rei)
                return casaAtacada(x,y,!branca);
        }
    }

    return 1;
}

int movimentoSeguro(int ox,int oy,int dx,int dy,
                    int branca)
{
    char copia[8][8];

    for(int y=0;y<8;y++)
        for(int x=0;x<8;x++)
            copia[y][x]=tabuleiro[y][x];

    tabuleiro[dy][dx]=tabuleiro[oy][ox];
    tabuleiro[oy][ox]=' ';

    int seguro=!reiEmXeque(branca);

    for(int y=0;y<8;y++)
        for(int x=0;x<8;x++)
            tabuleiro[y][x]=copia[y][x];

    return seguro;
}

void gerarMovimentos(void)
{
    int lista[8][8];
    int caps[8][8];

    limparMovimentos();

    char p=tabuleiro[origemY][origemX];
    int branca=ehBranca(p);

    gerarPseudo(origemX,origemY,lista,caps);

    for(int y=0;y<8;y++)
    {
        for(int x=0;x<8;x++)
        {
            if(lista[y][x] &&
               movimentoSeguro(origemX,origemY,x,y,branca))
            {
                movimentos[y][x]=1;
                capturas[y][x]=caps[y][x];
            }
        }
    }
}

int temJogadaLegal(int branca)
{
    int lista[8][8];
    int caps[8][8];

    for(int oy=0;oy<8;oy++)
    {
        for(int ox=0;ox<8;ox++)
        {
            char p=tabuleiro[oy][ox];

            if(p==' ') continue;
            if(branca && !ehBranca(p)) continue;
            if(!branca && !ehPreta(p)) continue;

            gerarPseudo(ox,oy,lista,caps);

            for(int y=0;y<8;y++)
            {
                for(int x=0;x<8;x++)
                {
                    if(lista[y][x] &&
                       movimentoSeguro(ox,oy,x,y,branca))
                        return 1;
                }
            }
        }
    }

    return 0;
}

void verificarPartida(void)
{
    emXeque=reiEmXeque(vezBrancas);

    if(!temJogadaLegal(vezBrancas))
    {
        if(emXeque)
        {
            fimDeJogo=1;
            vencedor=!vezBrancas;
        }
        else
        {
            fimDeJogo=2;
        }
    }
}

void bloco(GSGLOBAL *gs,int x,int y,int w,int h,
           u64 contorno,u64 cor)
{
    gsKit_prim_sprite(gs,x,y,x+w,y+h,2,contorno);
    gsKit_prim_sprite(gs,x+2,y+2,x+w-2,y+h-2,2,cor);
}

void desenharPeca(GSGLOBAL *gs,int x,int y,
                  char tipo,int branca)
{
    u64 contorno=branca
        ? GS_SETREG_RGBAQ(35,45,60,0x80,0)
        : GS_SETREG_RGBAQ(210,220,235,0x80,0);

    u64 cor=branca
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

void localizarRei(int branca,int *rx,int *ry)
{
    char rei=branca ? 'K' : 'k';

    for(int y=0;y<8;y++)
        for(int x=0;x<8;x++)
            if(tabuleiro[y][x]==rei)
            {
                *rx=x;
                *ry=y;
                return;
            }
}

/* Letras simples desenhadas com pixels para a tela de resultado. */
void desenharLetra(GSGLOBAL *gs,int x,int y,char letra,
                   int escala,u64 cor)
{
    unsigned char linhas[7]={0,0,0,0,0,0,0};

    switch(letra)
    {
        case 'A': { unsigned char a[7]={14,17,17,31,17,17,17}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'B': { unsigned char a[7]={30,17,17,30,17,17,30}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'C': { unsigned char a[7]={14,17,16,16,16,17,14}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'D': { unsigned char a[7]={30,17,17,17,17,17,30}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'E': { unsigned char a[7]={31,16,16,30,16,16,31}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'F': { unsigned char a[7]={31,16,16,30,16,16,16}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'G': { unsigned char a[7]={14,17,16,23,17,17,15}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'H': { unsigned char a[7]={17,17,17,31,17,17,17}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'I': { unsigned char a[7]={31,4,4,4,4,4,31}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'J': { unsigned char a[7]={7,2,2,2,18,18,12}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'K': { unsigned char a[7]={17,18,20,24,20,18,17}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'L': { unsigned char a[7]={16,16,16,16,16,16,31}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'M': { unsigned char a[7]={17,27,21,21,17,17,17}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'N': { unsigned char a[7]={17,25,21,19,17,17,17}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'O': { unsigned char a[7]={14,17,17,17,17,17,14}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'P': { unsigned char a[7]={30,17,17,30,16,16,16}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'Q': { unsigned char a[7]={14,17,17,17,21,18,13}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'R': { unsigned char a[7]={30,17,17,30,20,18,17}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'S': { unsigned char a[7]={15,16,16,14,1,1,30}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'T': { unsigned char a[7]={31,4,4,4,4,4,4}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'U': { unsigned char a[7]={17,17,17,17,17,17,14}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'V': { unsigned char a[7]={17,17,17,17,17,10,4}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'W': { unsigned char a[7]={17,17,17,21,21,21,10}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'X': { unsigned char a[7]={17,17,10,4,10,17,17}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'Y': { unsigned char a[7]={17,17,10,4,4,4,4}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case 'Z': { unsigned char a[7]={31,1,2,4,8,16,31}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case '-': { unsigned char a[7]={0,0,0,31,0,0,0}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        case '!': { unsigned char a[7]={4,4,4,4,4,0,4}; for(int i=0;i<7;i++) linhas[i]=a[i]; break; }
        default: break;
    }

    for(int linha=0;linha<7;linha++)
        for(int coluna=0;coluna<5;coluna++)
            if(linhas[linha] & (1 << (4-coluna)))
                gsKit_prim_sprite(gs,
                    x+coluna*escala,y+linha*escala,
                    x+(coluna+1)*escala,y+(linha+1)*escala,
                    6,cor);
}

void desenharTextoCentralizado(GSGLOBAL *gs,int y,
                               const char *texto,int escala,u64 cor)
{
    int quantidade=0;

    while(texto[quantidade]!='\0')
        quantidade++;

    int largura=quantidade*6*escala;
    int x=(gs->Width-largura)/2;

    for(int i=0;i<quantidade;i++)
        desenharLetra(gs,x+i*6*escala,y,texto[i],escala,cor);
}

void reiniciarPartida(void)
{
    char inicial[8][8] = {
        {'r','n','b','q','k','b','n','r'},
        {'p','p','p','p','p','p','p','p'},
        {' ',' ',' ',' ',' ',' ',' ',' '},
        {' ',' ',' ',' ',' ',' ',' ',' '},
        {' ',' ',' ',' ',' ',' ',' ',' '},
        {' ',' ',' ',' ',' ',' ',' ',' '},
        {'P','P','P','P','P','P','P','P'},
        {'R','N','B','Q','K','B','N','R'}
    };

    for(int y=0;y<8;y++)
        for(int x=0;x<8;x++)
            tabuleiro[y][x]=inicial[y][x];

    cursorX=0;
    cursorY=0;
    selecionado=0;
    vezBrancas=1;

    emXeque=0;
    fimDeJogo=0;
    vencedor=0;

    limparMovimentos();
}

int casaSelecionavel(int x,int y)
{
    char p=tabuleiro[y][x];
    return p!=' ' &&
        ((vezBrancas && ehBranca(p)) ||
         (!vezBrancas && ehPreta(p)));
}

void navegarCursor(int dx,int dy)
{
    int melhorX=cursorX;
    int melhorY=cursorY;
    int melhorPontos=99999;

    for(int y=0;y<8;y++)
    {
        for(int x=0;x<8;x++)
        {
            int valido=selecionado ? movimentos[y][x] : casaSelecionavel(x,y);
            if(!valido) continue;
            if(x==cursorX && y==cursorY) continue;

            int principal,secundario;

            if(dx!=0)
            {
                if((x-cursorX)*dx<=0) continue;
                principal=absoluto(x-cursorX);
                secundario=absoluto(y-cursorY);
            }
            else
            {
                if((y-cursorY)*dy<=0) continue;
                principal=absoluto(y-cursorY);
                secundario=absoluto(x-cursorX);
            }

            int pontos=principal*100+secundario*10;
            if(pontos<melhorPontos)
            {
                melhorPontos=pontos;
                melhorX=x;
                melhorY=y;
            }
        }
    }

    if(melhorPontos!=99999)
    {
        cursorX=melhorX;
        cursorY=melhorY;
    }
}

void processarControle(int botoes)
{
    if(fimDeJogo)
    {
        if(botoes & PAD_CROSS)
            reiniciarPartida();
        return;
    }

    if(botoes&PAD_LEFT) navegarCursor(-1,0);
    if(botoes&PAD_RIGHT) navegarCursor(1,0);
    if(botoes&PAD_UP) navegarCursor(0,-1);
    if(botoes&PAD_DOWN) navegarCursor(0,1);

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
                limparMovimentos();

                vezBrancas=!vezBrancas;
                verificarPartida();
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
                u16 pressionados=
                    atuais & (u16)(~botoesAnteriores);

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

        if(emXeque)
        {
            int rx=0,ry=0;
            localizarRei(vezBrancas,&rx,&ry);

            desenharBorda(gsGlobal,
                inicioX+rx*tamanho,
                inicioY+ry*tamanho,
                tamanho,
                GS_SETREG_RGBAQ(255,40,40,0x80,0));
        }

        if(fimDeJogo==1)
        {
            int rx=0,ry=0;
            localizarRei(vencedor,&rx,&ry);

            desenharBorda(gsGlobal,
                inicioX+rx*tamanho,
                inicioY+ry*tamanho,
                tamanho,
                GS_SETREG_RGBAQ(40,255,100,0x80,0));
        }

        if(!fimDeJogo)
        {
            desenharBorda(gsGlobal,
                inicioX+cursorX*tamanho,
                inicioY+cursorY*tamanho,
                tamanho,
                GS_SETREG_RGBAQ(30,150,255,0x80,0));
        }
        else
        {
            u64 painel=GS_SETREG_RGBAQ(5,12,28,0x80,0);
            u64 branco=GS_SETREG_RGBAQ(245,248,255,0x80,0);
            u64 azul=GS_SETREG_RGBAQ(70,175,255,0x80,0);

            gsKit_prim_sprite(gsGlobal,
                55,145,585,300,5,painel);

            desenharBorda(gsGlobal,55,145,530,
                GS_SETREG_RGBAQ(65,145,220,0x80,0));

            desenharTextoCentralizado(gsGlobal,165,
                fimDeJogo==1 ? "XEQUE-MATE!" : "EMPATE!",
                3,azul);

            if(fimDeJogo==1)
                desenharTextoCentralizado(gsGlobal,205,
                    vencedor ? "BRANCAS VENCERAM!" :
                               "PRETAS VENCERAM!",
                    2,branco);

            desenharTextoCentralizado(gsGlobal,250,
                "X - JOGAR NOVAMENTE",2,branco);
        }

        gsKit_queue_exec(gsGlobal);
        gsKit_sync_flip(gsGlobal);
        gsKit_queue_reset(gsGlobal->Per_Queue);
    }

    return 0;
}
