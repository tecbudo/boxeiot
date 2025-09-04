/**
 * @file display.cpp
 * @brief Implementação da classe SetaDisplay para controle de display OLED
 * 
 * Este arquivo contém a implementação dos métodos declarados em display.h,
 * fornecendo a funcionalidade completa para controle do display OLED.
 * 
 * @section implementation Detalhes de Implementação
 * - Utiliza a biblioteca Adafruit_SSD1306 para controle de hardware
 * - Implementa um sistema de log com rotação de mensagens
 * - Fornece funções para desenho de setas e exibição de texto
 * - Gerencia automaticamente a limpeza e atualização do display
 */
#include "display.h"

SetaDisplay::SetaDisplay() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1) {
  centroX = SCREEN_WIDTH / 2;
  centroY = YELLOW_AREA_HEIGHT + (BLUE_AREA_HEIGHT / 2);
}

void SetaDisplay::begin() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Display não encontrado!");
    while (1);
  }
  display.clearDisplay();
  display.display();
  
  display.setTextSize(1);
  display.setTextColor(WHITE);
}

void SetaDisplay::clear() {
  display.clearDisplay();
}

void SetaDisplay::update() {
  display.display();
}

void SetaDisplay::atualizarAreaAmarela() {
  // Limpa apenas a área amarela
  display.fillRect(0, 0, SCREEN_WIDTH, YELLOW_AREA_HEIGHT, BLACK);
}

void SetaDisplay::atualizarAreaAzul() {
  // Limpa apenas a área azul
  display.fillRect(0, YELLOW_AREA_HEIGHT, SCREEN_WIDTH, BLUE_AREA_HEIGHT, BLACK);
}

void SetaDisplay::seta(int angulo) {
  // Limpa apenas a área azul
  atualizarAreaAzul();

  switch (angulo) {
    case 0:   desenharSetaReta(1); break;
    case 45:  desenharSetaAngular(45); break;
    case 90:  desenharSetaReta(0); break;
    case 135: desenharSetaAngular(135); break;
    case 180: desenharSetaReta(3); break;
    case 225: desenharSetaAngular(225); break;
    case 270: desenharSetaReta(2); break;
    case 315: desenharSetaAngular(315); break;
  }

  display.display();
}

void SetaDisplay::seta(const char* tipo) {
  // Limpa apenas a área azul
  atualizarAreaAzul();

  if (strcmp(tipo, "f") == 0) {
    desenharSetaFrente();
  }

  display.display();
}

void SetaDisplay::printlog(String mensagem) {
  // Adiciona a nova mensagem no início do vetor (log mais recente primeiro)
  logMessages.insert(logMessages.begin(), mensagem);
  
  // Mantém apenas as últimas MAX_LOG_LINES mensagens
  if (logMessages.size() > MAX_LOG_LINES) {
    logMessages.pop_back(); // Remove a mensagem mais antiga
  }
  
  // Limpa apenas a área azul
  atualizarAreaAzul();
  display.setTextSize(1);
  
  // Desenha as mensagens de log (na área azul) - mais recentes no topo
  for (int i = 0; i < logMessages.size(); i++) {
    display.setCursor(0, YELLOW_AREA_HEIGHT + (i * 8));
    display.println(logMessages[i]);
  }
  
  display.display();
}

void SetaDisplay::print(String mensagem) {
  // Atualiza apenas a área amarela
  atualizarAreaAmarela();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  // Centraliza o texto na área amarela
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(mensagem, 0, 0, &x1, &y1, &w, &h);
  
  int x = (SCREEN_WIDTH - w) / 2;
  int y = (YELLOW_AREA_HEIGHT - h) / 2;
  
  display.setCursor(x, y);
  display.println(mensagem);
  display.display();
}

void SetaDisplay::printazul(String mensagem) {
  // Limpa apenas a área azul
  atualizarAreaAzul();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  
  // Centraliza o texto na área azul
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(mensagem, 0, 0, &x1, &y1, &w, &h);
  
  int x = (SCREEN_WIDTH - w) / 2;
  int y = YELLOW_AREA_HEIGHT + (BLUE_AREA_HEIGHT - h) / 2;
  
  display.setCursor(x, y);
  display.println(mensagem);
  display.display();
}

void SetaDisplay::showtime(String timestamp) {
  // Atualiza apenas a área amarela
  atualizarAreaAmarela();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  // Centraliza o texto na área amarela
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(timestamp, 0, 0, &x1, &y1, &w, &h);
  
  int x = (SCREEN_WIDTH - w) / 2;
  int y = (YELLOW_AREA_HEIGHT - h) / 2;
  
  display.setCursor(x, y);
  display.println(timestamp);
  display.display();
}

void SetaDisplay::desenharLinhaEspessa(int x1, int y1, int x2, int y2) {
  display.drawLine(x1, y1, x2, y2, WHITE);
  
  if (x1 == x2) {
    display.drawLine(x1+1, y1, x2+1, y2, WHITE);
  } else if (y1 == y2) {
    display.drawLine(x1, y1+1, x2, y2+1, WHITE);
  } else {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = sqrt(dx*dx + dy*dy);
    float perpx = -dy/length;
    float perpy = dx/length;
    
    display.drawLine(x1 + perpx, y1 + perpy, x2 + perpx, y2 + perpy, WHITE);
  }
}

void SetaDisplay::desenharSetaReta(int modo) {
  int metadeComprimento = SETA_COMPRIMENTO / 2;
  int comprimentoPonta = 10;
  int larguraBase = 8;

  if (modo == 0) {
    desenharLinhaEspessa(centroX - metadeComprimento, centroY, 
                         centroX + metadeComprimento - comprimentoPonta, centroY);
    display.fillTriangle(centroX + metadeComprimento, centroY,
                         centroX + metadeComprimento - comprimentoPonta, centroY - larguraBase,
                         centroX + metadeComprimento - comprimentoPonta, centroY + larguraBase,
                         WHITE);
  } else if (modo == 1) {
    desenharLinhaEspessa(centroX, centroY + metadeComprimento, 
                         centroX, centroY - metadeComprimento + comprimentoPonta);
    display.fillTriangle(centroX, centroY - metadeComprimento,
                         centroX - larguraBase, centroY - metadeComprimento + comprimentoPonta,
                         centroX + larguraBase, centroY - metadeComprimento + comprimentoPonta,
                         WHITE);
  } else if (modo == 2) {
    desenharLinhaEspessa(centroX + metadeComprimento, centroY, 
                         centroX - metadeComprimento + comprimentoPonta, centroY);
    display.fillTriangle(centroX - metadeComprimento, centroY,
                         centroX - metadeComprimento + comprimentoPonta, centroY - larguraBase,
                         centroX - metadeComprimento + comprimentoPonta, centroY + larguraBase,
                         WHITE);
  } else if (modo == 3) {
    desenharLinhaEspessa(centroX, centroY - metadeComprimento, 
                         centroX, centroY + metadeComprimento - comprimentoPonta);
    display.fillTriangle(centroX, centroY + metadeComprimento,
                         centroX - larguraBase, centroY + metadeComprimento - comprimentoPonta,
                         centroX + larguraBase, centroY + metadeComprimento - comprimentoPonta,
                         WHITE);
  }
}

void SetaDisplay::desenharSetaAngular(int angulo) {
  int metadeComprimento = SETA_COMPRIMENTO / 2;
  int comprimentoPonta = 10;
  int larguraBase = 8;

  float radianos = angulo * M_PI / 180.0;
  float cos_theta = cos(radianos);
  float sin_theta = sin(radianos);

  int inicioX = centroX - metadeComprimento * cos_theta;
  int inicioY = centroY + metadeComprimento * sin_theta;
  int baseX = centroX + (metadeComprimento - comprimentoPonta) * cos_theta;
  int baseY = centroY - (metadeComprimento - comprimentoPonta) * sin_theta;

  int verticeX = centroX + metadeComprimento * cos_theta;
  int verticeY = centroY - metadeComprimento * sin_theta;

  float perpX = -sin_theta;
  float perpy = -cos_theta;

  int base1X = baseX + larguraBase * perpX;
  int base1Y = baseY + larguraBase * perpy;
  int base2X = baseX - larguraBase * perpX;
  int base2Y = baseY - larguraBase * perpy;

  desenharLinhaEspessa(inicioX, inicioY, baseX, baseY);
  display.fillTriangle(verticeX, verticeY, base1X, base1Y, base2X, base2Y, WHITE);
}

void SetaDisplay::desenharSetaFrente() {
  display.drawCircle(centroX, centroY, 12, WHITE);
  display.drawCircle(centroX, centroY, 11, WHITE);
  display.fillCircle(centroX, centroY, 3, WHITE);
}

void SetaDisplay::setStatus(int status) {
  // Limpa apenas a área do símbolo (canto superior direito)
  display.fillRect(SCREEN_WIDTH - 16, 0, 16, YELLOW_AREA_HEIGHT, BLACK);
  
  // Desenha o símbolo correspondente
  desenharSimboloStatus(status);
  display.display();
}

void SetaDisplay::desenharSimboloStatus(int status) {
  switch(status) {
    case 1: // Desconectado (X)
      display.drawLine(SCREEN_WIDTH - 14, 2, SCREEN_WIDTH - 2, 14, WHITE);
      display.drawLine(SCREEN_WIDTH - 14, 14, SCREEN_WIDTH - 2, 2, WHITE);
      break;
      
    case 2: // Conectado Firebase (✓)
      display.drawLine(SCREEN_WIDTH - 14, 8, SCREEN_WIDTH - 10, 12, WHITE);
      display.drawLine(SCREEN_WIDTH - 10, 12, SCREEN_WIDTH - 2, 4, WHITE);
      break;
      
    case 3: // App conectado (✓✓)
      display.drawLine(SCREEN_WIDTH - 14, 8, SCREEN_WIDTH - 10, 12, WHITE);
      display.drawLine(SCREEN_WIDTH - 10, 12, SCREEN_WIDTH - 2, 4, WHITE);
      display.drawLine(SCREEN_WIDTH - 14, 12, SCREEN_WIDTH - 10, 16, WHITE);
      display.drawLine(SCREEN_WIDTH - 10, 16, SCREEN_WIDTH - 2, 8, WHITE);
      break;
  }
}

void SetaDisplay::showtimeCompact(String timestamp) {
  // Atualiza apenas a área de texto (deixando espaço para o símbolo)
  display.fillRect(0, 0, SCREEN_WIDTH - 18, YELLOW_AREA_HEIGHT, BLACK);
  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  // Centraliza o texto na área disponível
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(timestamp, 0, 0, &x1, &y1, &w, &h);
  
  int x = (SCREEN_WIDTH - 18 - w) / 2;
  int y = (YELLOW_AREA_HEIGHT - h) / 2;
  
  display.setCursor(x, y);
  display.println(timestamp);
  display.display();
}