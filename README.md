# File-Server

Testare: 
- executabilele se vor crea cu "make all" si toate fisierele care au fost facute se pot sterge cu "make clean";
- se va rula intr-un terminal "./bin/server" si in altul "./bin/client";
- se pot deschide oricate procese client, insa doar numarul declarat de clienti va putea fi procesat de server;
- comanda este formata din cod operatie si/sau parametrii, separati prin ";" ;
- se poate folosi comanda "help" de catre client pentru mai multe precizari;
- se poate opri executia aplicatiei folosind "CTRL + C" sau tastand "quit" de la standard input;

Precizari:
- daca nu s-a atins numarul maxim de conexiuni, clientul nou conectat va primi de la server mesajul "1" si va putea comunica cu serverul, altfel va avea mesajul "8 (SERVER_BUSY)" si isi va termina executia;
- thread-ul "listenThread" asculta noile conexiuni si ce primeste de la standard input;
- thread-ul "terminatorThread" intercepteaza un semnal SIGTERM sau SIGINT si va inchide programul, intr-un mod cat mai curat;
- thread-ul "indexingThread" va actualiza o structura de date ce pastreaza cele mai frecvente 10 cuvinte din fiecare fisier si se va modifica daca se apeleaza UPLOAD, DELETE, MOVE sau UPDATE; se va trimite un cond_signal() dupa fiecare dintre aceste operatii;
- thread-urile pentru fiecare client sunt detasabile, insa pentru a nu aparea leak-uri de memorie exista o variabila de conditie "listenCond" care va astepta intr-un loop pana cand "nrThreads" este 0;
- pe timpul rularii aplicatiei se retine un vector cu toate fisierele disponibile in acel moment, care se va modifica daca se apeleaza MOVE, DELETE sau UPLOAD;
- fiecare operatie se va salva intr-un fisier "log.txt";
- se vor citi cati octeti sunt precizati inainte de calea fisierului sau de ce va contine acesta, etc; daca este un numar mai mic de octei decat lungimea sirului, se vor citi doar numarul de octeti trimisi de client, altfel se va citi intreg string-ul;
- se va testa numarul exact de parametrii, in functie de cerintele fiecarei comenzi; de asemenea si ca numarul de octeti trimisi si fie de tipul intreg;
- la operatia UPDATE se va primi "4 (OUT_OF_MEMORY)" daca offset-ul trimis depaseste dimensiunea fisierului;
- se poate crea doar un subdirector fata de root ("/dir/file.txt");
- exista un numar limitat de fisiere ce pot fi incarcate; daca se va incerca adaugarea unui nou fisier si este atinsa capacitatea maxima, se va primi "4 (OUT_OF_MEMORY)";