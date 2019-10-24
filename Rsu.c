#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "TrafficLightState.h"

int main( int argc, char* argv[] )
{
  /* Receive traffic light state */
  //pthread_t thread_of_recv_traffic_light_state;
	//pthread_create( &thread_of_recv_traffic_light_state, NULL, recv_traffic_light_state, NULL );
  recv_traffic_light_state();
}
