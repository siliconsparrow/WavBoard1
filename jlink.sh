#!/bin/bash

# This will launch the JLink console and pre-set the parameters for the Kinetis KL17 chip.
# To flash your firmware you will need to manually type these two commands:
#
#   connect
#   loadfile <hex_file>

JLinkExe -device MKL17Z32xxx4 -if SWD

#JLinkExe -device MKL17Z32VLH4 -if SWD -speed 1000 -SelectEmuBySN 440067444

