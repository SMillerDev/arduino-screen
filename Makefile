simple-build:
	arduino-cli compile -b arduino:renesas_uno:minima --libraries libraries simple --verbose
	arduino-cli upload -b arduino:renesas_uno:minima simple --verbose

attach:
	arduino-cli monitor -p /dev/cu.usbmodem3101 -c baudrate=9600
