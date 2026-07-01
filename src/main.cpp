#include <Preferences.h>
#include <FastLED.h>
#include <Ps3Controller.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Preferences preferences;
const char *PREFS_NAMESPACE = "tetris";

// LEDs WS2812B
#define GRID_W (16)
#define GRID_H (16)
#define STRAND_LENGTH (GRID_W*GRID_H)
#define LED_DATA_PIN (23)
#define BACKWARDS (2)
#define PIECE_W (4)
#define PIECE_H (4)
#define NUM_PIECE_TYPES (7)

#define DROP_MINIMUM (25)
#define DROP_ACCELERATION (20)
#define INITIAL_MOVE_DELAY (100)
#define INITIAL_DROP_DELAY (500)
#define INITIAL_DRAW_DELAY (30)

// Estados del Juego
enum GameState { MENU, PLAYING, PAUSED, GAME_OVER };
GameState currentState = MENU;

int score = 0;
int top_score = 0; // Variable para almacenar el récord máximo
int level = 1;

long last_move;
long move_delay;
long last_drop;
long drop_delay;
long last_draw;
long draw_delay;
long last_fast_drop; 

CRGB leds[STRAND_LENGTH];
long grid[GRID_W*GRID_H];

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 21
#define OLED_SCL 22
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Variables de flanco para botones
bool lastCrossState = false; 
bool lastSelectState = false;
bool lastStartState = false;
bool lastCircleState = false;
bool lastR3State = false;

// --- Definición de las Piezas de Tetris (7 tipos) ---
// Cada pieza tiene 4 rotaciones, cada una es una matriz de 4x4
const char pieces[NUM_PIECE_TYPES][PIECE_H * PIECE_W * 4] = {
  // I - Cyan
  {
    0,0,0,0, 1,1,1,1, 0,0,0,0, 0,0,0,0,
    0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0,
    0,0,0,0, 1,1,1,1, 0,0,0,0, 0,0,0,0,
    0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0
  },
  // O - Yellow
  {
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    0,1,1,0, 0,1,1,0, 0,1,1,0, 0,1,1,0,
    0,1,1,0, 0,1,1,0, 0,1,1,0, 0,1,1,0,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
  },
  // T - Purple
  {
    0,0,0,0, 0,1,0,0, 0,0,0,0, 0,0,0,0,
    0,1,1,1, 0,1,1,0, 1,1,1,0, 0,1,0,0,
    0,0,0,0, 0,1,0,0, 0,0,0,0, 0,1,1,0,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
  },
  // S - Green
  {
    0,0,0,0, 0,1,0,0, 0,0,0,0, 0,0,0,0,
    0,0,1,1, 0,1,1,0, 1,1,0,0, 0,1,0,0,
    0,1,1,0, 0,0,1,0, 0,0,1,1, 0,1,1,0,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
  },
  // Z - Red
  {
    0,0,0,0, 0,0,1,0, 0,0,0,0, 0,0,0,0,
    0,1,1,0, 0,1,1,0, 0,1,1,0, 0,1,0,0,
    0,0,1,1, 0,1,0,0, 1,1,0,0, 0,1,1,0,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
  },
  // J - Blue
  {
    0,1,0,0, 0,0,0,0, 0,0,0,0, 0,0,1,1,
    0,1,1,1, 0,1,1,0, 1,1,1,0, 0,1,0,0,
    0,0,0,0, 0,1,0,0, 0,0,1,0, 0,1,0,0,
    0,0,0,0, 0,1,0,0, 0,0,0,0, 0,0,0,0
  },
  // L - Orange
  {
    0,0,1,0, 0,0,0,0, 0,0,0,0, 0,1,0,0,
    0,1,1,1, 0,1,1,0, 1,1,1,0, 0,1,0,0,
    0,0,0,0, 0,1,0,0, 1,0,0,0, 0,1,1,0,
    0,0,0,0, 0,1,0,0, 0,0,0,0, 0,0,0,0
  }
};

// Colores RGB para cada pieza (7 tipos)
const long piece_colors[NUM_PIECE_TYPES] = {
  0x00FFFF,  // I - Cyan
  0xFFFF00,  // O - Yellow
  0xFF00FF,  // T - Purple (Magenta)
  0x00FF00,  // S - Green
  0xFF0000,  // Z - Red
  0x0000FF,  // J - Blue
  0xFF8800   // L - Orange
};

// --- Matrices de caracteres personalizados para la Matriz 16x16 ---
// Pantalla Pausa: C (Continuar/Start) y R (Reiniciar/Círculo) lado a lado
const uint16_t bitmap_PauseScreen[] = {
  0b0011110001111100,
  0b0111111001111110,
  0b0110000001100110,
  0b0110000001111100,
  0b0110000001111110,
  0b0110000001100110,
  0b0111111001100110,
  0b0011110001100110,
  0b0000000000000000
};

// Pantalla Game Over: J (Start) y S (Salir/Círculo) lado a lado
const uint16_t bitmap_GameOverScreen[] = {
  0b0001100000111110,
  0b0001100001111110,
  0b0001100001100000,
  0b0001100001111100,
  0b0001100000111110,
  0b0101100000001110,
  0b0101100000001110,
  0b0111100001111110,
  0b0011000001111100,
  0b0000000000000000
};

// --- Marcador en OLED ---
void updateOledDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (currentState == MENU) {
    display.setTextSize(2);
    display.setCursor(25, 5);
    display.print("TETRIS");
    display.setTextSize(1);
    display.setCursor(15, 32);
    display.print("Control Conectado");
    display.setCursor(18, 50);
    display.print("Presiona START");
  } 
  else if (currentState == PAUSED) {
    display.setTextSize(2);
    display.setCursor(30, 5);
    display.print("PAUSA");
    display.setTextSize(1);
    display.setCursor(5, 35);
    display.print("START -> Continuar (C)");
    display.setCursor(5, 50);
    display.print("CIRCULO -> Reiniciar(R)");
  } 
  else if (currentState == GAME_OVER) {
    display.setTextSize(2);
    display.setCursor(10, 5);
    display.print("GAME OVER");
    display.setTextSize(1);
    display.setCursor(5, 35);
    display.print("START -> Jugar de nuevo");
    display.setCursor(5, 50);
    display.print("CIRCULO -> Salir");
  } 
  else if (currentState == PLAYING) {
    // Diseño del HUD clásico con récord máximo en pantalla
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("MAX SCORE:");
    display.setCursor(70, 0);
    display.print(top_score);

    display.drawLine(0, 12, 128, 12, SSD1306_WHITE);

    display.setTextSize(2);
    display.setCursor(0, 20);
    display.print("Score:");
    display.setCursor(0, 42);
    display.print(score);

    display.setTextSize(1);
    display.setCursor(85, 45);
    display.print("Niv: ");
    display.print(level);
  }
  display.display();
}

void updateDifficulty() {
  level = (score / 50) + 1; 
  drop_delay = INITIAL_DROP_DELAY - (level - 1) * 50; 
  if (drop_delay < DROP_MINIMUM) drop_delay = DROP_MINIMUM;
}

// --- Funciones de dibujo e iluminación de la Matriz ---
#define COLOR_ORDER GRB

void p(int x, int y, long color) {
  if (x < 0 || x >= GRID_W || y < 0 || y >= GRID_H) return;
  int a = (GRID_H - 1 - y) * GRID_W;
  if ((y % 2) == BACKWARDS) a += x;
  else a += GRID_W - 1 - x;
  a %= STRAND_LENGTH;
  
  if (color == 0) leds[a] = CRGB::Black;
  else leds[a] = CRGB((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
}

void draw_grid() {
  for (int y = 0; y < GRID_H; ++y) {
    for (int x = 0; x < GRID_W; ++x) {
      p(x, y, grid[y * GRID_W + x]);
    }
  }
  FastLED.show();
}

void drawMenuScreen(long ms) {
  FastLED.clear();
  for (int y = 0; y < GRID_H; y++) {
    for (int x = 0; x < GRID_W; x++) {
      uint8_t hue = (x * 12) + (y * 12) + (ms / 15);
      leds[((GRID_H - 1 - y) * GRID_W) + ((y % 2 == BACKWARDS) ? x : (GRID_W - 1 - x))] = CHSV(hue, 230, 80);
    }
  }
  FastLED.show();
}

void drawBitmap(const uint16_t* bitmap, int h, long color, int offsetX, int offsetY) {
  for (int y = 0; y < h; y++) {
    uint16_t row = bitmap[y];
    for (int x = 0; x < 16; x++) {
      if ((row >> (15 - x)) & 0x01) {
        p(x + offsetX, y + offsetY, color);
      }
    }
  }
}

// --- Lógica de piezas ---
int piece_id, piece_rotation, piece_x, piece_y;
char piece_sequence[NUM_PIECE_TYPES];
char sequence_i = NUM_PIECE_TYPES;

void choose_new_piece() {
  if (sequence_i >= NUM_PIECE_TYPES) {
    int i, j, k;
    for (i = 0; i < NUM_PIECE_TYPES; ++i) {
      do {
        j = rand() % NUM_PIECE_TYPES;
        for (k = 0; k < i;++k) {
          if (piece_sequence[k] == j) break;
        }
      } while (k < i);
      piece_sequence[i] = j;
    }
    sequence_i = 0;
  }
  piece_id = piece_sequence[sequence_i++];
  piece_y = 0;
  piece_x = GRID_W / 2 - 2;
  piece_rotation = 0;
}

void erase_piece_from_grid() {
  const char *piece = pieces[piece_id] + (piece_rotation * PIECE_H * PIECE_W);
  for (int y = 0; y < PIECE_H; ++y) {
    for (int x = 0; x < PIECE_W; ++x) {
      int nx = piece_x + x, ny = piece_y + y;
      if (ny >= 0 && ny < GRID_H && nx >= 0 && nx < GRID_W && piece[y * PIECE_W + x] == 1) {
        grid[ny * GRID_W + nx] = 0;
      }
    }
  }
}

void add_piece_to_grid() {
  const char *piece = pieces[piece_id] + (piece_rotation * PIECE_H * PIECE_W);
  for (int y = 0; y < PIECE_H; ++y) {
    for (int x = 0; x < PIECE_W; ++x) {
      int nx = piece_x + x, ny = piece_y + y;
      if (ny >= 0 && ny < GRID_H && nx >= 0 && nx < GRID_W && piece[y * PIECE_W + x] == 1) {
        grid[ny * GRID_W + nx] = piece_colors[piece_id];
      }
    }
  }
}

bool piece_can_fit(int px, int py, int pr) {
  const char *piece = pieces[piece_id] + (pr * PIECE_H * PIECE_W);
  for (int y = 0; y < PIECE_H; ++y) {
    for (int x = 0; x < PIECE_W; ++x) {
      if (piece[y * PIECE_W + x] == 1) {
        int nx = px + x, ny = py + y;
        if (nx < 0 || nx >= GRID_W || ny >= GRID_H) return false;
        if (ny < 0) continue; 
        if (grid[ny * GRID_W + nx] != 0) return false;
      }
    }
  }
  return true;
}

// Comprobar y guardar nuevo récord máximo de forma persistente
void checkTopScore() {
  if (score > top_score) {
    top_score = score;
    preferences.begin(PREFS_NAMESPACE, false);
    preferences.putInt("top_score", top_score);
    preferences.end();
  }
}

void delete_possible_lines() {
  int lines_cleared = 0;
  for (int y = GRID_H - 1; y >= 0; y--) {
    bool line_complete = true;
    for (int x = 0; x < GRID_W; x++) {
      if (grid[y * GRID_W + x] == 0) { line_complete = false; break; }
    }
    if (line_complete) {
      lines_cleared++;
      for (int move_y = y; move_y > 0; move_y--) {
        for (int x = 0; x < GRID_W; x++) grid[move_y * GRID_W + x] = grid[(move_y - 1) * GRID_W + x];
      }
      for (int x = 0; x < GRID_W; x++) grid[x] = 0; 
      y++; 
    }
  }
  if (lines_cleared > 0) {
    score += lines_cleared * 10;
    checkTopScore();
    updateDifficulty();
    updateOledDisplay();
  }
}

void resetGameVariables() {
  for (int i = 0; i < GRID_W * GRID_H; ++i) grid[i] = 0;
  FastLED.clear(true);
  score = 0;
  level = 1;
  updateDifficulty();
  choose_new_piece();
}

void try_to_drop_piece() {
  erase_piece_from_grid();
  if (piece_can_fit(piece_x, piece_y + 1, piece_rotation)) {
    piece_y++;
    add_piece_to_grid();
  } else {
    add_piece_to_grid(); 
    delete_possible_lines();
    choose_new_piece();
    if (!piece_can_fit(piece_x, piece_y, piece_rotation)) {
      currentState = GAME_OVER;
      checkTopScore();
      FastLED.clear(true);
      updateOledDisplay();
    } else {
      add_piece_to_grid(); 
    }
  }
}

// --- Setup ---
void setup() {
  Serial.begin(115200);

  FastLED.addLeds<WS2812, LED_DATA_PIN, COLOR_ORDER>(leds, STRAND_LENGTH);
  FastLED.clear(true);
  FastLED.setBrightness(64);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;);
  }

  // Cargar Récord Máximo desde la memoria flash del ESP32
  preferences.begin(PREFS_NAMESPACE, true);
  top_score = preferences.getInt("top_score", 0);
  preferences.end();

  updateOledDisplay();

  if (Ps3.begin("00:00:00:00:00:00")) { 
    Serial.println("Bluetooth Listo");
  }

  move_delay = INITIAL_MOVE_DELAY;
  drop_delay = INITIAL_DROP_DELAY;
  draw_delay = INITIAL_DRAW_DELAY;
  last_draw = last_drop = last_move = last_fast_drop = millis();

  randomSeed(ESP.getCycleCount());
}

// --- Loop principal ---
void loop() {
  if (!Ps3.isConnected()) return;

  long t = millis();

  // Captura de flancos para los botones del mando
  bool currentStart = Ps3.data.button.start;
  bool startPressed = currentStart && !lastStartState;
  lastStartState = currentStart;

  bool currentSelect = Ps3.data.button.select;
  bool selectPressed = currentSelect && !lastSelectState;
  lastSelectState = currentSelect;

  bool currentCircle = Ps3.data.button.circle;
  bool circlePressed = currentCircle && !lastCircleState;
  lastCircleState = currentCircle;

  // Botón de emergencia R3 (Saltar directamente al menú inicio estés donde estés)
  bool currentR3 = Ps3.data.button.r3;
  if (currentR3 && !lastR3State) {
    checkTopScore();
    currentState = MENU;
    resetGameVariables();
    updateOledDisplay();
  }
  lastR3State = currentR3;

  // ---- MÁQUINA DE ESTADOS COMPLETA ----
  switch (currentState) {
    
    case MENU: {
      drawMenuScreen(t); 
      if (startPressed) {
        resetGameVariables();
        currentState = PLAYING;
        add_piece_to_grid();
        updateOledDisplay();
      }
      break;
    }

    case PLAYING: {
      // Entrar a menú pausa con SELECT (START no hace nada en este modo)
      if (selectPressed) {
        currentState = PAUSED;
        updateOledDisplay();
        break;
      }

      // Movimiento lateral (Dpad Izquierda / Derecha)
      if (t - last_move > move_delay) {
        if (Ps3.data.button.left) {
          last_move = t;
          erase_piece_from_grid();
          if (piece_can_fit(piece_x - 1, piece_y, piece_rotation)) piece_x--;
          add_piece_to_grid();
        } else if (Ps3.data.button.right) {
          last_move = t;
          erase_piece_from_grid();
          if (piece_can_fit(piece_x + 1, piece_y, piece_rotation)) piece_x++;
          add_piece_to_grid();
        }
      }

      // Rotación con la X (Cruz)
      bool currentCross = Ps3.data.button.cross;
      if (currentCross && !lastCrossState) {
        erase_piece_from_grid();
        int new_pr = (piece_rotation + 1) % 4;
        if (piece_can_fit(piece_x, piece_y, new_pr)) piece_rotation = new_pr;
        add_piece_to_grid();
      }
      lastCrossState = currentCross; 

      // Caída rápida con R2
      if (Ps3.data.button.r2) {
        if (t - last_fast_drop > 40) { last_fast_drop = t; try_to_drop_piece(); }
      }

      // Caída automática regular
      if (t - last_drop > drop_delay) { last_drop = t; try_to_drop_piece(); }

      // Dibujado del Tablero
      if (t - last_draw > draw_delay) { last_draw = t; draw_grid(); }
      break;
    }

    case PAUSED: {
      // Muestra la C (Verde - Izquierda) y la R (Roja - Derecha) alineadas en los LEDs
      FastLED.clear();
      drawBitmap(bitmap_PauseScreen, 8, 0x00FF00, 0, 4); // C en Verde 
      
      // Dibujamos la R superpuesta en color Rojo a la derecha
      for(int y=0; y<16; y++) {
        for(int x=8; x<16; x++) {
          if((bitmap_PauseScreen[y] >> (15 - x)) & 0x01) p(x, y, 0xFF0000);
        }
      }
      FastLED.show();

      if (startPressed) { // CONTINUAR (C) -> Regresa a la partida actual
        currentState = PLAYING;
        updateOledDisplay();
        last_drop = millis(); 
      } 
      else if (circlePressed) { // REINICIAR (R) -> Borra el tablero actual y empieza otra partida de inmediato
        resetGameVariables();
        currentState = PLAYING;
        add_piece_to_grid();
        updateOledDisplay();
      }
      break;
    }

    case GAME_OVER: {
      // Muestra la J (Verde) y la S (Roja)
      FastLED.clear();
      drawBitmap(bitmap_GameOverScreen, 9, 0x00FF00, 0, 4); 
      for(int y=0; y<16; y++) {
        for(int x=8; x<16; x++) {
          if((bitmap_GameOverScreen[y] >> (15 - x)) & 0x01) p(x, y, 0xFF0000);
        }
      }
      FastLED.show();

      if (startPressed) { // Jugar de nuevo
        resetGameVariables();
        currentState = PLAYING;
        add_piece_to_grid();
        updateOledDisplay();
      } 
      else if (circlePressed) { // Salir al menú principal
        currentState = MENU;
        updateOledDisplay();
      }
      break;
    }
  }
}