/******************************************************************************
(C) Copyright Pumpkin, Inc. All Rights Reserved.

This file may be distributed under the terms of the License
Agreement provided with this software.

THIS FILE IS PROVIDED AS IS WITH NO WARRANTY OF ANY KIND,
INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE.

$Source: C:\\RCS\\D\\Pumpkin\\CubeSatKit\\Example\\all\\all\\CubeSatKit_Dev_Board\\Test\\Test1\\task_5sec.c,v $
$Author: aek $
$Revision: 3.1 $
$Date: 2009-11-02 00:45:07-08 $

******************************************************************************/
#include "main.h"
#include "task_nav.h"
#include "task_estimator.h"
#include <math.h>

// Pumpkin CubeSat Kit headers
#include "csk_io.h"
#include "csk_uart.h"

// Pumpkin Salvo headers
#include "salvo.h"

// Our headers
#include "rascal.h"

#define NEWTONS	0.001

// used to transmit message to task_externalcmds for processing and sending
extern void tx_frame(char msg[], int msg_size);

//velocity vDesire;
//velocity rSense_I;
//velocity v;
parameters params;
pose POSE_DESIRED;
thrusterinfo THRUSTER_INFO;
//static float BThrust[3][11];
//static float deltaVB[3][11];
typedef struct _three_by_eleven {
	float data[3][11];
} three_by_eleven;

/*
typedef struct _three_by_three {
  float data[3][3];
} three_by_three;
*/

// used in yHoldPotential
int sign(float x) {
  if(x > 0) return 1;
  else if(x == 0) return 0;
  else return -1;
}



/*
// Calculates inital values for dqdt
// dqdt = qdot(q(:,i), omega_BI(:,i));
// q = POSE_EST_q_values
// qdot.m in matlab
void output_dqdt(float p_dqdt[], float p_omega_BI[], float POSE_EST_q_values[])
{
  float omega_matrix[4][4] = {{0.000, p_omega_BI[2], -p_omega_BI[1], p_omega_BI[0]},
                              {-p_omega_BI[2], 0.000, p_omega_BI[0], p_omega_BI[1]},
                              {p_omega_BI[1], -p_omega_BI[0], 0.000, p_omega_BI[2]},
                              {-p_omega_BI[0], -p_omega_BI[1], -p_omega_BI[2], 0.000}};

  int c, d;
  float sum;
 
    for ( c = 0 ; c < 4 ; c++ ) {
      for ( d = 0 ; d < 4 ; d++ ){
        sum = sum + (0.5 * omega_matrix[c][d] * POSE_EST_q_values[d]);
      }

      p_dqdt[c] = sum;
      sum = 0; 
    }
}


// "Returns" updated q
// need to figure out expm() from matlab code
// see: q(:,i+1) = qdot(q(:,i), omega_BI(:,i+1), dt);
// qdot.m in matlab
void output_q(float POSE_EST_q_values[], float p_omega_BI[])
{
  float omega_matrix[4][4] = {{0.000, p_omega_BI[2], -p_omega_BI[1], p_omega_BI[0]},
                              {-p_omega_BI[2], 0.000, p_omega_BI[0], p_omega_BI[1]},
                              {p_omega_BI[1], -p_omega_BI[0], 0.000, p_omega_BI[2]},
                              {-p_omega_BI[0], -p_omega_BI[1], -p_omega_BI[2], 0.000}};

  float temp_q_values[4];

  // need to figure out exmp(omega_matrix) for this part
  // expm(0.5*omega_matrix*time)*q; found in qdot.m
  // then use multiplication loop below to multiply q * result of expm

  int c, d;
  float sum;
 
    for ( c = 0 ; c < 4 ; c++ ) {
      for ( d = 0 ; d < 4 ; d++ ){
        sum = sum + (0.5 * omega_matrix[c][d] * POSE_EST_q_values[d]);
      }

      temp_q_values[c] = sum;
      sum = 0; 
    }
  
  // assigns new q values into POSE
  POSE_EST.q1 = temp_q_values[0];
  POSE_EST.q2 = temp_q_values[1];
  POSE_EST.q3 = temp_q_values[2];
  POSE_EST.q4 = temp_q_values[3];
}
*/

// function used in select thruster to calculate vErr_B
// -C_ItoB * (POSE_DESIRE - POSE_EST)
void matrix_mul_C_ItoBxPOSE(float matrix1[][3], float multiply[])
{
  int c, d;

  // used to multiply array values by struct values
  float POSE_values[3];
  float sum;
  sum = 0;

  POSE_values[0] = POSE_DESIRED.xidot - POSE_EST.xidot;
  POSE_values[1] = POSE_DESIRED.yidot - POSE_EST.yidot;
  POSE_values[2] = POSE_DESIRED.zidot - POSE_EST.zidot;

    for ( c = 0 ; c < 3 ; c++ ) {
      for ( d = 0 ; d < 3 ; d++ ){
        sum = sum + (-matrix1[c][d] * POSE_values[d]);
      }  
 
      multiply[c] = sum;
      sum = 0; 
    }
}


void matrix_mul_vErr_BxdeltaVB(float matrix1[], float matrix2[][11], float p_score[], int p_j)
{
  int c;
  float sum;
  sum = 0;
  
  for ( c = 0 ; c < 3 ; c++ ) {
    sum += matrix1[c] * matrix2[p_j][c];
  }
  sum *=2;
  for (c = 0; c < 3; c++) {
     p_score[c] = sum + powf(matrix2[p_j][c], 2.00);
  }
  
}

/* 
  -- yHoldPotential function isn't used due to using it inline in selectThruster below (re: ARRAY DECAY!!) --
void yHoldPotential(parameters *params) {
  float yError = POSE_EST.yi - POSE_DESIRED.yi;
  float O = (2/PI)*(atanf(yError/5));
  float xDesire = params->xCruise * O;

  POSE_DESIRED.xidot = -(POSE_EST.xi - xDesire) / 250;
  POSE_DESIRED.yidot = -(3/2) * params->w * POSE_EST.xi;

  if(fabsf(POSE_DESIRED.zi) > 0.001) {
    if((fabsf(POSE_EST.zi) / POSE_DESIRED.zi) > 1) {
      POSE_DESIRED.zidot = -.01 * sign(POSE_EST.zi);
    } else {
      float t_phiz = sign(POSE_EST.zidot) * (1/params->w) * acosf(POSE_EST.zi) / POSE_DESIRED.zi;
      POSE_DESIRED.zidot = params->w * POSE_DESIRED.zi * sinf(params->w * t_phiz);
    }
  }
}
*/

//int selectThruster(three_by_eleven *deltaVB, three_by_three *C_ItoB) {
int selectThruster(three_by_eleven *deltaVB) {  
  int numThrustOptions = 11;
  int c, d;
  float score[3];
  float bestScore[3];
  //yHoldPotential(&params);

/* Begin yHoldPotential */
  float yError = POSE_EST.yi - POSE_DESIRED.yi;
  float O = (2/PI)*(atanf(yError/5));
  float xDesire = params.xCruise * O;

  POSE_DESIRED.xidot = -(POSE_EST.xi - xDesire) / 250;
  POSE_DESIRED.yidot = -(3.0/2.0) * params.w * POSE_EST.xi;

  if(fabsf(POSE_DESIRED.zi) > 0.001) {
    if((fabsf(POSE_EST.zi) / POSE_DESIRED.zi) > 1) {
      POSE_DESIRED.zidot = -.01 * sign(POSE_EST.zi);
    } else {
      float t_phiz = sign(POSE_EST.zidot) * (1/params.w) * acosf(POSE_EST.zi) / POSE_DESIRED.zi;
      POSE_DESIRED.zidot = params.w * POSE_DESIRED.zi * sinf(params.w * t_phiz);
    }
  }
/* End yHoldPotential */

  float vErr_B[3];

  //matrix_mul_C_ItoBxPOSE(C_ItoB, vErr_B);
/* Begin matrix_mul_C_ItoBxPOSE */
  // used to multiply array values by struct values
  float POSE_values[3];
  float sum;
  sum = 0;

  POSE_values[0] = POSE_DESIRED.xidot - POSE_EST.xidot;
  POSE_values[1] = POSE_DESIRED.yidot - POSE_EST.yidot;
  POSE_values[2] = POSE_DESIRED.zidot - POSE_EST.zidot;

    for ( c = 0 ; c < 3 ; c++ ) {
      for ( d = 0 ; d < 3 ; d++ ){
        sum = sum + (-C_ItoB.data[c][d] * POSE_values[d]);
      }  
 
      vErr_B[c] = sum;
      sum = 0; 
    }
/* End matrix_mul_C_ItoBxPOSE */

  int bestThruster = 0; // no thrust option
  bestScore[0] = -params.verror; // score if we dont thrust
  bestScore[1] = -params.verror;
  bestScore[2] = -params.verror;
  int j;
  for(j = 1; j < numThrustOptions; j++) {

    //matrix_mul_vErr_BxdeltaVB(vErr_B, deltaVB, score, j);
/* Begin matrix_mul_vErr_BxdeltaVB */
  
  sum = 0;
  
  for ( c = 0 ; c < 3 ; c++ ) {
    sum += vErr_B[c] * deltaVB->data[c][j];
  }
  sum *=2;
  for (c = 0; c < 3; c++) {
     score[c] = sum + powf(deltaVB->data[c][j], 2.00);
  }
/* End matrix_mul_vErr_BxdeltaVB */
    
    if ((score[0] < bestScore[0]) && (score[1] < bestScore[1]) && (score[2] < bestScore[2])) {
      bestThruster = j;
      bestScore[0] = score[0];
      bestScore[1] = score[1];
      bestScore[2] = score[2];
    } 
    
  }
  
  return bestThruster; 
}

/* Updates THRUSTER_INFO by receiving 12 ints to correspond to which thrusters are fired
   and for how long (in milliseconds).
   Called from switch(selectthruster) when thrusters are turned off
*/
void updateTHRUSTER_INFO(int one, int two, int three, int four, int five, int six, int seven, int eight, int nine, int ten, int eleven, int twelve, int thrusterOption) {
  THRUSTER_INFO.thruster_Azminus = one;
  THRUSTER_INFO.Azminustime = two;
  THRUSTER_INFO.thruster_Byplus = three;
  THRUSTER_INFO.Byplustime = four;
  THRUSTER_INFO.thruster_Cxminus = five;
  THRUSTER_INFO.Cxminustime = six;
  THRUSTER_INFO.thruster_Dzplus = seven;
  THRUSTER_INFO.Dzplustime = eight;
  THRUSTER_INFO.thruster_Eyminus = nine;
  THRUSTER_INFO.Eyminustime = ten;
  THRUSTER_INFO.thruster_Fxplus = eleven;
  THRUSTER_INFO.Fxplustime = twelve;
  THRUSTER_INFO.thrusterOption = thrusterOption;
} // END updateTHRUSTER_INFO


void task_nav(void) {
  // might need to change these to POSE_EST values
  /*
  rSense_I.x = 0.0033;
  rSense_I.y = -0.0027;
  rSense_I.z = 0.0;
  v.x = 0.0016;
  v.y = -0.0027;
  v.z = 0.0;
  */

 // defines and inits POSE_DESIRED
  POSE_DESIRED.q1 = 0.0;
  POSE_DESIRED.q2 = 0.0;
  POSE_DESIRED.q3 = 0.0;
  POSE_DESIRED.q4 = 0.0;
  POSE_DESIRED.q1dot = 0.0;
  POSE_DESIRED.q2dot = 0.0;
  POSE_DESIRED.q3dot = 0.0;
  POSE_DESIRED.q4dot = 0.0;
  POSE_DESIRED.xi = 0.0;
  POSE_DESIRED.yi = 100.0;
  POSE_DESIRED.zi = 0.0;
  POSE_DESIRED.xidot = 0.0;
  POSE_DESIRED.yidot = 0.0;
  POSE_DESIRED.zidot = 0.0;

  params.w = 0.0012;  /* (2*pi)/orbitPeriod in rad/s */
  params.verror = 0.00000225; /* 0.0015^2 in m/s */
  params.xdes = 0.0;
  params.ydes = 10.0;
  params.zdes = 0.0;
  params.xCruise = 10.0; /* next orbit location (orbit transfer location) */

// defines and inits THRUSTER_INFO
  THRUSTER_INFO.thruster_Azminus = 0;
  THRUSTER_INFO.Azminustime = 0;
  THRUSTER_INFO.thruster_Byplus = 0;
  THRUSTER_INFO.Byplustime = 0;
  THRUSTER_INFO.thruster_Cxminus = 0;
  THRUSTER_INFO.Cxminustime = 0;
  THRUSTER_INFO.thruster_Dzplus = 0;
  THRUSTER_INFO.Dzplustime = 0;
  THRUSTER_INFO.thruster_Eyminus = 0;
  THRUSTER_INFO.Eyminustime = 0;
  THRUSTER_INFO.thruster_Fxplus = 0;
  THRUSTER_INFO.Fxplustime = 0;

//static float BThrust[3][11] = {{0.0000, 1.0000, -1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000, 1.0000, -1.0000, -1.0000},
                               //{0.0000, 0.0000, 0.0000, 1.0000, -1.0000, 0.0000, 0.0000, 1.0000, -1.0000, 1.0000, -1.0000},
                               //{0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000, -1.0000, 0.0000, 0.0000, 0.0000, 0.0000}};

                // Thrust Options:    0        1       2        3       4      5       6        7       8        9        10    BODY FRAME COORDINATES
static float tmp_deltaVB[3][11] = {{0.0000, 0.0003, -0.0003, 0.0000, 0.0000, 0.0000, 0.0000, 0.0003, 0.0003, -0.0003, -0.0003}, //     X-axis
                                   {0.0000, 0.0000, 0.0000, 0.0003, -0.0003, 0.0000, 0.0000, 0.0003, -0.0003, 0.0003, -0.0003}, //     Y-axis
                                   {0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0003, -0.0003, 0.0000, 0.0000, 0.0000, 0.0000}};  //     Z-axis

static three_by_eleven deltaVB;

memcpy(deltaVB.data, tmp_deltaVB, sizeof(float[3][11]));


  
// all possible thruster combinations
static float tmp_BThrust[3][11] = {{0.0000, 1.0000, -1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000, 1.0000, -1.0000, -1.0000},
                               {0.0000, 0.0000, 0.0000, 1.0000, -1.0000, 0.0000, 0.0000, 1.0000, -1.0000, 1.0000, -1.0000},
                               {0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000, -1.0000, 0.0000, 0.0000, 0.0000, 0.0000}};

static three_by_eleven BThrust_1;

memcpy(BThrust_1.data, tmp_BThrust, sizeof(float[3][11]));

/*
  static float deltaVB_row0[] = {0.0000, 0.0003, -0.0003, 0.0000, 0.0000, 0.0000, 0.0000, 0.0003, 0.0003, -0.0003, -0.0003};
  static float deltaVB_row1[] = {0.0000, 0.0000, 0.0000, 0.0003, -0.0003, 0.0000, 0.0000, 0.0003, -0.0003, 0.0003, -0.0003};
  static float deltaVB_row2[] = {0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0003, -0.0003, 0.0000, 0.0000, 0.0000, 0.0000};
  static float *deltaVB[3];
  deltaVB[0] = deltaVB_row0;
  deltaVB[1] = deltaVB_row1;
  deltaVB[2] = deltaVB_row2;
*/

  static int thrusterOption;
  static int thrustTime = 100; //milliseconds - defined at compile time
  //static int i, j;
  static char* prpCMD; //static char to receive "message" from external_cmds
  static char cmd_tmp[TX_MSG_SIZE];

  while(1) {
    OS_Delay(250);
    if (THRUST_ENABLE_FLAG == ENABLED) {
	    // Looks for non-zero values in POSE_DESIRED to begin movement towards secondary spacecraft
	    if (POSE_DESIRED.xi != 0 || POSE_DESIRED.yi != 0 || POSE_DESIRED.zi != 0) {
	      thrusterOption = selectThruster(&deltaVB);

          // Update THRUSTER_INFO with thruster option from selectThruster
          // we are deciding to do this after the thruster has actually been fired

	      
	      // char tmp[20];
	      //sprintf(tmp, "This is x: %d", thrusterOption);
	      //sprintf(cmd_tmp, tmp);
	/*
	      for(i = 0; i < 3; i++) {
	        for(j = 0; j < 3; j++) {
	          sprintf(tmp, "%f ", C_ItoB.data[i][j]);
	          sprintf(cmd_tmp, tmp);
	        }
	      }
	*/

          // Maybe add some sort of flag error checking system to verify firing of an individual thruster
	      if (thrusterOption >= 1 && thrusterOption <=10) {
	 
	        
	        csk_io27_high(); // S1 ON!
	        csk_io30_high(); // S2 ON!
	        

	        OS_Delay(220);  //delay of about 2s to pressurize veins - per Bryant
	        
	        switch(thrusterOption) {
              case 0:
                // No thrust option, do nothing
                break;
	          case 1:
	            csk_io28_high(); sprintf(cmd_tmp, "F ON! (X+ Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);
	            break;
	          case 2:
	            csk_io24_high(); sprintf(cmd_tmp, "C ON! (X- Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);
	            break;
	          case 3:
	            csk_io25_high(); sprintf(cmd_tmp, "B ON! (Y+ Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);
	            break;
	          case 4:
	            csk_io31_high(); sprintf(cmd_tmp, "E ON! (Y- Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);
	            break;
	          case 5:
	            csk_io32_high(); sprintf(cmd_tmp, "D ON! (Z+ Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);
	            break;
	          case 6:
	            csk_io26_high(); sprintf(cmd_tmp, "A ON! (Z- Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);
	            break;
	          case 7:
	            csk_io28_high();
	            csk_io25_high(); sprintf(cmd_tmp, "F ON! (X+ Body axis)B ON! (Y+ Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);
	            break;
	          case 8:
	            csk_io28_high();
	            csk_io31_high(); sprintf(cmd_tmp, "F ON! (X+ Body axis)E ON! (Y- Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);
	            break;
	          case 9:
	            csk_io24_high();
	            csk_io25_high(); sprintf(cmd_tmp, "C ON! (X- Body axis)B ON! (Y+ Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);
	            break;
	          case 10:
	            csk_io24_high();
	            csk_io31_high(); sprintf(cmd_tmp, "C ON! (X- Body axis)E ON! (Y- Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);
	            break;
	        }
	        
	        OS_Delay(10); // fire thrusters for 100ms
	
	         // turn off main solenoids
	         csk_io27_low();
	         // commented out OS_Delay as Dr. Swartwout believes we can shut these off immediately 
             //OS_Delay(100);
	         csk_io30_low(); sprintf(cmd_tmp, "S1 OFF!S2 OFF!");
	
	         // turn off thrusters
	         switch(thrusterOption) {
              case 0:
                // Update THRUSTER_INFO - set everything to zeroes since no thruster was selected
                //THRUSTER_INFO = (struct thrusterinfo){0,0,0,0,0,0,0,0,0,0,0,0, thrustOption};
                updateTHRUSTER_INFO(0,0,0,0,0,0,0,0,0,0,0,0, thrusterOption);
                break;
	          case 1:
	            csk_io28_low(); sprintf(cmd_tmp, "F OFF! (X+ Body axis)");
                updateTHRUSTER_INFO(0,0,0,0,0,0,0,0,0,0,1,thrustTime, thrusterOption);
                break;
	          case 2:
	            csk_io24_low(); sprintf(cmd_tmp, "C OFF (X- Body axis)");
                updateTHRUSTER_INFO(0,0,0,0,1,thrustTime,0,0,0,0,0,0, thrusterOption);
	            break;
	          case 3:
	            csk_io25_low(); sprintf(cmd_tmp, "B OFF! (Y+ Body axis)");
	            updateTHRUSTER_INFO(0,0,1,thrustTime,0,0,0,0,0,0,0,0, thrusterOption);
                break;
	          case 4:
	            csk_io31_low(); sprintf(cmd_tmp, "E OFF! (Y- Body axis)");
	            updateTHRUSTER_INFO(0,0,0,0,0,0,0,0,1,thrustTime,0,0, thrusterOption);
                break;
	          case 5:
	            csk_io32_low(); sprintf(cmd_tmp, "D OFF! (Z+ Body axis)");
	            updateTHRUSTER_INFO(0,0,0,0,0,0,1,thrustTime,0,0,0,0, thrusterOption);
                break;
	          case 6:
	            csk_io26_low(); sprintf(cmd_tmp, "A OFF! (Z- Body axis)");
	            updateTHRUSTER_INFO(1,thrustTime,0,0,0,0,0,0,0,0,0,0, thrusterOption);
                break;
	          case 7:
	            csk_io28_low();
	            csk_io25_low(); sprintf(cmd_tmp, "F OFF! (X+ Body axis)B OFF! (Y+ Body axis)");
	            updateTHRUSTER_INFO(0,0,1,thrustTime,0,0,0,0,0,0,1,thrustTime, thrusterOption);
                break;
	          case 8:
	            csk_io28_low();
	            csk_io31_low(); sprintf(cmd_tmp, "F OFF! (X+ Body axis)E OFF! (Y- Body axis)");
	            updateTHRUSTER_INFO(0,0,0,0,0,0,0,0,1,thrustTime,1,thrustTime, thrusterOption);
                break;
	          case 9:
	            csk_io24_low();
	            csk_io25_low(); sprintf(cmd_tmp, "C OFF! (X- Body axis)B OFF! (Y+ Body axis)");
	            updateTHRUSTER_INFO(0,0,1,thrustTime,1,thrustTime,0,0,0,0,0,0, thrusterOption);
                break;
	          case 10:
	            csk_io24_low();
	            csk_io31_low(); sprintf(cmd_tmp, "C OFF! (X- Body axis)E OFF! (Y- Body axis)");
	            updateTHRUSTER_INFO(0,0,0,0,1,thrustTime,0,0,1,thrustTime,0,0, thrusterOption);
                break;
	        }
	      	      
	    } // END: if (thrusterOption >= 1 && thrusterOption <=10)
	
	
	      if(OSReadMsg(MSG_PRPTONAV_P)) {
	        prpCMD=((char*)(OSTryMsg(MSG_PRPTONAV_P)));
	        //memcpy(prpCMD, OSTryMsg(MSG_PRPTONAV_P), DATA_SIZE*sizeof(char));
	        char tmp[150];
	        
	        // Start message verification (for sanity (AND pointer) check)
	        if (prpCMD[12]=='P' && prpCMD[13]=='R' && prpCMD[14]=='P') { //if a (beings with PRP!!!) -- sanity check
	          // Thruster request!
	          /* Expects PRP12ABCDEFXXX,
	             PRP = propulsion command block (gonna send a message to NAV)
	             12ABCDEF = 0 or 1 (binary) and represent the thrusters labeled on Rascal as shown below.
	             XXX = number of deciseconds to fire thrusters
	           Thruster  GPIO
	           --------  ----
	                 A   IO.23
	            2 = S1   IO.22
	                 D   IO.21
	            4 = S2   IO.20
	                 E   IO.19
	                 B   IO.18
	                 C   IO.17
	                 F   IO.16
	          */  
    
	          // extracts burn time from prpCMD = last 3 digits of prpCMD
	          int x, y, z, burn_time_ds;
	          static int burn_time = 0;
	          //static int MAX_TIMEOUT = 255; // because OSBYTES_OF_DELAYS = 1 (1 Byte)
	          static int TIMEOUT_MULTIPLE = 0;
	          x = (prpCMD[11] - '0') * 100;
	          y = (prpCMD[12] - '0') * 10;
	          z = prpCMD[13] - '0';
	          burn_time_ds = (x+y+z);    // time in deciseconds
	          burn_time = burn_time_ds*10; // time in system ticks: 1 system tick = 10ms (@100Hz)
	          while (burn_time > 255) {
	            TIMEOUT_MULTIPLE++;
	            burn_time -= 255;
	          }
	          sprintf(tmp, "Burn time is: %d decisecondsBurn time is: %d system ticks  TIMEOUT_MULTIPLE: %d", burn_time_ds, burn_time, TIMEOUT_MULTIPLE);
	          tx_frame(cmd_tmp, TX_MSG_SIZE);
	       
	        
	          /** 21 Nov 2014 - DJU
	           ** The sequence this is wired and set to initiate is on purpose as for some
	           ** reason only 3 GPIOs can be turned on at once with the header board being used
	           ** to connect to the thrusters. Thrusters S1 and S2 will not work together on IO23 and IO22.
	           ** Unsure as to why but will note it for further research.
	          **/
              /** 12 Oct 2015 - DJU
               ** No issue with using more than 3 GPIOs as issue from above (21 Nov 2014) was trying
               ** to use GPIOs that had been assigned already. Meh.
               **/
	          // turns ON appropriate thruster(s)
	          
              sprintf(cmd_tmp, "Thrusters ON!");
	          if (prpCMD[4] == '1') csk_io27_high();
	          if (prpCMD[6] == '1') csk_io30_high();
	          sprintf(cmd_tmp, "Thrusters ON!S1 ON!S2 ON!");
              tx_frame(cmd_tmp, TX_MSG_SIZE);

	          OS_Delay(100);  //delay of about 1s to pressurize veins - per Bryant
	
	          if (prpCMD[3] == '1') {csk_io26_high(); sprintf(cmd_tmp, "A ON! (Z- Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);}
	          if (prpCMD[8] == '1') {csk_io25_high(); sprintf(cmd_tmp, "B ON! (Y+ Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);}
	          if (prpCMD[9] == '1') {csk_io24_high(); sprintf(cmd_tmp, "C ON! (X- Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);}
	          if (prpCMD[5] == '1') {csk_io32_high(); sprintf(cmd_tmp, "D ON! (Z+ Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);}
	          if (prpCMD[7] == '1') {csk_io31_high(); sprintf(cmd_tmp, "E ON! (Y- Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);}
	          if (prpCMD[10]== '1') {csk_io28_high(); sprintf(cmd_tmp, "F ON! (X+ Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);}
              
	        
	          // delay for length of burn_time
	          // from pg 211 of Salvo RTOS pdf on how to wait longer than maximum timeout
	          if (TIMEOUT_MULTIPLE > 0) {
	            static int i;
	            for (i = 0; i < TIMEOUT_MULTIPLE; i++) {
	              OS_Delay(255); 
	            }
	          }
	          OS_Delay(burn_time); // time left after loop
	          
	          // turns OFF appropriate thruster(s)
	          if (prpCMD[4] == '1') {csk_io27_low(); sprintf(cmd_tmp, "S1 OFF!"); tx_frame(cmd_tmp, TX_MSG_SIZE);}
	          
	          OS_Delay(250); // delay of 2.5s to shut S1 valve - per Bryant
	          if (prpCMD[6] == '1') {csk_io30_low(); sprintf(cmd_tmp, "S2 OFF!"); tx_frame(cmd_tmp, TX_MSG_SIZE);}
	
	          if (prpCMD[3] == '1') {csk_io26_low(); sprintf(cmd_tmp, "A OFF! (Z- Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);}
	          if (prpCMD[8] == '1') {csk_io25_low(); sprintf(cmd_tmp, "B OFF! (Y+ Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);}
	          if (prpCMD[9] == '1') {csk_io24_low(); sprintf(cmd_tmp, "C OFF! (X- Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);}
	          if (prpCMD[5] == '1') {csk_io32_low(); sprintf(cmd_tmp, "D OFF! (Z+ Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);}
	          if (prpCMD[7] == '1') {csk_io31_low(); sprintf(cmd_tmp, "E OFF! (Y- Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);}
	          if (prpCMD[10]== '1') {csk_io28_low(); sprintf(cmd_tmp, "F OFF! (X+ Body axis)"); tx_frame(cmd_tmp, TX_MSG_SIZE);}
	        
	          sprintf(cmd_tmp, "Thrusters OFF!");
	
	          } else if (prpCMD[12]=='S' && prpCMD[13]=='P' && prpCMD[14]=='R' && prpCMD[15]=='T') {
			    sprintf(cmd_tmp, "SPRT IN PROG\n"); tx_frame(cmd_tmp, TX_MSG_SIZE);
			    csk_io29_high();
				OS_Delay(250);  // 3 secs according to Nick
			    OS_Delay(50);
				csk_io29_low();
				sprintf(cmd_tmp, "Solenoid Extended\n"); tx_frame(cmd_tmp, TX_MSG_SIZE);
			  } else {
	            char tmps[80];
		        sprintf(cmd_tmp, "Command failed in task_nav: %s", prpCMD); tx_frame(cmd_tmp, TX_MSG_SIZE);
	          } 
	        } // END:  if(OSReadMsg(MSG_PRPTONAV_P))
	
	    /* COMMENTED OUT TO GET NAV FUNCTIONAL
		// Just some constant stuff for the calculations
			const int m = 3; // kg
			const int dt = 1; // seconds
			const int velError = 0.00000225; 
			int omega = (2*3.14159)/(60*90); 
			const int xCruise = 10; 	
			BThrust = {0*T(:,1) T T(:,1)+T(:,3) T(:,1)+T(:,4) T(:,2)+T(:,3) T(:,2)+T(:,4)};
			deltaVB = BThrust*(dt/m);
		// Get position and velocity (from task_estimator)
			// postion (name posSensorI (x,y,z))
			// velocity (name velSensorI (x,y,z)
		
		// Get Desired position coordinates sent to the code from SLUGND	
			extern int xposDes; 
			extern int yposDes;
			extern int zposDes;
		
		// Compute desired velocity (and velocity error) [yHoldPotential.m] -- This defines the error, rotationa; velocity, 
		// and orbit transfer locationa nd then uses teh desired position to give the desired velocity values
	
			yError = yposSensorI - yposDes; //sensor value is what is coming from the estimator
			turnTime = (2/pi)*atan(yError/5); //disctates how fast the orbit transfer location is reached
			xDesire = xCruise*turnTime
	
			//Driving the velocity values to the accurate value:
			xvelDes = -(xposSensorI-xDesire)/250; //this controls how "smooth" the motion to the next value is
			yvelDes = -(3/2)*omega*xposSensorI;
	
			if(abs(zposDes > 0.001)) {
				if(abs(zposSensorI/zposDes > 1)) {
					zvelDes = -.01*sign(zposSensorI);
				}
				else {
					t_phiz = sign(zvelSensorI*(1/omega)*acos(zposSensorI/zposDes);
					zvelDes = omega*zposDes*sin(omega*t_phiz);
				}	
			}			
	
		// Score the various thruster options [selectthruster.m]
			int T[3][6] = 0.001*{1, -1, 0,  0, 0,  0,
				       0,  0, 1, -1, 0,  0,
			    	   0,  0, 0,  0, 1, -1};  
			velocityErrorB = xvelDes
			bestThruster = 1; //No thrust option	
			const int BestScore = -velError;
				for (j=2:ThrustOptions) {
					score = 2*(velocityErrorB*deltaVB(:,j)+deltaVB(:,j).^2;
					if (score<BestScore) {
						bestThruster = j;
						BestScore = score;
					}
				}
	
		// Choose best thruster(s) -- BEST SCORE -- and operate thruster(s) [Satmotion3dDeliverable1 lines 107 - 116]
	 
		//This involves some 12x3 matricies, so I will attack this at another time.
		
	    END COMMENT */
    } 
    }
   }// END while(1)
} // END task_nav
