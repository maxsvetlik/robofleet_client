# robofleet_client

*The Robofleet 2.0 CPP Client*

This client pulls robot data from the robofleet server and receives it as a 
C++ struct. 

## Dependencies

* Make sure to clone submodules, or `git submodule init` and `git submodule update`
* C++ compiler for std >= 11
* CMake
* ROS Melodic
* Qt5 >= 5.5

## Configuration

The client must be configured before building.

1. Edit parameters in `src/config.hpp`

The config file is slim and specifies what robot status topics you want to listen for 
on robofleet. You will need to know the ROS_NAMESPACE that the robot client will 
be publishing to robofleet.


### Standalone Simulation

Start a local robofleet_server instance by cd into the `robofleet_server` directory and 
running `yarn start`

Start robot robofleet_client 0, (in the AMRL-main branch robofleet_server):

   $ ROS_NAMESPACE="maxbot0" make run

And publish the data locally

   $ ./scripts/basic_status.py

which are the steps to simulate robofleet normally. Now, to run the cpp robofleet client

   $ make run

in this repository root
