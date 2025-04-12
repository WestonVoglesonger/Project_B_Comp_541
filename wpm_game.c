/*
    wpm_game.c

    A self-contained typing speed game where the user types a single sentence.
    The sentence is selected pseudo-randomly from a pool of sample sentences.
    No external libraries are used, and no multiplication (*) or division (/) 
    operators appear in our code (we use precomputed constants and repeated subtraction).
    All definitions required by procs.c appear before its single inclusion.
    
    Written for COMP 541.
*/

/* --- Definitions required by procs.c --- */

// Define the color enumeration.
enum colors { black, red, green, yellow, blue, magenta, cyan, white };

// Define key arrays. (These MUST be defined before including procs.c)
char key_array[] = {
    ' ', '!', '\"', '#', '$', '%', '&', '\'',
    '(', ')', '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    ':', ';', '<', '=', '>', '?', '@',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '[', '\\', ']', '^', '_', '`',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '{', '|', '}', '~',
    '\n', '\r', '\b'
};

char key_array2[] = { 'j', 'l', 'i', 'k' };

/* --- Sprite Definitions --- */

// Define the sprite attribute type.
typedef struct {
    char char_to_display;
    int fg_color;
    int bg_color;
} sprite_attr;

// We use 4 sprites:
//   0: Default display (plain text)
//   1: Correctly typed character (green background)
//   2: Incorrectly typed character (red background)
//   3: Cursor indicator (yellow background)
#define Nchars 4
sprite_attr sprite_attributes[Nchars] = {
    {' ', white, black},   // Plain text: white on black
    {' ', white, green},   // Correct input: white on green
    {' ', white, red},     // Incorrect input: white on red
    {' ', black, yellow}   // Cursor indicator: black on yellow
};

/* --- Forward Declarations of Custom Functions --- */
int my_strlen(const char *str);
void my_memset(char *dest, char value, int size);
void my_strcpy(char *dest, const char *src);
void my_print(int row, int col, const char *str);
void my_print_int(int row, int col, int value);
int my_int_length(int value);
void my_print_time(int row, int col, int seconds, int hundredths);
int count_words(const char *sentence);
void update_display(const char *sample, const char *user);

/* --- Include procs.c (Do not modify procs.c) ---*/
#include "procs.c"

/* --- Custom Function Implementations --- */

// Returns the length of a string.
int my_strlen(const char *str) {
    int length = 0;
    while (str[length]) {
        length = length + 1;
    }
    return length;
}

// Fills a block of memory with the given value.
void my_memset(char *dest, char value, int size) {
    int i = 0;
    while (i < size) {
        dest[i] = value;
        i = i + 1;
    }
}

// Copies a string from src to dest.
void my_strcpy(char *dest, const char *src) {
    int i = 0;
    while (src[i]) {
        dest[i] = src[i];
        i = i + 1;
    }
    dest[i] = '\0';
}

// Prints a string at the specified row and column using the sprite system.
void my_print(int row, int col, const char *str) {
    int i = 0;
    foreground_color(white);
    background_color(black);
    while (str[i]) {
        cursor_to(row, col + i);
        sprite_attributes[0].char_to_display = str[i];
        print_sprite(0);
        i = i + 1;
    }
    reset_terminal_colors();
}

// Returns the number of digits in a positive integer.
int my_int_length(int value) {
    if (value < 10)
        return 1;
    if (value < 100)
        return 2;
    if (value < 1000)
        return 3;
    if (value < 10000)
        return 4;
    if (value < 100000)
        return 5;
    if (value < 1000000)
        return 6;
    return 7;
}

// Prints an integer without using multiplication or division.
// Uses a precomputed array of powers of ten and repeated subtraction.
void my_print_int(int row, int col, int value) {
    if (value == 0) {
        foreground_color(white);
        background_color(black);
        cursor_to(row, col);
        sprite_attributes[0].char_to_display = '0';
        print_sprite(0);
        return;
    }
    int powers[10] = {1000000000,100000000,10000000,1000000,100000,10000,1000,100,10,1};
    int started = 0;
    int i = 0;
    while (i < 10) {
        int power = powers[i];
        int digit = 0;
        while (value >= power) {
            value = value - power;
            digit = digit + 1;
        }
        if (digit > 0 || started) {
            started = 1;
            foreground_color(white);
            background_color(black);
            cursor_to(row, col);
            sprite_attributes[0].char_to_display = '0' + digit;
            print_sprite(0);
            col = col + 1;
        }
        i = i + 1;
    }
}

// Prints a time value (seconds and hundredths) without using multiplication or division.
void my_print_time(int row, int col, int seconds, int hundredths) {
    my_print_int(row, col, seconds);
    int sec_len = my_int_length(seconds);
    // Print decimal point.
    foreground_color(white);
    background_color(black);
    cursor_to(row, col + sec_len);
    sprite_attributes[0].char_to_display = '.';
    print_sprite(0);
    if (hundredths < 10) {
        foreground_color(white);
        background_color(black);
        cursor_to(row, col + sec_len + 1);
        sprite_attributes[0].char_to_display = '0';
        print_sprite(0);
        my_print_int(row, col + sec_len + 2, hundredths);
    } else {
        my_print_int(row, col + sec_len + 1, hundredths);
    }
    my_print(row, col + sec_len + 3, " seconds");
}

// Counts the number of words in a sentence.
int count_words(const char *sentence) {
    int count = 0;
    int in_word = 0;
    int i = 0;
    while (sentence[i]) {
        char c = sentence[i];
        int is_space = (c == ' ' || c == '\n' || c == '\r' || c == '\t');
        if (!is_space) {
            if (!in_word) {
                count = count + 1;
                in_word = 1;
            }
        } else {
            in_word = 0;
        }
        i = i + 1;
    }
    return count;
}

// Updates the display with the sample sentence and current user input.
// The cursor's column (2 + user length) is capped at column 39.
void update_display(const char *sample, const char *user) {
    int len = my_strlen(sample);
    my_print(1, 2, "Type the sentence below as fast and accurately as you can:");
    int user_len = my_strlen(user);
    int i = 0;
    while (i < len) {
        if (i < user_len) {
            if (user[i] == sample[i]) {
                sprite_attributes[1].char_to_display = sample[i];
                putChar_atXY(1, 2 + i, 3);
            } else {
                sprite_attributes[2].char_to_display = sample[i];
                putChar_atXY(2, 2 + i, 3);
            }
        } else {
            sprite_attributes[0].char_to_display = sample[i];
            putChar_atXY(0, 2 + i, 3);
        }
        i = i + 1;
    }
    my_print(5, 2, "Your typing: ");
    my_print(5, 14, user);
    int current_index = user_len;
    int cursor_col = 2 + current_index;
    if (cursor_col > 39) {  // Cap the cursor column to 39.
        cursor_col = 39;
    }
    putChar_atXY(3, cursor_col, 3);
}

/* --- Extended Pool of Sample Sentences --- */
#define SAMPLE_COUNT 15
const char *sample_sentences[SAMPLE_COUNT] = {
    "The quick brown fox jumps over the lazy dog.",
    "Pack my box with five dozen liquor jugs.",
    "How vexingly quick daft zebras jump!",
    "Sphinx of black quartz, judge my vow.",
    "Jackdaws love my big sphinx of quartz.",
    "The five boxing wizards jump quickly.",
    "Bright vixens jump; dozy fowl quack.",
    "A mad boxer shot a quick, gloved jab to the jaw of his dizzy opponent.",
    "Two driven jocks help fax my big quiz.",
    "Crazy Fredrick bought many very exquisite opal jewels.",
    "We promptly judged antique ivory buckles for the next prize.",
    "Sixty zippers were quickly picked from the woven jute bag.",
    "Amazingly few discotheques provide jukeboxes.",
    "Jaded zombies acted quaintly but kept driving their oxen forward.",
    "The job requires extra pluck and zeal from every young wage earner."
};
const char *sample_sentence;  // This will point to the chosen sentence

/* --- Pseudo-Random Generator Functions --- */

// Global seed variable.
unsigned int seed = 123456789;

// next_random() uses bit shifting and XOR to update the seed.
unsigned int next_random() {
    seed = seed ^ (seed << 3) ^ (seed >> 5) ^ (seed << 7);
    return seed;
}

// Returns value mod 'mod' using repeated subtraction.
unsigned int mod_value(unsigned int value, unsigned int mod) {
    while (value >= mod) {
        value = value - mod;
    }
    return value;
}

/* --- Main Function --- */
int main() {
    /* Initialize I/O and related systems (as defined in procs.c) */
    initialize_IO("smem.mem");
    
    /* Select one sentence pseudo-randomly from our pool */
    unsigned int r = next_random();
    unsigned int index = mod_value(r, SAMPLE_COUNT);
    sample_sentence = sample_sentences[index];
    
    clear_screen();
    my_print(1, 2, "Typing Speed Game");
    my_print(2, 2, "Type the following sentence as fast as you can:");
    my_print(4, 2, sample_sentence);
    my_print(6, 2, "Press any key to start...");
    while(get_key() == 0) {
        my_pause(10);
    }
    
    clear_screen();
    int sentence_length = my_strlen(sample_sentence);
    char user_input[512];
    my_memset(user_input, 0, 512);
    int current_index = 0;
    
    my_pause(50);  // A short pause before starting.
    
    /* Timing: each loop iteration counts for 0.1 second (10 hundredths) */
    unsigned int elapsed_time_hundredths = 0;
    unsigned int pause_amount = 10;
    update_display(sample_sentence, user_input);
    
    while (current_index < sentence_length) {
        int ch = getch();  // Read one character from input.
        if (ch != 0) {
            if (ch == '\b' || ch == 127) {  // Handle backspace.
                if (current_index > 0) {
                    current_index = current_index - 1;
                    user_input[current_index] = '\0';
                }
            } else {
                user_input[current_index] = (char)ch;
                current_index = current_index + 1;
                user_input[current_index] = '\0';
            }
            update_display(sample_sentence, user_input);
        }
        my_pause(pause_amount);
        elapsed_time_hundredths = elapsed_time_hundredths + pause_amount;
    }
    
    /* Signal sentence completion with a beep */
    put_sound(227273);
    my_pause(25);
    sound_off();
    
    /* Convert elapsed hundredths to seconds using repeated subtraction */
    unsigned int elapsed_seconds = 0;
    unsigned int display_hundredths = elapsed_time_hundredths;
    while (display_hundredths >= 100) {
        elapsed_seconds = elapsed_seconds + 1;
        display_hundredths = display_hundredths - 100;
    }
    
    /* Calculate performance:
       - "Standard" words (5 characters per word): each character contributes 20 units.
       - Actual words are counted via count_words(). */
    int character_count = my_strlen(sample_sentence);
    int word_count = count_words(sample_sentence);
    
    int standard_words_x100 = 0;
    int i = 0;
    while (i < character_count) {
        standard_words_x100 = standard_words_x100 + 20;
        i = i + 1;
    }
    
    int wpm = 0;
    int remaining = standard_words_x100;
    unsigned int minutes_x100 = 0;
    unsigned int temp_hundredths = elapsed_time_hundredths;
    while (temp_hundredths >= 6000) {
        minutes_x100 = minutes_x100 + 100;
        temp_hundredths = temp_hundredths - 6000;
    }
    while (temp_hundredths >= 60) {
        minutes_x100 = minutes_x100 + 1;
        temp_hundredths = temp_hundredths - 60;
    }
    if (minutes_x100 == 0) {
        minutes_x100 = 1;
    }
    while (remaining >= minutes_x100) {
        wpm = wpm + 1;
        remaining = remaining - minutes_x100;
    }
    
    int actual_words_x100 = 0;
    i = 0;
    while (i < word_count) {
        actual_words_x100 = actual_words_x100 + 100;
        i = i + 1;
    }
    int actual_wpm = 0;
    remaining = actual_words_x100;
    while (remaining >= minutes_x100) {
        actual_wpm = actual_wpm + 1;
        remaining = remaining - minutes_x100;
    }
    
    clear_screen();
    my_print(2, 2, "Sentence completed!");
    my_print(3, 2, "Your typing speed is: ");
    my_print_int(3, 24, wpm);
    my_print(3, 24 + my_int_length(wpm), " WPM (standard)");
    my_print(4, 2, "Your typing speed is: ");
    my_print_int(4, 24, actual_wpm);
    my_print(4, 24 + my_int_length(actual_wpm), " WPM (actual words)");
    my_print(5, 2, "Time taken: ");
    my_print_time(5, 14, elapsed_seconds, display_hundredths);
    my_print(7, 2, "Press any key to exit...");
    while(get_key() == 0) {
        my_pause(10);
    }
    
    return 0;
}

/*
    Written by Montek Singh
    Copyright and all rights reserved.
    Last Updated: Apr 6, 2025

    Credits: Thanks to Thayer Hicks for adding audio support (in procs.c).
    Permission granted for use in the COMP 541 course.
*/
