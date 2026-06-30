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

int button_pause = 7;
int button_start = 12;
int joystick_x = 36;
int joystick_y = 39;

int score = 0;
int top_score = 0;
int level = 1;

long last_move;
long move_delay;
long last_drop;
long drop_delay;
long last_draw;
long draw_delay;

CRGB leds[STRAND_LENGTH];
long grid[GRID_W*GRID_H];

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- Pantalla inicial ---
void showStartScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.print("TETRIS");
  display.setTextSize(1);
  display.setCursor(10, 40);
  display.print("Presiona START");
  display.display();
}

// --- Marcador en OLED ---
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Score:");
  display.setCursor(0, 20);
  display.print(score);

  display.setCursor(0, 45);
  display.print("Nivel ");
  display.print(level);

  display.display();
}

// Ajustar dificultad según score
void updateDifficulty() {
  level = (score / 50) + 1; // cada 50 puntos sube un nivel
  drop_delay = INITIAL_DROP_DELAY - (level - 1) * 50; // cada nivel acelera caída
  if (drop_delay < DROP_MINIMUM) drop_delay = DROP_MINIMUM;
}

// --- Definiciones de piezas (4 rotaciones de 4x4 por pieza) ---
const char piece_I[] = {
  0,0,0,0, 1,1,1,1, 0,0,0,0, 0,0,0,0,
  0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0,
  0,0,0,0, 0,0,0,0, 1,1,1,1, 0,0,0,0,
  0,1,0,0, 0,1,0,0, 0,1,0,0, 0,1,0,0
};

const char piece_L[] = {
  0,0,1,0, 1,1,1,0, 0,0,0,0, 0,0,0,0,
  0,1,0,0, 0,1,0,0, 0,1,1,0, 0,0,0,0,
  0,0,0,0, 1,1,1,0, 1,0,0,0, 0,0,0,0,
  1,1,0,0, 0,1,0,0, 0,1,0,0, 0,0,0,0
};

const char piece_J[] = {
  1,0,0,0, 1,1,1,0, 0,0,0,0, 0,0,0,0,
  0,1,1,0, 0,1,0,0, 0,1,0,0, 0,0,0,0,
  0,0,0,0, 1,1,1,0, 0,0,1,0, 0,0,0,0,
  0,1,0,0, 0,1,0,0, 1,1,0,0, 0,0,0,0
};

const char piece_O[] = {
  0,1,1,0, 0,1,1,0, 0,0,0,0, 0,0,0,0,
  0,1,1,0, 0,1,1,0, 0,0,0,0, 0,0,0,0,
  0,1,1,0, 0,1,1,0, 0,0,0,0, 0,0,0,0,
  0,1,1,0, 0,1,1,0, 0,0,0,0, 0,0,0,0
};

const char piece_S[] = {
  0,1,1,0, 1,1,0,0, 0,0,0,0, 0,0,0,0,
  0,1,0,0, 0,1,1,0, 0,0,1,0, 0,0,0,0,
  0,0,0,0, 0,1,1,0, 1,1,0,0, 0,0,0,0,
  1,0,0,0, 1,1,0,0, 0,1,0,0, 0,0,0,0
};

const char piece_T[] = {
  0,1,0,0, 1,1,1,0, 0,0,0,0, 0,0,0,0,
  0,1,0,0, 0,1,1,0, 0,1,0,0, 0,0,0,0,
  0,0,0,0, 1,1,1,0, 0,1,0,0, 0,0,0,0,
  0,1,0,0, 1,1,0,0, 0,1,0,0, 0,0,0,0
};

const char piece_Z[] = {
  1,1,0,0, 0,1,1,0, 0,0,0,0, 0,0,0,0,
  0,0,1,0, 0,1,1,0, 0,1,0,0, 0,0,0,0,
  0,0,0,0, 1,1,0,0, 0,1,1,0, 0,0,0,0,
  0,1,0,0, 1,1,0,0, 1,0,0,0, 0,0,0,0
};

const char *pieces[NUM_PIECE_TYPES] = {
  piece_S, piece_Z, piece_L, piece_J, piece_O, piece_T, piece_I
};

// Colores para cada figura
const long piece_colors[NUM_PIECE_TYPES] = {
  0x009900, // verde S
  0xFF0000, // rojo Z
  0xFF8000, // naranja L
  0x000044, // azul J
  0xFFFF00, // amarillo O
  0xFF00FF, // morado T
  0x00FFFF  // turquesa I
};

// --- Funciones de dibujo ---
#define COLOR_ORDER GRB

void p(int x, int y, long color) {
  if (x < 0 || x >= GRID_W || y < 0 || y >= GRID_H) return;
  int a = (GRID_H - 1 - y) * GRID_W;
  if ((y % 2) == BACKWARDS) {
    a += x;
  } else {
    a += GRID_W - 1 - x;
  }
  a %= STRAND_LENGTH;
  if (color == 0) {
    leds[a] = CRGB::Black;
  } else {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    leds[a] = CRGB(r, g, b);
  }
}

void draw_grid() {
  int x, y;
  for (y = 0; y < GRID_H; ++y) {
    for (x = 0; x < GRID_W; ++x) {
      if (grid[y * GRID_W + x] != 0) {
        p(x, y, grid[y * GRID_W + x]);
      } else {
        p(x, y, 0);
      }
    }
  }
  FastLED.show();
}

// --- Lógica de piezas ---
int piece_id;
int piece_rotation;
int piece_x;
int piece_y;

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
  int x, y;
  const char *piece = pieces[piece_id] + (piece_rotation * PIECE_H * PIECE_W);
  for (y = 0; y < PIECE_H; ++y) {
    for (x = 0; x < PIECE_W; ++x) {
      int nx = piece_x + x;
      int ny = piece_y + y;
      if (ny < 0 || ny >= GRID_H) continue;
      if (nx < 0 || nx >= GRID_W) continue;
      if (piece[y * PIECE_W + x] == 1) {
        grid[ny * GRID_W + nx] = 0;
      }
    }
  }
}

void add_piece_to_grid() {
  int x, y;
  const char *piece = pieces[piece_id] + (piece_rotation * PIECE_H * PIECE_W);
  for (y = 0; y < PIECE_H; ++y) {
    for (x = 0; x < PIECE_W; ++x) {
      int nx = piece_x + x;
      int ny = piece_y + y;
      if (ny < 0 || ny >= GRID_H) continue;
      if (nx < 0 || nx >= GRID_W) continue;
      if (piece[y * PIECE_W + x] == 1) {
        grid[ny * GRID_W + nx] = piece_colors[piece_id];
      }
    }
  }
}

// Función que valida si una pieza cabe en una posición determinada
bool piece_can_fit(int px, int py, int pr) {
  int x, y;
  const char *piece = pieces[piece_id] + (pr * PIECE_H * PIECE_W);
  for (y = 0; y < PIECE_H; ++y) {
    for (x = 0; x < PIECE_W; ++x) {
      if (piece[y * PIECE_W + x] == 1) {
        int nx = px + x;
        int ny = py + y;
        if (nx < 0 || nx >= GRID_W || ny >= GRID_H) return false;
        if (ny < 0) continue; // Permite que aparezcan desde arriba
        if (grid[ny * GRID_W + nx] != 0) return false;
      }
    }
  }
  return true;
}

// Función para eliminar las líneas completas
void delete_possible_lines() {
  int lines_cleared = 0;
  for (int y = GRID_H - 1; y >= 0; y--) {
    bool line_complete = true;
    for (int x = 0; x < GRID_W; x++) {
      if (grid[y * GRID_W + x] == 0) {
        line_complete = false;
        break;
      }
    }
    if (line_complete) {
      lines_cleared++;
      for (int move_y = y; move_y > 0; move_y--) {
        for (int x = 0; x < GRID_W; x++) {
          grid[move_y * GRID_W + x] = grid[(move_y - 1) * GRID_W + x];
        }
      }
      for (int x = 0; x < GRID_W; x++) {
        grid[0 + x] = 0;
      }
      y++; // Volver a revisar la misma fila ya que todo bajó un nivel
    }
  }
  if (lines_cleared > 0) {
    score += lines_cleared * 10;
    updateDifficulty();
    updateDisplay();
  }
}

// Función para intentar bajar la pieza
void try_to_drop_piece() {
  erase_piece_from_grid();
  if (piece_can_fit(piece_x, piece_y + 1, piece_rotation)) {
    piece_y++;
    add_piece_to_grid();
  } else {
    add_piece_to_grid(); // Fijarla donde estaba
    delete_possible_lines();
    choose_new_piece();
    if (!piece_can_fit(piece_x, piece_y, piece_rotation)) {
      // Game over: Limpiar tablero
      for (int i = 0; i < GRID_W * GRID_H; ++i) {
        grid[i] = 0;
      }
      score = 0;
      level = 1;
      updateDifficulty();
      updateDisplay();
    }
  }
}

// --- Funciones de control con PS3 ---
bool isPs3LeftPressed() { return Ps3.isConnected() && Ps3.data.button.left; }
bool isPs3RightPressed() { return Ps3.isConnected() && Ps3.data.button.right; }
bool isPs3UpPressed() { return Ps3.isConnected() && Ps3.data.button.up; }
bool isPs3DownPressed() { return Ps3.isConnected() && Ps3.data.button.down; }
bool isPs3StartPressed() { return Ps3.isConnected() && Ps3.data.button.start; }
bool isPs3SelectPressed() { return Ps3.isConnected() && Ps3.data.button.select; }

// --- Movimiento lateral usando D-Pad ---
int readAxisX() {
  if (Ps3.isConnected()) {
    if (isPs3LeftPressed()) return 0;
    if (isPs3RightPressed()) return 1023;
    return 512;
  }
  return analogRead(joystick_x);
}

// --- Movimiento vertical usando D-Pad ---
int readAxisY() {
  if (Ps3.isConnected()) {
    if (isPs3UpPressed()) return 1023;
    if (isPs3DownPressed()) return 0;
    return 512;
  }
  return analogRead(joystick_y);
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  Serial.println("Tetris ESP32 con PS3 y OLED");

  pinMode(button_pause, INPUT_PULLUP);
  pinMode(button_start, INPUT_PULLUP);

  FastLED.addLeds<WS2812, LED_DATA_PIN, COLOR_ORDER>(leds, STRAND_LENGTH);
  FastLED.clear(true);
  FastLED.setBrightness(64);

  // Inicializar OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Error inicializando OLED");
    for (;;);
  }
  display.clearDisplay();
  display.display();
  showStartScreen();

  // Inicializar mando PS3 (Agrega tu MAC entre las comillas si la conoces)
  if (Ps3.begin("00:00:00:00:00:00")) { 
    Serial.println("PS3 iniciado");
  } else {
    Serial.println("Error iniciando PS3");
  }

  // Inicializar variables
  move_delay = INITIAL_MOVE_DELAY;
  drop_delay = INITIAL_DROP_DELAY;
  draw_delay = INITIAL_DRAW_DELAY;

  last_draw = last_drop = last_move = millis();

  // Inicializar tablero
  for (int i = 0; i < GRID_W * GRID_H; ++i) {
    grid[i] = 0;
  }

  randomSeed(analogRead(joystick_y) + analogRead(2) + analogRead(3));
  choose_new_piece();
  updateDisplay();
}

// --- Loop principal ---
void loop() {
  long t = millis();

  // Movimiento lateral
  if (t - last_move > move_delay) {
    int dx = readAxisX();
    if (dx < 200) {
      last_move = t;
      Serial.println("Mover izquierda");
      erase_piece_from_grid();
      if (piece_can_fit(piece_x - 1, piece_y, piece_rotation)) piece_x--;
      add_piece_to_grid();
    } else if (dx > 800) {
      last_move = t;
      Serial.println("Mover derecha");
      erase_piece_from_grid();
      if (piece_can_fit(piece_x + 1, piece_y, piece_rotation)) piece_x++;
      add_piece_to_grid();
    }
  }

  // Rotación
  if (isPs3UpPressed()) {
    Serial.println("Rotar pieza");
    erase_piece_from_grid();
    int new_pr = (piece_rotation + 1) % 4;
    if (piece_can_fit(piece_x, piece_y, new_pr)) {
      piece_rotation = new_pr;
    }
    add_piece_to_grid();
    delay(200); // evitar múltiples rotaciones rápidas
  }

  // Caída rápida
  if (isPs3DownPressed()) {
    if (t - last_move > 50) { // Pequeño delay para controlar la velocidad de caída rápida
      last_move = t;
      Serial.println("Caída rápida");
      try_to_drop_piece();
    }
  }

  // Caída automática
  if (t - last_drop > drop_delay) {
    last_drop = t;
    try_to_drop_piece();
  }

  // Redibujar LEDs
  if (t - last_draw > draw_delay) {
    last_draw = t;
    draw_grid();
  }
}
