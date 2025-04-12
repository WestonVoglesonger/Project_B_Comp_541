/*

Written by Montek Singh
Copyright and all rights reserved by Montek Singh
Last Updated:  Apr 6, 2025

Credit:  A big thank you to Thayer Hicks for adding audio support
via the SDL2 library! (April 2025)

Permission granted to use this only for students, teaching/learning assistants
and instructors of the COMP 541 course at UNC Chapel Hill.
For any other use, contact Montek Singh first.

*/


/* INSTRUCTIONS

The first time you use this library of methods, you will need to install the
SDL2 library using this command:

        sudo apt install libsdl2-dev libsdl2-2.0-0 -y

If you get a message asking to reboot, save and close all your work,
then restart the VM by clicking on the on/off button at the top right
==> "Power Off/Log Out" ==> "Restart...".

To compile your program, make sure the last line of your code is:

        #include "procs.c"

... and use this command to produce the executable:

        cc your_code.c -lSDL2 -lm

The executable will be "a.out" unless you specify a different name, e.g.:

        cc your_code.c -lSDL2 -lm -o your_demo
        
Run the executable by typing "./a.out" or "./your_demo".

*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <signal.h>
#include <SDL2/SDL.h>
#include <math.h>
#include <pthread.h>

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 4096 


void cursor_off() {
	printf("\033[?25l");
}
void cursor_on() {
	printf("\033[?25h");
}
void cursor_to(int row, int col) {
	printf("\033[%d;%dH", row, col);
}

void my_pause(int N) {		// N is hundredths of a second
	struct timespec t1, t2;
	t1.tv_sec = N / 100;
	t1.tv_nsec = (N % 100) * 10000000ul;
	nanosleep(&t1, &t2);
}

void foreground_color(enum colors c) {
	printf("\033[1;%dm", c+90);
}

void background_color(enum colors c) {
	printf("\033[%dm", c+100);
}

void clear_screen() {
	printf("\033[2J");
}

void reset_terminal_colors() {
	printf("\033[0m");
}

void print_sprite(int charcode) {
	sprite_attr attr = sprite_attributes[charcode];
	foreground_color(attr.fg_color);
	background_color(attr.bg_color);
	printf("%c", attr.char_to_display);
}

int __accelX = 32*8;
int __accelY = 32*8;

void show_accel();

void accel_update(int direction) {
	switch (direction) {
		case 0: __accelX = __accelX + 32; if(__accelX > 32*15) __accelX=32*15; break; 	// Up
		case 1: __accelX = __accelX - 32; if(__accelX < 0) __accelX=0; break; 			// Down
		case 2: __accelY = __accelY - 32; if(__accelY < 0) __accelY=0; break; 			// Right
		case 3: __accelY = __accelY + 32; if(__accelY > 32*15) __accelY=32*15; break; 	// Left
	}
	show_accel();
}

void accel_process_arrows() {
	char buffer[10];
	int n = read(STDIN_FILENO, buffer, 2); 				// Try to read two more bytes of the escape sequence
	if(n<2) return; 									// Not an arrow key sequence
	if(buffer[0] != 91) return; 						// Not an arrow key sequence
	if((buffer[1] < 65) || (buffer[1] > 68)) return; 	// Not an arrow key sequence
	accel_update(buffer[1]-65); 						// Found an arrow key; update accelerometer value	
}

int getch() {
    char buffer[10];
	int n = read(STDIN_FILENO, buffer, 1);
	if(n==0) return 0; 		// No input to process
	if(buffer[0]==27) {		// Escape sequence starting
		accel_process_arrows(); 	// Check if input corresponds to arrow keys
		return 0;
	}
    return buffer[0];
}

void draw_bit_vector(int pattern, int length, int y, int x, int direction, char* prompt, int foreground, int background1, int background2) {
	cursor_to(y, x);
	char buffer[11];		// temp buffer to hold prompt (max size 10)
	snprintf(buffer, 10, "%s", prompt);
	foreground_color(foreground);
	background_color(background2);
	printf("%s", buffer);

	if(direction == 1) 		// vertical, move below prompt
		y++;

	for (int i=0; i<length; i++) {
		int bit = (pattern << i) & (1 << (length-1));
		if(direction == 1)
			cursor_to(y+i, x); 	// display vertically

		foreground_color(foreground);
		if(bit)
			background_color(background1);
		else
			background_color(background2);
		printf(" ");
	}

}

int __lights_reg = 0;

void show_leds() {
	// show the 16 LED lights as a pattern on a light bar
	draw_bit_vector(__lights_reg, 16, 31, 1, 0, "LEDs:   ", white, green, black);
}

void put_leds(int pattern) {
	// put_leds: set the LED lights to a specified pattern
	//   displays on row #y, col #x (on the screen display)

	__lights_reg = pattern;
	show_leds();
}

char __smem[30][40];

void putChar_atXY(int charcode, int col, int row) { 	// puts a character at screen location (X, Y)
	if((row > 29) || (col > 39)) {
		fprintf(stderr, "Writing outside screen bounds: row=%d, col=%d\n", row, col);
		exit(1);
	}
	__smem[row][col] = charcode;
	cursor_to(row+1, col+1);
	print_sprite(charcode);
}

int getChar_atXY(int col, int row) { 	// gets the character from screen location (X, Y)
	return __smem[row][col];
}

void initialize_smem(char* initfilename) {
	FILE * initfile = fopen(initfilename, "r");
	if(!initfile) {
		fprintf(stderr, "Could not open screen memory initialization file: %s\n", initfilename);
		fprintf(stderr, "Continuing in 5 seconds without screen initialization.\n");
		my_pause(500);
		clear_screen();
	}
	else {
		clear_screen();
		int charcode;
		for(int row=0; row<30; row++) {
			for(int col=0; col<40; col++) {
				if(fscanf(initfile, "%d", &charcode) != 0) {
					__smem[row][col] = charcode;
					putChar_atXY(charcode, col, row);
				}
			}	
		}
	}
}

int get_key() {
	int ch = getch();
	if(ch == 0) return 0;
	
	int size = sizeof(key_array) / sizeof(key_array[0]);

	for(int i = 0; i<size; i++) { 		// iterate through key_array to find match
    	int k = key_array[i];
    	if (k == ch) return 1+i;
    }
    return 0;
}

int get_key2() {
	int ch = getch();
	if(ch == 0) return 0;
	
	int size = sizeof(key_array2) / sizeof(key_array2[0]);

	for(int i = 0; i<size; i++) { 		// iterate through key_array2 to find match
    	int k = key_array2[i];
    	if (k == ch) return 1+i;
    }
    return 0;
}

int pause_and_getkey(int N) {
	my_pause(N);
	int key=0;
	int ch;
	while((ch = get_key()))
		key=ch;
	return key;
}

void pause_and_getkey_2player(int N, int* key1, int* key2) {
	my_pause(N);

	(*key1) = 0;
	(*key2) = 0;
	
	int size1 = sizeof(key_array) / sizeof(key_array[0]);
	int size2 = sizeof(key_array2) / sizeof(key_array2[0]);

	int ch;
	while((ch = getch())) {

		for(int i = 0; i<size1; i++) { 		// iterate through key_array to find match
    		if (ch == key_array[i]) {
    			(*key1) = 1+i;
    			break;
    		}
    	}

    	for(int i = 0; i<size2; i++) { 		// iterate through key_array to find match
    		if (ch == key_array2[i]) {
    			(*key2) = 1+i;
    			break;
    		}
    	}
    }
}

int note_intervals[] = { // These are *halfnotes* above the actual notes, i.e., midpoints between consecutive notes
	935713, 883196, 833626, 786838, 742676, 700993, 661649, 624514, 589463, 556379, 525151, 495677, 	//A2-A#2 to G#3-A3
	467857, 441598, 416813, 393419, 371338, 350497, 330825, 312257, 294731, 278189, 262576, 247838, 	//A3-A#3 to G#4-A4
	233928, 220799, 208406, 196710, 185669, 175248, 165412, 156128 										//A4-A#4 to E5-F5
};

typedef struct {
    volatile int current_period;  // Shared variable for frequency control
    volatile char sound_active;   // Shared flag to control sound (0 or 1)
    volatile float current_frequency;  // Shared frequency variable
} SoundControlState;

static SoundControlState sound_control = {0};
static SDL_AudioDeviceID g_audio_device = 0;
int __sound_reg = 0;

void show_sound() {
	int pattern;
	if(__sound_reg == 0) 		// sound is off
		pattern = 0;
	else {
		int i;
		for (i = 0; i < 32; ++i) {
			if(__sound_reg > note_intervals[i])
				break;
		}
		if(i>31)
			i=31;
		pattern = 1 << (31 - i);	// Higher notes shown towards LSB
	}
	draw_bit_vector(pattern, 32, 32, 1, 0, "Sound:  ", white, blue, black);
}

void audio_callback(void* userdata, Uint8* stream, int len) {
    static float current_phase = 0.0f;
    float* floatStream = (float*)stream;
    int samples = len / sizeof(float);
    
    char is_active = sound_control.sound_active;
    float frequency = sound_control.current_frequency;

    if (!is_active) {
        memset(stream, 0, len);
        return;
    }

    // Generate sine wave directly in the stream
    for (int i = 0; i < samples; i++) {
        floatStream[i] = sinf(current_phase);
        
        current_phase += 2.0f * M_PI * frequency / SAMPLE_RATE;
        
        if (current_phase > 2.0f * M_PI) {
            current_phase -= 2.0f * M_PI;
        }
    }
}

int init_sound() {
    SDL_Init(SDL_INIT_AUDIO);
    SDL_AudioSpec desired = {
        .freq = SAMPLE_RATE,
        .format = AUDIO_F32,
        .channels = 1,  // MONO
        .samples = BUFFER_SIZE / sizeof(float),
        .callback = audio_callback,
        .userdata = NULL
    };
    
    g_audio_device = SDL_OpenAudioDevice(
        NULL, 0, &desired, NULL,
        SDL_AUDIO_ALLOW_FORMAT_CHANGE
    );
    
    if (g_audio_device == 0) {
        fprintf(stderr, "Failed to open audio device: %s\n", SDL_GetError());
        return -1;
    }

    SDL_PauseAudioDevice(g_audio_device, 0);
    return 0;
}

void put_sound(int period) {
    __sound_reg = period;
    sound_control.current_period = period;
    
    int frequency = (period > 0) ? (int)(pow(10, 8) / period) : 0;
    
    if (frequency < 1) {
        sound_control.sound_active = 0;
        sound_control.current_frequency = 0;
    } else {
        frequency = (frequency > 20000) ? 20000 : frequency;
        sound_control.current_frequency = frequency;
        sound_control.sound_active = 1;
    }
    
    show_sound();
}

void sound_off() {
    __sound_reg = 0;
    sound_control.sound_active = 0;
    sound_control.current_frequency = 0;
    show_sound();
}

void cleanup_sound() {
    SDL_CloseAudioDevice(g_audio_device);
    SDL_Quit();
}


int get_accel() { 
	getch(); 		// Reads input and updates accelerometer value
	return (__accelX << 16) + __accelY;
}

int get_accelX() { return __accelX; }

int get_accelY() { return __accelY; }

void show_accelY() {
	draw_bit_vector(1 << (__accelY/32), 16, 33, 1, 0, "accelY: ", white, red, black);
}

void show_accelX() {
	draw_bit_vector(1 << (__accelX/32), 16, 1, 43, 1, "accelX:", white, red, black);
}

void show_accel() {
	show_accelX();
	show_accelY();
}

void set_screen_size(int rows, int cols) {
	printf("\033[8;%d;%dt", rows, cols);
}

struct termios oldtc;

void set_terminal_attr() {
	struct termios newtc;

    tcgetattr(STDIN_FILENO, &oldtc);
    newtc = oldtc;
    newtc.c_lflag &= ~(ICANON | ECHO);
    newtc.c_cc[VMIN] = 0;
    newtc.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newtc);
}

void restore_terminal_attr() {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldtc);
}

void cleanup_IO() {
	cursor_to(34, 1);
	restore_terminal_attr();
   	reset_terminal_colors();
	//clear_screen();
	cleanup_sound();
	cursor_on();
	printf("\n");
	exit(0);
}

void show_how_to_quit() {
	cursor_to(34, 34);
	foreground_color(blue);
	background_color(yellow);
	printf("<Ctrl-C to quit>");
}

void initialize_IO(char* smem_initfile) {
	set_screen_size(35, 52);
	setbuf(stdout, NULL);
	set_terminal_attr();
	reset_terminal_colors();
	clear_screen();
	cursor_off();
	initialize_smem(smem_initfile);
	show_accel();
	show_leds();
	show_sound();
	init_sound();
	show_how_to_quit();

	atexit(cleanup_IO); 	// set cleanup method upon exit
	signal(SIGINT, cleanup_IO);  // also cleanup on Ctrl-C
}