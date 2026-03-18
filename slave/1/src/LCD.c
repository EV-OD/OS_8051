#include "LCD.h"
#include "LCD_COMMAND.h"

void lcd_init(void) {
    lcd_clear();

    // 2. Set Font (Hardware Pins)
    font_selection(LCD_6x8_FONT);

    // 3. Set Text Home Address (Start at RAM 0x0000)
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_command(0x40); // TX HOME ADDRESS SET

    // 4. Set Text Area (Width of the screen in characters)
    // For 240 pixels wide / 8-dot font = 30 columns
    // For 240 pixels wide / 6-dot font = 40 columns
    lcd_write_data(40); 
    lcd_write_data(0x00);
    lcd_write_command(0x41); // TX AREA SET

    // 5. Set Graphic Home Address (Start after Text RAM, e.g., 0x1000)
    lcd_write_data(0x00);
    lcd_write_data(0x10);
    lcd_write_command(0x42); // GR HOME ADDRESS SET

    // 6. Set Graphic Area
    lcd_write_data(40); 
    lcd_write_data(0x00);
    lcd_write_command(0x43); // GR AREA SET

    // 7. Finally, turn the Display ON
    // 0x94 = Text ON, Graphics OFF
    // 0x98 = Graphics ON, Text OFF
    // 0x9C = Both ON
    lcd_write_command(0x94); 
    
    lcd_clear_ram(); // Recommended: Fill RAM with 0x00 to avoid "garbage" on boot
}
void lcd_clear(void){
    LCD_RST = 0;
    // delay_ms(1); // Give it time to stabilize
    LCD_RST = 1;
    // delay_ms(1);
}
void lcd_write_command(unsigned char cmd){
    wait_ready();
    LCD_CD = 1;      // Command mode
    LCD_DATA = cmd;
    LCD_RD = 1;
    LCD_WR = 0;
    LCD_CE = 0;
    _nop_(); 
    LCD_WR = 1;
    LCD_CE = 1;
}
void lcd_write_data(unsigned char data){
    wait_ready();
    LCD_CD = 0;      // Data mode
    LCD_DATA = data;
    LCD_RD = 1;
    LCD_WR = 0;
    LCD_CE = 0;
    _nop_();         // Timing delay
    LCD_WR = 1;
    LCD_CE = 1;
}
unsigned char lcd_status_read(void) {
    unsigned char s;
    LCD_DATA = 0xFF; // Set Port 0 for input (standard 8051 practice)
    LCD_CD = 1;      // Command mode
    LCD_RD = 0;      // Start Read
    LCD_CE = 0;      
    s = LCD_DATA;    // Read status
    LCD_CE = 1;
    LCD_RD = 1;
    return s;
}

void wait_ready(void) {
    // Check STA0 and STA1 (Bits 0 and 1)
    while ((lcd_status_read() & 0x03) != 0x03); 
}

unsigned int lcd_data_read(void){
    setup_data_read();
    unsigned int data = LCD_DATA; // Read the data from the data bus
    return data;
}


void enable_lcd(void){
    LCD_CE = 0; // Enable the LCD by setting CE low
}
void disable_lcd(void){
    LCD_CE = 1; // Disable the LCD by setting CE high
}


void set_mode_command_or_data(unsigned char is_command){
    if (is_command) {
        LCD_CD = 1; // Set C/D high for command mode
    } else {
        LCD_CD = 0; // Set C/D low for data mode
    }
}

void setup_write(void){
    enable_lcd();
    LCD_WR = 0; // Set WR low to indicate a write operation
    LCD_RD = 1; // Ensure RD is high during write operations
}
void setup_read(void){
    enable_lcd();
    LCD_RD = 0; // Set RD low to indicate a read operation
    LCD_WR = 1; // Ensure WR is high during read operations
}

void setup_command_write(void){
    set_mode_command_or_data(1); // Set to command mode
    setup_write(); // Prepare for writing
}
void setup_data_write(void){
    set_mode_command_or_data(0); // Set to data mode
    setup_write(); // Prepare for writing
}
void setup_command_read(void){
    set_mode_command_or_data(1); // Set to command mode
    setup_read(); // Prepare for reading
}
void setup_data_read(void){
    set_mode_command_or_data(0); // Set to data mode
    setup_read(); // Prepare for reading
}





unsigned char status_check(void){
    if(is_lcd_ready_for_command() && is_lcd_ready_for_data_or_read()) {
        return 1;
    } else {
        return 0;
    }
}

unsigned char auto_mode_status_check(void){
    unsigned int status = lcd_status_read(); // Read and return the status from the LCD
    unsigned char STA3 = (status >> 3) & 0x01; // Extract STA3 from the status
    unsigned char STA2 = (status >> 2) & 0x01; // Extract STA2 from the status
    if (STA3 == 1 && STA2 == 1) {
        return 1; // Both STA3 and STA2 are 1, ready for auto mode data read/write
    } else {
        return 0; // Not ready for auto mode data read/write
    }
}


unsigned int is_lcd_ready_for_command(void){
    unsigned int status = lcd_status_read(); // Read the status from the LCD
    unsigned char STA0 = status & 0x01; // Extract STA0 from the status

    if (STA0 == 1) {
        return 1; // LCD is ready for command write
    } else {
        return 0; // LCD is not ready for command write
    }
}
unsigned int is_lcd_ready_for_data_or_read(void){
    unsigned int status = lcd_status_read(); // Read the status from the LCD
    unsigned char STA1 = (status >> 1) & 0x01; // Extract STA1 from the status

    if (STA1 == 1) {
        return 1; // LCD is ready for data write
    } else {
        return 0; // LCD is not ready for data write
    }
}

void display_text_only(void){
    lcd_write_command(LCD_CMD_TEXT_ON_GRAPHIC_OFF); // Set display mode to text on, graphic off
}
void display_graphic_only(void){
    lcd_write_command(LCD_CMD_TEXT_OFF_GRAPHIC_ON); // Set display mode to text off, graphic on
}
void display_both(void){
    lcd_write_command(LCD_CMD_TEXT_ON_GRAPHIC_ON); // Set display mode to text on, graphic on
}


void set_address(unsigned char lowAddr, unsigned char highAddr){

    lcd_write_data(lowAddr);
    lcd_write_data(highAddr);

    if(!is_lcd_ready_for_command()) {
        return;
    }

    lcd_write_command(LCD_CMD_SET_ADDRESS_POINTER); 
}

void display_char_text_mode(unsigned char c){
    // display_text_only();
    // set_address(0x00, 0x00);
    lcd_write_data(c);
    lcd_write_command(LCD_CMD_DATA_WRITE_INCREMENT_ADP);
}


void font_selection(unsigned char font_type){
    if (font_type == LCD_6x8_FONT) {
        LCD_MD2 = 0;
        LCD_FS1 = 1;
    } else if (font_type == LCD_8x8_FONT) {
        LCD_MD2 = 1;
        LCD_FS1 = 0;
    }
}

void lcd_clear_ram(void) {
    unsigned int i;
    set_address(0x00, 0x00); // Start at beginning
    lcd_write_command(0xB0); // SET AUTO WRITE MODE
    for(i = 0; i < 8192; i++) {
        while((lcd_status_read() & 0x08) == 0); // Wait for STA3 (Auto Write Ready)
        lcd_write_data(0x00); // Send 0x00 without checking normal status
    }
    lcd_write_command(0xB2); // AUTO RESET (Crucial!)
}

void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 120; j++); // Adjust 120 based on your crystal frequency
}