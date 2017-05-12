## xbm2nokia
Converting [XBM](https://en.wikipedia.org/wiki/X_BitMap)
images to raw data for Nokia 3310/5110 LCDs

See also:
* [the accompanying blog article about this](http://sgreg.fi/blog/article/xbm-images-and-animations-on-a-nokia-lcd)
* [blog article about exaple code optimization](http://sgreg.fi/blog/article/code-optimization-for-xbm-images-on-nokia-lcd)

### Introduction

xbm2nokia is a shell script that creates raw data for Nokia 3310/5110 LCD
from a given set of XBM files. The created data can be either a set of
individual graphics, which then contains the full image graphic, or as an
animation, in which case the created data is only the pixel diff between
frames. Of course, the full data images can be used as well to create
animations on the display. However, the animation mode might be more space
efficient (exceptions exist though).

Although the main component is the `xbm2nokia.sh` shell script, the actual
conversion is done in C, since XBM images are in the end just C code, too.
So in the end, the shell script is generating additional C code and compiles
a binary for each individual XBM image. The generated binary itself will
generate yet more C code, which the shell script is collecting and writing
into the main output files, `nokia_gfx.c` and `nokia_gfx.h`. Those files
can then be used to display images on the LCD.

The generated files are targeted for AVR microcontrollers, but adjusting
the xbm2nokia code to suit for other targets shouldn't be too difficult.
The only actual AVR specific part is the added `PROGMEM` declaration to
store the data in program memory when compiling.

### Creating image data

Obviously some XBM images are required to run the script, and a Nokia LCD
to actually test it. There's an example included for an AVR ATmega328,
more on that a bit below.

Ideally, the images are 84x48 pixels, as this is the resolution of the
Nokia LCD. Other resolutions can be used, but this will require more
adjustments later on to copy the image data to the LCD memory. If the
images are 84x48 pixels, the whole image data can be just `memcpy`'d as is.

So let's say we have three images: `test1.xbm`, `test2.xbm` and `test3.xbm`.
To generate three, individual images out of this, the script is called with
the `-g` parameter:
```
./xbm2nokia.sh -g /path/to/test1.xbm /path/to/test2.xbm /path/to/test3.xbm
```
This will result in `nokia_gfx.c` and `nokia_gfx.h` files in the current
directory, with the `.h` files containing declarations like this:
```
#define NOKIA_GFX_COUNT 3

extern const uint8_t nokia_gfx_test1[];
extern const uint8_t nokia_gfx_test2[];
extern const uint8_t nokia_gfx_test3[];
```
and the `.c` file containing the actual data then. These are all just raw
`char` arrays with the image data.

To create an animation (it really just is frame transition diff data,
or however you want to call it), the script is called with the `-a`
parameter:
the `-g` parameter:
```
./xbm2nokia.sh -g /path/to/test1.xbm /path/to/test2.xbm /path/to/test3.xbm
```
This will again create the `nokia_gfx.c` and `nokia_gfx.h` files, but
this time the variables are structured like this:
```
#define NOKIA_GFX_ANIMATION
...
#define NOKIA_GFX_FRAME_COUNT 3

extern const uint8_t nokia_gfx_keyframe[];
extern const struct nokia_gfx_frame nokia_gfx_trans_test1_test2;
extern const struct nokia_gfx_frame nokia_gfx_trans_test2_test3;
extern const struct nokia_gfx_frame nokia_gfx_trans_test3_test1;
```
In addition, some data structure definitions are added as well, and the
data itself is a bit more complex than it was before. Except the
`nokia_gfx_keyframe` `char` array, this is again just full screen data
as before. The other `struct`s contain all individual pixel changes to
get from the keyframe `test1.xbm` to the next frame `test2.xbm`, from
there to `test3.xbm` and finally back to `test1.xbm` to have a full loop.

How to actually use the data is probably best understood by studying the
included example's code.

### Running the example

The example is targeted for an ATmega328 running at 8MHz, using the
avr-gcc toolchain and `make` for building, as well as an USBasp programmer.

The directory contains:
* some simple XBM images `x1.xbm`...`x9.xbm` of a crappy drawn face with
  moving eyes
* Nokia LCD control and helper functions `nokia_lcd.c` taking care of
  the actual data transfer
* the main example program `example.c`, which is probably the best starting
  point to study how the data is used. Also contains the ATmega pinout.
* `Makefile` for compiling and flashing

The example handles both, the individual graphics and the animations.
Either option is chosen based on the `NOKIA_GFX_ANIMATION` preprocessor
flag.

To actually test the example, connect a Nokia 5110 LCD to an ATmega328
as commented in the `example.c` file header.

First step, get to the `example` directory and generate the iamge data:
```
$ cd example
$ ../xbm2nokia.sh -a *.xbm
Creating animation
creating keyframe from x1.xbm
creating frame x1.xbm -> x2.xbm
creating frame x2.xbm -> x3.xbm
creating frame x3.xbm -> x4.xbm
creating frame x4.xbm -> x5.xbm
creating frame x5.xbm -> x6.xbm
creating frame x6.xbm -> x7.xbm
creating frame x7.xbm -> x8.xbm
creating frame x8.xbm -> x9.xbm
creating frame x9.xbm -> x1.xbm
$
```

The animation mode is probably more interesting to study, but creating
individual graphics instead is also possible, just use the `-g` parameter
instead. The example will handle either way - and you can also see how the
binary size will differ, while both modes result in the same LCD content.

Once the data is generated, everything else is set up. All it takes is to
compile it with `make`.

```
$ make
avr-gcc -c -g -Os -std=gnu99 -I. -Iusbdrv -I../lib -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -Wall -Wextra -Wstrict-prototypes -DF_CPU=8000000 -mmcu=atmega328p  -Wa,-adhlms=example.lst,-gstabs  example.c -o example.o
avr-gcc -c -g -Os -std=gnu99 -I. -Iusbdrv -I../lib -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -Wall -Wextra -Wstrict-prototypes -DF_CPU=8000000 -mmcu=atmega328p  -Wa,-adhlms=nokia_lcd.lst,-gstabs  nokia_lcd.c -o nokia_lcd.o
avr-gcc -c -g -Os -std=gnu99 -I. -Iusbdrv -I../lib -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -Wall -Wextra -Wstrict-prototypes -DF_CPU=8000000 -mmcu=atmega328p  -Wa,-adhlms=nokia_gfx.lst,-gstabs  nokia_gfx.c -o nokia_gfx.o
avr-gcc -g -Os -std=gnu99 -I. -Iusbdrv -I../lib -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -Wall -Wextra -Wstrict-prototypes -DF_CPU=8000000 -mmcu=atmega328p  example.o nokia_lcd.o nokia_gfx.o -o example.elf -Wl,-Map=example.map,--cref
avr-objcopy -O ihex -R .eeprom example.elf example.hex
   text    data     bss     dec     hex filename
   2210       0     523    2733     aad example.elf
$
```

All that's left is to flash the ATmega. The `Makefile` assumes the USBasp
is used for programming, you may need to adjust this.

```
$ make program
avrdude -p atmega328p -c usbasp -U flash:w:example.hex
...
avrdude done.  Thank you.
$
```

That's it. The display should show now some crappy face moving its eyes.
Next step could be using the other mode and re-create all image. In that
case, it is a good idea to clean the build directory before compiling.
So once `xbm2nokia.sh` finishes, run either `make clean` or `make distclean`
before running `make` again.

