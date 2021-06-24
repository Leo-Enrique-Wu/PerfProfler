# PerfProfler
 Practice to use PAPI to profile applications' performance
## Statistically profile an application
 Use `PAPI_profil` API to generate a PC histogram for event `PAPI_FP_INS`.
 ### Program: 
   https://github.com/Leo-Enrique-Wu/PerfProfler/blob/main/SerialCodeTest/MMult0_profil.cpp
 ### Compile command: 
   g++ -std=c++11 MMult0_profil.cpp -I${PAPI_DIR}/include -L${PAPI_DIR}/lib -o MMult0_profil -lpapi
 ### Execute command: 
   ./MMult0_profil