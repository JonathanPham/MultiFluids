//
//  objsdf.cpp
//  ChocolateSyrup
//
//  Created by Karl Li on 4/10/12.
//  Copyright (c) 2012 University of Pennsylvania. All rights reserved.
//

#include <iostream>
#include "objsdf.h"
#include <fstream>
#include <sstream>
#include <limits>

void update_minmax(float a1, float& amin, float& amax)
{
    if(a1<amin) amin=a1;
    else if(a1>amax) amax=a1;
}

void update_minmax(const glm::vec3 &x, glm::vec3 &xmin, glm::vec3 &xmax)
{
    for(unsigned int i=0; i<3; ++i) update_minmax(x[i], xmin[i], xmax[i]);
}

objsdf::objsdf(string filename){
    
    float dx = 1;
    float padding = 1;
    
    if(filename.size() < 5 || filename.substr(filename.size()-4) != std::string(".obj")) {
        std::cerr << "Error: Expected OBJ file with filename of the form <name>.obj.\n";
        exit(-1);
    }
    
    int ignored_lines = 0;
    std::string line;
    int obj_line = 0;
    min_box = glm::vec3(std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),std::numeric_limits<float>::max());
    max_box = glm::vec3(-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max());
    
    std::cout << "Reading data.\n";
    
    std::ifstream infile(filename.c_str());
    if(!infile) {
        std::cerr << "Failed to open. Terminating.\n";
        exit(-1);
    }
    
    while(!infile.eof()) {
        std::getline(infile, line);
        if(line.substr(0,1) == std::string("v") && line.substr(0,2) != std::string("vt") && line.substr(0,2) != std::string("vn")) {
            std::stringstream data(line);
            char c;
            glm::vec3 point;
            data >> c >> point[0] >> point[1] >> point[2];
            vertList.push_back(point);
            update_minmax(point, min_box, max_box);
            obj_line++;
        }
        else if(line.substr(0,1) == std::string("f")) {
            std::stringstream data(line);
            std::string v;
            std::getline(data, v, ' ');
            std::string delim1 = "//";
            std::string delim2 = "/";
            if(std::string::npos != line.find("//")){
                //std::cout << "Vertex-Normal Format" << std::endl;
                std::vector<int> pointList;
                while(getline(data, v, ' ')){
                    std::istringstream facestring(v);
                    std::string f;
                    std::getline(facestring, f, '/');
                    pointList.push_back(::atof(f.c_str())-1);
                }
                if(pointList.size()==3){
                    faceList.push_back(glm::vec3(pointList[0],pointList[1],pointList[2]));
                }else{
                    std::cout << "Error in reading line " << obj_line << ", skipping..." << std::endl;
                    ++ignored_lines;
                }
            }else if(std::string::npos != line.find("/")){
                std::vector<int> pointList;
                while(getline(data, v, ' ')){
                    std::istringstream facestring(v);
                    std::string f;
                    int i=0;
                    while(getline(facestring, f, '/')){
                        if(i==0){
                            pointList.push_back(::atof(f.c_str())-1);
                        }
                        i++;
                    }
                }
                if(pointList.size()==3){
                    faceList.push_back(glm::vec3(pointList[0],pointList[1],pointList[2]));
                }else{
                    std::cout << "Error in reading line " << obj_line << ", skipping..." << std::endl;
                    ++ignored_lines;
                }
            }else{
                std::string v;
                std::vector<int> pointList;
                while(getline(data, v, ' ')){
                    pointList.push_back(::atof(v.c_str())-1);
                }
                if(pointList.size()==3){
                    faceList.push_back(glm::vec3(pointList[0],pointList[1],pointList[2]));
                }else{
                    std::cout << "Error in reading line " << obj_line << ", skipping..." << std::endl;
                    ++ignored_lines;
                }
            }
            obj_line++;
        }
        else {
            ++ignored_lines; 
        }
    }
    infile.close();
    
    if(ignored_lines > 0)
        std::cout << "Warning: " << ignored_lines << " lines were ignored since they did not contain faces or vertices.\n";
    
    std::cout << "Read in " << vertList.size() << " vertices and " << faceList.size() << " faces." << std::endl;
    
    //Add padding around the box.
    glm::vec3 unit(1,1,1);
    min_box -= padding*dx*unit;
    max_box += padding*dx*unit;
    glm::vec3 sizes = glm::vec3((max_box - min_box)/dx);
    
    std::cout << "Bound box size: (" << min_box[0] << " " <<  min_box[1] << " "  << min_box[2 ]<< ") to (" << max_box[0] <<  " " << max_box[1] << " " << max_box[2] << ") with dimensions " << sizes[0] << " " << sizes[1] << " " << sizes[2] << "." << std::endl;
    
    std::cout << "Computing signed distance field.\n";

    make_level_set3(faceList, vertList, min_box, dx, sizes[0], sizes[1], sizes[2], phi_grid);

}

objsdf::~objsdf(){
}

float objsdf::getSignedDistanceAtCell(int x, int y, int z){
    //formula is (x*((zSize)*(ySize)))+(y*(zSize))+z
    int index = x*((phi_grid.nk)*(phi_grid.nj))+(y*(phi_grid.nk))+z;
    return phi_grid.a[index];
}


