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

velocity vDesire;
velocity rSense_I;
velocity v;
parameters params;

int sign(float x) {
  if(x > 0) return 1;
  else if(x == 0) return 0;
  else return -1;
}

velocity yHoldPotential(velocity rSense_I, velocity v, parameters params) {
  velocity vel_Des_I;
  vel_Des_I.x = 0.0;
  vel_Des_I.y = 0.0;
  vel_Des_I.z = 0.0;

  float yError = rSense_I.y - params.ydes;
  float O = (2/PI)*(atanf(yError/5));
  float xDesire = params.xCruise * O;

  vel_Des_I.x = -(rSense_I.x - xDesire) / 250;
  vel_Des_I.y = -(3/2) * params.w * rSense_I.x;

  if(fabsf(params.zdes) > 0.001) {
    if((fabsf(rSense_I.z) / params.zdes) > 1) {
      vel_Des_I.z = -.01 * sign(rSense_I.z);
    } else {
      float t_phiz = sign(v.z) * (1/params.w) * acosf(rSense_I.z) / params.zdes;
      vel_Des_I.z = params.w * params.zdes * sinf(params.w * t_phiz);
    }
  }
  return vel_Des_I;
}

/*
int selectThruster(velocity rSense_I, velocity v, deltaVoptions_B, parameters params, C_ItoB) {
  //TBD
}
*/

void task_nav(void) {
  rSense_I.x = 0.0033;
  rSense_I.y = -0.0027;
  rSense_I.z = 0.0;
  v.x = 0.0016;
  v.y = -0.0027;
  v.z = 0.0;
  params.w = 0.0012;
  params.verror = 0.00000225;
  params.xdes = 0.0;
  params.ydes = 10.0;
  params.zdes = 0.0;
  params.xCruise = 10.0;

  static char* prpCMD; //static char to receive "message" from external_cmds
  while(1) {
    OS_Delay(250);
    
    char tmp[150];
    //vDesire = yHoldPotential(rSense_I, v, params);
    
    //sprintf(tmp, "vDesire x: %f, y: %f, z: %f\r\n", vDesire);
    //csk_uart0_puts(tmp);

    if(OSReadMsg(MSG_PRPTONAV_P)) {
      prpCMD=((char*)(OSTryMsg(MSG_PRPTONAV_P)));
      char tmp[150];
        
      // Start message verification (for sanity (AND pointer) check)
      if (prpCMD[0]=='P' && prpCMD[1]=='R' && prpCMD[2]=='P') { //if a (beings with PRP!!!) -- sanity check
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
        sprintf(tmp, "Burn time is: %d deciseconds\r\n", burn_time_ds);
        csk_uart0_puts(tmp);
        sprintf(tmp, "Burn time is: %d system ticks  TIMEOUT_MULTIPLE: %d\r\n", burn_time, TIMEOUT_MULTIPLE);
        csk_uart0_puts(tmp);

       
        
        /** 21 Nov 2014 - DJU
         ** The sequence this is wired and set to initiate is on purpose as for some
         ** reason only 3 GPIOs can be turned on at once with the header board being used
         ** to connect to the thrusters. Thrusters S1 and S2 will not work together on IO23 and IO22.
         ** Unsure as to why but will note it for further research.
        **/
        // turns ON appropriate thruster(s)
        csk_uart0_puts("Thrusters ON!\r\n");
        if (prpCMD[4] == '1') {csk_io22_high(); csk_uart0_puts("S1 ON!\r\n");}
        if (prpCMD[6] == '1') {csk_io20_high(); csk_uart0_puts("S2 ON!\r\n");}
        
        OS_Delay(220);  //delay of about 2s to pressurize veins - per Bryant

        if (prpCMD[3] == '1') {csk_io23_high(); csk_uart0_puts("A ON!\r\n");}
        if (prpCMD[8] == '1') {csk_io18_high(); csk_uart0_puts("B ON!\r\n");}
        if (prpCMD[9] == '1') {csk_io17_high(); csk_uart0_puts("C ON!\r\n");}
        if (prpCMD[5] == '1') {csk_io21_high(); csk_uart0_puts("D ON!\r\n");}
        if (prpCMD[7] == '1') {csk_io19_high(); csk_uart0_puts("E ON!\r\n");}
        if (prpCMD[10]== '1') {csk_io16_high(); csk_uart0_puts("F ON!\r\n");}
        
        // delay for length of burn_time
        // from pg 211 of Salvo RTOS pdf on how to wait longer than maximum timeout
        if (TIMEOUT_MULTIPLE > 0) {
          static int i;
          for (i = 0; i < TIMEOUT_MULTIPLE; i++) {
            //OS_WaitSem(SEM_PRP_BURNTIME_P, MAX_TIMEOUT);
            OS_Delay(255); 
          }
        }
        OS_Delay(burn_time); // time left after loop
        
        // turns OFF appropriate thruster(s)
        if (prpCMD[4] == '1') {csk_io22_low(); csk_uart0_puts("S1 OFF!\r\n");}
        
        OS_Delay(250); // delay of 2.5s to shut S1 valve - per Bryant
        if (prpCMD[6] == '1') {csk_io20_low(); csk_uart0_puts("S2 OFF!\r\n");}

        if (prpCMD[3] == '1') {csk_io23_low(); csk_uart0_puts("A OFF!\r\n");}
        if (prpCMD[8] == '1') {csk_io18_low(); csk_uart0_puts("B OFF!\r\n");}
        if (prpCMD[9] == '1') {csk_io17_low(); csk_uart0_puts("C OFF!\r\n");}
        if (prpCMD[5] == '1') {csk_io21_low(); csk_uart0_puts("D OFF!\r\n");}
        if (prpCMD[7] == '1') {csk_io19_low(); csk_uart0_puts("E OFF!\r\n");}
        if (prpCMD[10]== '1') {csk_io16_low(); csk_uart0_puts("F OFF!\r\n");}
        
        csk_uart0_puts("Thrusters OFF!\r\n");

        /** tests for passing around POSEs **/
        /**
        sprintf(tmp, "This is POSE_DESIRED.x: %d\r\n", POSE_DESIRED.x++);
        csk_uart0_puts(tmp);
        sprintf(tmp, "This is POSE_EST.x: %d\r\n", POSE_EST.x);
        csk_uart0_puts(tmp);
        **/
      }
      else {
        char tmps[80];
	    sprintf(tmps, "Command failed: %s", prpCMD);
	    csk_uart0_puts(tmps);
      }
    }
    /** COMMENTED OUT TO GET NAV FUNCTIONAL
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
	
    END COMMENT **/
 	
 } // END while(1)
} 
