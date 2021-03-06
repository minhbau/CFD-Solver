#include "Dsys_Anal.h"
#include "json.hpp"
#include <vector>
#include "eigen/Eigen/Dense"
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <fstream>
#include <string>

using json = nlohmann::json;

// Class constructor
Dsys_Anal::Dsys_Anal(double dt, double tmax, 
                     VectorXd x0, VectorXd y0,
                     double (*u)(double, double, double),
                     double (*v)(double, double, double)){

    SetTime(dt, tmax);
    SetICs(x0, y0);
    SetVel(*u, *v);
}

// Sets time parameters
void Dsys_Anal::SetTime(double dt_in, double tmax_in){
    dt = dt_in;
    tmax = tmax_in;

    // Compute number of required steps
    t_count = (tmax / dt) + 1;

    t.resize(t_count);

    // Iterate through time vector
    for (std::size_t i = 0; i != t_count; ++i){
        t(i) = dt * i;
    }

    t_set = true;

    if (t_set && ic_set){
        ResizeParts();
    }
}

// Sets initial conditions
void Dsys_Anal::SetICs(VectorXd x0, VectorXd y0){
    // Verify same quantity of x and y coords
    if (x0.size() != y0.size()){
        throw std::invalid_argument("Mismatched IC Vectors");
    }

    parts.resize(x0.size());

    // ResizeParts position vectors for all particles
    for (std::size_t n = 0; n != parts.size(); ++n){
        parts[n].x.resize(1);
        parts[n].y.resize(1);

        parts[n].x(0) = x0(n);
        parts[n].y(0) = y0(n);
    }

    ic_set = true;

    if (t_set && ic_set){
        ResizeParts();
    }
}

// Sets velocity field equations
void Dsys_Anal::SetVel(double (*u_in)(double, double, double),
                       double (*v_in)(double, double, double)){
    u = *u_in;
    v = *v_in;
}

void Dsys_Anal::ResizeParts(){
    // Resize position vectors of all particles
    for (std::size_t n = 0; n != parts.size(); ++n){
        parts[n].x.conservativeResize(t_count + 1);
        parts[n].y.conservativeResize(t_count + 1);
    }
}

// Marches using Explicit Euler scheme
void Dsys_Anal::MarchEE(){
    // ResizeParts neighbor variables
    double u_n = 0, v_n = 0;

    // Time marching
    for (std::size_t i = 0; i != t_count; ++i){

        // Iterate through particles
        for (std::size_t n = 0; n != parts.size(); ++n){

            // Compute velocity components
            u_n = u(t(i), parts[n].x(i), parts[n].y(i));
            v_n = v(t(i), parts[n].x(i), parts[n].y(i));

            // Apply Euler
            parts[n].x(i + 1) = parts[n].x(i) + dt * u_n;
            parts[n].y(i + 1) = parts[n].y(i) + dt * v_n;
        }
    }
}

// Marches using Adams-Bashforth Scheme
void Dsys_Anal::MarchAB(){
    // ResizeParts neighbor variables
    double u_n = 0, v_n = 0;

    VectorXd u_nm1(parts.size());
    VectorXd v_nm1(parts.size());

    // Time marching
    for (std::size_t i = 0; i != t_count; ++i){

        // Iterate through particles
        for (std::size_t n = 0; n != parts.size(); ++n){

            // Compute velocity components
            u_n = u(t(i), parts[n].x(i), parts[n].y(i));
            v_n = v(t(i), parts[n].x(i), parts[n].y(i));

            // Apply Euler for first step
            if (i == 0){
                parts[n].x(i + 1) = parts[n].x(i) + dt * u_n;
                parts[n].y(i + 1) = parts[n].y(i) + dt * v_n;
            }

            // Apply Adams-Bashforth
            else{
                parts[n].x(i + 1) = parts[n].x(i) + dt * ((3. / 2.) * u_n - (1. / 2.) * u_nm1(n));
                parts[n].y(i + 1) = parts[n].y(i) + dt * ((3. / 2.) * v_n - (1. / 2.) * v_nm1(n));
            }

            // Write current velocities (n) to old variables (n-1)
            u_nm1(n) = u_n;
            v_nm1(n) = v_n;
        }
    }
}

// Prints trajectory data for given particle
void Dsys_Anal::PrintTraj(int n){
    // Print column headers
    printf("%6c%6c%6c\n", 't', 'x', 'y');

    // Iterate through position vectors
    for (std::size_t i = 0; i != t_count; ++i){
        printf("%6.2f%6.2f%6.2f\n", t(i), parts[n].x(i), parts[n].y(i));
    }
}

// Exports data to given file
void Dsys_Anal::ExportData(std::string filename){
    // Create json object
    json j;

    // Write t vector
    j["t"] = t;

    // Create empty array for particles
    j["parts"] = json::array();
    
    // Iterate through particles
    for (std::size_t n = 0; n != parts.size(); ++n){

        // Fill json
        j["parts"][n]["x"] = parts[n].x;
        j["parts"][n]["y"] = parts[n].y;
    }

    // Write file
    std::ofstream outfile(filename);
    outfile << std::setw(4) << j << std::endl;
}