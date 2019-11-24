#ifndef __MESSAGEPROTOCOL_H_
#define __MESSAGEPROTOCOL_H_

#define LOGIN     1
#define NOVOSLOVO 2
#define KRAJ      3

#define OK     1
#define NIJEOK 0

int Primi(int sock, int *vrsta, char **poruka);
int Posalji(int sock, int vrsta, const char *poruka);

#endif
