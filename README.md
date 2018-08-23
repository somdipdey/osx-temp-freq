# OSX CPU Temperature and Frequency

Outputs current CPU temperature (in °C) and frequency (in Hz) for OSX.

## Build for OSX:
[![Build Status](https://travis-ci.org/somdipdey/osx-temp-freq.svg?branch=master)](https://travis-ci.org/somdipdey/osx-temp-freq)
[![License: MIT](https://img.shields.io/badge/License-MIT-red.svg)](https://github.com/somdipdey/osx-temp-freq/blob/master/LICENSE)

## Usage

### Compiling

```bash
make
```

### Running

```bash
./osx-temp-freq
```

or

```bash
./osx-temp-freq my_output_file.csv # Output frequency and temperature inside a file in the same directory as the program with the specified file name, `my_output_file.csv`
```

or

```bash
sudo make install # installs to /usr/local/bin
osx-temp-freq
```

### Output example

```
CPU Temperature at 0:1:45 : 53.9°C
CPU Frequency at 0:1:45 : 2599944981
```

### Options

 * `my_output_file.csv` Output frequency and temperature inside a file in the same directory as the program with the specified file name, `my_output_file.csv`.

## Maintainer

Somdip Dey, <somdip.dey@essex.ac.uk>


### Inspiration

 * https://github.com/lavoiesl/osx-cpu-temp
 * http://www.eidac.de/smcfancontrol/
 * https://github.com/hholtmann/smcFanControl/tree/master/smc-command
