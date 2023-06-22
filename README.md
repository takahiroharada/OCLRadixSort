# OCLRadixSort

From book [Heterogeneous Computing with OpenCL](http://www.heterogeneouscompute.org/?page_id=7)

Only runs on **AMD** GPU as it is assuming the vector width is **64**. 

See [Introduction to GPU Radix Sort](<./docs/Introduction to GPU Radix Sort.pdf>) for implementation detail. 

```
@article{harada2011introduction,
   title={Introduction to GPU Radix Sort},
   author={Harada, T. and Howes, L.},
   booktitle={Heterogeneous Computing with OpenCL},
   year={2011}
}
```

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
# Run The Unit Test
- **Execute the executable from UnitTest directory**. 

---
# Build
- 64 bit executables are generated at ./dist/debug/bin/x86_64/ or ./dist/release/bin/x86_64/. 

## Windows
- Create Visual Studio Solution

`./tools/premake4/win/premake4.exe vs2013`


## OSX
- Create Xcode project

`chmod +x tools/premake4/osx/premake4`
`./tools/premake4/osx/premake4 xcode4`

- Set working directory

Product > Schema > Edit Schema, Option tab, there is Working Directory. 

## Linux
- Create Makefile

`chmod +x tools/premake4/linux64/premake4`
`./tools/premake4/linux64/premake4 gmake`

`make config=debug64`

`make config=release64`


# Known Issues
- Scan does not work for a large buffer (Unit test fails when scanning 1024K elems). 

The expected output from the unit test is
```bash
$ ../dist/release/bin/x86_64/UnitTest64.exe
[==========] Running 3 tests from 1 test case.
[----------] Global test environment set-up.
[----------] 3 tests from Demo
[ RUN      ] Demo.Sort32
test    1.0K elems
test    2.0K elems
test    4.0K elems
test    8.0K elems
test   16.0K elems
test   32.0K elems
test   64.0K elems
test  128.0K elems
test  256.0K elems
test  512.0K elems
test 1024.0K elems
[       OK ] Demo.Sort32 (1352 ms)
[ RUN      ] Demo.SortKeyValue
test    1.0K elems
test    2.0K elems
test    4.1K elems
test    8.2K elems
test   16.4K elems
test   32.8K elems
test   65.6K elems
test  131.2K elems
test  262.5K elems
test  525.0K elems
test 1050.0K elems
[       OK ] Demo.SortKeyValue (512 ms)
[ RUN      ] Demo.Scan
test    1.0K elems
test    2.0K elems
test    4.0K elems
test    8.0K elems
test   16.0K elems
test   32.0K elems
test   64.0K elems
test  128.0K elems
test  256.0K elems
test  512.0K elems
test 1024.0K elems
main.cpp(200): error: Value of: fail == 0
  Actual: false
Expected: true
[  FAILED  ] Demo.Scan (251 ms)
[----------] 3 tests from Demo (2118 ms total)

[----------] Global test environment tear-down
[==========] 3 tests from 1 test case ran. (2121 ms total)
[  PASSED  ] 2 tests.
[  FAILED  ] 1 test, listed below:
[  FAILED  ] Demo.Scan

 1 FAILED TEST
 ```
