#include "Conexao.h"
#include <time.h>

ConexaoManager conexao;

ConexaoManager::ConexaoManager() 
  : timeClient(ntpUDP, "pool.ntp.org", 0) {  // Alterado para UTC+0 br.pool.ntp.org
  deviceId = generateDeviceId();
}

void ConexaoManager::begin() {
  Serial.println("Iniciando WiFi...");
  setupWiFi();
  Serial.println("Iniciando NTP...");
  setupTime();
  Serial.println("Iniciando Firebase...");
  setupFirebase();
}

void ConexaoManager::setupWiFi() {
  wifiManager.setConfigPortalTimeout(180);
  String apName = "SacoBoxe_" + deviceId.substring(0, 6);
  
  if (!wifiManager.autoConnect(apName.c_str())) {
    Serial.println("Falha na conexão WiFi. Reiniciando...");
    delay(3000);
    ESP.restart();
  }
  
  Serial.print("Conectado ao WiFi: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void ConexaoManager::setupTime() {
  timeClient.begin();
  timeClient.setTimeOffset(0);  // Mantido em UTC+0
  
  int tentativas = 0;
  while (!timeClient.forceUpdate() && tentativas < 5) {
    Serial.println("Falha ao sincronizar com NTP. Tentando novamente...");
    delay(1000);
    tentativas++;
  }
  
  if (tentativas < 5) {
    Serial.print("Tempo sincronizado (UTC): ");
    Serial.println(timeClient.getFormattedTime());
    
    // Configurar o fuso horário do sistema para UTC-3 (Brasília)
    setenv("TZ", "GMT+3", 1);
    tzset();
  } else {
    Serial.println("Não foi possível sincronizar com NTP. Usando tempo local.");
  }
}

void ConexaoManager::setupFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  
  // Configurar buffers para melhor performance
  fbdo.setBSSLBufferSize(4096, 1024);
  fbdo.setResponseSize(2048);
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectNetwork(true);
  
  // Registrar dispositivo
  FirebaseJson deviceInfo;
  deviceInfo.set("deviceId", deviceId);
  deviceInfo.set("estado", "disponivel");
  deviceInfo.set("timestamp", getTimestamp());
  deviceInfo.set("ultimaConexao", getTimestamp());
  
  String path = "/devices/" + deviceId;
  if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &deviceInfo)) {
    Serial.println("Dispositivo registrado no Firebase");
  } else {
    Serial.println("Falha ao registrar dispositivo no Firebase");
    Serial.println(fbdo.errorReason());
  }
}

bool ConexaoManager::isConnected() {
  return WiFi.status() == WL_CONNECTED && Firebase.ready();
}

unsigned long ConexaoManager::getTimestamp() {
  // Retorna timestamp UTC (sem ajuste de fuso horário)
  return timeClient.getEpochTime();
}

String ConexaoManager::getTimeString() {
  if (timeClient.getEpochTime() == 0) {
    return "Não sincronizado";
  }

  // Obtém o tempo UTC e aplica o fuso horário local
  time_t epochTime = timeClient.getEpochTime();
  struct tm timeinfo;
  localtime_r(&epochTime, &timeinfo);

  char buffer[20];
  // Formato: Hora:Minuto:Segundo Dia/Mês
  strftime(buffer, sizeof(buffer), "%H:%M:%S %d/%m", &timeinfo);
  return String(buffer);
}

bool ConexaoManager::checkForCommands() {
  if (!isConnected()) return false;
  
  String path = "/devices/" + deviceId + "/medicoes";
  if (Firebase.RTDB.getJSON(&fbdo, path.c_str())) {
    FirebaseJson* json = fbdo.jsonObjectPtr();
    
    if (json != nullptr) {
      FirebaseJsonData jsonData;
      
      // Verificar se há um comando solicitado
      if (json->get(jsonData, "estado") && 
          jsonData.typeNum == FirebaseJson::JSON_STRING && 
          jsonData.stringValue == "solicitada") {
        
        currentMeasurement.estado = jsonData.stringValue;
        
        // Obter tipo se disponível
        if (json->get(jsonData, "tipo") && jsonData.typeNum == FirebaseJson::JSON_STRING) {
          currentMeasurement.tipo = jsonData.stringValue;
        }
        
        // Obter usuário se disponível
        if (json->get(jsonData, "usuario") && jsonData.typeNum == FirebaseJson::JSON_STRING) {
          currentMeasurement.usuario = jsonData.stringValue;
        }
        
        // Obter timestamp se disponível
        if (json->get(jsonData, "timestampSolicitacao") && jsonData.typeNum == FirebaseJson::JSON_INT) {
          currentMeasurement.timestampSolicitacao = jsonData.intValue;
        }
        
        return true;
      }
    }
  }
  
  return false;
}

bool ConexaoManager::checkForStopCommand() {
  if (!isConnected()) return false;
  
  String path = "/devices/" + deviceId + "/medicoes/comando";
  if (Firebase.RTDB.getString(&fbdo, path.c_str())) {
    if (fbdo.stringData() == "parar") {
      // Limpar o comando após processá-lo
      Firebase.RTDB.deleteNode(&fbdo, path.c_str());
      return true;
    }
  }
  
  return false;
}


bool ConexaoManager::sendCalibrationProgress(int progresso, int total) {
    if (!isConnected()) return false;
    
    String path = "/devices/" + deviceId + "/medicoes/progressoCalibracao";
    FirebaseJson progressoJson;
    progressoJson.set("completos", progresso);
    progressoJson.set("total", total);
    progressoJson.set("percentual", (progresso * 100) / total);
    
    return Firebase.RTDB.setJSON(&fbdo, path.c_str(), &progressoJson);
}

bool ConexaoManager::sendSensorCalibrated(int sensorIndex) {
    if (!isConnected()) return false;
    
    String path = "/devices/" + deviceId + "/medicoes/sensoresCalibrados/" + String(sensorIndex);
    return Firebase.RTDB.setBool(&fbdo, path.c_str(), true);
}

bool ConexaoManager::updateDeviceStatus(const String& status) {
  if (!isConnected()) return false;
  
  String path = "/devices/" + deviceId + "/estado";
  FirebaseJson update;
  update.set("estado", status);
  update.set("ultimaAtualizacao", getTimestamp());
  
  return Firebase.RTDB.updateNode(&fbdo, path.c_str(), &update);
}

bool ConexaoManager::updateDevicemMdicoes(const String status) {
  if (!isConnected()) return false;
  
  String path = "/devices/" + deviceId + "/medicoes";
  FirebaseJson update;
  update.set("estado", status);
  
  if (status == "concluida") {
    update.set("timestampConclusao", getTimestamp());
  }
  
  return Firebase.RTDB.updateNode(&fbdo, path.c_str(), &update);
}

bool ConexaoManager::updateDeviceEx() {
  if (!isConnected()) return false;
  
  String path = "/devices/" + deviceId + "/medicoes";
  FirebaseJson update;
  update.set("estado", "executando");
  update.set("timestampInicio", getTimestamp());
  
  return Firebase.RTDB.updateNode(&fbdo, path.c_str(), &update);
}

bool ConexaoManager::setMeasurementResult(float value) {
  if (!isConnected()) return false;
  
  String path = "/devices/" + deviceId + "/medicoes";
  FirebaseJson update;
  update.set("valor", value);
  update.set("timestampConclusao", getTimestamp());
  
  return Firebase.RTDB.updateNode(&fbdo, path.c_str(), &update);
}

bool ConexaoManager::setCurrentLed(int ledIndex) {
  if (!isConnected()) return false;
  
  String path = "/devices/" + deviceId + "/medicoes/ledAtual";
  return Firebase.RTDB.setInt(&fbdo, path.c_str(), ledIndex);
}

bool ConexaoManager::sendPrecisionResult(bool acerto, unsigned long tempoResposta, int sensorTocado, int ledAlvo) {
  if (!isConnected()) return false;
  
  String path = "/devices/" + deviceId + "/medicoes/ultimoResultado";
  FirebaseJson resultado;
  resultado.set("acerto", acerto);
  resultado.set("tempoResposta", tempoResposta);
  resultado.set("sensorTocado", sensorTocado);
  resultado.set("ledAlvo", ledAlvo);
  resultado.set("timestamp", getTimestamp());
  
  return Firebase.RTDB.setJSON(&fbdo, path.c_str(), &resultado);
}

bool ConexaoManager::sendPrecisionFinalResult(int totalAcertos, int totalErros) {
  if (!isConnected()) return false;
  
  String path = "/devices/" + deviceId + "/medicoes";
  FirebaseJson resultado;
  resultado.set("estado", "concluida");
  resultado.set("totalAcertos", totalAcertos);
  resultado.set("totalErros", totalErros);
  resultado.set("timestampConclusao", getTimestamp());
  
  return Firebase.RTDB.updateNode(&fbdo, path.c_str(), &resultado);
}

bool ConexaoManager::updatePrecisionStatus(const String& status) {
  if (!isConnected()) return false;
  
  String path = "/devices/" + deviceId + "/medicoes/estadoPrecisao";
  return Firebase.RTDB.setString(&fbdo, path.c_str(), status);
}

bool ConexaoManager::clearPrecisionData() {
  if (!isConnected()) return false;
  
  String path = "/devices/" + deviceId + "/medicoes/ledAtual";
  Firebase.RTDB.deleteNode(&fbdo, path.c_str());
  
  path = "/devices/" + deviceId + "/medicoes/ultimoResultado";
  Firebase.RTDB.deleteNode(&fbdo, path.c_str());
  
  return true;
}

Medicao ConexaoManager::getCurrentMeasurement() {
  return currentMeasurement;
}

String ConexaoManager::generateDeviceId() {
  String macAddress = WiFi.macAddress();
  macAddress.replace(":", "");
  
  uint8_t mac[6];
  for (int i = 0; i < 6; i++) {
    String byteString = macAddress.substring(i * 2, i * 2 + 2);
    mac[i] = strtoul(byteString.c_str(), NULL, 16);
  }
  
  uint64_t hash = 0xcbf29ce484222325ULL;
  const uint64_t prime = 0x100000001b3ULL;
  
  for (int i = 0; i < 6; i++) {
    hash ^= mac[i];
    hash *= prime;
  }
  
  char deviceId[17];
  sprintf(deviceId, "%016llX", hash);
  
  return String(deviceId);
}

// Métodos auxiliares privados
bool ConexaoManager::sendToFirebase(const String& path, const String& value) {
  return Firebase.RTDB.setString(&fbdo, path.c_str(), value);
}

bool ConexaoManager::sendToFirebase(const String& path, int value) {
  return Firebase.RTDB.setInt(&fbdo, path.c_str(), value);
}

bool ConexaoManager::sendToFirebase(const String& path, float value) {
  return Firebase.RTDB.setFloat(&fbdo, path.c_str(), value);
}

bool ConexaoManager::sendToFirebase(const String& path, bool value) {
  return Firebase.RTDB.setBool(&fbdo, path.c_str(), value);
}

bool ConexaoManager::sendToFirebase(const String& path, FirebaseJson& json) {
  return Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json);
}