# ProjectOnePAD

Componentele sunt 2 executabile:

* Client
* Server concurent


## Client

* Logare nume (nume+parola trebuie fișier pentru a memora datele, parola trebuie codata). Verificare dacă numele corespunde cu parola, altfel poți crea un cont cu alt nume.

* Lista functionalitati/ periodic afisezi cine este online, selectezi cu cine vrei sa comunici (cu toți clienții sau numai cu unul particular)

* Mesaje către o persoana – stringuri + verificare ca mesajul sa ajunga integral (limita de caractere scrise) **checksum**? – exista funcție și biblioteci. Mesajul trebuie sa fie impachetat + un int care numara cati octeți utili sunt acolo. Înainte de afișarea mesajul faci verificarea dacă a ajuns tot mesajul, dacă ajunge îl afisezi altfel failed sending. **BYTE COUNT MINIM**?


## Server

* Mai mulți clienți conectați în paralel (simultan)

* În cazul threadurilor putem pune într-un vector clienți și avem un maxim de clienți și threadurile au acces la acel vector și vor vedea clienți noi

* Serverul va primi mesaje de la utilizatorii conectați și va trimite imediat tuturor clienților mesajele primite.
