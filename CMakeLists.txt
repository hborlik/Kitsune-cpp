cmake_minimum_required(VERSION 3.16)
project(Kitsune_cpp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

add_executable(Kitsune_cpp test.cpp source/utils.cpp include/utils.h source/netStat.cpp include/netStat.h source/FeatureExtractor.cpp include/FeatureExtractor.h source/Autoencoder.cpp include/Autoencoder.h source/KitNET.cpp include/KitNET.h include/Cluster.h source/Cluster.cpp)