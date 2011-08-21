## ======================================================================== ##
## Copyright 2009-2011 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

ADD_LIBRARY(rtcore STATIC
  common/compute_bounds.cpp 
  common/spatial_binning.cpp 
  common/object_binning.cpp 
  common/object_binning_parallel.cpp 
  bvh2/bvh2.cpp   
  bvh2/bvh2_traverser.cpp   
  bvh2/bvh2_builder.cpp   
  bvh2/bvh2_builder_spatial.cpp   
  bvh2/bvh2_to_bvh4.cpp   
  bvh4/bvh4.cpp   
  bvh4/bvh4_traverser.cpp   
  bvh4/bvh4_builder.cpp   
  rtcore.cpp)

TARGET_LINK_LIBRARIES(rtcore sys)
