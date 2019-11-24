# Krizaljka

Prva zadaca iz kolegija Mreze racunala na PMF-MO. Krizaljka se nalazi u fileu ```zagonetka.txt```.

## Pokretanje

Kompilacija i pokretanje servera
```
gcc Server.c Protocol.c -lpthread -o SERVER
./SERVER [port] [broj_pokusaja]
```
gdje je [broj_pokusaja] dozvoljeni broj pokusaja koji ce klijenti imati za pogadanje krizaljke.

Kompilacija i pokretanje servera
```
gcc Client.c Protocol.c -o CLIENT
./CLIENT [ime] [ip] [port]
```