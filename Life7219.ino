#include "LedControl.h";

#define COLS 8 // usually x
#define ROWS 8 // usually y
// planar world or toroidal world?
#define TOROID 1

#define swap(a, b) { uint16_t t = a; a = b; b = t; }

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
  lc.shutdown(0,false);
  lc.setIntensity(0,0);
  lc.clearDisplay(0);
  randomSeed(analogRead(1));
  randomSeed(1);
}

void loop() {
  Life();
  Particles();
  lc.clearDisplay(0);
}

//--------------------------------------------------------------------------------
// functions

void Particles() {
  const int numparticles = 2;

  long p_framecount = 0;

  int velx[numparticles]; 
  int vely[numparticles]; 
  int posx[numparticles]; 
  int posy[numparticles];

  // How big do you want the virtual particle space to be?  Higher resolution
  // means slower but more precise particles (without shortening the delay)
  int res = 7;

  // And how big is the display space? (actual pixels, not off-by-one)
  #define DISPLAY_X 8
  #define DISPLAY_Y 8

  int max_x = DISPLAY_X<<res;
  int max_y = DISPLAY_Y<<res;

  int maxspeed = 30;

  int row, col, i;

  for(i = 0; i<numparticles; i++) {
    velx[i] = random(11)+5;
    vely[i] = random(11)+5;
    posx[i] = (i%2)<<res;
    posy[i] = (i/2)<<res;
  }

  while(p_framecount <= 4000) {
    p_framecount++;
    lc.clearDisplay(0);
    
    // This only really works for particles % 2 = 0
    // Draw the Qix for each pair of particles
    for( i = 0; i < numparticles; i = i+2) {
      drawLine(posx[i]>>res, posy[i]>>res, posx[i+1]>>res, posy[i+1]>>res);
    }

    // Update the location of the particles for the next frame.
    for( i = 0; i<numparticles; i++) {

      // The new positions are the old positions plus the velocity in the correct axis.
      posx[i]+=velx[i];
      posy[i]+=vely[i];
      
      // If the x position will go below 0
      if(posx[i] < 0) {
	posx[i] = 1; // the new x position is 1;
	velx[i] = -velx[i]; // The velocity on the x axis inverts (boing!)
	vely[i] = vely[i]+random(-1,1); // The velocity on the y axis gets a small random change.

	if(vely[i]>maxspeed) vely[i] = maxspeed; // if the new y velocity is too fast, pin it to the maximum speed.
	else if(vely[i]<-maxspeed) vely[i] = -maxspeed;
      }

      else if(posx[i]>=max_x) {
	posx[i] = max_x+(max_x-posx[i]);
	velx[i] = -velx[i];
	vely[i] = vely[i]+random(-1,1);
	
	if(vely[i]>maxspeed) vely[i] = maxspeed;
	else if(vely[i]<-maxspeed) vely[i] = -maxspeed;
      }
      
      if(posy[i]<0) {
	posy[i] = 1;
	vely[i] = -vely[i];
	velx[i] = velx[i]+random(-1,1);

	if(velx[i]>maxspeed) velx[i] = maxspeed;
	else if(velx[i]<-maxspeed) velx[i] = -maxspeed;
      }
      else if(posy[i]>=max_y) {
	posy[i] = max_y+(max_y-posy[i]);
	vely[i] = -vely[i];
	velx[i] = velx[i]+random(-1,1);
      
	if(velx[i]>maxspeed) velx[i] = maxspeed;
	else if(velx[i]<-maxspeed) velx[i] = -maxspeed;
      }

    }
    delay(5);
  }
}



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
      //      Serial.println("Death due to still life.");
      draw_frame();
      delay(500);
      break;
    }
    
    // If the next frame is the same as a frame from 20 generations ago, we're in a loop.
    if( next_equals_logged_frame() == 1 ) {
      //      Serial.println("Death due to oscillator.");
      draw_frame();
      delay(500);
      break;
    }

    if( generation >= 2000) {
      //      Serial.print("Died due to methuselah's syndrome at generation "); Serial.println(generation); 
      delay(500);
      break;
    }
    
    draw_frame();
    delay(150);
    frame_number++;
    generation++;
    
    //    Serial.println(generation);
    
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


void drawLine(int8_t x0, int8_t y0, int8_t x1, int8_t y1) {
  uint16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }
  
  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }
  
  uint16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);
  
  int16_t err = dx / 2;
  int16_t ystep;
  
  if (y0 < y1) {
    ystep = 1;
  } 
  else {
    ystep = -1;
  }
  
  for (; x0<=x1; x0++) {
    if (steep) {
      lc.setLed(0,y0,x0,1);
    }
    else {
      lc.setLed(0,x0,y0,1);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}
