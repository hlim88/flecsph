/*~--------------------------------------------------------------------------~*
 * Copyright (c) 2017 Los Alamos National Security, LLC
 * All rights reserved.
 *~--------------------------------------------------------------------------~*/

 #include <iostream>
#include <algorithm>
#include <cmath>

#include "hdf5ParticleIO.h"


const double distance = 0.05;  // Distance between the particles 
const double m_ = 1.0e-5;
const double rho_ = 0.0;
const double u_ = 2.0; 
const double smoothing_length = 5e-2;
const char* fileprefix = "hdf5_fluid";
const double timestep = 1e-3;
int32_t dimension = 3;

int main(int argc, char * argv[]){
 
  int nx = 10;//atoi(argv[1]);
  int ny = 10;//atoi(argv[2]);
  int nz = 10;//atoi(argv[3]);


  if(argc!=4){
    printf("./fluid_generator nx ny nz\n");
    fprintf(stderr,"Generation with default values= 10*10*10=1000 particles");
  }else{
    nx = atoi(argv[1]);
    ny = atoi(argv[2]);
    nz = atoi(argv[3]);
  }

  int rank, size; 
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  MPI_Comm_size(MPI_COMM_WORLD,&size);

   int64_t nparticles = nx*ny*nz;
  int64_t nparticlesproc = nparticles/size;
  if(rank==size-1){
    nparticlesproc = nparticles - nparticlesproc*(size-1);
  }

  if(rank==0){
    printf("Generating %ld particles\n",nparticles);
    printf("%ld particles per proc (last %ld)\n",nparticlesproc,
        nparticles-nparticlesproc*(size-1));
  }

  if(nz == 0){
    dimension = 2;
  }
  
  // Position
  double* x = new double[nparticlesproc]();
  double* y = new double[nparticlesproc]();
  double* z = new double[nparticlesproc]();
  // Velocity
  double* vx = new double[nparticlesproc]();
  double* vy = new double[nparticlesproc]();
  double* vz = new double[nparticlesproc]();
  // Acceleration
  double* ax = new double[nparticlesproc]();
  double* ay = new double[nparticlesproc]();
  double* az = new double[nparticlesproc]();
  // Smoothing length 
  double* h = new double[nparticlesproc]();
  // Density 
  double* rho = new double[nparticlesproc]();
  // Internal Energy 
  double* u = new double[nparticlesproc]();
  // Pressure
  double* P = new double[nparticlesproc]();
  // Mass
  double* m = new double[nparticlesproc]();
  // Id
  int64_t* id = new int64_t[nparticlesproc]();
  // Timestep 
  double* dt = new double[nparticlesproc]();
  
  // Generate data
  double nlines = nx; 
  double nlinesproc = nlines/size;
  double ncols = ny;
  double linestart = rank * nlinesproc * distance;
  double ndepth = nz;

  std::cout<<"Generating: "<<nlines<<"*"<<ncols<<std::endl;

  // Id of my first particle 
  int64_t posid = nparticlesproc*rank;

  // Header data 
  // the number of particles = nparticles 
  // The value for constant timestep 
   
  double curline = linestart;
  double curcol = 0.0;
  double curdepth = 0.0;
  int col = 0;
  int depth = 0;

  for(int64_t part=0; part<nparticlesproc; ++part){
    x[part] = curline;
    y[part] = curcol; 
    z[part] = curdepth;

    u[part] = u_; 
    rho[part] = rho_;
    m[part] = m_; 
    // Y and Z not used 
    // VX, VY, VZ and AX, AY, AZ stay to 0
    h[part] = smoothing_length;
    // P stay to 0
    id[part] = posid++; 
    
    curcol += distance;
    //curdepth += distance; 
    col++;
    //depth++;
    // Move to the next particle
    if(col == ncols){
      curdepth += distance; 
      depth++;
      col = 0;
      curcol = 0.0;
      if( depth == ndepth  ){
        curline += distance; 
        curcol = 0.0;
        curdepth = 0.0;
        col = 0;
        depth = 0;
      }
    }
    //std::cout<<x[part]<<": "<<h[part]<<std::endl;
  }
  
  char filename[128];
  sprintf(filename,"%s.h5part",fileprefix);
  remove(filename);

  Flecsi_Sim_IO::HDF5ParticleIO testDataSet; 
  testDataSet.createDataset(filename,MPI_COMM_WORLD);
    
  // add the global attributes
  testDataSet.writeDatasetAttribute("nparticles","int64_t",nparticles);
  testDataSet.writeDatasetAttribute("timestep","double",timestep);
  testDataSet.writeDatasetAttribute("dimension","int32_t",dimension);
  testDataSet.writeDatasetAttribute("use_fixed_timestep","int32_t",1);

  //char * simName = "fluid_2D";
  //testDataSet.writeDatasetAttributeArray("name","string",simName);
  testDataSet.closeFile();

  testDataSet.openFile(MPI_COMM_WORLD);
  testDataSet.setTimeStep(0);

  Flecsi_Sim_IO::Variable _d1,_d2,_d3;

  _d1.createVariable("x",Flecsi_Sim_IO::point,"double",nparticlesproc,x);
  _d2.createVariable("y",Flecsi_Sim_IO::point,"double",nparticlesproc,y);
  _d3.createVariable("z",Flecsi_Sim_IO::point,"double",nparticlesproc,z);

  testDataSet.vars.push_back(_d1);
  testDataSet.vars.push_back(_d2);
  testDataSet.vars.push_back(_d3);

  testDataSet.writeVariables();

  _d1.createVariable("vx",Flecsi_Sim_IO::point,"double",nparticlesproc,vx);
  _d2.createVariable("vy",Flecsi_Sim_IO::point,"double",nparticlesproc,vy);
  _d3.createVariable("vz",Flecsi_Sim_IO::point,"double",nparticlesproc,vz);

  testDataSet.vars.push_back(_d1);
  testDataSet.vars.push_back(_d2);
  testDataSet.vars.push_back(_d3);

  testDataSet.writeVariables();

  _d1.createVariable("ax",Flecsi_Sim_IO::point,"double",nparticlesproc,ax);
  _d2.createVariable("ay",Flecsi_Sim_IO::point,"double",nparticlesproc,ay);
  _d3.createVariable("az",Flecsi_Sim_IO::point,"double",nparticlesproc,az);

  testDataSet.vars.push_back(_d1);
  testDataSet.vars.push_back(_d2);
  testDataSet.vars.push_back(_d3);

  testDataSet.writeVariables();


  _d1.createVariable("h",Flecsi_Sim_IO::point,"double",nparticlesproc,h);
  _d2.createVariable("rho",Flecsi_Sim_IO::point,"double",nparticlesproc,rho);
  _d3.createVariable("u",Flecsi_Sim_IO::point,"double",nparticlesproc,u);
  
  testDataSet.vars.push_back(_d1);
  testDataSet.vars.push_back(_d2);
  testDataSet.vars.push_back(_d3);

  testDataSet.writeVariables();

  _d1.createVariable("P",Flecsi_Sim_IO::point,"double",nparticlesproc,P);
  _d2.createVariable("m",Flecsi_Sim_IO::point,"double",nparticlesproc,m);
  _d3.createVariable("id",Flecsi_Sim_IO::point,"int64_t",nparticlesproc,id);
  
  testDataSet.vars.push_back(_d1);
  testDataSet.vars.push_back(_d2);
  testDataSet.vars.push_back(_d3);

  testDataSet.writeVariables();

  testDataSet.closeFile(); 
  
  delete[] x;
  delete[] y;
  delete[] z;
  delete[] vx;
  delete[] vy;
  delete[] vz;
  delete[] ax;
  delete[] ay;
  delete[] az;
  delete[] h;
  delete[] rho;
  delete[] u;
  delete[] P;
  delete[] m;
  delete[] id;
  delete[] dt;

  // Generate wall particles 


  MPI_Finalize();
  return 0;
}
