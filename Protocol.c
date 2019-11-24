#ifdef KROFNA
#define dbg(x) printf("%s:%d - %s\n", __func__, __LINE__, x), fflush(stdout)
#else
#define dbg(x) (void)x
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "Protocol.h"

// HEADER [duljina vrsta] poruka

int Primi(int sock, int *vrsta, char **poruka)
{
    int primljeno, primljeno_zadnje;
    int duljina_poruka_n, duljina_poruke;

    primljeno_zadnje = recv(sock, &duljina_poruka_n, sizeof(duljina_poruka_n), 0);
    if (primljeno_zadnje != sizeof(duljina_poruka_n))
        return !OK;

    duljina_poruke = ntohl(duljina_poruka_n);
    
    int vrsta_n;
    primljeno_zadnje = recv(sock, &vrsta_n, sizeof(vrsta_n), 0);
    if (primljeno_zadnje != sizeof(primljeno_zadnje))
        return !OK;

    *vrsta = ntohl(vrsta_n);

    *poruka = (char *) malloc((duljina_poruke + 1) * sizeof(char));
    primljeno = 0;
    while (primljeno != duljina_poruke)
    {
        primljeno_zadnje = recv(sock, *poruka + primljeno, duljina_poruke - primljeno, 0);
        if (primljeno_zadnje == -1 || primljeno_zadnje == 0)
            return !OK;

        primljeno += primljeno_zadnje;
    }

    (*poruka)[duljina_poruke] = '\0';

    return OK;
}

int Posalji(int sock, int vrsta, const char *poruka)
{
    int duljina_poruke = strlen(poruka);
    int poslano, poslano_zadnje;
    int duljina_poruke_n = htonl(duljina_poruke);
    poslano_zadnje = send(sock, &duljina_poruke_n, sizeof(duljina_poruke_n), 0);
    if (poslano_zadnje != sizeof(duljina_poruke_n))
        return !OK;

    int vrsta_n = htonl(vrsta);
    poslano_zadnje = send(sock, &vrsta_n, sizeof(vrsta_n), 0);
    if (poslano_zadnje != sizeof(vrsta_n))
        return !OK;

    poslano = 0;
    while (poslano != duljina_poruke)
    {
        poslano_zadnje = send(sock, poruka + poslano, duljina_poruke - poslano, 0);
        if (poslano_zadnje == -1 || poslano_zadnje == 0)
            return !OK;

        poslano += poslano_zadnje;
    }

    return OK;
}
