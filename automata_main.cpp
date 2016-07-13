// Made for usage with Windows.
// Requires the SDL https://www.libsdl.org/download-2.0.php and MIDIIO / MIDIData from http://openmidiproject.osdn.jp/index_en.html.

#include "stdafx.h"
#include <cstdlib> 
#include <thread>
#include <bitset>
#include <chrono>
#include <iostream>
#include <mutex>

#include <SDL.h>
#include <SDL_ttf.h> // for text to graphics using SDL
#include <Windows.h>
#include <MIDIIO.h> // Open MIDI Library
#include <MIDIData.h> // Open MIDI Library for file generation

//--------------------------------------

//#define SCR_SI 1280
//#define SCR_SI 960 //for full 12*4.
#define SCR_SI 1120 //for sharpless

//--------------------------------------
int it;	int sec_it;	int rule_dec; bool timer_allow = true;
int population[8]; int intensity[8];

// Birth, decay and delay are hardcoded for initialization.
// Able to change during runtime.
int birth_amount[] = { 0, 0, 8, 0, 0, 0, 0, 0 };
int decay_amount[] = { 3, 2, 1, 1, 2, 3, 4, 5, 8 };
int default_birth[] = { 0, 0, 8, 0, 0, 0, 0, 0 };
int default_decay[] = { 3, 2, 1, 1, 2, 3, 4, 5, 8 };
int game_of_life_birth[] = { 0, 0, 1, 0, 0, 0, 0, 0 };
int game_of_life_decay[] = { 1, 1, 0, 0, 1, 1, 1, 1, 1 };

// Generally you want the SCR_SI / 4 to be divisible by cell_width
int cell_width = 8; // 1 . 2 . 4 . 8 . 10
int click_size = 0; int delay = 3; bool delay_allow = true;
int menu_location_v = 0; 
bool recording = false; int record_len = 0;
const int grid_width = SCR_SI;
const int grid_height = SCR_SI / 2;
const int screen_height = SCR_SI / 2 + 100;
const int dim_len = grid_width / cell_width; int steps = 1000;
const int dim_hei = grid_height / cell_width;

// This is for a full 12 note octaves including sharps. Sounds horror-like.
//int notes_in_octave = 12; int octaves = 4;
// This is for an octave without sharps.
int notes_in_octave = 7; int octaves = 5;
unsigned char *midi_notes;
int *population_hex; std::mutex excl_lock;
int *lifetime;
int *new_dimension; int *old_dimension;
MIDIOut *pMIDIOut;
bool quit = false;
bool paused = false;
bool instant_death = false;
// bpm in miliseconds. secondsperminute*milisecondsinseconds/beatsperminute
int bpmmilis = 60 * 1000 / 120;
int bpm = 120; int left_click_health = 8;

// HARD CODED strings of instrument names.
std::string instruments[] = {
	"Acoustic Grand Piano", "Bright Piano", "Electric Grand Piano", "Honky-tonk piano", "Electric Piano 1", "Electric Piano 2", "Harpsichord", "Clavinet",
	"Celesta", "Glockenspiel", "Music Box", "Vibraphone", "Marimba", "Xylophone", "Tubular Bells", "Dulcimer",
	"Drawbar Organ", "Percussive Organ", "Rock Organ", "Church Organ", "Reed Organ", "Accordion", "Harmonica", "Tango Accordion",
	"Acoustic Guitar (nylon)", "Acoustic Guitar (steel)", "Electric Guitar (jazz)", "Electric Guitar (clean)", "Electric Guitar (muted)", "Overdriven Guitar", "Distortion Guitar", "Guitar Harmonics",
	"Acoustic Bass", "Electric Bass (finger)", "Electric Bass (pick)", "Fretless Bass", "Slap Bass 1", "Slap Bass 2", "Synth Bass 1", "Synth Bass 2",
	"Violin", "Viola", "Cello", "Contrabass", "Tremolo Strings", "Pizzicato Strings", "Orchestral Harp", "Timpani",
	"String Ensemble 1", "String Ensemble 2", "Synth Strings 1", "Synth Strings 2", "Choir Aahs", "Voice Oohs", "Synth Voice", "Orchestra Hit",
	"Trumpet", "Trombone", "Tuba", "Muted Trumpet", "French Horn", "Brass Section", "Synth Brass 1", "Synth Brass 2",
	"Soprano Sax", "Alto Sax", "Tenor Sax", "Baritone Sax", "Oboe", "English Horn", "Bassoon", "Clarinet",
	"Piccolo", "Flute", "Recorder", "Pan Flute", "Blown Bottle", "Shakuhachi", "Whistle", "Ocarina",
	"Lead 1 (square)", "Lead 2 (sawtooth)", "Lead 3 (calliope)", "Lead 4 (chiff)", "Lead 5 (charang)", "Lead 6 (voice)", "Lead 7 (fifths)", "Lead 8 (bass + lead)",
	"Pad 1 (new age)", "Pad 2 (warm)", "Pad 3 (polysynth)", "Pad 4 (choir)", "Pad 5 (bowed)", "Pad 6 (metallic)", "Pad 7 (halo)", "Pad 8 (sweep)",
	"FX 1 (rain)", "FX 2 (soundtrack)", "FX 3 (crystal)", "FX 4 (atmosphere)", "FX 5 (brightness)", "FX 6 (goblins)", "FX 7 (echoes)", "FX 8 (sci-fi)",
	"Sitar", "Banjo", "Shamisen", "Koto", "Kalimba", "Bagpipe", "Fiddle", "Shanai",
	"Tinkle Bell", "Agogo", "Steel Drums", "Woodblock", "Taiko Drum", "Melodic Tom", "Synth Drum",
	"Reverse Cymbal", "Guitar Fret Noise", "Breath Noise", "Seashore", "Bird Tweet", "Telephone Ring", "Helicopter", "Applause", "Gunshot"
};
MIDIData* output_data;

void gen2D(int*, int*);
void play_generation(int*);
void quick_sort(int*, int, int, unsigned char*, int*);
void calculations(); //2D calc

void timer();
void create_render_text_dyn(std::string, int, int);
SDL_Texture *text_to_texture(TTF_Font, std::string, bool, SDL_Color);
//--------------------------------------

bool gfx_init();
void gfx_close();

SDL_Texture *gfx_texture_ca = NULL;
SDL_Texture *gfx_texture_over = NULL;
TTF_Font *font_big; TTF_Font *font_small; SDL_Event sdl_event;
SDL_Window *gfx_window = NULL;
SDL_Renderer *gfx_renderer = NULL;
SDL_Color blue_ish = { 112, 141, 177, 255 };
SDL_Color grey = { 119, 136, 144, 255 };

Uint32 *pixels = new Uint32[grid_width*grid_height];
Uint32 *menu = new Uint32[grid_width*grid_height];

MIDITrack* track;
//--------------------------------------

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))

//	Menu item type.
//	Contains boolean (selected or not selected option) and the graphic of option.
typedef struct {
	bool selected;
	SDL_Texture* option_graphic;
} menuitem_t;


//	Menu type.
//	Contains number of items in menu, previous menu used, this menu's items and locations.
typedef struct menu_s {
	int num_items;
	struct menu_s* prev_menu;
	menuitem_t*	menu_items;
	SDL_Rect* menu_item_locations;
	int last_on;
	void(*routine)(int loc, int choice);
	void(*display)();
} menu_t;

menu_t* current_menu;
menu_t* menus;
menu_t* temp_menu = NULL;
SDL_Texture** textures_zero_to_9 = NULL;
SDL_Texture** options_zero_to_9 = NULL;
SDL_Texture *selected_overlay = NULL;

void M_MainMenuChange(int, int);

void M_PresetMenuDisplay();
void M_BirthMenuDisplay();
void M_DecayMenuDisplay();
void M_DelayMenuDisplay();
void M_BPMDisplay();
void M_InstdMenuDisplay();
void M_ResetMenuDisplay();

void M_PresetMenuChange(int, int);
void M_BirthMenuChange(int, int);
void M_DecayMenuChange(int, int);
void M_DelayMenuChange(int, int);
void M_BPMChange(int, int);
void M_InstdMenuChange(int, int);
void M_ResetMenuChange(int, int);
void M_ReturnOneMenu();


//--------------------------------------

/*
	This, once started as a thread, intializes the notes to be used,
	sorts the notes by an algorithm below and plays them in at most triads.
	It will not let the same note play twice. Will not sustain that note.
*/

void play_generation_2D(){
	// A list of midi notes in hex format.
	midi_notes = new unsigned char[notes_in_octave*octaves + 3];
	int note_mod = 0;
	for (int it_y = 0; it_y < octaves; it_y++){
		for (int it_x = 0; it_x < notes_in_octave; it_x++){
			midi_notes[it_x + it_y*notes_in_octave] = (char)(0x24 + note_mod);
			if (it_x == 2 || it_x == 6)
				note_mod++;
			else note_mod += 2;
		}
	}

	unsigned char volume = 0x44;
	unsigned char *last_played;
	last_played = new unsigned char[3];
	// Yes it's initalized with 6 spaces but only 3 items.
	unsigned char midi_message[6] = { 0x90, 0x00, 0x40 };
	int number_of_notes = 8; int triad = 3;
	while (!quit){
		if (!paused){
			if (!timer_allow){
				// Mutex
				excl_lock.lock();
				// Once allowed, sorts the area 'populations' - amount of notes older 
				// than x generations in a single column. Largest column gets selected as the value of the region.
				quick_sort(population_hex, 0, notes_in_octave*octaves, midi_notes, lifetime);
				midi_message[0] = 0x90; midi_message[3] = volume;
				int modifier = 4;
				// Checking if note has played in last generation.
				for (int it = 0; it < triad; it++){
					if (population_hex[it] > 0){
						bool will_play = true;
						for (int note_check = 0; note_check < triad; note_check++){
							if (last_played[note_check] == midi_notes[it]){
								will_play = false;
							}
						}
						if (will_play){
							last_played[it] = midi_notes[it];
						}
						else{
							last_played[it] = (char)0;
						}
					}
					else{
						last_played[it] = (char)0;
					}
				}
				// Un-Mutex
				excl_lock.unlock();
				// If recording, add data at bpmmilis
				if (recording)	record_len += MIDIData_MillisecToTime(output_data,bpmmilis);
				// Play -> sleep -> Pause
				for (int turn = 0; turn < 2; turn++){
					if (turn == 1) {
						std::this_thread::sleep_for(std::chrono::milliseconds(bpmmilis));
						midi_message[0] = 0x80;
					}
					for (int it = 0; it < triad; it++){
						unsigned char note = last_played[it];
						if (note != (char)0){
							midi_message[1] = note; midi_message[3] = volume - it * modifier;
							MIDIOut_PutMIDIMessage(pMIDIOut, midi_message, 3);
						}
						if (turn == 0 && recording)
							MIDITrack_InsertNote(track, record_len, 0, (int)note, midi_message[3], MIDIData_MillisecToTime(output_data, bpmmilis));
					}
				}
			}
			else std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
		else std::this_thread::sleep_for(std::chrono::milliseconds(bpmmilis));

	}
	delete[] midi_notes;
	delete[] last_played;
}

/*
	XOR quick sort.
*/
void quick_sort(int arr[], int left, int right, unsigned char mid[], int l_arr[]) {
	int i = left, j = right;
	unsigned char tmp_not;
	int pivot = arr[(left + right) / 2];
	while (i <= j) {
		while (arr[i] > pivot)
			i++;
		while (arr[j] < pivot)
			j--;
		if (i <= j) {
			arr[i] = arr[i] + arr[j]; arr[j] = arr[i] - arr[j]; arr[i] = arr[i] - arr[j];
			l_arr[i] = l_arr[i] + l_arr[j]; l_arr[j] = l_arr[i] - l_arr[j]; l_arr[i] = l_arr[i] - l_arr[j];
			tmp_not = mid[i]; mid[i] = mid[j]; mid[j] = tmp_not;
			i++; j--;
		}
	};
	if (left < j)
		quick_sort(arr, left, j, mid, l_arr);
	if (i > right)
		quick_sort(arr, i, right, mid, l_arr);
}

/*
	Generates the next CA generation.
	Cells can be: alive, dead, null, permanently alive and permanently dead.
	If the cell was alive, its 'health' will decrease by an amount that you can control.
	If the cell's 'health' drops to or below 0, it will be dead.
	After a cell dies a new cell cannot be 'born' there for a number of generations.
	If the cell is already dead, the amount of generations left until a cell can be there goes down by 1 (life rises by 1).
	Permanently alive and permanently dead cells count as alive and dead respectively when counted as neighbours.
*/

void gen2D(int *new_dim, int *old_dim){
	int total_alive = 0; 
	bool save_x = false; bool save_y = false;
	// Iteration through CA grid.
	for (int it_h = 0; it_h < dim_len; it_h++)
		for (int it_v = 0; it_v < dim_hei; it_v++){
			total_alive = 0;
			int cell_val = old_dim[it_h + (it_v*dim_len)];
			// If the cell is not alive
			if (cell_val <= 0){
				// check if it has recently died, then increment
				if (cell_val < 0 && cell_val > -100){
					new_dim[it_h + (it_v*dim_len)] = cell_val + 1;
				}
				// if not, a cell can be born here
				else if(cell_val == 0) {
					// calculate the amount of neighbours alive
					for (int x = -1; x <= 1; x++)
						for (int y = -1; y <= 1; y++){
							// Check the surroundings without the cell itself
							// If x/y = their borders, do separate ifs
							int pos_x = it_h + x; int pos_y = it_v + y;
							if (pos_x < 0){ pos_x = dim_len - 1; }	if (pos_y < 0){ pos_y = dim_hei - 1; }
							if (pos_x >= dim_len){ pos_x = 0; }		if (pos_y >= dim_hei){ pos_y = 0; }
							int h_x; int v_y;
							// Looping system for restricted areas - areas blocked off with permanently dead tiles (minor bugs).
							if (old_dim[pos_x + pos_y * dim_len] == -100){
								int new_x = it_h+x; int new_y = it_v+y;
								if (new_x < 0){ new_x = dim_len - 1; }	if (new_y < 0){ new_y = dim_hei - 1; }
								if (new_x >= dim_len){ new_x = 0; }		if (new_y >= dim_hei){ new_y = 0; }
								if (!(abs(x) == 1 != abs(y) == 1)){
									h_x = new_x + x*-1;	v_y = new_y + y*-1;
									if (h_x < 0){ h_x = dim_len - 1; }	if (v_y < 0){ v_y = dim_hei - 1; }
									if (h_x >= dim_len){ h_x = 0; }		if (v_y >= dim_hei){ v_y = 0; }
									if (old_dim[h_x + new_y * dim_len] == -100) save_y = true;
									if (old_dim[new_x + v_y * dim_len] == -100) save_x = true;
								}
								do{
									if (!(abs(x) == 1 != abs(y) == 1)){
										if (save_x != save_y){
											if (save_x) new_x += x*-1;
											if (save_y) new_y += y*-1;
										}
										else {	new_x += x*-1;	new_y += y*-1;}
									}
									else {	new_x += x*-1;	new_y += y*-1;	}
									if (new_x < 0){ new_x = dim_len-1; }	if (new_y < 0){ new_y = dim_hei-1; }
									if (new_x >= dim_len){ new_x = 0; }		if (new_y >= dim_hei){ new_y = 0; }
								} while (old_dim[new_x + new_y * dim_len] != -100);
								if (!(abs(x) == 1 != abs(y) == 1)){
									if (save_x != save_y){
										if (save_x) new_x += x;
										if (save_y) new_y += y;
									}
									else {	new_x += x;	new_y += y;	}
								}
								else {	new_x += x;	new_y += y;	}
								if (new_x < 0){ new_x = dim_len - 1; }	if(new_y < 0){ new_y = dim_hei - 1; }
								if (new_x >= dim_len){ new_x = 0; }		if (new_y >= dim_hei){ new_y = 0; }
								pos_x = new_x; pos_y = new_y;
								save_x = false; save_y = false;
							}
							bool self = (x == 0) && (y == 0);
							int temp_val = old_dim[pos_x + pos_y * dim_len];
							if (temp_val > 0 && !self){
								total_alive++;
							}
						}
					// Set health depending on amount of neighbours alive
					// Health value can be changed in menu
					if (birth_amount[total_alive - 1] > 0){
						new_dim[it_h + (it_v*dim_len)] = birth_amount[total_alive - 1];
						lifetime[it_h + (it_v*dim_len)]++;
					}
					// Reset the amount of generations it was alive
					if (lifetime[it_h + (it_v*dim_len)] > 0)
						lifetime[it_h + (it_v*dim_len)] = 0;
				}
			}
			// If the cell is alive
			else if (cell_val > 0 && cell_val < 100){
				int next_gen_val = cell_val;
				// check for the amount of neighbours
				for (int x = -1; x <= 1; x++)
					for (int y = -1; y <= 1; y++){
						// Check the surroundings without the cell itself
						// If x/y = their borders, do separate ifs.
						int pos_x = it_h + x; int pos_y = it_v + y;
						if (pos_x < 0){ pos_x = dim_len - 1; }	if (pos_y < 0){ pos_y = dim_hei - 1; }
						if (pos_x >= dim_len){ pos_x = 0; }		if (pos_y >= dim_hei){ pos_y = 0; }
						bool self = (x == 0) && (y == 0);
						int temp_val = old_dim[pos_x + pos_y*dim_len];
						if (temp_val > 0 && !self){
							total_alive++;
						}
					}
				
				// With default config:
				// With 1-3 cells around, health will decay at a natural rate.
				// Any more than 3 and it's overcrowded, 8 - cannibalized by others.
				// With 0 cells around, while you get enough food, mental state is damaged, -x health.
				if (!instant_death)
					next_gen_val -= decay_amount[total_alive];
				else if (decay_amount[total_alive] > 0)
					next_gen_val = 0;
				
				// Activates infertile land
				if (next_gen_val <= 0){
					if (delay_allow){
						next_gen_val = delay*-1;
					}
					else next_gen_val = 0;
				}

				if (next_gen_val > 0){
					lifetime[it_h + (it_v*dim_len)]++;
				}
				else{
					lifetime[it_h + (it_v*dim_len)] = 0;
				}
				new_dim[it_h + (it_v*dim_len)] = next_gen_val;
			}
		}
}

bool gfx_init(){
	// Init flag
	bool success = true;
	// Init SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0){
		std::cout << ("SDL could not initialize. Error: %s\n", SDL_GetError());
		success = false;
	}
	else {
		memset(pixels, 38, grid_width * grid_height * sizeof(Uint32));
		// Create window
		gfx_window = SDL_CreateWindow("Cellular Automata Graphics",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, grid_width, screen_height, SDL_WINDOW_SHOWN);
		if (gfx_window == NULL){
			// if it fails, it fails.
			std::cout << ("Window could not be created. Error: %s\n", SDL_GetError());
			success = false;
		} // If it doesn't, the window will be working.
		else {
			gfx_renderer = SDL_CreateRenderer(gfx_window, -1, SDL_RENDERER_ACCELERATED);
			gfx_texture_ca = SDL_CreateTexture(gfx_renderer,
				SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, grid_width, grid_height);
			gfx_texture_over = SDL_CreateTexture(gfx_renderer,
				SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, grid_width, grid_height);
			SDL_UpdateTexture(gfx_texture_ca, NULL, pixels, grid_width * sizeof(Uint32));
		}
	}

	return success;
}

void gfx_close(){
	delete[] pixels;
	delete[] menu;
	// Destroy texture / deallocate memory
	SDL_DestroyTexture(gfx_texture_ca);
	gfx_texture_ca = NULL;
	SDL_DestroyTexture(gfx_texture_over);
	gfx_texture_over = NULL;
	// Destroy renderer / deallocate memory
	SDL_DestroyRenderer(gfx_renderer);
	gfx_renderer = NULL;
	// Destroy window / deallocate memory
	SDL_DestroyWindow(gfx_window);
	gfx_window = NULL;
	// Quit SDL sub-systems
	SDL_Quit();
}

/*
	Population / generation lifetime calculations.
*/
void calculations(){ //2D
	for (int it_x = 0; it_x < dim_len; it_x++)
		for (int it_y = 0; it_y < dim_hei; it_y++)
			new_dimension[it_x + (it_y*dim_len)] = 0;

	// grid_width / cell_width = horizontal space available.
	// there are 12 notes horizontally, 4 vertically, 48 total, so 48 areas, in this version at least.
	// population_hex = new int[notes_in_octave * octaves];
	population_hex = new int[notes_in_octave * octaves + 3];

	while (!quit){
		if (!paused){
			if (timer_allow){
				// Mutex
				excl_lock.lock();
				// Copying generations before newly generating next one
				for (int it_x = 0; it_x < dim_len; it_x++)
					for (int it_y = 0; it_y < dim_hei; it_y++)
						old_dimension[it_x + (it_y*dim_len)] = new_dimension[it_x + (it_y*dim_len)];

				for (int i = 0; i < notes_in_octave * octaves; i++){
					population_hex[i] = 0;
				}

				// Generation method
				gen2D(new_dimension, old_dimension);

				// Sorting by longest in population hex ver.
				for (int it_oct_x = 0; it_oct_x < notes_in_octave; it_oct_x++){
					for (int it_x = it_oct_x*dim_len / notes_in_octave; it_x < (it_oct_x + 1)*dim_len / notes_in_octave; it_x++){
						for (int it_oct_y = 0; it_oct_y < octaves; it_oct_y++){
							int tot_y_div = 0;
							for (int it_y = it_oct_y*dim_hei / octaves; it_y < (it_oct_y + 1)*dim_hei / octaves; it_y++){
								if (lifetime[it_x + (it_y * dim_len)] > 1)
									tot_y_div++;
							}
							if (tot_y_div > population_hex[it_oct_x + it_oct_y*notes_in_octave])
								population_hex[it_oct_x + it_oct_y * notes_in_octave] = tot_y_div;
						}
					}
				}

				/*
				for (int y = 0; y < octaves; y++){
					for (int i = 0; i < notes_in_octave; i++)
						std::cout << std::to_string(population_hex[y*notes_in_octave+i]) + ' ';
					std::cout << std::endl;
				}
				std::cout << "post\n";
				*/

				excl_lock.unlock();
				timer_allow = false;
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
}

/*
	Timer. The timer value can be changed in the menu.
*/
void timer(){
	while (!quit){
		if (!timer_allow) timer_allow = !timer_allow;
		//std::this_thread::sleep_for(std::chrono::milliseconds(18));

		std::this_thread::sleep_for(std::chrono::milliseconds(bpmmilis));
	}
}

/*
	Integer to hex. Only used once at the instrument selection.
*/
std::string int_to_hex(unsigned int n){
	std::string res;
	do{
		res += "0123456789ABCDEF"[n % 16];
		n >>= 4;
	} while (n);

	return std::string(res.rbegin(), res.rend());
}

void affect_cells_with_mouse(bool lmb, SDL_Event ev_in, Sint32 mouse_x, Sint32 mouse_y){
	SDL_Event ev = ev_in;
	if (mouse_y < grid_height)
	for (int x = click_size*-1; x <= click_size; x++)
		for (int y = click_size* -1; y <= click_size; y++){
			int pos_x = mouse_x / cell_width + x; int pos_y = ((mouse_y / cell_width) + y) * dim_len;
			if (pos_x < 0){ pos_x = dim_len - pos_x; }
			if (pos_y < 0){ pos_y = (dim_hei * dim_len) - pos_y; }
			if (pos_x >= dim_len){ pos_x = pos_x - dim_len; }
			if (pos_y >= dim_hei*dim_len){ pos_y = pos_y - dim_hei*dim_len; }
			if (lmb){
				if (new_dimension[pos_x + pos_y] <= 0)
					new_dimension[pos_x + pos_y] = left_click_health;
			}
			else{ new_dimension[pos_x + pos_y] = 0; }
		}
}

SDL_Texture* text_to_texture(TTF_Font *font, std::string text, bool menu, SDL_Color color){
	int mod = 1;
	if (menu) mod = 2;
	SDL_Surface *surface_of_text = TTF_RenderText_Blended_Wrapped(font, text.c_str(), color, grid_width - cell_width * 20 / mod);
	SDL_Texture *texture_of_text = SDL_CreateTextureFromSurface(gfx_renderer, surface_of_text);
	SDL_FreeSurface(surface_of_text);
	return texture_of_text;
}

void create_render_text_dyn(std::string text, int location_x, int location_y, SDL_Color color){
	SDL_Surface *surface_of_text = TTF_RenderText_Blended_Wrapped(font_big, text.c_str(), color, grid_width - cell_width * 20);
	SDL_Texture *texture_of_text = SDL_CreateTextureFromSurface(gfx_renderer, surface_of_text);
	SDL_FreeSurface(surface_of_text);
	SDL_Rect area_rect = { NULL, NULL, NULL, NULL };
	SDL_QueryTexture(texture_of_text, NULL, NULL, &area_rect.w, &area_rect.h);
	area_rect.x = location_x - area_rect.w / 2;
	area_rect.y = location_y - area_rect.h / 4;
	SDL_RenderCopy(gfx_renderer, texture_of_text, NULL, &area_rect);
}

void create_render_text_dyn(SDL_Texture* texture, std::string text, int location_x, int location_y, SDL_Color color){
	SDL_Surface *surface_of_text = TTF_RenderText_Blended_Wrapped(font_big, text.c_str(), color, grid_width - cell_width * 20);
	texture = SDL_CreateTextureFromSurface(gfx_renderer, surface_of_text);
	SDL_FreeSurface(surface_of_text);
	SDL_Rect area_rect = { NULL, NULL, NULL, NULL };
	SDL_QueryTexture(texture, NULL, NULL, &area_rect.w, &area_rect.h);
	area_rect.x = location_x - area_rect.w / 2;
	area_rect.y = location_y - area_rect.h / 4;
	SDL_RenderCopy(gfx_renderer, texture, NULL, &area_rect);
}

// Menus.
void M_MainMenuChange(int loc, int choice){
	temp_menu = current_menu;
	temp_menu->last_on = loc;
	current_menu = &menus[loc];
	loc = current_menu->last_on;
	current_menu->prev_menu = temp_menu;
	menu_location_v = 0;
}

void M_PresetMenuDisplay(){
	for (int i = 0; i < current_menu->num_items; i++){
		if (current_menu->menu_items[i].selected == true){
			SDL_RenderCopy(gfx_renderer, selected_overlay, NULL, &current_menu->menu_item_locations[i]);
			break;
		}
	}
}

void M_BirthMenuDisplay(){
	SDL_Rect temp;
	for (int i = 0; i < current_menu->num_items; i++){
		//gfx-rend, sdl_texture, null, sdl_rect
		temp = current_menu->menu_item_locations[i];
		SDL_QueryTexture(options_zero_to_9[birth_amount[i]], NULL, NULL, &temp.w, &temp.h);
		temp.x += temp.w + 10;
		SDL_RenderCopy(gfx_renderer, options_zero_to_9[birth_amount[i]], NULL, &temp);
	}
}

void M_DecayMenuDisplay(){
	SDL_Rect temp;
	for (int i = 0; i < current_menu->num_items; i++){
		//gfx-rend, sdl_texture, null, sdl_rect
		temp = current_menu->menu_item_locations[i];
		SDL_QueryTexture(options_zero_to_9[decay_amount[i]], NULL, NULL, &temp.w, &temp.h);
		temp.x += temp.w + 10;
		SDL_RenderCopy(gfx_renderer, options_zero_to_9[decay_amount[i]], NULL, &temp);
	}
}

void M_DelayMenuDisplay(){
	if (delay_allow){
		current_menu->menu_items[0].selected = false;
		current_menu->menu_items[1].selected = true;
	}
	else {
		current_menu->menu_items[0].selected = true;
		current_menu->menu_items[1].selected = false;
	}
	for (int i = 0; i < current_menu->num_items; i++){
		if (current_menu->menu_items[i].selected == true){
			SDL_RenderCopy(gfx_renderer, selected_overlay, NULL, &current_menu->menu_item_locations[i]);
			break;
		}
	}
	SDL_Rect temp;
	temp = current_menu->menu_item_locations[1];
	SDL_QueryTexture(current_menu->menu_items->option_graphic, NULL, NULL, &temp.w, &temp.h);
	temp.x += temp.w + 10;
	SDL_QueryTexture(options_zero_to_9[delay], NULL, NULL, &temp.w, &temp.h);
	SDL_RenderCopy(gfx_renderer, options_zero_to_9[delay], NULL, &temp);
}

void M_BPMDisplay(){
	for (int i = 0; i < current_menu->num_items; i++){
		if (current_menu->menu_items[i].selected == true){
			SDL_RenderCopy(gfx_renderer, selected_overlay, NULL, &current_menu->menu_item_locations[i]);
			break;
		}
	}
}

void M_InstdMenuDisplay(){
	if (instant_death){
		current_menu->menu_items[0].selected = false;
		current_menu->menu_items[1].selected = true;
	}
	else {
		current_menu->menu_items[0].selected = true;
		current_menu->menu_items[1].selected = false;
	}
	for (int i = 0; i < current_menu->num_items; i++){
		if (current_menu->menu_items[i].selected == true){
			SDL_RenderCopy(gfx_renderer, selected_overlay, NULL, &current_menu->menu_item_locations[i]);
			break;
		}
	}
}

void M_ResetMenuDisplay(){
	// empty.
}

void M_PresetMenuChange(int loc, int choice){
	if (choice >= 0){

	}
	else{
		for (int i = 0; i < current_menu->num_items; i++)
			current_menu->menu_items[i].selected = false;
		switch (loc){
		case 0:	
			for (int it = 0; it < 9; it++){birth_amount[it] = default_birth[it]; decay_amount[it] = default_decay[it];}
			instant_death = false; delay_allow = true; delay = 3; break;
		case 1:
			for (int it = 0; it < 9; it++){ birth_amount[it] = game_of_life_birth[it]; decay_amount[it] = game_of_life_decay[it]; }
			instant_death = true; delay_allow = false; delay = 0; break;
		default:break;
		}
		current_menu->menu_items[loc].selected = true;
	}
}

void M_BirthMenuChange(int loc, int choice){
	if (choice == 1){
		if (birth_amount[loc] >= 9)
			birth_amount[loc] = 0;
		else
			birth_amount[loc]++;
	}
	else{
		if (birth_amount[loc] <= 0)
			birth_amount[loc] = 9;
		else
			birth_amount[loc]--;
	}
}

void M_DecayMenuChange(int loc, int choice){
	if (choice == 1){
		if (decay_amount[loc] >= 9)
			decay_amount[loc] = 0;
		else
			decay_amount[loc]++;
	}
	else{
		if (decay_amount[loc] <= 0)
			decay_amount[loc] = 9;
		else
			decay_amount[loc]--;
	}
}

void M_DelayMenuChange(int loc, int choice){
	if (choice >= 0){
		if (loc == 1){
			if (choice == 1){
				if (delay >= 9)
					delay = 0;
				else
					delay++;
			}
			else{
				if (delay <= 0)
					delay = 9;
				else
					delay--;
			}
		}
	}
	else{
		if (loc == 0){
			delay_allow = false;
			current_menu->menu_items[0].selected = true;
			current_menu->menu_items[1].selected = false;
		}
		else{
			delay_allow = true;
			current_menu->menu_items[0].selected = false;
			current_menu->menu_items[1].selected = true;
		}
	}
}

void M_BPMChange(int loc, int choice){
	if (choice >= 0){

	}
	else{
		switch (loc){
		case 0: bpm = 60; break;
		case 1: bpm = 90; break;
		case 2: bpm = 120; break;
		case 3: bpm = 150; break;
		case 4: bpm = 180; break;
		case 5: bpm = 210; break;
		case 6: bpm = 240; break;
		default: break;
		}
		bpmmilis = 60 * 1000 / bpm;
		for (int i = 0; i < current_menu->num_items; i++)
			current_menu->menu_items[i].selected = false;
		current_menu->menu_items[loc].selected = true;
	}
}

void M_InstdMenuChange(int loc, int choice){
	if (choice >= 0){
		
	}
	else{
		if (loc == 0){
			instant_death = true;
			current_menu->menu_items[0].selected = true;
			current_menu->menu_items[1].selected = false;
		}
		else{
			instant_death = false;
			current_menu->menu_items[0].selected = false;
			current_menu->menu_items[1].selected = true;
		}
	}
}

void M_ResetMenuChange(int loc, int choice){
	if (choice >= 0){
		
	}
	else{
		if (current_menu->menu_items[loc].selected == 1){
			int note_mod = 0;
			for (int it_y = 0; it_y < octaves; it_y++){
				for (int it_x = 0; it_x < notes_in_octave; it_x++){
					midi_notes[it_x + it_y*notes_in_octave] = (char)(0x24 + note_mod);
					if (it_x == 2 || it_x == 6)
						note_mod++;
					else note_mod += 2;
				}
			}
			std::cout << "Reset note grid." << std::endl;
		}
		else {
			M_ReturnOneMenu();
		}
	}
}

void item_locations(menuitem_t *menuitem, SDL_Rect* locs, int size, bool right, int cols){
	int col = 0; int col_it = 1;
	int col_limit = size / cols;
	int height = 0;
	for (int i = 0; i < size; i++){
		SDL_QueryTexture(menuitem[i].option_graphic, NULL, NULL, &locs[i].w, &locs[i].h);
		//menu_item_loc[i].x = (grid_width - cell_width * 20) / 4 + cell_width * 10 - menu_item_loc[i].w / 2; // CENTER ALIGN
		if (i >= col_limit){
			col_it++;
			col_limit = size / cols * col_it;
			col++; height = 0;
		}
		if (!right)	locs[i].x = (grid_width - cell_width * 20) / 30 * (1 + col * 15/cols) + cell_width * 10;	// LEFT ALIGN
		else locs[i].x = (grid_width - cell_width * 20) / 30 * (1 + 15 + col * 15 / cols) + cell_width * 10;	// LEFT ALIGN
		locs[i].y = (grid_height - cell_width * 14) / (size/cols + 1) * (height + 1) + cell_width * 7 - locs[i].h / 2;
		height++;
	}
}

void M_ReturnOneMenu(){
	current_menu->last_on = 0;
	current_menu = current_menu->prev_menu;
	menu_location_v = current_menu->last_on;
}

int main(int argc, char *argv[])
{
	// Initialization of graphics and audio.
	if (!gfx_init()) std::cout << "Failed to initalize SDLgfx. Aborting.\n";
	else
	{
		if (TTF_Init() != 0){
			std::cout << TTF_GetError();
			SDL_Quit();
			return 1;
		}
		int font_size = grid_height / 12;
		font_big = TTF_OpenFont("consola.ttf", font_size);
		if (font_big == nullptr){
			std::cout << "Font not found.";
			system("pause");
			return 1;
		}
		font_small = TTF_OpenFont("consola.ttf", 20);

		// This is the name of the Microsoft MIDI Output by Roland.
		// wchar_t *wcstring = L"Microsoft GS Wavetable Synth";
		// This is the virtual soundfonter.
		wchar_t *wcstring;
		
		wcstring = L"CoolSoft VirtualMIDISynth";
		pMIDIOut = MIDIOut_Open(wcstring);
		if (pMIDIOut == NULL)
		{
			std::cout << ("MIDIOut_Open failed on VirtualMIDISynth.\n");
			wcstring = L"Microsoft GS Wavetable Synth";
			pMIDIOut = MIDIOut_Open(wcstring);
			if (pMIDIOut == NULL){
				std::cout << ("MIDIOut_Open failed on Microsoft GS Wavetable Synth.\n");
				system("PAUSE");
				return 0;
			}
		}

		// instr list https://www.midi.org/specifications/item/general-midi
		// recommended instruments: 5, 60-64
		// 0xCchannel, instr. code.
		// 0x00 grand piano, x01 bright piano. 0x18 nylon, x19 steel guitar.
		// 0x69 / x6A shamisen/koto.
		// x30 string ensemble.

		std::string input_text = "";
		SDL_Rect grid_window = { NULL, NULL, NULL, NULL };
		grid_window.h = grid_height;
		grid_window.w = grid_width;
		SDL_Rect tut_window1 = { NULL, NULL, NULL, NULL };
		SDL_Rect tut_window2 = { NULL, NULL, NULL, NULL };
		SDL_Rect tut_window3 = { NULL, NULL, NULL, NULL };
		
		// Possible actions.
		std::string tut_text1 = "     ESC Pause | Enter Select option | Arrow keys Change option | Right CTRL QUIT";
		std::string tut_text2 = "     Backspace Back one menu | R Reset | F Fill random | T Record | H Toggle menu";
		std::string tut_text3 = "     Number keys 1-6 Creation size | 8 Permanent Alive | 9 Permanent Dead | 0 Normal";
		SDL_Texture *tutorial1 = text_to_texture(font_small, tut_text1, false, blue_ish);
		SDL_Texture *tutorial2 = text_to_texture(font_small, tut_text2, false, blue_ish);
		SDL_Texture *tutorial3 = text_to_texture(font_small, tut_text3, false, blue_ish);
		
		tut_window1.y = grid_height + 15;
		SDL_QueryTexture(tutorial1, NULL, NULL, &tut_window1.w, &tut_window1.h);
		tut_window2.y = tut_window1.y + tut_window1.h + 2;
		SDL_QueryTexture(tutorial2, NULL, NULL, &tut_window2.w, &tut_window2.h);
		tut_window3.y = tut_window2.y + tut_window2.h + 2;
		SDL_QueryTexture(tutorial3, NULL, NULL, &tut_window3.w, &tut_window3.h);


		Uint32 grey_pixel[1];
		grey_pixel[0] = 0x19BBBBBB;
		//SDL_QueryTexture(intro_title_t, NULL, NULL, &title_rect.w, &title_rect.h);
		bool text_input = true;
		SDL_RenderClear(gfx_renderer);
		//SDL_SetRenderDrawColor(gfx_renderer, 0, 0, 0, 0);
		//------------------------------------------------------------------------------------
		// Creating title string, declaring instrument number/description.
		std::string title = "Insert number of midi instrument Numbers allowed: 1 to 128";
		SDL_Texture *intro_title_t = text_to_texture(font_big,title, false, blue_ish);
		SDL_Rect title_rect = { NULL, NULL, NULL, NULL };
		SDL_QueryTexture(intro_title_t, NULL, NULL, &title_rect.w, &title_rect.h);
		title_rect.x = grid_width / 2 - title_rect.w / 2; title_rect.y = (grid_height - cell_width * 14) / 4 + cell_width * 7 / 6 - title_rect.h / 4;
		SDL_Surface *instr_num = NULL;	SDL_Texture *instr_num_t = NULL;
		SDL_Surface *instr_name = NULL;	SDL_Texture *instr_name_t = NULL;
		SDL_Rect num_rect = { NULL, NULL, NULL, NULL };
		SDL_Rect name_rect = { NULL, NULL, NULL, NULL };
		//------------------------------------------------------------------------------------
		selected_overlay = SDL_CreateTexture(gfx_renderer,
			SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 1, 1);
		SDL_SetTextureBlendMode(selected_overlay, SDL_BLENDMODE_BLEND);
		SDL_UpdateTexture(selected_overlay, NULL, grey_pixel, sizeof(Uint32));
		SDL_RenderCopy(gfx_renderer, selected_overlay, NULL, NULL);
		SDL_RenderCopy(gfx_renderer, intro_title_t, NULL, &title_rect);
		SDL_RenderPresent(gfx_renderer);
		SDL_PumpEvents();
		SDL_StartTextInput();
		while (!quit && text_input){
			bool render_text = false;
			SDL_RenderClear(gfx_renderer);
			while (SDL_PollEvent(&sdl_event)){
				switch (sdl_event.type){
				case SDL_KEYDOWN:
					switch (sdl_event.key.keysym.sym)
					{
					case SDLK_BACKSPACE:
						if (input_text.length() > 0)
						{
							input_text.pop_back(); render_text = true;
						}
						break;
					case SDLK_RCTRL: std::cout << "Quitting" << std::endl; quit = true; break;
					case SDLK_RETURN: std::cout << "Entered" << std::endl;  text_input = false; break;
					default: break;
					}
					/*
					// Copy/paste mechanism. Ended up with no use for it.
					if (SDLK_c && SDL_GetModState() & KMOD_CTRL)
					{SDL_SetClipboardText(input_text.c_str()); render_text = true;}
					if (SDLK_v && SDL_GetModState() & KMOD_CTRL)
					SDL_GetClipboardText();
					*/
					break;
				case SDL_TEXTINPUT:
					if (!((sdl_event.text.text[0] == 'c' || sdl_event.text.text[0] == 'C') &&
						(sdl_event.text.text[0] == 'v' || sdl_event.text.text[0] == 'V') &&
						SDL_GetModState() & KMOD_CTRL) && (sdl_event.text.text[0] == '1' || sdl_event.text.text[0] == '2' ||
						sdl_event.text.text[0] == '3' || sdl_event.text.text[0] == '4' || sdl_event.text.text[0] == '5' ||
						sdl_event.text.text[0] == '6' || sdl_event.text.text[0] == '7' || sdl_event.text.text[0] == '8' ||
						sdl_event.text.text[0] == '9' || sdl_event.text.text[0] == '0'))
						if (input_text.length() <= 3)
						{
							int temp = std::stoi(input_text.append(sdl_event.text.text));
							if (temp < 129 && temp > 0)
							{
								input_text.pop_back(); input_text.append(sdl_event.text.text); render_text = true;
							}
							else { input_text.pop_back(); }
						}
					break;
				default: break;
				}

				if (render_text){
					if (input_text != "" && !input_text.empty() && input_text.length() > 0){
						create_render_text_dyn(instr_num_t,input_text,
							(grid_width - cell_width * 20) / 4 + cell_width * 10,
							(grid_height - cell_width * 14) / 4 * 3 + cell_width * 7,
							blue_ish);
						create_render_text_dyn(instr_name_t,instruments[std::stoi(input_text) - 1],
							(grid_width - cell_width * 20) / 4 * 3 + cell_width * 10,
							(grid_height - cell_width * 14) / 4 * 3 + cell_width * 7,
							blue_ish);
						SDL_RenderCopy(gfx_renderer, intro_title_t, NULL, &title_rect);
						SDL_RenderPresent(gfx_renderer);
						SDL_PumpEvents();

					}
					else {
						create_render_text_dyn(instr_num_t, " ", NULL, NULL, blue_ish);
						create_render_text_dyn(instr_name_t, " ", NULL, NULL, blue_ish);
						SDL_RenderCopy(gfx_renderer, intro_title_t, NULL, &title_rect);
						SDL_RenderPresent(gfx_renderer);
						SDL_PumpEvents();
					}
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(14));
		}
		if (input_text.empty()) input_text = '1';
		int instrument = std::stoi(input_text);
		instrument = instrument - 1;
		int_to_hex(instrument);
		unsigned char change_instrument[3] = { 0xC0, (char)instrument };
		MIDIOut_PutMIDIMessage(pMIDIOut, change_instrument, 2);

		wchar_t* filename = L"Output.mid";
		output_data = MIDIData_Create(MIDIDATA_FORMAT0, 1, MIDIDATA_TPQNBASE, 120);
		
		track = MIDIData_GetFirstTrack(output_data);
		//MIDITrack_InsertPatchChange(track, 0, 0, 0xC0, 0x3F, 0xC03F);
		MIDITrack_InsertProgramChange(track, 0, (long)0xC0, instrument);
		
		// Construct dimensions.
		new_dimension = new int[dim_len*dim_hei]; //2D in 1 line. Less workload.
		old_dimension = new int[dim_len*dim_hei];
		lifetime = new int[dim_len*dim_hei];


		std::thread timer_thread(timer);
		std::thread calc_thread(calculations);
		std::thread thread_play(play_generation_2D);

		bool lmb_down = false;	bool rmb_down = false;
		bool into_pause = false;	bool display_menu = true;

		SDL_Texture *selected = text_to_texture(font_big,">", false, grey);
		
		
		SDL_Rect select_arrow_loc = { NULL, NULL, NULL, NULL };
		
		SDL_QueryTexture(selected, NULL, NULL, &select_arrow_loc.w, &select_arrow_loc.h);
		
		// MORE menus
		options_zero_to_9 = new SDL_Texture*[10]{
			text_to_texture(font_big,"0", true, grey),
			text_to_texture(font_big,"1", true, grey),
			text_to_texture(font_big,"2", true, grey),
			text_to_texture(font_big,"3", true, grey),
			text_to_texture(font_big,"4", true, grey),
			text_to_texture(font_big,"5", true, grey),
			text_to_texture(font_big,"6", true, grey),
			text_to_texture(font_big,"7", true, grey),
			text_to_texture(font_big,"8", true, grey),
			text_to_texture(font_big,"9", true, grey)
		};

		textures_zero_to_9 = new SDL_Texture*[10]{
			text_to_texture(font_big,"0", true, blue_ish),
			text_to_texture(font_big,"1", true, blue_ish),
			text_to_texture(font_big,"2", true, blue_ish),
			text_to_texture(font_big,"3", true, blue_ish),
			text_to_texture(font_big,"4", true, blue_ish),
			text_to_texture(font_big,"5", true, blue_ish),
			text_to_texture(font_big,"6", true, blue_ish),
			text_to_texture(font_big,"7", true, blue_ish),
			text_to_texture(font_big,"8", true, blue_ish),
			text_to_texture(font_big,"9", true, blue_ish)
		};

		menuitem_t main_menu_items[7] = {
			{ false, text_to_texture(font_big,"Presets", true, blue_ish) },
			{ false, text_to_texture(font_big,"Birth", true, blue_ish) },
			{ false, text_to_texture(font_big,"Decay", true, blue_ish) },
			{ false, text_to_texture(font_big,"Birth delay", true, blue_ish) },
			{ false, text_to_texture(font_big,"Beats per minute", true, blue_ish) },
			{ false, text_to_texture(font_big,"Instant death", true, blue_ish) },
			{ false, text_to_texture(font_big,"Reset notes", true, blue_ish) }
		};

		menuitem_t preset_menu_items[2] = {
			{ true, text_to_texture(font_big,"Default Rule", true, blue_ish) },
			{ false, text_to_texture(font_big,"Game of Life", true, blue_ish) }
		};

		menuitem_t birth_menu_items[8] = {
			{ false, textures_zero_to_9[1] },
			{ false, textures_zero_to_9[2] },
			{ false, textures_zero_to_9[3] },
			{ false, textures_zero_to_9[4] },
			{ false, textures_zero_to_9[5] },
			{ false, textures_zero_to_9[6] },
			{ false, textures_zero_to_9[7] },
			{ false, textures_zero_to_9[8] }
		};

		menuitem_t decay_menu_items[9] = {
			{ false, textures_zero_to_9[0] },
			{ false, textures_zero_to_9[1] },
			{ false, textures_zero_to_9[2] },
			{ false, textures_zero_to_9[3] },
			{ false, textures_zero_to_9[4] },
			{ false, textures_zero_to_9[5] },
			{ false, textures_zero_to_9[6] },
			{ false, textures_zero_to_9[7] },
			{ false, textures_zero_to_9[8] }
		};

		menuitem_t delay_menu_items[2] = {
			{ false, text_to_texture(font_big,"Disable delay", true, blue_ish) },
			{ true, text_to_texture(font_big,"Enable delay", true, blue_ish) }
		};

		menuitem_t beats_per_minute_items[7] = {
			{ false, text_to_texture(font_big,"60", true, blue_ish) },
			{ false, text_to_texture(font_big,"90", true, blue_ish) },
			{ true, text_to_texture(font_big,"120", true, blue_ish) },
			{ false, text_to_texture(font_big,"150", true, blue_ish) },
			{ false, text_to_texture(font_big,"180", true, blue_ish) },
			{ false, text_to_texture(font_big,"210", true, blue_ish) },
			{ false, text_to_texture(font_big,"240", true, blue_ish) },
		};

		menuitem_t instd_menu_items[2] = {
			{ false, text_to_texture(font_big,"Yes", true, blue_ish) },
			{ true, text_to_texture(font_big,"No", true, blue_ish) },
		};

		menuitem_t reset_menu_items[2] = {
			{ true, text_to_texture(font_big,"Confirm reset", true, blue_ish) },
			{ false, text_to_texture(font_big, "Cancel reset", true, blue_ish) },
		};

		SDL_Rect* main_menu_item_loc = new SDL_Rect[ARRAY_SIZE(main_menu_items)];
		item_locations(main_menu_items, main_menu_item_loc, ARRAY_SIZE(main_menu_items), false, 1);

		SDL_Rect* preset_menu_item_loc = new SDL_Rect[ARRAY_SIZE(preset_menu_items)];
		item_locations(preset_menu_items, preset_menu_item_loc, ARRAY_SIZE(preset_menu_items), true, 1);

		SDL_Rect* birth_menu_item_loc = new SDL_Rect[ARRAY_SIZE(birth_menu_items)];
		item_locations(birth_menu_items, birth_menu_item_loc, ARRAY_SIZE(birth_menu_items), true, 2);

		SDL_Rect* decay_menu_item_loc = new SDL_Rect[ARRAY_SIZE(decay_menu_items)];
		item_locations(decay_menu_items, decay_menu_item_loc, ARRAY_SIZE(decay_menu_items), true, 3);

		SDL_Rect* delay_menu_item_loc = new SDL_Rect[ARRAY_SIZE(delay_menu_items)];
		item_locations(delay_menu_items, delay_menu_item_loc, ARRAY_SIZE(delay_menu_items), true, 1);

		SDL_Rect* beats_per_minute_loc = new SDL_Rect[ARRAY_SIZE(beats_per_minute_items)];
		item_locations(beats_per_minute_items, beats_per_minute_loc, ARRAY_SIZE(beats_per_minute_items), true, 1);

		SDL_Rect* instd_menu_item_loc = new SDL_Rect[ARRAY_SIZE(instd_menu_items)];
		item_locations(instd_menu_items, instd_menu_item_loc, ARRAY_SIZE(instd_menu_items), true, 1);

		SDL_Rect* reset_menu_item_loc = new SDL_Rect[ARRAY_SIZE(reset_menu_items)];
		item_locations(reset_menu_items, reset_menu_item_loc, ARRAY_SIZE(reset_menu_items), true, 1);

		menu_t main_menu = {
			7,
			NULL,
			main_menu_items,
			main_menu_item_loc,
			NULL,
			M_MainMenuChange,
			NULL
		};

		menus = new menu_t[7];

		menu_t preset_menu = {
			2,
			NULL,
			preset_menu_items,
			preset_menu_item_loc,
			NULL,
			M_PresetMenuChange,
			M_PresetMenuDisplay
		};

		menu_t birth_menu = {
			8,
			NULL,
			birth_menu_items,
			birth_menu_item_loc,
			NULL,
			M_BirthMenuChange,
			M_BirthMenuDisplay
		};

		menu_t decay_menu = {
			9,
			NULL,
			decay_menu_items,
			decay_menu_item_loc,
			NULL,
			M_DecayMenuChange,
			M_DecayMenuDisplay
		};

		menu_t delay_menu = {
			2,
			NULL,
			delay_menu_items,
			delay_menu_item_loc,
			NULL,
			M_DelayMenuChange,
			M_DelayMenuDisplay
		};

		menu_t bpm_menu = {
			7,
			NULL,
			beats_per_minute_items,
			beats_per_minute_loc,
			NULL,
			M_BPMChange,
			M_BPMDisplay
		};

		menu_t instd_menu = {
			2,
			NULL,
			instd_menu_items,
			instd_menu_item_loc,
			NULL,
			M_InstdMenuChange,
			M_InstdMenuDisplay
		};

		menu_t reset_menu = {
			2,
			NULL,
			reset_menu_items,
			reset_menu_item_loc,
			NULL,
			M_ResetMenuChange,
			M_ResetMenuDisplay
		};

		menus[0] = preset_menu;
		menus[1] = birth_menu;
		menus[2] = decay_menu;
		menus[3] = delay_menu;
		menus[4] = bpm_menu;
		menus[5] = instd_menu;
		menus[6] = reset_menu;

		current_menu = &main_menu;
		// Menus over

		while (!quit)
		{
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						//ACTIONS AT ANY POINT: PAUSE, RESET, RECORD, FILL, CLICK SIZE, QUIT.
					case SDLK_ESCAPE:	paused = !paused; into_pause = true;
						if (paused){ current_menu->prev_menu = NULL; display_menu = true; current_menu = &main_menu; std::cout << "Paused." << std::endl; }
						else std::cout << "Unpaused." << std::endl; break;
					case SDLK_p:		paused = !paused; into_pause = true;
						if (paused){ current_menu->prev_menu = NULL; current_menu = &main_menu; std::cout << "Paused." << std::endl; }
						else std::cout << "Unpaused." << std::endl; break;
					case SDLK_RCTRL:	quit = true;
						std::cout << "Quitting." << std::endl; break;
					case SDLK_r:
						for (int it = 0; it < dim_len; it++)
							for (int ij = 0; ij < dim_hei; ij++)
								new_dimension[it + ij * dim_len] = 0;
						std::cout << "Reset CA grid." << std::endl; break;
					case SDLK_t:
						if (recording){
							recording = false; 
							MIDITrack_InsertEndofTrack(track, record_len); // insert end-of-track
							record_len = 0; 
							MIDIData_SaveAsSMF(output_data, filename);  // Save to standard MIDI file 
							MIDIData_Delete(output_data);  // delete MIDI data 
							std::cout << "Recorded to Output.midi." << std::endl;
						}
						else {
							recording = true;
							output_data = MIDIData_Create(MIDIDATA_FORMAT0, 1, MIDIDATA_TPQNBASE, bpm);
							track = MIDIData_GetFirstTrack(output_data);
							MIDITrack_InsertProgramChange(track, 0, 0, instrument);
							std::cout << "Recording to Output.midi." << std::endl; 
						}
						break;
					case SDLK_f:
						for (int it = 0; it < dim_len; it++)
							for (int ij = 0; ij < dim_hei; ij++){
								float rndm = rand() / (float)RAND_MAX;
								if (rndm > 0.5)
									new_dimension[it + ij * dim_len] = 4; // Need randomization.
							}
						std::cout << "Filled field." << std::endl; break;
					case SDLK_1: click_size = 0; std::cout << "Click size " + std::to_string(click_size + 1) << std::endl; break;
					case SDLK_2: click_size = 1; std::cout << "Click size " + std::to_string(click_size + 1) << std::endl; break;
					case SDLK_3: click_size = 2; std::cout << "Click size " + std::to_string(click_size + 1) << std::endl; break;
					case SDLK_4: click_size = 3; std::cout << "Click size " + std::to_string(click_size + 1) << std::endl; break;
					case SDLK_5: click_size = 4; std::cout << "Click size " + std::to_string(click_size + 1) << std::endl; break;
					case SDLK_6: click_size = 5; std::cout << "Click size " + std::to_string(click_size + 1) << std::endl; break;
					case SDLK_8: left_click_health = 100; std::cout << "Left click - permanent cell." << std::endl; break;
					case SDLK_9: left_click_health = -100; std::cout << "Left click - restriction field." << std::endl; break;
					case SDLK_0: left_click_health = 8; std::cout << "Left click - 8 health." << std::endl; break;
					//ACTIONS AT PAUSE: CHANGE RULES, GRAVEMODE
					case SDLK_h:
						if (paused){
							display_menu = !display_menu;
							std::cout << "Toggling menu." << std::endl;
						}
						break;
					case SDLK_UP:
						if (paused)
							if(menu_location_v > 0)
								menu_location_v--;
							else menu_location_v = current_menu->num_items - 1;
						break;
					case SDLK_DOWN:
						if (paused){
							if(menu_location_v < current_menu->num_items - 1)
								menu_location_v++;
							else menu_location_v = 0;
						}
						break;
					case SDLK_LEFT:
						if (paused) current_menu->routine(menu_location_v, 0);
						break;
					case SDLK_RIGHT:
						if (paused) current_menu->routine(menu_location_v, 1);
						break;
					case SDLK_RETURN:
						if (paused){ 
							current_menu->routine(menu_location_v, -1);
						} break;
					case SDLK_BACKSPACE:
						if (paused){
							if (current_menu->prev_menu != NULL){
								M_ReturnOneMenu();
							}
						} break;
					default: break;
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					switch (event.button.button)
					{
					case SDL_BUTTON_LEFT:
						affect_cells_with_mouse(true, event, event.button.x, event.button.y);
						lmb_down = true;
						break;
					case SDL_BUTTON_RIGHT:
						affect_cells_with_mouse(false, event, event.button.x, event.button.y);
						rmb_down = true;
						break;
					default: break;
					}
					break;
				case SDL_MOUSEBUTTONUP:
					switch (event.button.button)
					{
					case SDL_BUTTON_LEFT: lmb_down = false; break;
					case SDL_BUTTON_RIGHT: rmb_down = false; break;
					}
					break;
				case SDL_MOUSEMOTION:
					if (lmb_down) affect_cells_with_mouse(true, event, event.motion.x, event.motion.y);
					if (rmb_down) affect_cells_with_mouse(false, event, event.motion.x, event.motion.y);
					break;
				}

			}
			/*
			// White and Black
			for (int it_x = 0; it_x < dim_len; it_x++)
			for (int it_y = 0; it_y < dim_hei; it_y++)
			for (int ver_p = 0; ver_p < cell_width; ver_p++)
			for (int hor_p = 0; hor_p < cell_width; hor_p++)
			if (new_dimension[it_x + (it_y*dim_len)] - '0' > 0){
				switch (new_dimension[it_x + (it_y*dim_len)] - '0'){
				case 4: pixels[it_x*cell_width + hor_p + ((it_y*cell_width + ver_p)*grid_width)] = 0xFFB9B9B9; break;
				case 3: pixels[it_x*cell_width + hor_p + ((it_y*cell_width + ver_p)*grid_width)] = 0xFF969696; break;
				case 2: pixels[it_x*cell_width + hor_p + ((it_y*cell_width + ver_p)*grid_width)] = 0xFF636363; break;
				case 1: pixels[it_x*cell_width + hor_p + ((it_y*cell_width + ver_p)*grid_width)] = 0xFFFFFFFF; break;
				default: pixels[it_x*cell_width + hor_p + ((it_y*cell_width + ver_p)*grid_width)] = 0xFFD3D3D3; break;
				}
			}
			else
			pixels[it_x*cell_width + hor_p + ((it_y*cell_width + ver_p)*grid_width)] = 0xFF000000;
			*/

			//Pastel blue
			for (int it_x = 0; it_x < dim_len; it_x++)
				for (int it_y = 0; it_y < dim_hei; it_y++)
					for (int ver_p = 0; ver_p < cell_width; ver_p++)
						for (int hor_p = 0; hor_p < cell_width; hor_p++)
							if (new_dimension[it_x + (it_y*dim_len)] > 0){
								switch (new_dimension[it_x + (it_y*dim_len)]){
								case 100:  pixels[it_x*cell_width + hor_p + ((it_y*cell_width + ver_p)*grid_width)] = 0xFF83A1C1; break;
								case 4: pixels[it_x*cell_width + hor_p + ((it_y*cell_width + ver_p)*grid_width)] = 0xFF425B79; break;
								case 3: pixels[it_x*cell_width + hor_p + ((it_y*cell_width + ver_p)*grid_width)] = 0xFF4B678A; break;
								case 2: pixels[it_x*cell_width + hor_p + ((it_y*cell_width + ver_p)*grid_width)] = 0xFF54739B; break;
								case 1: pixels[it_x*cell_width + hor_p + ((it_y*cell_width + ver_p)*grid_width)] = 0xFF708DB1; break;
								default: pixels[it_x*cell_width + hor_p + ((it_y*cell_width + ver_p)*grid_width)] = 0xFF334363; break;
								}
							}
							else if (new_dimension[it_x + (it_y*dim_len)] < 0){
								if (new_dimension[it_x + (it_y*dim_len)] == -100)
									pixels[it_x*cell_width + hor_p + ((it_y*cell_width + ver_p)*grid_width)] = 0xFF333333;
								else 
									pixels[it_x*cell_width + hor_p + ((it_y*cell_width + ver_p)*grid_width)] = 0xFF220000;
							}
							else if (it_x % (dim_len / notes_in_octave) == 0 || it_y % (dim_hei / octaves) == 0) pixels[it_x*cell_width + hor_p + ((it_y*cell_width + ver_p)*grid_width)] = 0xFF1F2036;
							else pixels[it_x*cell_width + hor_p + ((it_y*cell_width + ver_p)*grid_width)] = 0xFF17181A;

		SDL_UpdateTexture(gfx_texture_ca, NULL, pixels, grid_width * sizeof(Uint32));
		SDL_SetRenderDrawColor(gfx_renderer, 0, 0, 0, 0);
		SDL_RenderClear(gfx_renderer);
		SDL_RenderCopy(gfx_renderer, gfx_texture_ca, NULL, &grid_window);

		// If paused, overlay menu over automaton grid.
		if (paused){
			if (into_pause){
				for (int it_x = cell_width * 7; it_x < grid_width - cell_width * 7; it_x++)
					for (int it_y = cell_width * 4; it_y < grid_height - cell_width * 4; it_y++){
						bool border = false;
						if (it_x < cell_width * 10 || it_y < cell_width * 7) border = true;
						if (it_x > grid_width - cell_width * 10 - 1 || it_y > grid_height - cell_width * 7 - 1) border = true;
						if (border)
							menu[it_x + it_y*grid_width] = 0xFF101010;
						else
							menu[it_x + it_y*grid_width] = 0xBB000000;
					}
				into_pause = false;
			}
			if (display_menu){
				SDL_SetTextureBlendMode(gfx_texture_over, SDL_BLENDMODE_BLEND);
				SDL_UpdateTexture(gfx_texture_over, NULL, menu, grid_width * sizeof(Uint32));
				SDL_RenderCopy(gfx_renderer, gfx_texture_over, NULL, &grid_window);

				if (current_menu->prev_menu != NULL){
					for (int i = 0; i < current_menu->prev_menu->num_items; i++){
						SDL_RenderCopy(gfx_renderer, current_menu->prev_menu->menu_items[i].option_graphic, NULL, &current_menu->prev_menu->menu_item_locations[i]);
					}
				}
				if (current_menu == &main_menu){
					for (int i = 0; i < menus[menu_location_v].num_items; i++)
						SDL_RenderCopy(gfx_renderer, menus[menu_location_v].menu_items[i].option_graphic, NULL, &menus[menu_location_v].menu_item_locations[i]);

				}
				else {
					current_menu->display();
					select_arrow_loc.x =
						current_menu->prev_menu->menu_item_locations[current_menu->prev_menu->last_on].x +
						current_menu->prev_menu->menu_item_locations[current_menu->prev_menu->last_on].w;
					select_arrow_loc.y =
						current_menu->prev_menu->menu_item_locations[current_menu->prev_menu->last_on].y;
					SDL_RenderCopy(gfx_renderer, selected, NULL, &select_arrow_loc);
				}

				select_arrow_loc.x =
					current_menu->menu_item_locations[menu_location_v].x -
					select_arrow_loc.w;
				select_arrow_loc.y =
					current_menu->menu_item_locations[menu_location_v].y;
				SDL_RenderCopy(gfx_renderer, selected, NULL, &select_arrow_loc);
				for (int i = 0; i < current_menu->num_items; i++){
					//SDL_RenderCopy(gfx_renderer, main_menu_items[i].option_graphic, NULL, &menu_item_loc[i]);
					SDL_RenderCopy(gfx_renderer, current_menu->menu_items[i].option_graphic, NULL, &current_menu->menu_item_locations[i]);
				}
				
			}
		}
		else {
			if (!into_pause) into_pause = true;
		}
		
		SDL_RenderCopy(gfx_renderer, tutorial1, NULL, &tut_window1);
		SDL_RenderCopy(gfx_renderer, tutorial2, NULL, &tut_window2);
		SDL_RenderCopy(gfx_renderer, tutorial3, NULL, &tut_window3);
		std::this_thread::sleep_for(std::chrono::milliseconds(6));
		SDL_RenderPresent(gfx_renderer);
		SDL_PumpEvents();
		}

		if (recording){
			recording = false;
			MIDITrack_InsertEndofTrack(track, record_len); // insert end-of-track
			record_len = 0; // FIND A WAY TO CHANGE INSTRUMENTS
			MIDIData_SaveAsSMF(output_data, filename);  // Save to standard MIDI file 
			MIDIData_Delete(output_data);  // delete MIDI data 
			std::cout << "Recorded to Output.midi." << std::endl;
		}

		// Finish threads
		calc_thread.join();
		thread_play.join();
		timer_thread.join();

		// Delete data / free memory.
		delete[] population_hex;
		delete[] new_dimension;
		delete[] old_dimension;
		delete[] lifetime;

		// Shutting down
		pMIDIOut = NULL;

		gfx_close();
	}
	return 0;
}
