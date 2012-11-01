#include "LedControl.h";

#define COLS 8 // usually x
#define ROWS 8 // usually y
// planar world or toroidal world?
#define TOROID 1

byte world[COLS][ROWS][2]; // Create a double buffered world.
byte frame_log[COLS][ROWS];

LedControl lc=LedControl(12,11,10,1);

/*
Under the setLed method:

88039ms total for 4571 frames = ~19.26ms per frame (~1m29s wall clock time)

Code output:
Beginning at: 2
Ending at: 88041
Execution time: 88039
Frame count: 4571

using the setRow method:
14576ms total for 4571 frames = ~3ms per frame (14s wall clock)

Code output:
Beginning at: 2
Ending at: 14578
Execution time: 14576
Frame count: 4571
*/

//--------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  lc.shutdown(0,false);
  lc.setIntensity(0,1);
  lc.clearDisplay(0);
  randomSeed(analogRead(1));
  randomSeed(1);
}

void loop() {
  Life();
  lc.clearDisplay(0);
  Serial.println("Loop over!");
}

//--------------------------------------------------------------------------------
// functions

void Life() {
  int frame_number, generation;
  frame_number = 0;
  generation = 0;
  initialize_frame_log(); // blank out the frame_log world
  
  for(int y=0; y < ROWS; y++) { for(int x=0; x < COLS; x++) { world[x][y][1] = 1; } }
  draw_frame();
  delay(1000); 
  for(int y=0; y < ROWS; y++) { for(int x=0; x < COLS; x++) { world[x][y][1] = 0; } }
  draw_frame();
  delay(500); 

  // draw the initial generation
  set_random_next_frame();
  draw_frame();
  delay(150);
  
  while(1) {
    // Log every 20th frame to monitor for repeats
    if( frame_number == 0 ) { log_current_frame(); }
    
    // generate the next generation
    generate_next_generation();
    
    // if there are no changes between the current generation and the next generation (still life), break out of the loop.
    if( current_equals_next() == 1 ) {
      Serial.println("Death due to still life.");
      draw_frame();
      delay(500);
      break;
    }
    
    // If the next frame is the same as a frame from 20 generations ago, we're in a loop.
    if( next_equals_logged_frame() == 1 ) {
      Serial.println("Death due to oscillator.");
      draw_frame();
      delay(500);
      break;
    }

    if( generation >= 2000) {
      Serial.print("Died due to methuselah's syndrome at generation "); Serial.println(generation); 
      delay(500);
      break;
    }
    
    draw_frame();
    delay(150);
    frame_number++;
    generation++;
    
    Serial.println(generation);
    
    if(frame_number == 20 ) {
      frame_number = 0;
    }
  }
}

void initialize_frame_log() {
  for(int y=0; y < ROWS; y++) {
    for(int x=0; x < COLS; x++) {
      frame_log[x][y] = -1;
    }
  }
}

void log_current_frame() {
  for(int y=0; y < ROWS; y++) {
    for(int x=0; x < COLS; x++) {
      frame_log[x][y] = world[x][y][0];
    }
  }
}

void set_random_next_frame(void) {
  // blank out the world
  resetDisplay();
  
  int density = random(40,80);
  
  //  Serial.print("Initial density: ");
  //  Serial.println(density);
  
  for(int y=0; y<ROWS; y++) {
    for(int x=0; x<COLS; x++) {
      if(random(100) > density) {
	world[x][y][1] = 1;
      }
    }
  }
//  world[1][1][1] = 1;
//  world[2][1][1] = 1;
//  world[3][1][1] = 1;
}

char current_equals_next() {
  char x, y;
  for(y=0; y<ROWS; y++) {
    for(x=0; x<COLS; x++) {
      if( world[x][y][0] != world[x][y][1] ) {
	return 0;
      }
    }
  }
  return 1;
}

int next_equals_logged_frame(){
  for(int y = 0; y<ROWS; y++) {
    for(int x = 0; x<COLS; x++) {
      if(world[x][y][1] != frame_log[x][y] ) {
	return 0;
      }
    }
  }
  return 1;
}

void generate_next_generation(void){  //looks at current generation, writes to next generation array                                                    
  char x,y, neighbors;
  for ( y=0; y<ROWS; y++ ) {
    for ( x=0; x<COLS; x++ ) {
      //count the number of current neighbors - currently planar.  I'd love to make it toroidal.                                                        
      neighbors = 0;
      if( get_led_xy((x-1),(y-1)) > 0 ) { neighbors++; } //NW                                                                                       
      if( get_led_xy(( x ),(y-1)) > 0 ) { neighbors++; } //N                                                                                        
      if( get_led_xy((x+1),(y-1)) > 0 ) { neighbors++; } //NE                                                                                       
      if( get_led_xy((x-1),( y )) > 0 ) { neighbors++; } //W                                                                                        
      if( get_led_xy((x+1),( y )) > 0 ) { neighbors++; } //E                                                                                        
      if( get_led_xy((x-1),(y+1)) > 0 ) { neighbors++; } //SW                                                                                       
      if( get_led_xy(( x ),(y+1)) > 0 ) { neighbors++; } //S                                                                                        
      if( get_led_xy((x+1),(y+1)) > 0 ) { neighbors++; } //SE                                                                                       

      if( world[x][y][0] > 0 ){
        //current cell is alive                                                                                                                         
        if( neighbors < 2 ){
          //Any live cell with fewer than two live neighbours dies, as if caused by under-population.                                                   
          world[x][y][1] = 0;
        }
        if( (neighbors == 2) || (neighbors == 3) ){
          //Any live cell with two or three live neighbours lives on to the next generation.                                                            

          world[x][y][1] = 1;
        }
        if( neighbors > 3 ){
          //Any live cell with more than three live neighbours dies, as if by overcyding.                                                             
          world[x][y][1] = 0;
        }

      }
      else {
        //current cell is dead                                                                                                                          
        if( neighbors == 3 ){
          // Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.                                               
          world[x][y][1] = 1;
	  
        }
        else {
          //stay dead for next generation                                                                                                               
          world[x][y][1] = 0;
        }
      }
    }
  }
}

//char get_led_xy (char col, char row) {
//  if(col < 0 | col > COLS-1) { return 0; }
//  if(row < 0 | row > ROWS-1) { return 0;  }
//  return world[col][row][0];
//}

char get_led_xy (char col, char row) {
  if(TOROID == 1) {
    if(col < 0)      { col = COLS-1; }
    if(col > COLS-1) { col = 0; }
    if(row < 0)      { row = ROWS-1; }
    if(row > ROWS-1) { row = 0; }
  }
  else {
    if(col < 0 | col > COLS-1) { return 0; }
    if(row < 0 | row > ROWS-1) { return 0;  }
  }
  return world[col][row][0];
}

void resetDisplay() {
  for(int y = 0; y < ROWS; y++) {
    for(int x = 0; x < COLS; x++) {
      world[x][y][0] = 0;
      world[x][y][1] = 0;
    }
  }
}

void draw_frame (void) {
  draw_frame_by_row();
}

// Draws the current data in world[0] in the most efficient manner
void draw_frame_by_row (void) {
  char x, y;
  for(y=0; y < ROWS; y++) {
    int row = 0;
    for(x=0; x < COLS; x++) {
      if(world[x][y][1] > 0) {
	row += (1 << x);
      }
      world[x][y][0] = world[x][y][1];
    }
    lc.setRow(0,y,row);
  }
}

// Draws the current data in world[0] pixel by pixel - slower than by row.
void draw_frame_by_pixel (void) {
  char x, y;
  for(y=0; y < ROWS; y++) {
    for(x=0; x < COLS; x++) {
      lc.setLed(0,x,y,world[x][y][1]);
      world[x][y][0] = world[x][y][1];
    }
  }
}

