# Program za Raspoznavanje Poker Karata

Ovaj projekat predstavlja C++ implementaciju rješenja za prepoznavanje poker karata iz JPG datoteka korišćenjem tehnika obrade slika (Image Processing) i upoređivanja šablona.

## 1. Funkcionalnost
Program `main` analizira ulaznu sliku `karta.jpg`, izoluje vrijednost i znak karte, te ispisuje prepoznatu kartu (npr. "As Srca") u konzoli.

* **Ulaz:** Očekuje se datoteka pod nazivom `karta.jpg` u istom direktorijumu.
* **Obrada:** Koristi se set uzoraka karata za poređenje (Card_Imgs).
* **Izlaz:** Prepoznata karta se ispisuje u konzoli kao "Detektovani rank/suit: As/Srce".

---

## 2. Preduslovi i Biblioteke

### A. Tehnologije
* **Jezik:** C++
* **Kompajler:** GCC (g++)
* **OS:** Razvijeno i testirano na Linux platformama.

### B. Specijalne Biblioteke
Ovaj projekat koristi specifične, samostalno uključene (self-contained) biblioteke za obradu slike kako bi se izbjegla instalacija glomaznih eksternih zavisnosti.

1.  **OpenCV:** Core algoritmi projekta zasnovani su na konceptima iz popularne OpenCV biblioteke, ali je sama biblioteka apstrahovana i **nije potrebna za kompajliranje**.
2.  **stb_image:** Korišćena za učitavanje i snimanje slika (nalazi se u `stb-master/`).

---

## 3. Struktura Projekta

| Naziv datoteke/foldera | Opis |
| :--- | :--- |
| `main.cpp` | **Glavni kod projekta.** Sadrži svu logiku za obradu slike i raspoznavanje. |
| `karta.jpg` | **Ulazna slika.** Ova slika se koristi za testiranje pri pokretanju programa. |
| `Card_Imgs/` | **Dataset uzoraka (Templates).** Slike karata koje služe kao šabloni za upoređivanje (npr. slike simbola i vrednosti). |
| `tst_slike/` | Set test slika 1. |
| `tst_slike2/` | Set test slika 2. |
| `stb-master/` | Sadrži *third-party* implementaciju funkcija za učitavanje i upis slika. |

---

## 4. Kompajliranje i Pokretanje

Projekt je dizajniran da se kompajlira sa **jednostavnom `g++` komandom** u Linux/macOS okruženju, bez potrebe za eksternim `make` fajlovima.

### A. Kompajliranje

Navedena komanda pretpostavlja da se sav potreban kod nalazi u `main.cpp` i da uključuje neophodne fajlove iz `stb-master/`.

```bash
g++ main.cpp -o main