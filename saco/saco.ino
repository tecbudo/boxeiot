/**
 * @file saco.ino
 * @brief Programa saco v3 para ESP32 com integração Firebase - Versão Melhorada
 */

#include "Conexao.h"
#include "Sensores.h"
#include <freertos/semphr.h>

// Definições de pinos
#define PINO_LED 15
#define ENDERECO_MPU6500 0x68

uint32_t tempoPisca = 500;

enum class Estado {
  Inicial,
  Forca,
  Agilidade,
  Calibrar,
  Precisao
};

// Variáveis globais
Estado estadoAtual = Estado::Inicial;
SemaphoreHandle_t xEstadoMutex;

/**
 * @brief Tarefa que serve para indicar em que estado o programa se encontra.
 */
void tarefaPiscarLED(void* arg) {
  pinMode(PINO_LED, OUTPUT);
  
  while (1) {
    xSemaphoreTake(xEstadoMutex, portMAX_DELAY);
    Estado estadoLocal = estadoAtual;
    xSemaphoreGive(xEstadoMutex);
    
    if (estadoLocal == Estado::Inicial) {
      digitalWrite(PINO_LED, HIGH);
      vTaskDelay(tempoPisca / portTICK_PERIOD_MS);
      digitalWrite(PINO_LED, LOW);
    }
    if (estadoLocal != Estado::Inicial) {
      digitalWrite(PINO_LED, HIGH);
    }

    vTaskDelay(tempoPisca / portTICK_PERIOD_MS);
  }
}

/**
 * @brief Tarefa para calibração dos sensores
 */void tarefaCalibra(void* arg) {
  bool calibracaoAtiva = false;
  int ultimoProgresso = -1;
  
  while (1) {
    xSemaphoreTake(xEstadoMutex, portMAX_DELAY);
    Estado estadoLocal = estadoAtual;
    xSemaphoreGive(xEstadoMutex);
    
    if (estadoLocal == Estado::Calibrar) {
      if (!calibracaoAtiva) {
        // Iniciar nova calibração
        sensores.iniciarCalibracaoInterativa();
        calibracaoAtiva = true;
        ultimoProgresso = -1;
        
        // Enviar status inicial para Firebase
        conexao.updatePrecisionStatus("executando");
        conexao.sendCalibrationProgress(0, NUM_SENSORES);
        
        Serial.println("Calibração interativa iniciada");
      }
      
      // Processar um passo da calibração
      bool calibracaoCompleta = sensores.processarCalibracaoInterativa();
      
      // Enviar progresso para o Firebase se mudou
      int progressoAtual = sensores.getProgressoCalibracao();
      if (progressoAtual != ultimoProgresso) {
        ultimoProgresso = progressoAtual;
        conexao.sendCalibrationProgress(progressoAtual, NUM_SENSORES);
        
        // Enviar quais sensores foram calibrados
        for (int i = 0; i < NUM_SENSORES; i++) {
          if (sensores.getSensorCalibrado(i) == 1) {
            conexao.sendSensorCalibrated(i);
          }
        }
      }
      
      if (calibracaoCompleta) {
        // Calibração concluída
        conexao.setMeasurementResult(0.0);
        conexao.updateDevicemMdicoes("concluida");
        conexao.updatePrecisionStatus("concluida");
        
        xSemaphoreTake(xEstadoMutex, portMAX_DELAY);
        estadoAtual = Estado::Inicial;
        xSemaphoreGive(xEstadoMutex);
        calibracaoAtiva = false;
        Serial.println("Calibração interativa concluída");
      }
    } else {
      calibracaoAtiva = false;
    }
    
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

/**
 * @brief Tarefa para gerenciar a comunicação com Firebase
 */
void tarefaComunicacao(void* arg) {
  while (1) {
    // Verificar comandos do Firebase
    if (conexao.checkForCommands()) {
      Medicao medicao = conexao.getCurrentMeasurement();  
      if (conexao.updateDeviceEx()) { 
        Serial.print("Comando recebido: ");
        Serial.println(medicao.tipo);
      }
      
      // Definir estado com base no tipo de medição
      xSemaphoreTake(xEstadoMutex, portMAX_DELAY);
      if (medicao.tipo == "forca") {
        estadoAtual = Estado::Forca;
      } else if (medicao.tipo == "precisao") {
        estadoAtual = Estado::Precisao;
      } else if (medicao.tipo == "tempo_reacao") {
        estadoAtual = Estado::Agilidade;
      } else if (medicao.tipo == "tCalibrar") {
        estadoAtual = Estado::Calibrar;
      }         
      xSemaphoreGive(xEstadoMutex);
      
      conexao.updateDeviceStatus("ocupado");
    }
    
    // Manter conexão ativa
    if (!conexao.isConnected()) {
      conexao.begin();
    }
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

/**
 * @brief Tarefa que estuda o tempo de reação do usuario
 */
void tarefaAgilidade(void* arg) {
  pinMode(LED_BUILTIN, OUTPUT);
  
  while (1) {
    xSemaphoreTake(xEstadoMutex, portMAX_DELAY);
    Estado estadoLocal = estadoAtual;
    xSemaphoreGive(xEstadoMutex);
    
    if (estadoLocal == Estado::Agilidade) {
      digitalWrite(LED_BUILTIN, HIGH);
      int intervalo = random(2000, 7000);
      delay(intervalo);
      digitalWrite(LED_BUILTIN, LOW);
      
      unsigned long tempoInicio = millis();
      int sensorTocado = -1;
      
      while (sensorTocado < 0) {
        sensorTocado = sensores.detectarToque();
        vTaskDelay(10 / portTICK_PERIOD_MS);
        
        // Verificar se ainda está no modo agilidade
        xSemaphoreTake(xEstadoMutex, portMAX_DELAY);
        bool aindaNoModo = (estadoAtual == Estado::Agilidade);
        xSemaphoreGive(xEstadoMutex);
        
        if (!aindaNoModo) {
          break; // Sai se o modo foi alterado
        }
      }
      
      if (sensorTocado >= 0) {
        float tempoReacao = (millis() - tempoInicio) / 1000.0;
        
        // Enviar resultado
        conexao.setMeasurementResult(tempoReacao);
        conexao.updateDevicemMdicoes("concluida");
      }
      
      xSemaphoreTake(xEstadoMutex, portMAX_DELAY);
      estadoAtual = Estado::Inicial;
      xSemaphoreGive(xEstadoMutex);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

/**
 * @brief Tarefa que estuda a precisão dos golpes - Versão Melhorada
 */
void tarefaPrecisao(void* arg) {
  // Variáveis estáticas para manter os totais entre as iterações do loop
  static int totalAcertos = 0;
  static int totalErros = 0;
  
  while (1) {
    xSemaphoreTake(xEstadoMutex, portMAX_DELAY);
    Estado estadoLocal = estadoAtual;
    xSemaphoreGive(xEstadoMutex);
    
    if (estadoLocal == Estado::Precisao) {
      int ledSorteado = random(0, 9);
      sensores.ledOn(ledSorteado);
      
      // Enviar informação do LED sorteado para Firebase
      conexao.setCurrentLed(ledSorteado);
      
      int sensorTocado = -1;
      unsigned long tempoInicio = millis();
      const unsigned long timeoutMs = 30000; // Timeout de 30 segundos
      
      while (sensorTocado < 0 && (millis() - tempoInicio) < timeoutMs) {
        sensorTocado = sensores.detectarToque();
        
        // Verificar se há comando de parada
        if (conexao.checkForStopCommand()) {
          break;
        }
        
        // Verificar se ainda está no modo precisão
        xSemaphoreTake(xEstadoMutex, portMAX_DELAY);
        bool aindaNoModo = (estadoAtual == Estado::Precisao);
        xSemaphoreGive(xEstadoMutex);
        
        if (!aindaNoModo) {
          break;
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);
      }
      
      sensores.ledOff(ledSorteado);
      
      if (sensorTocado >= 0) {
        bool acerto = (sensorTocado == ledSorteado);
        unsigned long tempoResposta = millis() - tempoInicio;
        
        // Atualizar contadores
        if (acerto) {
          totalAcertos++;
          Serial.print("Acerto! Total: ");
          Serial.println(totalAcertos);
        } else {
          totalErros++;
          Serial.print("Erro! Total: ");
          Serial.println(totalErros);
        }
        
        // Enviar resultado para Firebase
        conexao.sendPrecisionResult(acerto, tempoResposta, sensorTocado, ledSorteado);
      }
      
      // Verificar se foi cancelado ou timeout
      if (conexao.checkForStopCommand() || (millis() - tempoInicio) >= timeoutMs) {
        // Enviar resultados finais
        conexao.sendPrecisionFinalResult(totalAcertos, totalErros);
        conexao.updateDevicemMdicoes("concluida");
        
        // Resetar contadores para a próxima sessão
        totalAcertos = 0;
        totalErros = 0;
        
        xSemaphoreTake(xEstadoMutex, portMAX_DELAY);
        estadoAtual = Estado::Inicial;
        xSemaphoreGive(xEstadoMutex);
        Serial.println("Sessão de precisão finalizada. Contadores resetados.");
      }
    }
    
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

/**
 * @brief Tarefa que estuda a força do soco
 */
void tarefaForca(void* arg) {
  while (1) {
    xSemaphoreTake(xEstadoMutex, portMAX_DELAY);
    Estado estadoLocal = estadoAtual;
    xSemaphoreGive(xEstadoMutex);
    
    if (estadoLocal == Estado::Forca) {
      float forca = sensores.calcularForca();
      
      // Enviar resultado
      conexao.setMeasurementResult(forca);
      conexao.updateDevicemMdicoes("concluida");
      
      xSemaphoreTake(xEstadoMutex, portMAX_DELAY);
      estadoAtual = Estado::Inicial;
      xSemaphoreGive(xEstadoMutex);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

/**
 * @brief Função de configuração inicial do programa
 */
void setup() {
  pinMode(PINO_LED, OUTPUT);
  Serial.begin(115200);
  delay(2000);
  Serial.println("Iniciando sistema...");
  
  // Criar mutex para proteção do estado
  xEstadoMutex = xSemaphoreCreateMutex();
  if (xEstadoMutex == NULL) {
    Serial.println("Falha ao criar mutex");
    while(1);
  }
  
  // Inicializar conexões
  conexao.begin();
  Serial.println("Conexões inicializadas");
  
  // Inicializar sensores
  sensores.iniciar();
  Serial.println("Sensores inicializados");
  
  // Calibrar sensores de toque
  sensores.calibrarSensoresToque();
  Serial.println("Sensores calibrados");
  
  // Iniciar tarefas
  xTaskCreate(tarefaPiscarLED, "tarefaPiscarLED", 4096, NULL, 1, NULL);
  xTaskCreate(tarefaComunicacao, "tarefaComunicacao", 8192, NULL, 2, NULL);
  xTaskCreate(tarefaAgilidade, "tarefaAgilidade", 4096, NULL, 1, NULL);
  xTaskCreate(tarefaPrecisao, "tarefaPrecisao", 4096, NULL, 1, NULL);
  xTaskCreate(tarefaForca, "tarefaForca", 4096, NULL, 1, NULL);
  xTaskCreate(tarefaCalibra, "tarefaCalibra", 4096, NULL, 1, NULL);
  
  Serial.println("Todas as tarefas iniciadas");
}

/**
 * @brief Função de loop principal
 */
void loop() {
  // Ajuste dinâmico das referências dos sensores
  // sensores.ajusteDinamicoReferencias();
  
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}