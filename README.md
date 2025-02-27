# Pico WWVB

Pico WWVB is a single board computer which gets the time via [NTP](https://en.wikipedia.org/wiki/Network_Time_Protocol) and transmits a WWVB signal to synchronize radio controlled clocks. It uses the [Raspberry Pi Pico W](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html).

This is a simplification of a previous project which used GPS to obtain the time.  See [micro-wwvb](https://github.com/chgenly/micro-wwvb).  No PC board has to
be fabricated, many fewer components to solder, and no worries about whether or
not you can receive GPS inside a house.

![pico-wwvb board with antenna](images/pico_wwvb_board_with_antenna.jpg)
Pico WWVB with antenna 

![pico-wwvb in a case](images/pico_wwvb_in_case.jpg)
I used a food container as the case.

## The Radio Controlled Clocks

I have three radio controlled clocks.

- Marathon
- Oregon Scientific
- Casio 2688 Watch

I can set all three clocks with the Pico WWVB board. The Marathon is the most
sensitive and can be the furthest away, about 8 inches.  I've found that the
watch and the Oregon Scientific work well, but only if they are away from
my monitor. 

![My radio controlled clocks](images/radio_controlled_clocks.jpg)

## The lights

There are three LEDs used to report status of the board. These LEDs should be interpreted as a binary number.
 The most significant bit is the leftmost one, the one furthest away from the USB connector. (as seen in the 
 pictures of the board above). 
 This table indicates the meaning of the displayed values.  You will see a count down, and then
 as each step completes its value will be shown. If an error occurs on a step the LEDs will
 flash the value of the failed step.  If a value lingers, it means the next step is taking
 some time.

| *Display* | *Meaning*                                    |
| --------- | ---------                                    |
| Rapidly flashing 7 through 0 | This is the 8 second startup countdown. Counting down from 7 to 0.
| 1 | The state structure has been allocated               |
| 2 | The WIFI subsystem has been initialized              |
| 3 | A UDP receive handler was setup.                     |
| 4 | WIFI login was successful                            |
| 5 | A DNS response for the time server has been received |
| 6 | NTP time received                                    |

The pico's built-in led is used to show when the WWVB signal is sending at high power.  So it changes once a second.

If the LEDs are slowly flashing, then an error has occurred.  Lookup the number in the table above to understand which step failed.

The startup delay is to give the user time to start a terminal, such as putty, to look at serial output over the USB port.

## Windows Development environment

### Environment variables

You have to set two environment variables. WIFI_SSID and WIFI_PASSWORD. These are the ssid and password used by the pico's cygw43 chip to login to your wifi network.  The cmake file will refuse to build Pico WWVB until you set these.

You can set them using using windows settings, or you can edit the file env.cmake.  

Setting windows environment variables:
- Press the windows key, type "environment", 
- select "Edit the system environment variables".  A dialog pops up.
- Click on the "Environment Variables" button.
- Click "New..." and add your environment variables.
- Restart visual studio code so it sees the new environment variables.

Settings environment variables using env.cmake:
- Edit env.cmake.
- Uncomment the set commands.
- Change the values.
- Save
- No need to restart visual studio code.

### Raspberry Pi Pico SDK

Upon starting visual studio code, it will ask if you want to install the recommended extensions.  Answer yes to this and it will install the raspberry pi pico extension which will in turn install the Pico SDK.  This may take a while.

## To Build from source

Open the Raspberry PI PicoProject view.  The icon looks like a pico chip. Click on Clean CMake.  You won't have to do this again.  Now click on Compile Project.

There is also a test_dow you can run which tests the day of week code.

If you don't see the CMAKE icon in the VS code action bar, make sure
you have the PICO SDK installed.  Then you can try Ctrl-Shift-P CMAKE: reset.
When VS code asks for a kit, select "unspecified".

## Programming the pico.

The simplest way to reprogram the Pico W’s flash is to use the USB mode. To do this, power-down the board, then hold
the BOOTSEL button down during board power-up (e.g. hold BOOTSEL down while connecting the USB). The Pico W will
then appear as a USB mass storage device. Dragging a special '.uf2' file onto the disk will write this file to the flash and restart the Pico W.

- Remove power from the pico.
- press the BOOTSEL button on the pico.
- Plug your pico into your computer.
- release the button.
- A folder should open up showing the contents of the pico flash.
- Copy build/src/pico_wwvb.uf2

## Parts

| Name                |Price| URL                      |
| ------------------- | ------ | --------------------- |
| CANADUINO® 60kHz Fine Tuned Loop Stick Antenna for Atomic Clock Receiver    | $4.99  | [amazon](https://www.amazon.com/CANADUINO-60kHz-Antenna-Atomic-Receiver/dp/B07PK7WJYR/ref=sr_1_1?crid=11EAPWFGVDHZ2&keywords=60khz+antenna&qid=1697772066&s=electronics&sprefix=60khz+antenn%2Celectronics%2C154&sr=1-1) |
| male to female pin cables (20)	| $2.10 | [digikey](https://www.digikey.com/en/products/detail/sparkfun-electronics/PRT-12794/5993859) |
| 3 pin connector for antenna |	$0.37 | PPPC031LFBN-RC 	[digikey](https://www.digikey.com/en/products/detail/sullins-connector-solutions/PPPC031LFBN-RC/810175) |
| Three Green LEDs | |
| Three 500 ohm resistors || |
| Capacitor 10uf cer |	$0.49 |	FG18X5R1E106MRT06 [digikey](https://www.digikey.com/en/products/detail/samsung-electro-mechanics/CL31A106KOHNNNE/3886795https://www.digikey.com/en/products/detail/tdk-corporation/FG18X5R1E106MRT06/7384735) |
| micro usb cable. | | |
| PI debug probe | $12.00 | [Debug probe](https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html), [Sparkfun](https://www.sparkfun.com/raspberry-pi-debug-probe.html) |

The debug probe is optional.  I found it very useful while developing the code.

I also bought a $30 [FNIRSI oscilliscope](https://www.amazon.com/dp/B0CD1WKP33/ref=pe_386300_440135490_TE_simp_item_image) so I could check the signal at the antenna.  It was a great help in debugging.

## The build

### LEDs

Cut down the resistor leads to about 1/4 inch long.
Cut the LED positive lead to about 1/4 long.
Solder a resistor in-line with the LED positive lead.
Slide the the resistor wire into the GPIO 10 hole until the resistor body touches the board. Slide the negative LED wire into the ground next to it. Solder the wires into the holes, cut the excess wire.

Repeat these steps for GPIO 11, and 12.  I connected the ground ends of these two LEDs to the ground wire of the first LED.

These three lights are the status LEDs.  The most
significant bit is on GPIO 12.

![Status LEDs](images/status_leds.jpg)

### Antenna wires

Take two of the male to female pin cables.  Cut the wire about two inches away from the male connector. (Throw away the female side.)  Solder one of these into the hole for GPIO 2, and the other to the adjacent hole for ground.

![pico-wwvb board](images/pico_wwvb_board.jpg)
PICO W board showing LEDs and antenna connection. Picture taken before antenna was moved to GPIO 2.

### Antenna

Solder the 10 &micro;F capacitor in line with one of the antenna leads.  Solder the other antenna lead to the three pin header.  Solder the free side of the capacitor to another pin on the header.

You can now plug the pins from the wires connected to the board into the antenna connector.

## Software

The build is performed by CMake.  The src folder contains
the main software, the test folder contains some small tests.

**pico_wwvb.c** is the main source file.  This initializes the board and runs the top level loop of get time via NTP, and then start a ten minute broadcast.

Just before starting the broadcast, execution is delayed
until a one second boundary is reached.

**pico_ntp_client.c**  This is a simple implementation of 
the ntp protocol.  It logs into wifi, makes a DNS request
to get an ntp pool machine IP address, and then finally 
makes the ntp request itself.  The time between 
transmission and reception of the NTP packet is divided by 
two to get an estimate of the transmission delay. This 
delay is then added to the time returned in the NTP packet
to make it a bit more accurate.

The network operations are in a loop which will retry 
the operations if an NTP response is not received in
30 seconds.

**wwvb_led.c** initializes LED hardware and controls
reporting status and errors.

**wwvb_pwm.c** Interface to send the high power and low
power phases of an NTP bit.

## Debug

Follow the instructions on the rapsberry pi site for setting up the debug probe.


# printf doesn't work with tiny usb

These are notes on tracking down why printf doesn't
work when tinyusb is used.

Printf sends character to a device via driver.  The `stdio_set_driver_enabled` call adds and removes drivers from the driver list.
```c
        stdio_set_driver_enabled(&stdio_usb, true);
```

This is called from these:

`bool stdio_usb_init(void)` // In stdio_usb.c

`void stdio_semihosting_init()` //In stdio_semihosting

`void stdio_uart_init_full(struct uart_inst *uart, uint baud_rate, int tx_pin, int rx_pin)` //In stdio_uart.c

Which is called from `stdin_uart_init()`