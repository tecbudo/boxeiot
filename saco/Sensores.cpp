#include "Sensores.h"
#include <Wire.h>
#include <algorithm>

// Definir pinos de toque (mantidos da versão anterior)
const byte Sensores::pinosToque[NUM_SENSORES] = {T8, T6, T7, T10, T1, T5, T4, T2, T3};

Sensores::Sensores() : mpu(ENDERECO_MPU6500) {
    // Inicializar arrays
    float limitesIniciais[NUM_SENSORES] = {20000,15000,20000,15000,20000,7000,20000,20000,20000};
    
    for(int i = 0; i < NUM_SENSORES; i++) {
        baselineToque[i] = 0;
        thresholdsToque[i] = limitesIniciais[i];
        sensorCalibrado[i] = false;
        mediasToque[i] = 0;
        limitesToque[i] = limitesIniciais[i];
        
        for(int j = 0; j < 10; j++) {
            maxValoresToque[i][j] = 0;
        }
    }
    
    // Inicializar variáveis de calibração interativa
    calibracaoInterativaAtiva = false;
    tempoInicioCalibracao = 0;
    sensoresCalibrados = 0;
    
    for(int i = 0; i < NUM_SENSORES; i++) {
        contadorCapturas[i] = 0;
    }
}

void Sensores::iniciar() {
    // Inicializar MPU6500
    Wire.begin();
    
    if (!mpu.init()) {
        Serial.println("MPU6500 não responde");
        return;
    }
    
    // Configurar MPU6500
    mpu.autoOffsets();
    mpu.enableGyrDLPF();
    mpu.setGyrDLPF(MPU6500_DLPF_7);
    mpu.setSampleRateDivider(1);
    mpu.setGyrRange(MPU6500_GYRO_RANGE_2000);
    mpu.setAccRange(MPU6500_ACC_RANGE_16G);
    mpu.enableAccDLPF(true);
    mpu.setAccDLPF(MPU6500_DLPF_7);
    
    Serial.println("Sensores inicializados");
}

void Sensores::coletarBaseline() {
    Serial.println("Coletando valores de repouso...");
    
    for(int i = 0; i < NUM_SENSORES; i++) {
        long soma = 0;
        
        for(int j = 0; j < 50; j++) {
            soma += touchRead(pinosToque[i]);
            delay(20);
        }
        
        baselineToque[i] = soma / 50;
        Serial.printf("Sensor %s: Baseline = %d\n", nomesSensores[i].c_str(), baselineToque[i]);
        sensorCalibrado[i] = false;
    }
}

void Sensores::iniciarCalibracaoInterativa() {
    Serial.println("Iniciando calibração interativa...");
    calibracaoInterativaAtiva = true;
    sensoresCalibrados = 0;
    tempoInicioCalibracao = millis();
    
    // Inicializar arrays
    for (int i = 0; i < NUM_SENSORES; i++) {
        contadorCapturas[i] = 0;
        sensorCalibrado[i] = false;
    }
    
    // Coletar baseline inicial
    coletarBaseline();
    
    Serial.println("Toque em cada sensor para calibração...");
}

bool Sensores::processarCalibracaoInterativa() {
    if (!calibracaoInterativaAtiva) {
        return false;
    }
    
    // Verificar timeout (60 segundos)
    if (millis() - tempoInicioCalibracao > 60000) {
        Serial.println("Timeout na calibração interativa");
        finalizarCalibracaoInterativa();
        return true;
    }
    
    // Processar cada sensor
    for (int i = 0; i < NUM_SENSORES; i++) {
        if (sensorCalibrado[i]) {
            continue; // Sensor já calibrado
        }
        
        int valorAtual = touchRead(pinosToque[i]);
        int diferenca = abs(valorAtual - baselineToque[i]);
        int limiarTemporario = (baselineToque[i] * 25) / 100;
        
        if (diferenca > limiarTemporario) {
            if (contadorCapturas[i] < 10) {
                maxValoresToque[i][contadorCapturas[i]] = valorAtual;
                contadorCapturas[i]++;
                Serial.printf("Captura %d para sensor %d\n", contadorCapturas[i], i);
            } else {
                // Ordenar valores e marcar como calibrado
                std::sort(maxValoresToque[i], maxValoresToque[i] + 10);
                sensorCalibrado[i] = true;
                sensoresCalibrados++;
                Serial.printf("Sensor %d calibrado. Total: %d/%d\n", i, sensoresCalibrados, NUM_SENSORES);
            }
        }
    }
    
    // Verificar se todos os sensores foram calibrados
    if (sensoresCalibrados == NUM_SENSORES) {
        calcularThresholds();
        finalizarCalibracaoInterativa();
        return true;
    }
    
    return false; // Calibração ainda em andamento
}

int Sensores::getProgressoCalibracao() {
    return sensoresCalibrados;
}

int Sensores::getSensorCalibrado(int index) {
    if (index < 0 || index >= NUM_SENSORES) {
        return -1; // Índice inválido
    }
    return sensorCalibrado[index] ? 1 : 0;
}

void Sensores::finalizarCalibracaoInterativa() {
    calibracaoInterativaAtiva = false;
    Serial.println("Calibração interativa finalizada");
}

void Sensores::capturarToquesCalibracao() {
    int sensoresCalibrados = 0;
    unsigned long tempoInicial = millis();
    int contadorCapturas[NUM_SENSORES] = {0};

    while(sensoresCalibrados < NUM_SENSORES && millis() - tempoInicial < 60000) {
        for(int i = 0; i < NUM_SENSORES; i++) {
            if(sensorCalibrado[i]) continue;

            int valorAtual = touchRead(pinosToque[i]);
            int diferenca = abs(valorAtual - baselineToque[i]);
            int limiarTemporario = (baselineToque[i] * 25) / 100;

            if(diferenca > limiarTemporario) {
                if(contadorCapturas[i] < 10) {
                    maxValoresToque[i][contadorCapturas[i]] = valorAtual;
                    contadorCapturas[i]++;
                } else {
                    // Ordenar valores e marcar como calibrado
                    std::sort(maxValoresToque[i], maxValoresToque[i] + 10);
                    sensorCalibrado[i] = true;
                    sensoresCalibrados++;
                    Serial.printf("Sensor %s calibrado.\n", nomesSensores[i].c_str());
                }
            }
        }
        delay(100);
    }
}

void Sensores::calcularThresholds() {
    for(int i = 0; i < NUM_SENSORES; i++) {
        if(sensorCalibrado[i]) {
            long soma = 0;
            for(int j = 0; j < 10; j++) {
                soma += maxValoresToque[i][j];
            }
            int mediaMaximos = soma / 10;
            thresholdsToque[i] = (baselineToque[i] + mediaMaximos) / 2;
            limitesToque[i] = thresholdsToque[i];
            Serial.printf("Sensor %s: Threshold = %d\n", nomesSensores[i].c_str(), thresholdsToque[i]);
        } else {
            // Fallback para valores padrão se não calibrado
            thresholdsToque[i] = baselineToque[i] * 1.2;
            limitesToque[i] = thresholdsToque[i];
            Serial.printf("Sensor %s: Threshold padrão = %d\n", nomesSensores[i].c_str(), thresholdsToque[i]);
        }
    }
}

void Sensores::calibrarSensoresToqueAvancado() {
    coletarBaseline();
    capturarToquesCalibracao();
    calcularThresholds();
    Serial.println("Calibração avançada concluída!");
}

void Sensores::calibrarSensoresToque() {
    for (int i = 0; i < NUM_SENSORES; i++) {
        uint32_t tempoInicial = millis();
        uint32_t numAmostras = 0;
        unsigned long somaToque = 0;

        while (millis() - tempoInicial < DURACAO_CALIBRACAO_MS) {
            somaToque += touchRead(pinosToque[i]);
            numAmostras++;
            delay(50);
        }
        
        mediasToque[i] = (float)somaToque / numAmostras;
        Serial.printf("Sensor %s: Média = %.2f\n", nomesSensores[i].c_str(), mediasToque[i]);
    }
}

int Sensores::detectarToque() {
    for (int i = 0; i < NUM_SENSORES; i++) {
        if(touchRead(pinosToque[i]) > thresholdsToque[i]) {
            return i;
        }
    }
    return -1;
}

float Sensores::lerSensorToque(int indice) {
    if (indice < 0 || indice >= NUM_SENSORES) {
        return -1;
    }
    return touchRead(pinosToque[indice]);
}

float Sensores::calcularForca() {
    static float maiorPico = 0;
    maiorPico = 0;
    
    for (int i = 0; i < 1000; i++) {
        float forcaAtual = detectarPico(integraFFT());
        if (forcaAtual > maiorPico) {
            maiorPico = forcaAtual;
        }
        delay(1);
    }
    
    detectarPico(0, true); // Reset
    return maiorPico;
}

float Sensores::integraFFT(bool reset) {
    static double vReal[SAMPLES];
    static double vImag[SAMPLES];
    static double vRealR[SAMPLES];
    static float media = 0;
    static float somaSample = 0;
    static int ind = 0;
    static int numSamples = 0;
    
    if (reset) {
        for(int i = 0; i < SAMPLES; i++) {
            vRealR[i] = 0;
        }
        media = 0;
        somaSample = 0;
        ind = 0;
        numSamples = 0;
        return 0;
    }

    xyzFloat gValue = mpu.getGValues();
    float newSample = mpu.getResultantG(gValue);

    if (numSamples < SAMPLES) {
        somaSample += newSample;
        vRealR[numSamples] = newSample;
        numSamples++;
    } else {
        somaSample += newSample - vRealR[ind];
        vRealR[ind] = newSample;
    }
    
    ind = (ind + 1) % SAMPLES;
    media = somaSample / SAMPLES;

    if (numSamples >= SAMPLES) {
        for (int k = 0; k < SAMPLES; k++) {
            vReal[k] = vRealR[(ind + k) % SAMPLES] - media;
            vImag[k] = 0;
        }
        
        ArduinoFFT<double> FFT(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY);
        FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
        FFT.compute(FFTDirection::Forward);
        FFT.complexToMagnitude();
        
        float soma = 0;
        for (int i = 1; i < SAMPLES / 2; i++) {
            soma += vReal[i];
        }
        return soma;
    }
    return 0;
}

float Sensores::detectarPico(float entrada, bool reset) {
    static float maior = 0;
    static bool subindo = true;

    if (reset) {
        maior = 0;
        subindo = true;
        return 0;
    }

    if (subindo && (entrada < maior)) {
        subindo = false;
    } else if (!subindo && (entrada > maior)) {
        subindo = true;
    }
    
    if (entrada > maior) {
        maior = entrada;
    }
    
    return maior;
}

void Sensores::ajusteDinamicoReferencias() {
    for (int i = 0; i < NUM_SENSORES; i++) {
        uint16_t valorAtual = touchRead(pinosToque[i]);
        if (abs(valorAtual - mediasToque[i]) < 10) {
            mediasToque[i] = (mediasToque[i] * 0.9) + (valorAtual * 0.1);
        }
    }
}

int Sensores::getBaseline(int indice) const {
    if (indice < 0 || indice >= NUM_SENSORES) return -1;
    return baselineToque[indice];
}

int Sensores::getThreshold(int indice) const {
    if (indice < 0 || indice >= NUM_SENSORES) return -1;
    return thresholdsToque[indice];
}

String Sensores::getNomeSensor(int indice) const {
    if (indice < 0 || indice >= NUM_SENSORES) return "INVALIDO";
    return nomesSensores[indice];
}

bool Sensores::isSensorCalibrado(int indice) const {
    if (indice < 0 || indice >= NUM_SENSORES) return false;
    return sensorCalibrado[indice];
}

Sensores sensores;