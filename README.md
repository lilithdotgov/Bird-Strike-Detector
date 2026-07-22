# Bird-Strike-Detector
Maintained by the Urban Ecology Lab (UEL) at the New Jersey Institute of Technology (NJIT)

### This project is a work-in-progress!

## Description
Formally known as the *Looking Glass*, it is an accelerometer-based device for detecting when a bird flies into a window. This repository will eventually hold all the files needed for one to create their own device and monitor bird strikes! Further information with a step-by-step assembly guide will eventually be hosted on another website.

## Background
Bird strikes are one of the leading causes of human-driven mass mortality of avian species, with an estimated 1 Billion fatalities each year from window collisions in the United States alone. Yet, key details of these events such as the exact window or time of collision are not known as most studies rely on witnessing the aftermath of a strike (e.g., a carcass or blood on the window). Our *Looking Glass* devices intend to bridge this gap in knowledge by being an affordable, long-lasting solution to this problem. 

We implore any and all researchers or citizen scientists to try creating their own device (when this repository is properly updated) and capture data as well!

## Mode of Operation
The *Looking Glass* is simple, it currently consists of a microcontroller (Pico 2 W), an accelerometer (ADXL343), some batteries (NiMH), an enclosure, and suction cups which allow it to attach to a window. It is designed to go to a low-power state and wake up when the accelerometer detects a spike in its data stream. It then records the accelerometer data, stores it locally as a binary file, timestamps it via NTP, and attempts to send the data to a github repository for storage. If it fails to send the data it will be stored locally, and upon the next time a strike occurs it will attempt to send both the new data and old data to the github repository.

This data can then be later turned into a `.csv` for analysis to remove erroneous data points or make inferences on strike patterns.

## Assembly
As it currently stands neither the full step-by-step instructions nor all the necessary files for assembly are currently hosted on this repository. This is partly due to a current effort to further improve the design and assembly process before committing to a proper guide. However, please reach out privately to [lilithdotgov](https://github.com/lilithdotgov) to discuss interest in assembly. 

Currently, if planning to create a decent batch size (20+) the cost per device is roughly $20–$30 USD (may vary with taxes and shipping costs). Although, the price is very flexible as many parts could be replaced with other parts at a cost of convenience, likewise, an easier assembly can be achieved for a greater cost. Simply having access to a 3d printer would eliminate as much as 1/4th the material costs and much of the assembly process. However, efforts will be made to provide individuals with many different options depending on whether or not they wish to prioritize cost or assembly time.

## Directory
`pico` contains all the code that each device is to be flashed with. Currently it is written in MicroPython but a C port is in the works which will replace it.

`analysis` currently only contains `bin2csv.py` which converts the `.bin` files that contain strike data into a `.csv`. 

`dependencies` is currently empty.

### License
We have yet to choose a license, when all the materials for assembly are added expect to see an update in that regard!
