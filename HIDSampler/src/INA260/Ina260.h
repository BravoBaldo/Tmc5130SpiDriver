#pragma once
#include <Arduino.h>
#include <Adafruit_INA260.h> // Richiede la libreria "Adafruit INA260 Library"

class cINA260 {
public:
    // Spostato l'enum fuori o mantenuto pubblico per l'uso esterno (es. oggetto.getVal(cINA260::eCurr))
    enum eValues : uint8_t { eCurr, eVolt, ePwr, eCnt };

private:
    Adafruit_INA260 ina260;
    
    // Inizializzazione moderna C++ e uso di float espliciti (f)
    float m_InaVals[eCnt] = {0.0f, 0.0f, 0.0f};

    uint8_t idx = 0;
    unsigned long timeStart = 0;

public:
    cINA260() = default; // Sfrutta il costruttore di default del compilatore (più efficiente)

    // Inizializzazione della periferica
    bool init(uint8_t address = 0x40) {
        Serial.print(F("Initializing I2C @ 0x"));
        Serial.print(address, HEX);
        Serial.print(F(": INA260..."));
        
        if (!ina260.begin(address)) {
            Serial.println(F(" Error: INA260 not found. Check wiring!!"));
            return false;
        }
        
        Serial.println(F(" OK"));
        timeStart = millis();
        return true;
    }

    // Gestione della macchina a stati non bloccante
    void AlwaysRun() {
        const unsigned long currentMillis = millis();
        if (currentMillis - timeStart < 100) {
            return;
        }

        // Ottimizzazione: rimosso lo switch-case.
        // Sfruttiamo i puntatori a funzione membro della libreria Adafruit per azzerare il codice condizionale.
        using InaReadPtr = float (Adafruit_INA260::*)();
        static const InaReadPtr readFunctions[ePwr + 1] = {
            &Adafruit_INA260::readCurrent,
            &Adafruit_INA260::readBusVoltage,
            &Adafruit_INA260::readPower
        };

        // Chiamata diretta alla funzione corretta senza branch (salto condizionale dello switch)
        m_InaVals[idx] = (ina260.*readFunctions[idx])();

        idx++;
        if (idx >= ePwr + 1) { // Incremento e reset pulito legato alle funzioni di lettura
            idx = 0;
        }
        timeStart = currentMillis;
    }
    
    // CORRETTO: Aggiunto 'const'. Permette di leggere i dati anche se l'oggetto è passato come const reference.
    float getVal(eValues index) const {
        if (index < eCnt) {
            return m_InaVals[index];
        }
        return 0.0f;
    }

    // Stampa i dati sulla porta Seriale
    void show() const {
        // Se usi ESP32/ESP8266/RP2040 printf funziona. Altrimenti usa i classici Serial.print
        Serial.printf("INA260 -> Current: %.3f mA | Voltage: %.3f mV | Power: %.3f mW\n", 
                      m_InaVals[eCurr], m_InaVals[eVolt], m_InaVals[ePwr]);
    }
};
