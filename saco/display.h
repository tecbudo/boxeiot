/**
 * @file display.h
 * @brief Definição da classe SetaDisplay para controle de display OLED
 * 
 * Esta classe fornece uma interface para controlar um display OLED SSD1306,
 * com funcionalidades para desenhar setas, exibir logs e mostrar mensagens.
 * 
 * @section features Funcionalidades
 * - Desenho de setas em 8 direções diferentes
 * - Sistema de log com rotação de mensagens
 * - Exibição de mensagens centralizadas em fonte grande
 * - Controle completo do display OLED
 * 
 * @section dependencies Dependências
 * - Wire.h
 * - Adafruit_GFX.h
 * - Adafruit_SSD1306.h
 * - vector (para armazenamento de logs)
 * 
 * @section usage Uso
 * 1. Inclua o arquivo display.h em seu projeto
 * 2. Crie uma instância de SetaDisplay
 * 3. Chame o método begin() para inicializar o display
 * 4. Use os métodos disponíveis para controlar o display
 */
#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#include <vector>
#include <string>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define YELLOW_AREA_HEIGHT 16
#define BLUE_AREA_HEIGHT 48
#define MAX_LOG_LINES 6
#define SETA_COMPRIMENTO 47

class SetaDisplay {
  private:
    Adafruit_SSD1306 display;
    int centroX;
    int centroY;
    std::vector<String> logMessages;
    
    void desenharSetaReta(int modo);
    void desenharSetaAngular(int angulo);
    void desenharSetaFrente();
    void desenharLinhaEspessa(int x1, int y1, int x2, int y2);
    void atualizarAreaAmarela();
    void atualizarAreaAzul();
    void desenharSimboloStatus(int status);
  public:
    SetaDisplay();
    void begin();
    void clear();
    void update();
    
    // Funções principais
    void seta(int angulo);
    void seta(const char* tipo);
    void printlog(String mensagem);
    void print(String mensagem);
    void printazul(String mensagem);
    void showtime(String timestamp);
    void setStatus(int status); // 1: desconectado, 2: conectado Firebase, 3: app conectado
  void showtimeCompact(String timestamp); // Nova função para tempo compacto
};

#endif  