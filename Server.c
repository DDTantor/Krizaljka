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


#define MAX_DRETVI 10

typedef struct
{
    int sock;
    int dretva_id;
    char ime[8];
    char *pogodeno;
    int pokusaja;
} Klijent;

// 0 - NEAKTIVNA
// 1 - AKTIVNA
// 2 - SPRENMA ZA JOIN

int MAX_POKUSAJA;
int aktivne_dretve[MAX_DRETVI] = {0};
Klijent klijent_dretve[MAX_DRETVI];

pthread_mutex_t lokot_aktivne_dretve = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lokot_filesystem = PTHREAD_MUTEX_INITIALIZER;

int obradi_LOGIN(Klijent *klijent)
{
    char *ime = klijent->ime;
    int sock = klijent->sock;

    // POSTAVI NA DEFAULT
    klijent->pokusaja = MAX_POKUSAJA;
    char *pogodeno = klijent->pogodeno;
    
    FILE *f;
    f = fopen("zagonetka.txt", "r");
    char zagonetka[40];
    fscanf(f, "%s", zagonetka);
    klijent->pogodeno = (char *) malloc(strlen(zagonetka));
    for (int i = 0; i < strlen(zagonetka); ++i)
        klijent->pogodeno[i] = '*';

    fclose(f);
    return OK;
}

int obradi_KRAJ(Klijent* klijent)
{
    char *ime = klijent->ime;
    int sock = klijent->sock;
    char *pogodeno = klijent->pogodeno;
    int *pokusaja = &klijent->pokusaja;
    int ja = 1;
    for (int i = 0; i < strlen(pogodeno); ++i)
        ja &= pogodeno[i] != '*';
    
    char ljestvica[1000], ime_igraca[9], odgovor[1000];
    memset(ljestvica, '\0', sizeof(ljestvica));
    int pogodio, rank = 0, preostalo, br_igraca = 1, stavljen = 0;  
    FILE *f;
    pthread_mutex_lock(&lokot_filesystem);
    f = fopen("ljestvica.txt", "r");
    // RANK POGODAK PREOSTALO_POKUSAJA IME
    while (f != NULL && fscanf(f, "%d %d %d %s",
                  &rank, &pogodio, &preostalo, ime_igraca) == 4)
    {
        if (!stavljen && (ja > pogodio ||
                          (ja == pogodio && *pokusaja > preostalo)))
        {
            stavljen = 1;
            sprintf(ljestvica + strlen(ljestvica), "%d %d %d %s\n",
                    rank, ja, *pokusaja, ime);
        }

        sprintf(ljestvica + strlen(ljestvica),"%d %d %d %s\n",
                rank + stavljen, pogodio, preostalo, ime_igraca);
    }

    if (!stavljen)
        sprintf(ljestvica + strlen(ljestvica), "%d %d %d %s\n",
                rank + 1, ja, *pokusaja, ime);

    if (f != NULL)
        fclose(f);
    
    f = fopen("ljestvica.txt", "w+");
    fprintf(f, ljestvica);
    pthread_mutex_unlock(&lokot_filesystem);

    fclose(f);
    Posalji(sock, KRAJ, ljestvica);
    return OK;
}

int obradi_NOVOSLOVO(Klijent* klijent, const char *poruka)
{
    char *ime = klijent->ime;
    int sock = klijent->sock;
    char *pogodeno = klijent->pogodeno;
    int *pokusaja = &klijent->pokusaja;
    if (strlen(poruka) != 1)
        return !OK;

    int bar_jedno = 0;
    char zagonetka[40];
    FILE *f;
    f = fopen("zagonetka.txt", "r");
    fscanf(f, "%s", zagonetka);
    for (int i = 0; i < strlen(zagonetka); ++i)
        if (pogodeno[i] == '*' && zagonetka[i] == poruka[0])
            pogodeno[i] = zagonetka[i], bar_jedno = 1;

    (*pokusaja) -= (bar_jedno == 0);
    char odgovor[100];
    sprintf(odgovor, "%d %s", *pokusaja, pogodeno);
    Posalji(sock, NOVOSLOVO, odgovor);
    return OK;
}

void kraj_komunikacije(void* parametar)
{
    Klijent *klijent = (Klijent *) parametar;
    int sock = klijent->sock;
    int dretva_id = klijent->dretva_id;
    char *ime = klijent->ime;

    printf("Kraj komunikacije [dretva=%d, ime=%s]...\n", dretva_id, ime);

    pthread_mutex_lock(&lokot_aktivne_dretve);

    aktivne_dretve[dretva_id] = 2;
    pthread_mutex_unlock(&lokot_aktivne_dretve);

    close(sock);
}

void *obradi_klijenta(void* parametar)
{
    Klijent *klijent = (Klijent *) parametar;
    int sock = klijent->sock, vrsta;
    int dretva_id = klijent->dretva_id;
    char *ime = klijent->ime, *poruka;
 
    if (Primi(sock, &vrsta, &poruka) != OK)
        return kraj_komunikacije(parametar), NULL;
    
    if (vrsta != LOGIN || strlen(poruka) > 8)
        return kraj_komunikacije(parametar), NULL;

    strcpy(ime, poruka);
    if (obradi_LOGIN(klijent) != OK)
        return kraj_komunikacije(parametar), NULL;

    free(poruka);

    int gotovo = 0;
    while (!gotovo)
    {
        if (Primi(sock, &vrsta, &poruka) != OK)
        {
            gotovo = 1;
            kraj_komunikacije(parametar);
            continue;
        }

        if (vrsta == NOVOSLOVO)
            obradi_NOVOSLOVO(klijent, poruka);
        else
            gotovo = 1, obradi_KRAJ(klijent);

        free(poruka);
    }

    kraj_komunikacije(parametar);
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Upotreba: %s port broj_pokusaja\n", argv[0]);
        exit(0);
    }

    int port;
    sscanf(argv[1], "%d", &port);
    sscanf(argv[2], "%d", &MAX_POKUSAJA);
    int listen_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (listen_socket == -1)
        perror("socket");

    struct sockaddr_in moja_adresa;

    moja_adresa.sin_family = AF_INET;
    moja_adresa.sin_port = htons(port);
    moja_adresa.sin_addr.s_addr = INADDR_ANY;
    memset(moja_adresa.sin_zero, '\0', 8);
    if (bind(listen_socket,
             (struct sockaddr *) &moja_adresa,
             sizeof(moja_adresa)) == -1)
        perror("bind");

    if (listen(listen_socket, 10) == -1)
        perror("listen");

    pthread_t dretve[MAX_DRETVI];

    while (1)
    {
        struct sockaddr_in klijent_adresa;
        unsigned int len_addr = sizeof(klijent_adresa);
        int sock = accept(listen_socket,
                          (struct sockaddr *) &klijent_adresa,
                          &len_addr);

        if (sock == -1)
            perror("accept");

        char *dekadski_IP = inet_ntoa(klijent_adresa.sin_addr);
        printf("Prihvatio konekciju od %s", dekadski_IP);

        pthread_mutex_lock(&lokot_aktivne_dretve);
        int indeks_neaktivne = -1;
        for (int i = 0; i < MAX_DRETVI; ++i)
        {
            if (aktivne_dretve[i] == 0)
                indeks_neaktivne = i;
            else if (aktivne_dretve[i] == 2)
            {
                pthread_join(dretve[i], NULL);
                aktivne_dretve[i] = 0;
                indeks_neaktivne = i;
            }
        }

        if (indeks_neaktivne == -1)
        {
            close(sock);
            printf("--> ipak odbijam konekciju jer nemam vise dretvi.\n");
        }
        else
        {
            aktivne_dretve[indeks_neaktivne] = 1;
            klijent_dretve[indeks_neaktivne].sock = sock;
            klijent_dretve[indeks_neaktivne].dretva_id = indeks_neaktivne;
            printf("--> koristim dretvu broj %d.\n", indeks_neaktivne);

            pthread_create(
                &dretve[indeks_neaktivne], NULL,
                obradi_klijenta, &klijent_dretve[indeks_neaktivne]);
        }

        pthread_mutex_unlock(&lokot_aktivne_dretve);
    }
}

