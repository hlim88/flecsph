/*~--------------------------------------------------------------------------~*
 * Copyright (c) 2017 Los Alamos National Security, LLC
 * All rights reserved.
 *~--------------------------------------------------------------------------~*/

 /*~--------------------------------------------------------------------------~*
 * 
 * /@@@@@@@@  @@           @@@@@@   @@@@@@@@ @@@@@@@  @@      @@
 * /@@/////  /@@          @@////@@ @@////// /@@////@@/@@     /@@
 * /@@       /@@  @@@@@  @@    // /@@       /@@   /@@/@@     /@@
 * /@@@@@@@  /@@ @@///@@/@@       /@@@@@@@@@/@@@@@@@ /@@@@@@@@@@
 * /@@////   /@@/@@@@@@@/@@       ////////@@/@@////  /@@//////@@
 * /@@       /@@/@@//// //@@    @@       /@@/@@      /@@     /@@
 * /@@       @@@//@@@@@@ //@@@@@@  @@@@@@@@ /@@      /@@     /@@
 * //       ///  //////   //////  ////////  //       //      //  
 *
 *~--------------------------------------------------------------------------~*/

/**
 * @file main_driver.cc
 * @author Julien Loiseau
 * @date April 2017
 * @brief Specialization and Main driver used in FleCSI. 
 * The Specialization Driver is normally used to register data and the main 
 * code is in the Driver.  
 */

#include <iostream>
#include <numeric> // For accumulate
#include <iostream>

#include <mpi.h>
#include <legion.h>
#include <omp.h>

#include "flecsi/execution/execution.h"
#include "flecsi/data/data_client.h"
#include "flecsi/data/data.h"

#include <bodies_system.h>

#include "physics/eos_analytics.h"
#include "physics/physics.h"

namespace flecsi{
namespace execution{

void
mpi_init_task(int startiteration){
  // TODO find a way to use the file name from the specialiszation_driver
  
  int rank;
  int size;
  MPI_Comm_size(MPI_COMM_WORLD,&size);
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  
  int totaliters = 30000;
  int iteroutput = 50;
  double totaltime = 0.0;
  double maxtime = 10.0;
  int iter = startiteration; 

  // Init if default values are not ok
  physics::dt = 0.01;
  physics::alpha = 1; 
  physics::beta = 2; 
  physics::stop_boundaries = true;
  physics::min_boundary = {0.1};
  physics::max_boundary = {1.0};
  physics::gamma = 5./3.;

  body_system<double,gdimension> bs;
  bs.read_bodies("dwd_id.h5part",startiteration);
  //io::inputDataHDF5(rbodies,"hdf5_sodtube.h5part",totalnbodies,nbodies);

  //eos_analytics eos(1.4);

  double h = bs.getSmoothinglength();
  physics::epsilon = 0.01*h*h;

#ifdef OUTPUT
  bs.write_bodies("output_dwd",iter);
  //io::outputDataHDF5(rbodies,"output_sodtube.h5part",0);
  //tcolorer.mpi_output_txt(rbodies,iter,"output_sodtube"); 
#endif

  ++iter; 
  do
  { 
    MPI_Barrier(MPI_COMM_WORLD);
    if(rank==0)
      std::cout<<std::endl<<"#### Iteration "<<iter<<std::endl;
    MPI_Barrier(MPI_COMM_WORLD);

    // Compute and prepare the tree for this iteration 
    // - Compute the Max smoothing length 
    // - Compute the range of the system using the smoothinglength
    // - Cmopute the keys 
    // - Distributed qsort and sharing 
    // - Generate and feed the tree
    // - Exchange branches for smoothing length 
    // - Compute and exchange ghosts in real smoothing length 
    bs.update_iteration();
   
    // Do the DWD physics
    if(rank==0)
      std::cout<<"Density"<<std::flush; 
    bs.apply_in_smoothinglength(physics::compute_density);
    if(rank==0)
      std::cout<<".done"<<std::endl;

    if(rank==0)
      std::cout<<"Pressure"<<std::flush; 
    bs.apply_all(physics::compute_pressure_wd);
    if(rank==0)
      std::cout<<".done"<<std::endl;

    if(rank==0)
      std::cout<<"Linear Momentum"<<std::flush; 
    //bs.apply_all(physics::compute_lin_momentum);
    if(rank==0)
      std::cout<<".done"<<std::endl;

    if(rank==0)
      std::cout<<"Soundspeed"<<std::flush; 
    bs.apply_all(physics::compute_soundspeed);
    if(rank==0)
      std::cout<<".done"<<std::endl;
    
    // Refresh the neighbors within the smoothing length 
    bs.update_neighbors(); 

    if(rank==0)
      std::cout<<"Hydro acceleration"<<std::flush; 
    bs.apply_in_smoothinglength(physics::compute_hydro_acceleration);
    if(rank==0)
      std::cout<<".done"<<std::endl;
 
    if(rank==0)
      std::cout<<"Internalenergy"<<std::flush; 
    bs.apply_in_smoothinglength(physics::compute_internalenergy);
    if(rank==0)
      std::cout<<".done"<<std::endl; 
   
    if(iter==1){ 
      if(rank==0)
        std::cout<<"leapfrog"<<std::flush; 
      bs.apply_all(physics::leapfrog_integration_first_step);
      if(rank==0)
        std::cout<<".done"<<std::endl;
    }else{
      if(rank==0)
        std::cout<<"leapfrog"<<std::flush; 
      bs.apply_all(physics::leapfrog_integration);
      if(rank==0)
        std::cout<<".done"<<std::endl;
    }

    if(rank==0)
      std::cout<<"dudt integration"<<std::flush; 
    bs.apply_all(physics::dudt_integration);
    if(rank==0)
      std::cout<<".done"<<std::endl;

   
#ifdef OUTPUT
    if(iter % iteroutput == 0){ 
      bs.write_bodies("output_dwd",iter/iteroutput);
    }
#endif
    ++iter;
    
  }while(iter<totaliters);
}

flecsi_register_mpi_task(mpi_init_task);

void 
specialization_tlt_init(int argc, char * argv[]){
  
  // Default start at iteration 0
  int startiteration = 0;
  if(argc == 2){
    startiteration = atoi(argv[1]);
  }

  std::cout << "In user specialization_driver" << std::endl;
  /*const char * filename = argv[1];*/
  /*std::string  filename(argv[1]);
  std::cout<<filename<<std::endl;*/
  flecsi_execute_mpi_task(mpi_init_task,startiteration); 
} // specialization driver

void 
driver(int argc,  char * argv[]){
  std::cout << "In user driver" << std::endl;
} // driver


} // namespace
} // namespace


