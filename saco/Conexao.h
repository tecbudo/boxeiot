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

// Definições para Firebase
#define FIREBASE_PROJECT_ID "boxeiot"
#define API_KEY "AIzaSyBLsU5CMvKQOm9Qs9mu6X1K27fVI8WuBQg"
#define DATABASE_URL "https://boxeiot-default-rtdb.firebaseio.com"
#define USER_EMAIL "boxeiotesp@jbsantosfilho.com"
#define USER_PASSWORD "123456789"

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
  Medicao getCurrentMeasurement();

  FirebaseData fbdo;
  FirebaseAuth auth;
  FirebaseConfig config;
  Medicao currentMeasurement;

  String deviceId;
  String generateDeviceId();
  
  
private:
  WiFiManager wifiManager;
  WiFiUDP ntpUDP;
  NTPClient timeClient;
  
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