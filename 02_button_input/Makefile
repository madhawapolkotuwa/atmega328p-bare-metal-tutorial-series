BUILD_DIR := build
OUTPUT_FILE := out

source := src/main
out	   := $(BUILD_DIR)/$(OUTPUT_FILE)

all:
	mkdir -p $(BUILD_DIR)
	avr-gcc -Os -mmcu=atmega328p -DF_CPU=16000000UL -o $(out).elf $(source).c
	avr-objcopy -O ihex -R .eeprom $(out).elf $(out).hex

clean:
	rm -rf $(BUILD_DIR)

flash:
	sudo avrdude -F -V -c arduino -p atmega328p -P /dev/ttyACM0 -b 115200 -U flash:w:$(out).hex