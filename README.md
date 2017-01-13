Ball Pit
========

This repository contains code for an Arduino ball-counting device to be used in your family ball pit.

Hardware
--------

The hardware used includes:

* [POP-BOT](https://www.sparkfun.com/products/retired/9501) with ATmega168 microcontroller
* SLCD16x2 Serial LCD module
* GP2D120 Infrared distance sensor
* Two on-board buttons
* Two external buttons

Setup
-----

The infrared sensor should be mounted in such a way that all balls entering the ball pit pass in front of it. The `BALL_DISTANCE_THRESHOLD` variable should be configured appropriately for the position of the sensor.

Modes
-----

* Normal mode:
    Counts the amount of balls successfully get
* Timed mode:
    Counts how many balls you can get within a time limit (60s)
* Target mode:
    Times how long it takes you to successfully get a target number of balls (50)
