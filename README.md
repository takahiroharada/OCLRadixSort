# OCLRadixSort

From book [Heterogeneous Computing with OpenCL](http://www.heterogeneouscompute.org/?page_id=7)

Only runs on **AMD** GPU as it is assuming the vector width is **64**. 

See [Introduction to GPU Radix Sort](./docs/Introduction to GPU Radix Sort.pdf) for implementation detail. 

# Set up OpenCL
An environmental variable needs to be set correctly to find OpenCL. 
- AMD
    - Set AMDAPPSDKROOT = OpenCL root (e.g., C:/workspace/CL1000/). It searches for the enviromnent variable. 
- NVIDIA
    - Set CUDA_PATH to OpenCL root. 

---
# Directories
- Main directories
    - UnitTest: Unit tests
    - Tahoe: Where Parallel Primitives are

- Others
    - Adl: OpenCL wrapper
    - docs: Documentation
    - contrib: External libraries
    - dist: Generated files (executables, libraries)
    - tools: Premake related

---
# Build
- **Execute the executable from UnitTest directory**. 
- 64 bit executables are generated at ./dist/debug/bin/x86_64/ or ./dist/release/bin/x86_64/. 

## Windows
- Create Visual Studio Solution

`./tools/premake4/win/premake4.exe vs2013`


## OSX
- Create Xcode project

`./tools/premake4/osx/premake4 xcode4`

- Set working directory

Product > Schema > Edit Schema, Option tab, there is Working Directory. 

## Linux
- Create Makefile

`./tools/premake4/linux64/premake4 gmake`

`make config=debug64`

`make config=release64`


# Known Issues
- Scan does not work for a large buffer (Unit test fails when scanning 1024K elems). 
