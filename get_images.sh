#!/usr/bin/env bash

# Copy images from BeagleBoard SD card.
# We have SSH public key authentication setup.

rsync -avzP sean@$BEAGLE_IP:/media/sdcard/camera/ scratch/beagle
