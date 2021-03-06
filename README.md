PAD – proiect 1
Program care oferă o camera de discuții mai multor utilizatori. (Chatroom in C

Folosim 2 componente:
Server concurent
Clientul / Clienții
 
Server
Permite ca mai mulți clienți sa fie conectați în paralel (simultan)
Clienții vor reprezenta thread-uri altfel putem pune într-un vector clienți și avem un maxim de clienți și threadurile au acces la acel vector și vor vedea clienți noi
Serverul va primi mesaje de la utilizatorii conectați și va trimite imediat tuturor clienților mesajele primite.

Clientul
Se poate conecta folosind nume+parola (folosim un fișier pentru a memora datele de autentificare, parola va fi codata. Se va face o verificare dacă numele corespunde cu parola atunci te poți conecta, altfel poți crea un cont cu alt nume).
Lista functionalitatilor: periodic afisezi cine este online, selectezi cu cine vrei sa comunici (cu toți clienții sau numai cu unul particular)
Mesaje către o persoana [– stringuri + verificare ca mesajul sa ajunga integral (limita de caractere scrise) checksum – exista funcție și biblioteci. Mesajul trebuie sa fie impachetat + un int care numara cati octeți utili sunt acolo. Înainte de afișarea mesajul faci verificarea dacă a ajuns tot mesajul, dacă ajunge îl afisezi altfel failed sending. BYTE COUNT MINIM]
