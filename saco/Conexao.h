#ifndef CONEXAO_H
#define CONEXAO_H

// Desativa módulos desnecessários do Firebase
#define FIREBASE_DISABLE_SERIAL
#define ENABLE_RTDB 1
#define ENABLE_FCM 0
#define ENABLE_FB_STORAGE 0
#define ENABLE_GC_STORAGE 0
#define ENABLE_FB_FUNCTIONS 0

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Firebase_ESP_Client.h>

#include "config.h"

// Estrutura para medições
struct Medicao {
  String tipo;
  String estado;
  float valor;
  String usuario;
  unsigned long timestampSolicitacao;
  unsigned long timestampConclusao;
};

// Estrutura para resultados de precisão
struct ResultadoPrecisao {
  bool acerto;
  unsigned long tempoResposta;
  int sensorTocado;
  int ledAlvo;
};

// Classe para gerenciar conexões
class ConexaoManager {
public:
  ConexaoManager();
  void begin();
  bool isConnected();
  unsigned long getTimestamp();
  String getTimeString();  // Nova função para obter data e hora formatada
  bool checkForCommands();
  bool checkForStopCommand();
  bool updateDeviceStatus(const String& status);
  bool updateDevicemMdicoes(const String status);
  bool updateDeviceEx();
  bool setMeasurementResult(float value);
  bool setCurrentLed(int ledIndex);
  bool sendPrecisionResult(bool acerto, unsigned long tempoResposta, int sensorTocado, int ledAlvo);
  bool sendPrecisionFinalResult(int totalAcertos, int totalErros);
  bool updatePrecisionStatus(const String& status);
  bool clearPrecisionData();
  bool sendCalibrationProgress(int progresso, int total);
  bool sendSensorCalibrated(int sensorIndex);
  bool setMeasurementLed(int ledIndex);
  Medicao getCurrentMeasurement();
  int getSensorCalibracao();
  bool setSensorCalibracao(int sensor);
  int getLedPrecisao();
  FirebaseData fbdo;
  FirebaseAuth auth;
  FirebaseConfig config;
  Medicao currentMeasurement;
  NTPClient timeClient;  // Tornando público para acesso
  String getDeviceState();
  String deviceId;
  String generateDeviceId();
  bool sendForceUpdate(float forca);
  
private:
  WiFiManager wifiManager;
  WiFiUDP ntpUDP;
  
  void setupWiFi();
  void setupTime();
  void setupFirebase();
  bool sendToFirebase(const String& path, const String& value);
  bool sendToFirebase(const String& path, int value);
  bool sendToFirebase(const String& path, float value);
  bool sendToFirebase(const String& path, bool value);
  bool sendToFirebase(const String& path, FirebaseJson& json);
};

extern ConexaoManager conexao;

#endif