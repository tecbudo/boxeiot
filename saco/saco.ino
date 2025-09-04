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
      setaDisplay.showtime(dataHora);
      xSemaphoreGive(xDisplayMutex);
    }
 }
 /**
  * @brief Tarefa para calibração dos sensores
  */
 void tarefaCalibra(void* arg) {
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
         xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
         setaDisplay.printlog("Calibração iniciada");
         setaDisplay.print("CALIBRANDO");
         xSemaphoreGive(xDisplayMutex);
       }
       
       // Processar um passo da calibração
       bool calibracaoCompleta = sensores.processarCalibracaoInterativa();
       
       // Enviar progresso para o Firebase se mudou
       int progressoAtual = sensores.getProgressoCalibracao();
       if (progressoAtual != ultimoProgresso) {
         ultimoProgresso = progressoAtual;
         conexao.sendCalibrationProgress(progressoAtual, NUM_SENSORES);
         
         // Atualizar display
         xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
         setaDisplay.printlog("Calibrado: " + String(progressoAtual) + "/" + String(NUM_SENSORES));
         xSemaphoreGive(xDisplayMutex);
         
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
         xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
         setaDisplay.printlog("Calibração concluída");
         setaDisplay.print("PRONTO");
         xSemaphoreGive(xDisplayMutex);
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
   unsigned long ultimaAtualizacaoHora = 0;
   
   while (1) {
     // Verificar comandos do Firebase
     if (conexao.checkForCommands()) {
       Medicao medicao = conexao.getCurrentMeasurement();  
       if (conexao.updateDeviceEx()) { 
         Serial.print("Comando recebido: ");
         Serial.println(medicao.tipo);
         xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
         setaDisplay.printlog("Comando: " + medicao.tipo);
         xSemaphoreGive(xDisplayMutex);
         
         // Mostrar tipo de medição no display
         xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
         if (medicao.tipo == "forca") {
           setaDisplay.print("MODO FORÇA");
         } else if (medicao.tipo == "precisao") {
           setaDisplay.print("MODO PRECISÃO");
         } else if (medicao.tipo == "tempo_reacao") {
           setaDisplay.print("MODO AGILIDADE");
         } else if (medicao.tipo == "tCalibrar") {
           setaDisplay.print("MODO CALIBRAÇÃO");
         }
         xSemaphoreGive(xDisplayMutex);
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
       xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
       setaDisplay.printazul("conectado");
       xSemaphoreGive(xDisplayMutex);
     }
     

     // Manter conexão ativa
     if (!conexao.isConnected()) {
       conexao.begin();
       xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
       setaDisplay.printlog("Reconectando...");
       xSemaphoreGive(xDisplayMutex);
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
       xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
       setaDisplay.print("AGUARDANDO...");
       xSemaphoreGive(xDisplayMutex);
       
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
         
         // Mostrar resultado no display
         xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
         setaDisplay.printazul("Tempo: " + String(tempoReacao) + "s");
         setaDisplay.printlog("Reação: " + String(tempoReacao) + "s");
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
       
       // Usar display para mostrar a seta correspondente
       xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
       if (ledSorteado == 8) {
         setaDisplay.seta("f");  // Centro - círculo com ponto
       } else {
         setaDisplay.seta(ledSorteado * 45);  // Converter índice para ângulo
       }
       setaDisplay.printazul("PRECISÃO");
       xSemaphoreGive(xDisplayMutex);
       
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
       
       if (sensorTocado >= 0) {
         bool acerto = (sensorTocado == ledSorteado);
         unsigned long tempoResposta = millis() - tempoInicio;
         
         // Atualizar contadores
         if (acerto) {
           totalAcertos++;
           Serial.print("Acerto! Total: ");
           Serial.println(totalAcertos);
           xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
           setaDisplay.printlog("Acerto! " + String(totalAcertos));
           setaDisplay.printazul("ACERTOU!");
           xSemaphoreGive(xDisplayMutex);
         } else {
           totalErros++;
           Serial.print("Erro! Total: ");
           Serial.println(totalErros);
           xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
           setaDisplay.printlog("Erro! " + String(totalErros));
           setaDisplay.printazul("ERROU!");
           xSemaphoreGive(xDisplayMutex);
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
         xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
         setaDisplay.printlog("Precisão finalizada");
         setaDisplay.print("PRONTO");
         setaDisplay.clear();
         setaDisplay.update();
         xSemaphoreGive(xDisplayMutex);
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
       xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
       setaDisplay.printazul("MEDINDO FORÇA");
       xSemaphoreGive(xDisplayMutex);
       
       float forca = sensores.calcularForca();
       
       // Enviar resultado
       conexao.setMeasurementResult(forca);
       conexao.updateDevicemMdicoes("concluida");
       
       // Mostrar resultado no display
       xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
       setaDisplay.printazul("Força: " + String(forca));
       xSemaphoreGive(xDisplayMutex);
       
       xSemaphoreTake(xEstadoMutex, portMAX_DELAY);
       estadoAtual = Estado::Inicial;
       xSemaphoreGive(xEstadoMutex);
       
       xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
       setaDisplay.print("PRONTO");
       xSemaphoreGive(xDisplayMutex);
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
   sensores.calibrarSensoresToque();
   Serial.println("Sensores calibrados");
   setaDisplay.printlog("Sensores calibrados");
   
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

   setaDisplay.print("PRONTO");
   
   // Iniciar tarefas
   xTaskCreate(tarefaPiscarLED, "tarefaPiscarLED", 4096, NULL, 1, NULL);
   xTaskCreate(tarefaComunicacao, "tarefaComunicacao", 8192, NULL, 2, NULL);
   xTaskCreate(tarefaAgilidade, "tarefaAgilidade", 4096, NULL, 1, NULL);
   xTaskCreate(tarefaPrecisao, "tarefaPrecisao", 4096, NULL, 1, NULL);
   xTaskCreate(tarefaForca, "tarefaForca", 4096, NULL, 1, NULL);
   xTaskCreate(tarefaCalibra, "tarefaCalibra", 4096, NULL, 1, NULL);
   xTaskCreate(tarefaDataHora, "tarefaDataHora", 4096, NULL, 1, NULL);

   
   Serial.println("Todas as tarefas iniciadas");
   setaDisplay.printlog("Sistema pronto");
 }
 
 /**
  * @brief Função de loop principal
  */
 void loop() {
   // Ajuste dinâmico das referências dos sensores
   // sensores.ajusteDinamicoReferencias();
   
   // Mostrar "Em espera" quando inativo
   xSemaphoreTake(xEstadoMutex, portMAX_DELAY);
   Estado estadoLocal = estadoAtual;
   xSemaphoreGive(xEstadoMutex);
   
   if (estadoLocal == Estado::Inicial) {
     xSemaphoreTake(xDisplayMutex, portMAX_DELAY);
     setaDisplay.printazul("EM ESPERA");
     xSemaphoreGive(xDisplayMutex);
   }
   
   vTaskDelay(1000 / portTICK_PERIOD_MS);
 }