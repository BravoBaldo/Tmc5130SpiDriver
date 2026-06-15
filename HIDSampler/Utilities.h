#pragma once

void ShowBuffer(const uint8_t* data, uint16_t len);

// Funzione constexpr per convertire le tre lettere del mese in un numero da 1 a 12
constexpr uint8_t getMonth(const char* date) {
    return (date[0] == 'J' && date[1] == 'a' && date[2] == 'n') ? 1 :
           (date[0] == 'F' && date[1] == 'e' && date[2] == 'b') ? 2 :
           (date[0] == 'M' && date[1] == 'a' && date[2] == 'r') ? 3 :
           (date[0] == 'A' && date[1] == 'p' && date[2] == 'r') ? 4 :
           (date[0] == 'M' && date[1] == 'a' && date[2] == 'y') ? 5 :
           (date[0] == 'J' && date[1] == 'u' && date[2] == 'n') ? 6 :
           (date[0] == 'J' && date[1] == 'u' && date[2] == 'l') ? 7 :
           (date[0] == 'A' && date[1] == 'u' && date[2] == 'g') ? 8 :
           (date[0] == 'S' && date[1] == 'e' && date[2] == 'p') ? 9 :
           (date[0] == 'O' && date[1] == 'c' && date[2] == 't') ? 10 :
           (date[0] == 'N' && date[1] == 'o' && date[2] == 'v') ? 11 :
           (date[0] == 'D' && date[1] == 'e' && date[2] == 'c') ? 12 : 0;
}

// Funzione constexpr per estrarre il giorno (gestisce anche lo spazio iniziale se < 10)
constexpr uint8_t getDay(const char* date) {
    return ((date[4] == ' ' ? 0 : date[4] - '0') * 10) + (date[5] - '0');
}

// Funzione constexpr per estrarre l'anno a 4 cifre
constexpr uint16_t getYear(const char* date) {
    return ((date[7] - '0') * 1000) + ((date[8] - '0') * 100) + ((date[9] - '0') * 10) + (date[10] - '0');
}

constexpr uint16_t getYear2(const char* date) {
    return ((date[9] - '0') * 10) + (date[10] - '0');
}

constexpr uint8_t getHour   (const char* time) { return ((time[0] - '0') * 10) + (time[1] - '0');  }
constexpr uint8_t getMinute (const char* time) { return ((time[3] - '0') * 10) + (time[4] - '0');  }
constexpr uint8_t getSecond (const char* time) { return ((time[6] - '0') * 10) + (time[7] - '0');  }
