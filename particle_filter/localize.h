//
//include file for localization file
//

#include "particleFilter3d.h"
#include "sensors.h"

typedef struct{
    float loc[8];
    float prev_vio[5];
    float total_vio[5];
    int init_cnt;
    float BEACON_COOR[12]; //matrix of beacon coordinates where each row is x,y,z for a beacon 
    float UWB_BIAS;
    float UWB_STD; 
    int N_BATCHES;
    int N;
    float VIO_STD_XYZ;
    float VIO_STD_THETA;
    float VIO_STD_SCALE;
    float INITZ;
    float INITZ_STD; 
}state_t;

void init_state(state_t * s){
    for(int i=0;i<5;i++){
        s.loc[i]=0;
        s.prev_vio[i]=0;
        s.total_vio[i]=0;
    }
    s.loc[5]=0; s.loc[6]=0; s.loc[7]=0;
    s.init_cnt=0;
    //s.BEACON_COOR[0]...
    s.UWB_BIAS = 0.0;
    s.UWB_STD = 0.1;
    s.N_BATCHES = 10;
    s.N = 10000;
    s.VIO_STD_XYZ=1e-3;
    s.VIO_STD_THETA=1e-6;
    s.VIO_STD_SCALE=1e-3;
    s.INITZ=1.3;
    s.INITZ_STD = 0.05;

    return;
}
