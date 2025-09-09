/**
 * @file saco.ino
 * @brief Programa saco para ESP32 com integração Firebase - Versão com Display OLED
 */

 #include "Conexao.h"
 #include "Sensores.h"
 #include "display.h"
 #include <freertos/semphr.h>
 
 // Definições de pinos
 #define PINO_LED 15
 #define ENDERECO_MPU6500 0x68
 
 uint32_t tempoPisca = 1500;
 
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
 
 // Instância do display OLED
 SetaDisplay setaDisplay;
 SemaphoreHandle_t xDisplayMutex;
 
 /**
  * @brief Tarefa que serve para indicar em que estado o programa se encontra.
  */
 void tarefaPiscarLED(void* arg) {
  
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
  * @brief Tarefa que serve para indicar em que estado o programa se encontra.
  */
 void tarefaDataHora(void* arg) {
    while (1) {
      vTaskDelay(4000/ portTICK_PERIOD_MS);
      String dataHora = conexao.getTimeString();
      xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
      setaDisplay.showtimeCompact(dataHora);
      xSemaphoreGive(xDisplayMutex);
    }
 }
 /**
  * @brief Tarefa para calibração dos sensores
  */
void tarefaCalibra(void* arg) {
    int ultimoSensor = -1;
    
    while (1) {
        xSemaphoreTake(xEstadoMutex, portMAX_DELAY);
        Estado estadoLocal = estadoAtual;
        xSemaphoreGive(xEstadoMutex);
        
        if (estadoLocal == Estado::Calibrar) {
            // Obter o sensor específico a calibrar
            int sensorParaCalibrar = conexao.getSensorCalibracao();
            
            if (sensorParaCalibrar >= 1 && sensorParaCalibrar <= 9) {
                // Sensor mudou? Atualizar display
                if (sensorParaCalibrar != ultimoSensor) {
                    xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
                    if (sensorParaCalibrar == 9) {
                        setaDisplay.seta("f");  // Centro
                    } else {
                        setaDisplay.seta((sensorParaCalibrar - 1) * 45);
                    }
                    xSemaphoreGive(xDisplayMutex);
                    ultimoSensor = sensorParaCalibrar;
                }
                
                // Informar início da calibração
                conexao.updateDeviceEx();  
                
                // Calibrar sensor específico (convertendo 1-9 para 0-8)
                bool sucesso = sensores.calibrarSensorIndividual(sensorParaCalibrar - 1);
                
                // Informar resultado
                if (sucesso) {
                    conexao.updateDevicemMdicoes("concluida");
                    // Marcar sensor como calibrado
                    conexao.sendSensorCalibrated(sensorParaCalibrar);
                } else {
                    conexao.updateDevicemMdicoes("erro");
                }
            }
            vTaskDelay(500 / portTICK_PERIOD_MS);
        } else {
            ultimoSensor = -1; // Reset quando sair do modo calibração
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}


  /**
 * @brief Tarefa para gerenciar a comunicação com Firebase - Versão Refatorada
 */
void tarefaComunicacao(void* arg) {
  unsigned long ultimaAtualizacaoHora = 0;
  int ultimoStatus = 1; // Inicia como desconectado
  
  while (1) {
    // Verificar se está conectado ao Firebase
    bool conectado = conexao.isConnected();
    
    if (!conectado) {
      // Tentar reconectar
      conexao.begin();
      
      // Atualizar status para desconectado se necessário
      if (ultimoStatus != 1) {
        xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
        setaDisplay.setStatus(1);
        setaDisplay.printlog("Reconectando...");
        xSemaphoreGive(xDisplayMutex);
        ultimoStatus = 1;
      }
      
      vTaskDelay(5000 / portTICK_PERIOD_MS); // Espera 5 segundos antes de tentar novamente
      continue;
    }
    
    // Se conectado, verificar o estado do dispositivo
    String estadoDispositivo = conexao.getDeviceState();
    
    // Atualizar o status de conexão no display
    if (estadoDispositivo == "ocupado" && ultimoStatus != 3) {
      xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
      setaDisplay.setStatus(3); // App conectado
      xSemaphoreGive(xDisplayMutex);
      ultimoStatus = 3;
    } 
    else if (estadoDispositivo == "disponivel" && ultimoStatus != 2) {
      xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
      setaDisplay.setStatus(2); // Conectado ao Firebase
      xSemaphoreGive(xDisplayMutex);
      ultimoStatus = 2;
    }
    else if (estadoDispositivo == "manutencao" && ultimoStatus != 4) {
      xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
      setaDisplay.setStatus(4); // Modo manutenção
      xSemaphoreGive(xDisplayMutex);
      ultimoStatus = 4;
    }
    
    // Só verificar comandos se o dispositivo estiver ocupado
    if (estadoDispositivo == "ocupado") {
      if (conexao.checkForCommands()) {
        Medicao medicao = conexao.getCurrentMeasurement();  
        
        if (conexao.updateDeviceEx()) { 
          Serial.print("Comando recebido: ");
          Serial.println(medicao.tipo);
          
          // Obter todos os mutexes necessários primeiro
          xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
          xSemaphoreTake(xEstadoMutex, portMAX_DELAY);
          
          // Processar o tipo de medição uma única vez
          if (medicao.tipo == "forca") {
            setaDisplay.printazul("FORCA");
            estadoAtual = Estado::Forca;
          } 
          else if (medicao.tipo == "precisao") {
            setaDisplay.printazul("PRECISAO");
            estadoAtual = Estado::Precisao;
          } 
          else if (medicao.tipo == "tempo_reacao") {
            setaDisplay.printazul("AGILIDADE");
            estadoAtual = Estado::Agilidade;
          } 
          else if (medicao.tipo == "tCalibrar") {
            setaDisplay.printazul("CALIBRAR");
            estadoAtual = Estado::Calibrar;
          }
          
          // Liberar mutexes
          xSemaphoreGive(xEstadoMutex);
          xSemaphoreGive(xDisplayMutex);
        }
      }
    }
    
    
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
 
 /**
  * @brief Tarefa que estuda o tempo de reação do usuario
  */
 void tarefaAgilidade(void* arg) {
   
   while (1) {
     xSemaphoreTake(xEstadoMutex, portMAX_DELAY);
     Estado estadoLocal = estadoAtual;
     xSemaphoreGive(xEstadoMutex);
     
     if (estadoLocal == Estado::Agilidade) {
       xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
       setaDisplay.printazul("Prepare-se...");
      
       int intervalo = random(2000, 7000);
       delay(intervalo);
       setaDisplay.printazul("Ataque");  
       xSemaphoreGive(xDisplayMutex);
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
         
         // Mostrar resultado no display
         xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
         setaDisplay.printazul("Tempo: " + String(tempoReacao) + "s");
         xSemaphoreGive(xDisplayMutex);
       }
       
       xSemaphoreTake(xEstadoMutex, portMAX_DELAY);
       estadoAtual = Estado::Inicial;
       xSemaphoreGive(xEstadoMutex);
       
       xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
       setaDisplay.print("PRONTO");
       xSemaphoreGive(xDisplayMutex);
     }
     vTaskDelay(100 / portTICK_PERIOD_MS);
   }
 }
 
 /**
 * @brief Tarefa para teste de precisão - Versão Simplificada
 * Agora a interface web controla a sequência de LEDs
 */
void tarefaPrecisao(void* arg) {
    int ultimoLed = -1;
    unsigned long tempoInicio = 0;
    
    while (1) {
        xSemaphoreTake(xEstadoMutex, portMAX_DELAY);
        Estado estadoLocal = estadoAtual;
        xSemaphoreGive(xEstadoMutex);
        
        if (estadoLocal == Estado::Precisao) {
            // Obter o LED específico a ser acionado
            int ledParaAcender = conexao.getLedPrecisao();
            
            if (ledParaAcender >= 1 && ledParaAcender <= 9) {
                // LED mudou? Atualizar display
                if (ledParaAcender != ultimoLed) {
                    xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
                    if (ledParaAcender == 9) {
                        setaDisplay.seta("f");  // Centro
                    } else {
                        setaDisplay.seta((ledParaAcender - 1) * 45);
                    }
                    setaDisplay.printazul("PRECISÃO");
                    xSemaphoreGive(xDisplayMutex);
                    ultimoLed = ledParaAcender;
                    tempoInicio = millis();
                }
                
                // Aguardar toque no sensor
                int sensorTocado = sensores.detectarToque();
                
                if (sensorTocado >= 0) {
                    // Calcular tempo de resposta
                    unsigned long tempoResposta = millis() - tempoInicio;
                    
                    // Determinar se foi acerto (sensor indexado 0-8, LED 1-9)
                    bool acerto = (sensorTocado == (ledParaAcender - 1));
                    
                    // Enviar resultado para Firebase
                    conexao.sendPrecisionResult(acerto, tempoResposta, sensorTocado, ledParaAcender);
                    
                    // Feedback visual no display
                    xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
                    if (acerto) {
                        setaDisplay.printazul("ACERTOU!");
                    } else {
                        setaDisplay.printazul("ERROU!");
                    }
                    xSemaphoreGive(xDisplayMutex);
                    
                    // Pequena pausa para feedback
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    
                    // Preparar para próximo LED
                    ultimoLed = -1;
                }
            }
            vTaskDelay(50 / portTICK_PERIOD_MS);
        } else {
            ultimoLed = -1; // Reset quando sair do modo precisão
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
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
       xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
       setaDisplay.printazul("MEDINDO FORÇA");
       xSemaphoreGive(xDisplayMutex);
       
       float forca = sensores.calcularForca();
       
       // Enviar resultado
       conexao.setMeasurementResult(forca);
       conexao.updateDevicemMdicoes("concluida");
       
       // Mostrar resultado no display
       xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
       setaDisplay.printazul("F: " + String(forca));
       xSemaphoreGive(xDisplayMutex);
       
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
   
   // Criar mutex para proteção do display
   xDisplayMutex = xSemaphoreCreateMutex();
   if (xDisplayMutex == NULL) {
     Serial.println("Falha ao criar mutex do display");
     while(1);
   }
   
   // Inicializar display
   setaDisplay.begin();
   setaDisplay.clear();
   setaDisplay.update();
   setaDisplay.print("INICIANDO...");
   setaDisplay.setStatus(1);
   setaDisplay.printlog("Sistema iniciando");
   
   // Inicializar conexões
   conexao.begin();
   Serial.println("Conexões inicializadas");
   setaDisplay.printlog("Conexões OK");
   
   // Inicializar sensores
   sensores.iniciar();
   Serial.println("Sensores inicializados");
   setaDisplay.printlog("Sensores OK");
   
   // Calibrar sensores de toque
   //sensores.calibrarSensoresToque();
   //Serial.println("Sensores calibrados");
   //setaDisplay.printlog("Sensores calibrados");
   
   // Mostrar hora atual
   String dataHora = conexao.getTimeString();

// Usar no display
      xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
      setaDisplay.showtime(dataHora);
      xSemaphoreGive(xDisplayMutex);

      // Ou usar para logging
      Serial.println("Data/hora atual: " + dataHora);
      xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
      setaDisplay.printlog("Hora: " + dataHora);
      xSemaphoreGive(xDisplayMutex);
  
    
   // Iniciar tarefas
   xTaskCreate(tarefaPiscarLED, "tarefaPiscarLED", 4096, NULL, 1, NULL);
   xTaskCreate(tarefaComunicacao, "tarefaComunicacao", 8192, NULL, 2, NULL);
   xTaskCreate(tarefaAgilidade, "tarefaAgilidade", 4096, NULL, 1, NULL);
   xTaskCreate(tarefaPrecisao, "tarefaPrecisao", 8192, NULL, 1, NULL);
   xTaskCreate(tarefaForca, "tarefaForca", 4096, NULL, 1, NULL);
   xTaskCreate(tarefaCalibra, "tarefaCalibra", 8192, NULL, 1, NULL);
   xTaskCreate(tarefaDataHora, "tarefaDataHora", 4096, NULL, 1, NULL);  
   Serial.println("Todas as tarefas iniciadas");
   setaDisplay.printlog("Sistema pronto");
   conexao.updateDeviceStatus("disponivel");
   setaDisplay.setStatus(2);
 }
 
 /**
  * @brief Função de loop principal
  */
 void loop() {
   
   vTaskDelay(1000 / portTICK_PERIOD_MS);
 }