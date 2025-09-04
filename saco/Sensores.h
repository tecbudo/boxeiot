#ifndef SENSORES_H
#define SENSORES_H

#include <Arduino.h>
#include <MPU6500_WE.h>
#include <arduinoFFT.h>

#define NUM_SENSORES 9
#define SAMPLES 64
#define SAMPLING_FREQUENCY 100
#define ENDERECO_MPU6500 0x68
#define DURACAO_CALIBRACAO_MS 1000

class Sensores {
public:
    Sensores();
    void iniciar();
    void calibrarSensoresToque();
    void calibrarSensoresToqueAvancado();
    void calibracaoManual();
    float lerSensorToque(int indice);
    float calcularForca();
    int detectarToque();
    void ajusteDinamicoReferencias();
    
    // Métodos para calibração interativa
    void iniciarCalibracaoInterativa();
    bool processarCalibracaoInterativa();
    int getProgressoCalibracao();
    int getSensorCalibrado(int index);
    void finalizarCalibracaoInterativa();

    // Métodos para acesso aos dados dos sensores
    int getBaseline(int indice) const;
    int getThreshold(int indice) const;
    String getNomeSensor(int indice) const;
    bool isSensorCalibrado(int indice) const;

private:
    int baselineToque[NUM_SENSORES];
    int maxValoresToque[NUM_SENSORES][10];
    int thresholdsToque[NUM_SENSORES];
    bool sensorCalibrado[NUM_SENSORES];
    
    static const byte pinosToque[NUM_SENSORES];
    
    float mediasToque[NUM_SENSORES];
    float limitesToque[NUM_SENSORES];
    const String nomesSensores[NUM_SENSORES] = {
        "FrenteAlta", "FrenteBaixa", "DireitaBaixa", 
        "DireitaAlta", "EsquerdaBaixa", "EsquerdaAlta",
        "TrasAlta", "TrasBaixa", "Centro"
    };
    
    // Variáveis para calibração interativa
    bool calibracaoInterativaAtiva;
    unsigned long tempoInicioCalibracao;
    int sensoresCalibrados;
    int contadorCapturas[NUM_SENSORES];
    
    // Métodos de calibração avançada
    void coletarBaseline();
    void capturarToquesCalibracao();
    void calcularThresholds();
    
    MPU6500_WE mpu;
    float integraFFT(bool reset = false);
    float detectarPico(float entrada, bool reset = false);
};

extern Sensores sensores;

#endif