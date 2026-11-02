BUILD_DIR := build
OUTPUT_FILE := out

source 	:= src/main
out	   	:= $(BUILD_DIR)/$(OUTPUT_FILE)
INC_DIR := include

all:
	mkdir -p $(BUILD_DIR)
	avr-gcc -Os -DF_CPU=16000000UL -mmcu=atmega328p -I$(INC_DIR) -c -o $(out).o $(source).c
	avr-gcc -mmcu=atmega328p -o $(out).bin $(out).o
	avr-objcopy -O ihex -R .eeprom  $(out).bin  $(out).hex

clean:
	rm -rf $(BUILD_DIR)

flash:
	sudo avrdude -F -V -c arduino -p atmega328p -P /dev/ttyACM0 -b 115200 -U flash:w:$(out).hex