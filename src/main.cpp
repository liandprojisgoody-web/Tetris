#include <Preferences.h>

// LEDs WS2812B (NeoPixel) — usar FastLED para ESP32
#include <FastLED.h>
#ifdef USE_PS3
#include <Ps3Controller.h>
#endif

Preferences preferences;
const char *PREFS_NAMESPACE = "tetris";

      
// Tamaño del campo de juego
#define GRID_W              (16)
#define GRID_H              (16)     // El campo de juego es de 16x16 LEDs
#define STRAND_LENGTH       (GRID_W*GRID_H)
#define LED_DATA_PIN        (23)     // Pin digital al que va conectado (ESP32)
#define BACKWARDS           (2)    // Conexión tipo Z 
#define PIECE_W             (4)   // Cada figura del tetris ocupará 4 LEDs
#define PIECE_H             (4)
#define NUM_PIECE_TYPES     (7)  // Cantidad de figuras del Tetris
// Parámetros del Joystick
#define JOYSTICK_DEAD_ZONE  (30)
#define JOYSTICK_PIN        (2)   
// Parámetros para la caída de figuras
#define DROP_MINIMUM        (25)  // velocidad máxima que la gravedad puede alcanzar
#define DROP_ACCELERATION   (20)  // qué tan rápido aumenta la gravedad
#define INITIAL_MOVE_DELAY  (100) // retraso inicial
#define INITIAL_DROP_DELAY  (500)
#define INITIAL_DRAW_DELAY  (30)

int button_pause = 7;     // Pin digital al que va conectado
int button_start = 12;   // Pin digital al que va conectado
// buzzer removed per user request
int joystick_x = 36;   // Pin analógico al que va conectado (ESP32 VP)
int joystick_y = 39;  // Pin analógico al que va conectado (ESP32 VN)

//Variables
byte adr = 0x08;
byte num = 0x00;
int i = 0;
int top_score = 0;     // Valor inicial del puntaje máximo
int score = 0;        // Valor inicial del puntaje
int top_score_1 = 0;
int top_score_2 = 0;
int top_score_3 = 0;
int top_score_4 = 0;
int score_1 = 0;
int score_2 = 0;
int score_3 = 0;
int score_4 = 0;
bool Pause = false;
bool pause_onece = false;
bool pause_pressed = false;
unsigned long previousMillis = 0; 
unsigned long currentMillis = 0;


// Colores por figura
// Cada pieza tiene un máximo de 4 de ancho, 4 de alto y 4 rotaciones
const char piece_I[] = {
  0,0,0,0,
  1,1,1,1,
  0,0,0,0,
  0,0,0,0,

  0,0,1,0,
  0,0,1,0,
  0,0,1,0,
  0,0,1,0,
  
  0,0,0,0,
  0,0,0,0,
  1,1,1,1,
  0,0,0,0,

  0,1,0,0,
  0,1,0,0,
  0,1,0,0,
  0,1,0,0,
};

const char piece_L[] = {
  0,0,1,0,
  1,1,1,0,
  0,0,0,0,
  0,0,0,0,
  
  0,1,0,0,
  0,1,0,0,
  0,1,1,0,
  0,0,0,0,

  0,0,0,0,
  1,1,1,0,
  1,0,0,0,
  0,0,0,0,
  
  1,1,0,0,
  0,1,0,0,
  0,1,0,0,
  0,0,0,0,
};

const char piece_J[] = {
  1,0,0,0,
  1,1,1,0,
  0,0,0,0,
  0,0,0,0,
  
  0,1,1,0,
  0,1,0,0,
  0,1,0,0,
  0,0,0,0,

  0,0,0,0,
  1,1,1,0,
  0,0,1,0,
  0,0,0,0,
  
  0,1,0,0,
  0,1,0,0,
  1,1,0,0,
  0,0,0,0,

};

const char piece_T[] = {
  0,1,0,0,
  1,1,1,0,
  0,0,0,0,
  0,0,0,0,

  0,1,0,0,
  0,1,1,0,
  0,1,0,0,
  0,0,0,0,
  
  0,0,0,0,
  1,1,1,0,
  0,1,0,0,
  0,0,0,0,

  0,1,0,0,
  1,1,0,0,
  0,1,0,0,
  0,0,0,0,

};

const char piece_S[] = {
  0,1,1,0,
  1,1,0,0,
  0,0,0,0,
  0,0,0,0,

  0,1,0,0,
  0,1,1,0,
  0,0,1,0,
  0,0,0,0,

  0,0,0,0,
  0,1,1,0,
  1,1,0,0,
  0,0,0,0,

  1,0,0,0,
  1,1,0,0,
  0,1,0,0,
  0,0,0,0,
};

const char piece_Z[] = {
  1,1,0,0,
  0,1,1,0,
  0,0,0,0,
  0,0,0,0,
  
  0,0,1,0,
  0,1,1,0,
  0,1,0,0,
  0,0,0,0,

  0,0,0,0,
  1,1,0,0,
  0,1,1,0,
  0,0,0,0,
  
  0,1,0,0,
  1,1,0,0,
  1,0,0,0,
  0,0,0,0,
};

const char piece_O[] = {
  1,1,0,0,
  1,1,0,0,
  0,0,0,0,
  0,0,0,0,
  
  1,1,0,0,
  1,1,0,0,
  0,0,0,0,
  0,0,0,0,
  
  1,1,0,0,
  1,1,0,0,
  0,0,0,0,
  0,0,0,0,
  
  1,1,0,0,
  1,1,0,0,
  0,0,0,0,
  0,0,0,0,
};


// Figuras
const char *pieces[NUM_PIECE_TYPES] = {
  piece_S,
  piece_Z,
  piece_L,
  piece_J,
  piece_O,
  piece_T,
  piece_I,
};

// Color para cada figura
const long piece_colors[NUM_PIECE_TYPES] = {
  0x009900, // verde S
  0xFF0000, // rojo Z
  0xFF8000, // naranja L
  0x000044, // azul J
  0xFFFF00, // amarillo O
  0xFF00FF, // morado T
  0x00FFFF,  // turquesa I
};

// Se define la matriz con la que se esta trabajando y se le da un nombre
#define COLOR_ORDER GRB
CRGB leds[STRAND_LENGTH];


// Ayuda a saber el estado anterior del botón
int old_button=0;
// Arduino puede detectar cuando hay nuevos movimientos
int old_px = 0;
// arduino pueda saber cuándo el usuario intenta girar
int old_i_want_to_turn=0;

// Arduino puede recordar la anterior figura que cae
int piece_id;
int piece_rotation;
int piece_x;
int piece_y;

//Secuencia de figuras al azar
char piece_sequence[NUM_PIECE_TYPES];
char sequence_i=NUM_PIECE_TYPES;

// velocidad en la que se puede mover el jugador
long last_move;
long move_delay;  // 100ms = 5 times a second

// Controla la caída automática de las figuras
long last_drop;
long drop_delay;  // 500ms = 2 times a second

long last_draw;
long draw_delay;  // 60 fps

// Arduino mantiene un historial de donde se encuentran las figuras que ya cayeron
long grid[GRID_W*GRID_H];


//MÉTODOS

// I want to turn on point P(x,y), which is X from the left and Y from the top.
// I might also want to hold it on for us microseconds.
void p(int x,int y,long color) {
  int a = (GRID_H-1-y)*GRID_W;
  if( ( y % 2 ) == BACKWARDS ) {  // % is modulus.
    // y%2 is false when y is an even number - rows 0,2,4,6.
    a += x;
  } else {
    // y%2 is true when y is an odd number - rows 1,3,5,7.
    a += GRID_W - 1 - x;
  }
  a%=STRAND_LENGTH;
  if(color==0) {
    leds[a] = CRGB::Black;
  } else {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    leds[a] = CRGB(r,g,b);
  }
}


// la cuadrícula contiene la memoria de arduino del tablero de juego, incluida la pieza que está cayendo
void draw_grid() {
  int x, y;
  for(y=0;y<GRID_H;++y) {
    for(x=0;x<GRID_W;++x) {
      if( grid[y*GRID_W+x] != 0 ) {
        p(x,y,grid[y*GRID_W+x]);
      } else {
        p(x,y,0);
      }
    }
  }
  FastLED.show();
}

// elige una nueva pieza de la secuencia de orden aleatorio
void choose_new_piece() {
  if( sequence_i >= NUM_PIECE_TYPES ) {
    // list exhausted
    int i,j, k;
    for(i=0;i<NUM_PIECE_TYPES;++i) {
      do {
        // pick a random piece
        j = rand() % NUM_PIECE_TYPES;
        // make sure it isn't already in the sequence.
        for(k=0;k<i;++k) {
          if(piece_sequence[k]==j) break;  // already in sequence
        }
      } while(k<i);
      // not in sequence.  Add it.
      piece_sequence[i] = j;
    }
    // rewind sequence counter
    sequence_i=0;
  }
  
// Escoge la siguiente figura
  piece_id = piece_sequence[sequence_i++];
  // always start the piece top center.
  piece_y=-4;  // -4 squares off the top of the screen.
  piece_x=3;
// Siempre inicia en la misma orientación 
  piece_rotation=0;
}


void erase_piece_from_grid() {
  int x, y;
  
  const char *piece = pieces[piece_id] + (piece_rotation * PIECE_H * PIECE_W);
  
  for(y=0;y<PIECE_H;++y) {
    for(x=0;x<PIECE_W;++x) {
      int nx=piece_x+x;
      int ny=piece_y+y;
      if(ny<0 || ny>GRID_H) continue;
      if(nx<0 || nx>GRID_W) continue;
      if(piece[y*PIECE_W+x]==1) {
        grid[ny*GRID_W+nx]=0;  // zero erases the grid location.
      }
    }
  }
}


void add_piece_to_grid() {
  int x, y;
  
  const char *piece = pieces[piece_id] + (piece_rotation * PIECE_H * PIECE_W);
  
  for(y=0;y<PIECE_H;++y) {
    for(x=0;x<PIECE_W;++x) {
      int nx=piece_x+x;
      int ny=piece_y+y;
      if(ny<0 || ny>GRID_H) continue;
      if(nx<0 || nx>GRID_W) continue;
      if(piece[y*PIECE_W+x]==1) {
        grid[ny*GRID_W+nx]=piece_colors[piece_id];  // zero erases the grid location.
      }
    }
  }
}


// Borrar fila cada vez que se completa 
void delete_row(int y) {
  // small visual feedback instead of buzzer
  for(int i=0;i<3;i++){
    FastLED.setBrightness(200);
    FastLED.show();
    delay(25);
    FastLED.setBrightness(64);
    FastLED.show();
    delay(25);
  }
  score = score + 10;       // Cada fila borrada añade 10 puntos 
  if(score > top_score)
  {
    saveTopScore();
  }

  loadTopScore();
  top_score_1 = top_score - (  ( top_score/10 ) * 10  );
  top_score_2 = ((top_score - (  ( top_score/100 ) * 100  )) - top_score_1)/10;
  top_score_3 = ((top_score - (  ( top_score/1000 ) * 1000  )) - top_score_1 - top_score_2)/100;
  top_score_4 = (top_score - top_score_1 - top_score_2 - top_score_3) / 1000;
  
  score_1 = score - (  ( score/10 ) * 10  );
  score_2 = ((score - (  ( score/100 ) * 100  )) - score_1)/10;
  score_3 = ((score - (  ( score/1000 ) * 1000  )) - score_1 - score_2)/100;
  score_4 = (score - score_1 - score_2 - score_3) / 1000;

  Serial.println("Score:" + String(score));  
  int x;
  for(;y>0;--y) {
    for(x=0;x<GRID_W;++x) {
      grid[y*GRID_W+x] = grid[(y-1)*GRID_W+x];
    }
  }
// todo se movió hacia abajo 1, por lo que la fila superior debe estar vacía o el juego terminaría.
  for(x=0;x<GRID_W;++x) {
    grid[x]=0;
  }
}

//Caída rápida de figuras
void fall_faster() { 
  if(drop_delay > DROP_MINIMUM) drop_delay -= DROP_ACCELERATION;
}

//Remover filas completas 
void remove_full_rows() {
  int x, y, c;
  char row_removed=0;
  
  for(y=0;y<GRID_H;++y) {
    //cuenta los espacios vacios de la fila
    c = 0;
    for(x=0;x<GRID_W;++x) {
      if( grid[y*GRID_W+x] > 0 ) c++;
    }
    if(c==GRID_W) {
      // row full!
      delete_row(y);
      fall_faster();
    }
  }  
}

// Instrucciones para cada angulo del joystick
void try_to_move_piece_sideways() {
  int dx = map(readAxisX(),0,1023,512,-512);
  
  int new_px = 0;
  if(dx> JOYSTICK_DEAD_ZONE) {
    new_px=-1;
  }
  if(dx<-JOYSTICK_DEAD_ZONE) {
    new_px=1;
  }
  
  if(new_px!=old_px && piece_can_fit(piece_x+new_px,piece_y,piece_rotation)==1) {
    piece_x+=new_px;
  }

  old_px = new_px;
}


void try_to_rotate_piece() {
  int i_want_to_turn=0;
    
  // Rotaciones con el joystick o botón PS3 X
  int dy = map(readAxisY(),0,1023,512,-512);
  if(dy>JOYSTICK_DEAD_ZONE
#ifdef USE_PS3
     || isPs3CrossPressed()
#endif
    ) i_want_to_turn=1;
  
  if(i_want_to_turn==1 && i_want_to_turn != old_i_want_to_turn) {
    // figure out what it will look like at that new angle
    int new_pr = ( piece_rotation + 1 ) % 4;
    // if it can fit at that new angle (doesn't bump anything)
    if(piece_can_fit(piece_x,piece_y,new_pr)) {
      // then make the turn.
      piece_rotation = new_pr;
    } else {
      // wall kick
      if(piece_can_fit(piece_x-1,piece_y,new_pr)) {
        piece_x = piece_x-1;
        piece_rotation = new_pr;
      } else if(piece_can_fit(piece_x+1,piece_y,new_pr)) {
        piece_x = piece_x+1;
        piece_rotation = new_pr;
      } 
    }
  }
  old_i_want_to_turn = i_want_to_turn;
}


//Encajar figuras entre si
int piece_can_fit(int px,int py,int pr) {
  if( piece_off_edge(px,py,pr) ) return 0;
  if( piece_hits_rubble(px,py,pr) ) return 0;
  return 1;
}

// Cuando las figurs sobrepasen el borde
int piece_off_edge(int px,int py,int pr) {
  int x,y;
  const char *piece = pieces[piece_id] + (pr * PIECE_H * PIECE_W);
  
  for(y=0;y<PIECE_H;++y) {
    for(x=0;x<PIECE_W;++x) {
      int nx=px+x;
      int ny=py+y;
      if(ny<0) continue;  // off top, don't care
      if(piece[y*PIECE_W+x]>0) {
        if(nx<0) return 1;  // Fuera del lado izquierdo
        if(nx>=GRID_W ) return 1;  // Fuera del lado derecho
      }
    }
  }
  
  return 0;  // dentro de los lìmites
}

//Saber cuando borrar o no una fila
int piece_hits_rubble(int px,int py,int pr) {
  int x,y;
  const char *piece = pieces[piece_id] + (pr * PIECE_H * PIECE_W);
  
  for(y=0;y<PIECE_H;++y) {    
    int ny=py+y;
    if(ny<0) continue;
    for(x=0;x<PIECE_W;++x) {
      int nx=px+x;
      if(piece[y*PIECE_W+x]>0) {
        if(ny>=GRID_H ) return 1;  // Enviar un 1 si revasa los lìmites de la matriz    
        if(grid[ny*GRID_W+nx]!=0 ) return 1;  // Enviar un 0 si la red esta completa aún
      }
    }
  }
  
  return 0;   
}


//Dibujar la "R" de reinicio
void draw_restart()
{
  for(int i=0;i<STRAND_LENGTH;i++) leds[i] = CRGB::Black;
  // puntos blancos formando la R
  const int indices[] = {55,74,77,83,85,90,91,92,93,98,101,106,107,108,109};
  for(auto idx: indices) if(idx>=0 && idx<STRAND_LENGTH) leds[idx] = CRGB::White;
  FastLED.show();
}


void all_white()
{
  for(int i=0;i<STRAND_LENGTH;i++){
    leds[i] = CRGB::White;
    FastLED.show();
    delay(3);
  }
   
}


void game_over() {
  score = 0;
  game_over_loop_leds();
  delay(1000);
  int x,y;

  long over_time = millis();
  draw_restart();
  currentMillis = millis();
  previousMillis = currentMillis;
  int led_number = 1;
  while(millis() - over_time < 8000) {
    currentMillis = millis();
    if(currentMillis - previousMillis >= 1000){
      previousMillis += 1000;
      // flash a led cada segundo
      int idx = 55-led_number;
      if(idx>=0 && idx<STRAND_LENGTH) leds[idx] = CRGB::White;
      FastLED.show();
      led_number += 1; 
    }    
   
    // restart request?
    bool restartRequested = !digitalRead(button_start);
#ifdef USE_PS3
    restartRequested |= isPs3SelectPressed();
#endif
    if(restartRequested) {
      all_white();
      delay(400);
      break;
    }
  }
  all_white();
  setup();
  return;
}


void game_over_loop_leds()
{
  for(int i=0;i<STRAND_LENGTH;i++){
    leds[i] = CRGB(0,150,0);
    FastLED.show();
    delay(10);
  }
}

//Girar una figura 
void try_to_drop_piece() {
  erase_piece_from_grid();  
  if(piece_can_fit(piece_x,piece_y+1,piece_rotation)) {
    piece_y++;  // move piece down
    add_piece_to_grid();
  } else {
    // hit something!
    // put it back
    add_piece_to_grid();
    remove_full_rows();
    if(game_is_over()==1) {
      game_over();
    }
    // game isn't over, choose a new piece
    choose_new_piece();
  }
}

// Cuando el joystick se encuentra hacia abajo, aumentar velocidad de caída
void try_to_drop_faster() {
  int y = map(readAxisY(),0,1023,512,-512); 
  if(y<-JOYSTICK_DEAD_ZONE
#ifdef USE_PS3
     || isPs3CirclePressed()
#endif
    ) {
    // player is holding joystick down, or PS3 circle button, drop a little faster.
    try_to_drop_piece();
  }
}

// Read axis values, prefer PS3 controller when enabled/connected
int readAxisX(){
#ifdef USE_PS3
  if(Ps3.isConnected()){
    int v = Ps3.data.analog.stick.lx;
    return map(v,-128,127,0,1023);
  }
#endif
  return analogRead(joystick_x);
}

int readAxisY(){
#ifdef USE_PS3
  if(Ps3.isConnected()){
    int v = Ps3.data.analog.stick.ly;
    return map(v,-128,127,0,1023);
  }
#endif
  return analogRead(joystick_y);
}

#ifdef USE_PS3
bool isPs3StartPressed() {
  return Ps3.isConnected() && Ps3.data.button.start;
}

bool isPs3SelectPressed() {
  return Ps3.isConnected() && Ps3.data.button.select;
}

bool isPs3CrossPressed() {
  return Ps3.isConnected() && Ps3.data.button.cross;
}

bool isPs3CirclePressed() {
  return Ps3.isConnected() && Ps3.data.button.circle;
}
#endif

void loadTopScore(){
  preferences.begin(PREFS_NAMESPACE, true);
  top_score = preferences.getUInt("top_score", 0);
  preferences.end();
}

void saveTopScore(){
  preferences.begin(PREFS_NAMESPACE, false);
  preferences.putUInt("top_score", top_score);
  preferences.end();
}

void react_to_player() {
  erase_piece_from_grid();
  try_to_move_piece_sideways();
  try_to_rotate_piece();
  add_piece_to_grid();
  
  try_to_drop_faster();
}

// ¿Puede la pieza caber en esta nueva ubicación?
int game_is_over() {
  int x,y;
  const char *piece = pieces[piece_id] + (piece_rotation * PIECE_H * PIECE_W);
  
  for(y=0;y<PIECE_H;++y) {
    for(x=0;x<PIECE_W;++x) {      
      int ny=piece_y+y;
      int nx=piece_x+x;
      if(piece[y*PIECE_W+x]>0) {
        if(ny<0) return 1;  // yes: off the top!
      }
    }
  }
  
  return 0;  // not over yet...
}

// called once when arduino reboots
void setup() {
  Serial.begin(115200);  
  Serial.println("Tetris");
  delay(1000);

  pinMode(button_pause,INPUT_PULLUP);
  pinMode(button_start,INPUT_PULLUP);

  // buzzer removed

  loadTopScore();
  
  top_score_1 = top_score - (  ( top_score/10 ) * 10  );
  top_score_2 = ((top_score - (  ( top_score/100 ) * 100  )) - top_score_1)/10;
  top_score_3 = ((top_score - (  ( top_score/1000 ) * 1000  )) - top_score_1 - top_score_2)/100;
  top_score_4 = (top_score - top_score_1 - top_score_2 - top_score_3) / 1000;

  Serial.print("Score: ");
  Serial.println(score);
  Serial.print("Top score: ");
  Serial.println(top_score);
  
  int i;
  // setup the LEDs (FastLED)
  FastLED.addLeds<WS2812, LED_DATA_PIN, COLOR_ORDER>(leds, STRAND_LENGTH);
  FastLED.clear(true);
  FastLED.setBrightness(64);
  
  // set up joystick button
  pinMode(JOYSTICK_PIN,INPUT);
  digitalWrite(JOYSTICK_PIN,HIGH);

#ifdef USE_PS3
  if(Ps3.begin()) {
    Serial.println("PS3 support enabled");
    Serial.print("ESP32 BT address: ");
    Serial.println(Ps3.getAddress());
  } else {
    Serial.println("PS3 begin failed or controller not paired");
  }
#endif

  // make sure arduino knows the grid is empty.
  for(i=0;i<GRID_W*GRID_H;++i) {
    grid[i]=0;
  }
  
  // make the game a bit more random - pull a number from space and use it to 'seed' a crop of random numbers.
  randomSeed(analogRead(joystick_y)+analogRead(2)+analogRead(3));
  
  // get ready to start the game.
  choose_new_piece();
  
  move_delay=INITIAL_MOVE_DELAY;
  drop_delay=INITIAL_DROP_DELAY;
  draw_delay=INITIAL_DRAW_DELAY;

  // start the game clock after everything else is ready.
  last_draw = last_drop = last_move = millis();
}


//Void used for sound pause
void waitMilliseconds(uint16_t msWait)
{
  uint32_t start = millis();
  
  while ((millis() - start) < msWait)
  {
    delay(1);
  }
}

//Dibujar la "P" de Pausa 
void draw_pause()
{  
  for(int i=0;i<STRAND_LENGTH;i++) leds[i] = CRGB::Black;
  const int idxs[] = {53,61,66,67,68,69,74,77,82,83,84,85};
  for(auto id: idxs) if(id>=0 && id<STRAND_LENGTH) leds[id] = CRGB::White;
  FastLED.show();
   if(!pause_onece)
   {
    pause_onece = true;
    delay(1000);
   }
}


// called over and over after setup()
void loop() {
  long t = millis();

#ifdef USE_PS3
  bool ps3StartNow = isPs3StartPressed();
  static bool ps3StartPreviouslyPressed = false;
  bool ps3StartPressedEdge = ps3StartNow && !ps3StartPreviouslyPressed;
#else
  const bool ps3StartPressedEdge = false;
#endif

  if(!Pause)
  {
    if((!digitalRead(button_pause) && !pause_pressed)
#ifdef USE_PS3
       || ps3StartPressedEdge
#endif
      )
    {
      Pause = !Pause;
      pause_pressed = true;
      pause_onece = false;
    }
    if(digitalRead(button_pause) && pause_pressed)
    {      
      pause_pressed = false;
    }
#ifdef USE_PS3
    ps3StartPreviouslyPressed = ps3StartNow;
#endif
    
    // the game plays at one speed,
    if(t - last_move > move_delay ) {
      last_move = t;
      react_to_player();
    }
    
    // ...and drops the falling block at a different speed.
    if(t - last_drop > drop_delay ) {
      last_drop = t;
      try_to_drop_piece();
    }
    
    // when it isn't doing those two things, it's redrawing the grid.
    if(t - last_draw > draw_delay ) {
      last_draw = t;
      draw_grid();
    }
  }//end of !pause

  else
  {
    if((!digitalRead(button_pause) && !pause_pressed)
#ifdef USE_PS3
       || ps3StartPressedEdge
#endif
      )
    {
      Pause = !Pause;
      pause_pressed = true;
    }
    if(digitalRead(button_pause) && pause_pressed)
    {      
      pause_pressed = false;
    }
#ifdef USE_PS3
    ps3StartPreviouslyPressed = ps3StartNow;
#endif
    draw_pause();
    delay(1);
  }
}