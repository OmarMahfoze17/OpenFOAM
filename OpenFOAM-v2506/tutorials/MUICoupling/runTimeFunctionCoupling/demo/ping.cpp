#include "mui.h"
#include "system/muiconfig.h"
int main() {
  mui::mpi_split_by_app();
  // Option 1: Declare MUI interface and samplers using specialisms (i.e. 1 = 1 dimensional, d = double)
  printf( "Ping started ....");
  
  std::vector<std::unique_ptr<mui::uniface<mui::mui_config>>> mui_ifs;
  mui::sampler_exact<mui::mui_config> spatial_sampler;
  mui::temporal_sampler_exact<mui::mui_config> temporal_sampler;
  mui::point3d push_point;
  mui::point3d fetch_point;  
  
  std::vector<std::string> ifsName;
  ifsName.emplace_back("ifs");
  mui_ifs=mui::create_uniface<mui::mui_config>( "ping", ifsName );
	    
  double state = 50.0;

  for ( int t = 0; t < 2; ++t ) {
    
    state++;
    // Push value stored in "state" to the MUI interface
    push_point[0] = 0;
    push_point[1] = 0;
    push_point[2] = 0;
    mui_ifs[0]->push( "dataFromPing", push_point, state );
    // Commit (transmit by MPI) the value
    mui_ifs[0]->commit( t );
    // Fetch the value from the interface (blocking until data at "t" exists according to temporal_sampler)
    fetch_point[0] = 0;
    fetch_point[1] = 0;
    fetch_point[2] = 0;
    state = mui_ifs[0]->fetch( "dataFromOF", fetch_point, t, spatial_sampler, temporal_sampler );
    std::cout << " xxxxxxxxxxxxxx ping at time   " << t << " fitched  "<< state <<std::endl;
  }
  
  // printf( "Final pong state: %d\n", state );
  printf( "Ping finished ....\n");
  //for (int i = 0; i >-1; ++i)
    //        {
      //          int j; 
        //        j=i;
        
          //  }
  return 0;
}
