# Linux_coreUsage_BM

## Aplikacja konsolowa monitorująca zużycie corów procesora dystrubycji linux wyrażona w procentach. Dane aktualizowane są co 1s.

##### Program został napisany na dystrybucji Ubuntu (poprzez wirtualną maszynę) w visual studio code.

### Program zawiera:
- Wątki: Reader, Analyzer, Printer, Watchdog, Logger
- Opcjonalny wątek hardwarowego Watchdoga (wymagany softdog + konfiguracja)
- Przechwytywanie sygnału Sigterm
- Zapisywanie logów programu do pliku txt (uwaga, jest ich dużo. Warto zauważyć, że niektóre wątki np. printer wykonuje się co 1s)
- Softwarowy watchdog monitorujący działanie wątków.
- Dodatkowo w repozytorium umieszony został valgrind log w którym przeprowadzono diagnostykę pamięci. Nie jestem jednak zadowolony z wyników. 

***Rekomendacja Softdog***: Programowy Watchdog (softdog) został wyłączony z użytku (zakomentowany) z tego powodu, że resetuje on cały system. 

***Rekomendacja instalacji***: Uruchamiany na wirtualnym linuxie d. Ubuntu potrzebował sudo do zapisywania loggera plików

## Kompilacja oraz build
- Do kompilacji programu zostały użyte kompilatory clang z flagą -Wevrything oraz gcc z flagą -Wall -Wextra.
- Posiada system budowania oparty na makefile

## Spełnione założenia programowe
- Wątki komunikują się po globalnych buforach
- Program napisany jest w duchu KISS.
- Zastosowano strukture danych Ringbuffer
- Problem producenta i konsumenta rozwiązano za pomocą semaforów

### Planowane zmiany
- Dopracowanie Watchdoga
- Zmiana organizowania pamieci (malloc,free)
- Optymalizacja pamięci
- Podział na pliki *c, *h w celu organizacji kodu.
- W niektórych dystrubucjach dochodzi do przepełnienia jednej zmiennej, w innych nie, muszę sprawdzić dlaczego :) 
- 



