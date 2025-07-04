#include "mui.h"

int main() {
  // Option 1: Declare MUI interface and samplers using specialisms (i.e. 1 = 1 dimensional, d = double)
  mui::uniface1d interface( "mpi://pong/ifs" );
  mui::sampler_exact1d<int> spatial_sampler;
  mui::temporal_sampler_exact1d temporal_sampler;
  mui::point1d push_point;
  mui::point1d fetch_point;

  // Option 2: Declare MUI interface and samplers using templates in config.h
  // note: please update types stored in default_config in config.h first to 1-dimensional before compilation
  //mui::uniface<mui::default_config> interface( "mpi://pong/ifs" );
  //mui::sampler_exact<mui::default_config> spatial_sampler;
  //mui::temporal_sampler_exact<mui::default_config> temporal_sampler;
  //mui::point<mui::default_config::REAL, 1> push_point;
  //mui::point<mui::default_config::REAL, 1> fetch_point;

  //int state;

 // for ( int t = 0; t < 100; ++t ) {
    // Fetch the value from the interface (blocking until data at "t" exists according to temporal_sampler)
   // fetch_point[0] = 0;
   // state = interface.fetch( "data", fetch_point, t, spatial_sampler, temporal_sampler );
   // state--;
    // Push value stored in "state" to the MUI interface
   // push_point[0] = 0;
   // interface.push( "data", push_point, state );
    // Commit (transmit by MPI) the value
   // interface.commit( t );
 // }

 // printf( "Final pong state: %d\n", state );

  return 0;
}
