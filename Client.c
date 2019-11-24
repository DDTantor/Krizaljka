#ifdef KROFNA
#define dbg(x) printf("%s:%d - %s\n", __func__, __LINE__, x), fflush(stdout)
#else
#define dbg(x) (void)x
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <pthread.h>

#include "Protocol.h"

void obradi_LOGIN(int sock, char *ime)
{
    if(Posalji(sock, LOGIN, ime) != OK)
        perror("Pogreska u LOGIN...izlazim.\n"), exit(0);
}

int obradi_NOVOSLOVO(int sock, char *ime)
{
    char poruka[1];
    
    printf("Koje slovo pogadate:\n");
    scanf("%s", poruka);

    if (Posalji(sock, NOVOSLOVO, poruka) != OK)
        perror("Pogreska u NOVOSLOVO...izlazim.\n"), exit(0);

    int vrsta;
    char *odgovor;
    if (Primi(sock, &vrsta, &odgovor) != OK)
        perror("Pogreska u NOVOSLOVO...izlazim.\n"), exit(0);

    int broj_pokusaja;
    char zagonetka[40];
    sscanf(odgovor, "%d %s", &broj_pokusaja, zagonetka);

    printf("\nPogodeno:\n%s\nPreostalo pokusaja: %d\n\n", zagonetka, broj_pokusaja);
    
    int gotov = 1;
    for (int i = 0; i < strlen(zagonetka); ++i)
        gotov &= (zagonetka[i] != '*');
    
    return gotov || (broj_pokusaja == 0);
}

void obradi_KRAJ(int sock, char *ime)
{
    if (Posalji(sock, KRAJ, "") != OK)
        perror("Pogreska u KRAJ...izlazim.\n"), exit(0);

    char *ljestvica;
    int vrsta;
    if (Primi(sock, &vrsta, &ljestvica) != OK)
        perror("Pogreska u KRAJ...izlazim.\n"), exit(0);

    printf("RANG LJESTVICA:\n");
    printf("RANG IME\n");
    int rank, f, n, len = 0;
    char igrac[9];
    while (sscanf(ljestvica + len, "%d %d %d %s %n", &rank, &f, &f, &igrac, &n) == 4)
    {
        printf("%4d %s\n", rank, igrac);
        len += n;
    }
    
}

int main(int argc, char **argv)
{
    if (argc != 4)
        printf("Upotreba: %s ime ip port\n", argv[0]), exit(0);
    
    char ime[9];
    strcpy(ime, argv[1]);
    ime[8] = '\0';
    char *dekadski_IP = argv[2];
    int port;
    sscanf(argv[3], "%d", &port);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        perror("socket"), exit(0);

    struct sockaddr_in adresa_servera;

    adresa_servera.sin_family = AF_INET;
    adresa_servera.sin_port = htons(port);
    if (inet_aton(dekadski_IP, &adresa_servera.sin_addr) == 0)
        printf("%s nije dobra adresa!\n", dekadski_IP), exit(0);

    memset(adresa_servera.sin_zero, '\0', 8);
    if (connect(sock,
                (struct sockaddr *) &adresa_servera,
                sizeof(adresa_servera)) == -1)
        perror("socket"), exit(0);
    
    obradi_LOGIN(sock, ime);
    int gotovo = 0;
    while (!obradi_NOVOSLOVO(sock, ime));
    
    obradi_KRAJ(sock, ime);
    return 0;
}
