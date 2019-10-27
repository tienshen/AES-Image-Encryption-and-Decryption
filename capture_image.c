#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aes.c"

#define VIDEO_HEIGHT          320
#define VIDEO_WIDTH           240
#define PIXELS_PER_BLOCK      8

#define KEY_BASE              0xFF200050
#define SWITCH_BASE           0xFF200040
#define VIDEO_IN_BASE         0xFF203060
#define FPGA_ONCHIP_BASE      0xC8000000
#define CHAR_BUFFER           0xC9000000
#define LED_BASE              0xFF200000

const uint8_t AES_KEY[16] =  { (uint8_t) 0x2b, (uint8_t) 0x7e, (uint8_t) 0x15, (uint8_t) 0x16,
							   (uint8_t) 0x28, (uint8_t) 0xae, (uint8_t) 0xd2, (uint8_t) 0xa6, 
							   (uint8_t) 0xab, (uint8_t) 0xf7, (uint8_t) 0x15, (uint8_t) 0x88, 
							   (uint8_t) 0x09, (uint8_t) 0xcf, (uint8_t) 0x4f, (uint8_t) 0x3c };

// Function prototype
void put_jtag(char);
void print_uart(char*);
void load_menu();
void encrypt_image(int);
void decrypt_image(int);

/* This program demonstrates the use of the D5M camera with the DE1-SoC Board
 * It performs the following: 
 * 	1. Capture one frame of video when any key is pressed.
 * 	2. Display the captured frame when any key is pressed.		  
*/
/* Note: Set the switches SW1 and SW2 to high and rest of the switches to low for correct exposure timing while compiling and the loading the program in the Altera Monitor program.
*/
int main(void)
{
	// Test JTAG UART port
	char text_string[] = "\nJTAG_UART_TEST\n";
	char encrypt_finish[] = "FINISHED ENCRYPT\n";
	volatile char *str, c;

	// UART transmit the string
	for (str = text_string; *str != 0; ++str) {
		put_jtag(*str);
	}


	volatile int * KEY_ptr				= (int *) KEY_BASE;
	volatile int * LED_ptr = (int *) LED_BASE;
	volatile int * SWITCH_ptr = (int *) SWITCH_BASE;
	volatile int * Video_In_DMA_ptr	= (int *) VIDEO_IN_BASE;
	volatile short * Video_Mem_ptr	= (short *) FPGA_ONCHIP_BASE;
	volatile int * Video_Char_ptr = (int *) CHAR_BUFFER;

	int x, y, state;
	*(Video_In_DMA_ptr + 3)	= 0x4;				// Enable the video

	while (1)
	{

		load_menu();

		if ((*SWITCH_ptr & 0x00000001) == 0)						// check if any KEY was pressed
		//if ((*LED_ptr & 0x00000001) != 0x0)
		{
			// UART transmit the string
			for (str = text_string; *str != 0; ++str) {
				put_jtag(*str);
			}
			//print_uart(&text_string);
			*(Video_In_DMA_ptr + 3) = 0x0;		// Disable the video to capture one frame

			/* CODE HERE FOR AES */

			for (str = encrypt_finish; *str != 0; ++str) {
				put_jtag(*str);
			}
			//la;sjfdsal;kjfgda;slkf
			load_menu();

			while ((*SWITCH_ptr & 0x00000001) == 0)				// wait for pushbutton KEY release
			{
				if (*SWITCH_ptr == 0x2) 		// encrypt when switch 1 is on
				{
					encrypt_image(1);
					state = 1;
					while (*SWITCH_ptr == 0x2);
				}

				if (state == 1)					// decrypt when switch is turned off
				{
					decrypt_image(1);
					state = 0;
				}

				if (*SWITCH_ptr == 0x4)			// encry when switch 2 is on
				{
					encrypt_image(0);
					state = 2;
					while (*SWITCH_ptr == 0x4);

				}
				if (state == 2)					// decrypt when switch is turned off
					decrypt_image(0);
					state = 0;


			}


			*(Video_In_DMA_ptr + 3)	= 0x4;				// Enable the video
			//break;
		}

		*LED_ptr = *SWITCH_ptr;
	}

	// accessing pixel buffer
	for (y = 0; y < VIDEO_WIDTH; y++) {
		for (x = 0; x < VIDEO_HEIGHT; x++) {
			//short temp2 = *(Video_Mem_ptr + (y << 9) + x);
			//*(Video_Mem_ptr + (y << 9) + x) = temp2;
		}
	}
	
	// using AES encryption and decryption
	
	// 128 bit key
	uint8_t key[16] =        { (uint8_t) 0x2b, (uint8_t) 0x7e, (uint8_t) 0x15, (uint8_t) 0x16,
							   (uint8_t) 0x28, (uint8_t) 0xae, (uint8_t) 0xd2, (uint8_t) 0xa6, 
							   (uint8_t) 0xab, (uint8_t) 0xf7, (uint8_t) 0x15, (uint8_t) 0x88, 
							   (uint8_t) 0x09, (uint8_t) 0xcf, (uint8_t) 0x4f, (uint8_t) 0x3c };
							 
	// placeholder for plaintext
	uint8_t plaintext[16] =  { (uint8_t) 0x6b, (uint8_t) 0xc1, (uint8_t) 0xbe, (uint8_t) 0xe2, 
							   (uint8_t) 0x2e, (uint8_t) 0x40, (uint8_t) 0x9f, (uint8_t) 0x96,
							   (uint8_t) 0xe9, (uint8_t) 0x3d, (uint8_t) 0x7e, (uint8_t) 0x11,
							   (uint8_t) 0x73, (uint8_t) 0x93, (uint8_t) 0x17, (uint8_t) 0x2a };
							   
	// init AES with key
	struct AES_ctx ctx;
    AES_init_ctx(&ctx, key);
	
	// ecb
	AES_ECB_encrypt(&ctx, plaintext);
	AES_ECB_decrypt(&ctx, plaintext);
	
	// cbc , note that 16 is the blockchain length, do not change
	AES_CBC_encrypt_buffer(&ctx, plaintext,16);
	AES_CBC_decrypt_buffer(&ctx, plaintext,16);

}

/********************************************************************************
* Function for ENCRYPTING the pixel buffer with AES
* Default operation is encrypting in ECB mode
* When isCBC=1, decrypts in ECB mod
********************************************************************************/
void encrypt_image(int isCBC) {
	// init AES with key
	struct AES_ctx ctx;
    AES_init_ctx(&ctx, AES_KEY);

	volatile short * Video_Mem_ptr	= (short *) FPGA_ONCHIP_BASE;
	//short unencrypted_pixels[8];
	uint8_t plaintext[16] =  { (uint8_t) 0x6b, (uint8_t) 0xc1, (uint8_t) 0xbe, (uint8_t) 0xe2, 
							   (uint8_t) 0x2e, (uint8_t) 0x40, (uint8_t) 0x9f, (uint8_t) 0x96,
							   (uint8_t) 0xe9, (uint8_t) 0x3d, (uint8_t) 0x7e, (uint8_t) 0x11,
							   (uint8_t) 0x73, (uint8_t) 0x93, (uint8_t) 0x17, (uint8_t) 0x2a };

	uint8_t pixel_val1 = 0; // keep track of value of current pixel in 2 variables
	uint8_t pixel_val2 = 0;
	int pixel_cnt = 0; // keeps track of the current number of pixels in the plaintext buffer, since we only can process 8 at a time

	// array to store 

	// accessing pixel buffer
	int x = 0;
	int y = 0;
	short * start_block_ptr = Video_Mem_ptr + (y << 9) + x;
	for (y = 0; y < VIDEO_WIDTH; y++) {
		for (x = 0; x < VIDEO_HEIGHT; x++) {

			if (pixel_cnt == PIXELS_PER_BLOCK*2) {

				//uint8_t *plaintext = (uint8_t *) unencrypted_pixels; // Reinterpret memory for AES

				// Do AES encryption
				if (isCBC) AES_CBC_encrypt_buffer(&ctx, plaintext, 16);
				else AES_ECB_encrypt(&ctx, plaintext);

				//short *encrypted_pixels = (short *) plaintext; // Reinterpret memory for setting pixels back

				// set pixel values in VideoMemPtr
				unsigned int i;
				for (i = 0; i < PIXELS_PER_BLOCK*2; i+=2) {
					*(start_block_ptr + i/2) = plaintext[i] << 8 | plaintext[i+1];
				}

				// Reset pixel counter
				pixel_cnt = 0;
				start_block_ptr = Video_Mem_ptr + (y << 9) + x;
			}
			short temp2 = *(Video_Mem_ptr + (y << 9) + x);
			pixel_val1 = (temp2 >> 8) & 0xFF; // left value in val1
			pixel_val2 = (temp2) & 0xFF; // right value in val2
			plaintext[pixel_cnt] = pixel_val1;
			pixel_cnt++;
			plaintext[pixel_cnt] = pixel_val2;
			pixel_cnt++;
		}
	}
}

void decrypt_image(int isCBC) {
	// init AES with key
	struct AES_ctx ctx;
    AES_init_ctx(&ctx, AES_KEY);

	volatile short * Video_Mem_ptr	= (short *) FPGA_ONCHIP_BASE;
	//short unencrypted_pixels[8];
	uint8_t plaintext[16] =  { (uint8_t) 0x6b, (uint8_t) 0xc1, (uint8_t) 0xbe, (uint8_t) 0xe2, 
							   (uint8_t) 0x2e, (uint8_t) 0x40, (uint8_t) 0x9f, (uint8_t) 0x96,
							   (uint8_t) 0xe9, (uint8_t) 0x3d, (uint8_t) 0x7e, (uint8_t) 0x11,
							   (uint8_t) 0x73, (uint8_t) 0x93, (uint8_t) 0x17, (uint8_t) 0x2a };
							   
	uint8_t pixel_val1 = 0; // keep track of value of current pixel
	uint8_t pixel_val2 = 0;
	int pixel_cnt = 0; // keeps track of the current number of pixels in the plaintext buffer, since we only can process 8 at a time

	// array to store 

	// accessing pixel buffer
	int x = 0;
	int y = 0;
	short * start_block_ptr = Video_Mem_ptr + (y << 9) + x;
	for (y = 0; y < VIDEO_WIDTH; y++) {
		for (x = 0; x < VIDEO_HEIGHT; x++) {

			if (pixel_cnt == PIXELS_PER_BLOCK*2) {

				//uint8_t *plaintext = (uint8_t *) unencrypted_pixels; // Reinterpret memory for AES

				// Do AES encryption
				if (isCBC) AES_CBC_decrypt_buffer(&ctx, plaintext, 16);
				else AES_ECB_decrypt(&ctx, plaintext);

				//short *encrypted_pixels = (short *) plaintext; // Reinterpret memory for setting pixels back

				// set pixel values in VideoMemPtr
				unsigned int i;
				for (i = 0; i < PIXELS_PER_BLOCK*2; i+=2) {
					*(start_block_ptr + i/2) = plaintext[i] << 8 | plaintext[i+1]; // put encrypted text back into the memory address
				}

				// Reset pixel counter
				pixel_cnt = 0;
				start_block_ptr = Video_Mem_ptr + (y << 9) + x;
			}
			short temp2 = *(Video_Mem_ptr + (y << 9) + x);
			pixel_val1 = (temp2 >> 8) & 0xFF; // left value in val1
			pixel_val2 = (temp2) & 0xFF; // right value in val2
			plaintext[pixel_cnt] = pixel_val1; //pixel count increments by 2 for each loop because each pixel is split up and store into 2 variables
			pixel_cnt++;
			plaintext[pixel_cnt] = pixel_val2;
			pixel_cnt++;
		}
	}
}
/********************************************************************************
* Function for DECRYPTING the pixel buffer with AES
* Default operation is decrypting in ECB mode
* When isCBC=1, decrypts in ECB mode
********************************************************************************/
// void decrypt_image(int isCBC) {

// }

/********************************************************************************
* Subroutine to send a character to the JTAG UART
********************************************************************************/
void put_jtag(char c ) {
	volatile int* JTAG_UART_ptr = (int *) 0xFF201000; // JTAG UART address
	
	int control;
	control = *(JTAG_UART_ptr + 1); // read the JTAG_UART control register
	if(control & 0xFFFF0000) {      // if space, echo character, else ignore
		*(JTAG_UART_ptr) = c;
	}
}

/********************************************************************************
* Subroutine to send a character to the JTAG UART
********************************************************************************/
void print_uart(char *str) {
	// UART transmit the string
	volatile char *str_cnt;
	for (str_cnt = str; *str_cnt != 0; ++str) {
		put_jtag(*str_cnt);
	}
}

/*********************************************************************************
* Function to draw menu over the camera image
*********************************************************************************/
void load_menu() {
	volatile int * SWITCH_ptr = (int *) SWITCH_BASE;
	volatile char * Video_Char_ptr = (char *) CHAR_BUFFER;
	volatile char *str;
	
	// Check the values of the SW9, SW8, and SW7
	// SW0 is for pausing/unpausing
	// SW1 is for encrypting/decrypting AES CBC mode
	// SW2 is for encrypting/decrypting AES ECB mode

	// Bits for these switches are 1st, 2nd, and 3rd in register
	int paused  = (*SWITCH_ptr & 0x1) != 0;
	int isCBC = (*SWITCH_ptr & 0x2) != 0;
	int isECB = (*SWITCH_ptr & 0x4) != 0;

	char paused_message[] = "Pause/Unpause: SW0\0";
	char cbc_message[] = "AES-CBC Encrypt/Decrypt: SW1\0";
	char ecb_message[] = "AES-ECB Encrypt/Decrypt: SW2\0";

	int x1 = 1;
	int y1 = 1;
	int offset = (y1<<7) + x1;

	str = paused_message;
	while (*str) {
		*(Video_Char_ptr + offset) = *str;
		++str;
		++offset;

	}

	y1 = 2;
	offset = (y1<<7) + x1;
	str = cbc_message;
	while (*str) {
		*(Video_Char_ptr + offset) = *str;
		++str;
		++offset;

	}

	y1 = 3;
	offset = (y1<<7) + x1;
	str = ecb_message;
	while (*str) {
		*(Video_Char_ptr + offset) = *str;
		++str;
		++offset;

	}

}


