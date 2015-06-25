/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*                        Copyright (c) James Pearman                          */
/*                                   2012                                      */
/*                            All Rights Reserved                              */
/*                                                                             */
/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*    Module:     SmartMotorLib.c                                              */
/*    Author:     James Pearman                                                */
/*    Created:    2 Oct 2012                                                   */
/*                                                                             */
/*    Revisions:                                                               */
/*                V1.00  21 Oct 2012 - Initial release                         */
/*                V1.01   7 Dec 2012                                           */
/*                       small bug in SmartMotorLinkMotors                     */
/*                       fix for High Speed 393 Ke constant                    */
/*                       kNumbOfTotalMotors replaced with kNumbOfRealMotors    */
/*                       _Target_Emulator_ defined for versions of ROBOTC      */
/*                       prior to 3.55                                         */
/*                       change to motor enums for V3.60 ROBOTC compatibility  */
/*               V1.02  27 Jan 2013                                            */
/*                      Linking an encoded and non-encoded motor was not       */
/*                      working correctly, added new field to the structure    */
/*                      eport to allow one motor to access the encoder for     */
/*                      another correctly.                                     */
/*               V1.03  10 March 2013                                          */
/*                      Due to new version of ROBOTC (V3.60) detection of PID  */
/*                      version changed. V3.60 was originally planned to have  */
/*                      different motor definitions.                           */
/*                      Added the ability to assign any sensor to be used      */
/*                      for rpm calculation, a bit of a kludge as I didn't     */
/*                      want to add a new variable so reused encoder_id        */
/*               V1.04  27 June 2013                                           */
/*                      Change license (added) to the Apache License           */
/*               V1.05  11 Nov 2013                                            */
/*                      Fix bug when speed limited and changing directions     */
/*                      quickly.                                               */
/*               V1.06  3 Sept 2014                                            */
/*                      Added support for the 393 Turbo Gears and ROBOTC V4.26 */
/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*    The author is supplying this software for use with the VEX cortex        */
/*    control system. This file can be freely distributed and teams are        */
/*    authorized to freely use this program , however, it is requested that    */
/*    improvements or additions be shared with the Vex community via the vex   */
/*    forum.  Please acknowledge the work of the authors when appropriate.     */
/*    Thanks.                                                                  */
/*                                                                             */
/*    Licensed under the Apache License, Version 2.0 (the "License");          */
/*    you may not use this file except in compliance with the License.         */
/*    You may obtain a copy of the License at                                  */
/*                                                                             */
/*      http://www.apache.org/licenses/LICENSE-2.0                             */
/*                                                                             */
/*    Unless required by applicable law or agreed to in writing, software      */
/*    distributed under the License is distributed on an "AS IS" BASIS,        */
/*    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. */
/*    See the License for the specific language governing permissions and      */
/*    limitations under the License.                                           */
/*                                                                             */
/*    Portions of this code are based on work by Chris Siegert aka vamfun on   */
/*    the Vex forums.                                                          */
/*    blog:  vamfun.wordpress.com for model details and vex forum threads      */
/*    email: vamfun_at_yahoo_dot_com                                           */
/*    Mentor for team 599 Robodox and team 1508 Lancer Bots                    */
/*                                                                             */
/*    The author can be contacted on the vex forums as jpearman                */
/*    email: jbpearman_at_mac_dot_com                                          */
/*    Mentor for team 8888 RoboLancers, Pasadena CA.                           */
/*                                                                             */
/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*    Description:                                                             */
/*                                                                             */
/*    This library is designed to work with encoded motors and provides        */
/*    functions to obtain velocity, instantaneous current and estimated PTC    */
/*    temperature.  This data is then used to allow fixed threshold or         */
/*    temperature based current limiting.                                      */
/*                                                                             */
/*    The algorithms used are based on experiments and work done during the    */
/*    summer of 2012 by Chris Siegert and James Pearman.                       */
/*                                                                             */
/*    This library makes extensive use of pointers and therefore needs ROBOTC  */
/*    V3.51 or later.                                                          */
/*                                                                             */
/*    The following vexforum threads have much of the background information   */
/*    used in this library.                                                    */
/*    http://www.vexforum.com/showthread.php?t=72100                           */
/*    http://www.vexforum.com/showthread.php?t=73318                           */
/*    http://www.vexforum.com/showthread.php?t=73960                           */
/*    http://www.vexforum.com/showthread.php?t=74594                           */
/*                                                                             */
/*    Global Memory use for V1.02 is 1404 bytes                                */
/*      1240 for motor data                                                    */
/*       156 for controller data                                               */
/*         8 misc                                                              */
/*                                                                             */
/*    CPU time for SmartMotorTask                                              */
/*    Motor calculations ~ 530uS,  approx 5% cpu bandwidth                     */
/*    Controller calculations with LED status ~ 1.25mS                         */
/*    Worse case is therefore about 1.8mS which occurs every 100mS             */
/*                                                                             */
/*    CPU time for SmartMotorSlewRateTask                                      */
/*    approx 400uS per 15mS loop, about 3% cpu bandwidth                       */
/*                                                                             */
/*-----------------------------------------------------------------------------*/
#pragma SystemFile
#ifndef __SMARTMOTORLIB__
#define __SMARTMOTORLIB__

// Version 1.05
#define kSmartMotorLibVersion   106

// We make extensive use of pointers so need recent versions of ROBOTC
#include "FirmwareVersion.h"
#if kRobotCVersionNumeric < 351
#error "SmartMotorLib requires ROBOTC Version 3.51 or newer"
#endif

// _Target_Emulator_ is new for 3.55, define for older versions
#if kRobotCVersionNumeric < 355
 #if (_TARGET == "Emulator")
  #ifndef _Target_Emulator_
   #define _Target_Emulator_   1
  #endif
 #endif
#endif

// Fix for versions of ROBOTC earlier than 4.26
#if kRobotCVersionNumeric < 426
#define tmotorVex393TurboSpeed_HBridge   9998
#define tmotorVex393TurboSpeed_MC29      9999
#endif

// ROBOTC with PID changed motor definitions - I used to use version number
// but 3.60 broke that and we don't know what version it will be now
// This will have to do for now until it's released after worlds
#ifdef bSmartMotorsWithEncoders
#define kRobotCHasPid
#endif


// System parameters - don't change
#define SMLIB_R_SYS             0.3
#define SMLIB_PWM_FREQ          1150
#define SMLIB_V_DIODE           0.75

// parameters for vex 393 motor
#define SMLIB_I_FREE_393        0.2
#define SMLIB_I_STALL_393       4.8
#define SMLIB_RPM_FREE_393      110
#define SMLIB_R_393             (7.2/SMLIB_I_STALL_393)
#define SMLIB_L_393             0.000650
#define SMLIB_Ke_393            (7.2*(1-SMLIB_I_FREE_393/SMLIB_I_STALL_393)/SMLIB_RPM_FREE_393)
#define SMLIB_I_SAFE393         0.90

// parameters for vex 269 motor
#define SMLIB_I_FREE_269        0.18
#define SMLIB_I_STALL_269       2.88
#define SMLIB_RPM_FREE_269      120
#define SMLIB_R_269             (7.2/SMLIB_I_STALL_269)
#define SMLIB_L_269             0.000650
#define SMLIB_Ke_269            (7.2*(1-SMLIB_I_FREE_269/SMLIB_I_STALL_269)/SMLIB_RPM_FREE_269)
#define SMLIB_I_SAFE269         0.75

// parameters for cortex and Power expander
// spec says 4A but we have set a little lower here
// may increase in a subsequent release.
#define SMLIB_I_SAFECORTEX      3.0
#define SMLIB_I_SAFEPE          3.0

// encoder counts per revolution depending on motor
#define SMLIB_TPR_269           240.448
#define SMLIB_TPR_393R          261.333
#define SMLIB_TPR_393S          392
#define SMLIB_TPR_393T          627.2
#define SMLIB_TPR_QUAD          360.0
#define SMLIB_TPR_POT           6000.0 // estimate

// Initial ambient temp 72deg F in deg C
#define SMLIB_TEMP_AMBIENT      (( 72.0-32.0) * 5 / 9)
// Trip temperature for PTC 100 deg C, may be a little low
#define SMLIB_TEMP_TRIP         100.0
// Trip hysteresis in deg C, once tripped we need a 10 deg drop to enable normal operation
#define SMLIB_TEMP_HYST         10.0
// Reference temperature for data below, 25 deg C
#define SMLIB_TEMP_REF          25.0

// Hold current is the current where thr PTC should not trip
// Time to trip is the time at 5 x hold current
// K_TAU is a safety factor, probably 0.5 ~ 0.8
// we are being conservative here, it trip occurs too soon then increase
//
// PTC HR16-400 used in cortex and power expander
#define SMLIB_I_HOLD_CORTEX     3.0
#define SMLIB_T_TRIP_CORTEX     1.7
#define SMLIB_K_TAU_CORTEX      0.5
#define SMLIB_TAU_CORTEX        (SMLIB_K_TAU_CORTEX * SMLIB_T_TRIP_CORTEX * 5.0 * 5.0)
#define SMLIB_C1_CORTEX         ( (SMLIB_TEMP_TRIP - SMLIB_TEMP_REF) / (SMLIB_I_HOLD_CORTEX * SMLIB_I_HOLD_CORTEX) )
#define SMLIB_C2_CORTEX         (1.0 / (SMLIB_TAU_CORTEX * 1000.0))

// PTC HR30-090 used in 393
//#define SMLIB_I_HOLD_393      0.9
#define SMLIB_I_HOLD_393        1.0 // increased a little to slow down trip
#define SMLIB_T_TRIP_393        7.1
#define SMLIB_K_TAU_393         0.5
#define SMLIB_TAU_393           (SMLIB_K_TAU_393 * SMLIB_T_TRIP_393 * 5.0 * 5.0)
#define SMLIB_C1_393            ( (SMLIB_TEMP_TRIP - SMLIB_TEMP_REF) / (SMLIB_I_HOLD_393 * SMLIB_I_HOLD_393) )
#define SMLIB_C2_393            (1.0 / (SMLIB_TAU_393 * 1000.0))

// PTC HR16-075 used in 269
#define SMLIB_I_HOLD_269        0.75
#define SMLIB_T_TRIP_269        2.0
#define SMLIB_K_TAU_269         0.5
#define SMLIB_TAU_269           (SMLIB_K_TAU_269 * SMLIB_T_TRIP_269 * 5.0 * 5.0)
#define SMLIB_C1_269            ( (SMLIB_TEMP_TRIP - SMLIB_TEMP_REF) / (SMLIB_I_HOLD_269 * SMLIB_I_HOLD_269) )
#define SMLIB_C2_269            (1.0 / (SMLIB_TAU_269 * 1000.0))

// No way to test 3 wire - Vamfun had them more or less the same as the 269
// PTC MINISMDC-075F used in three wire
#define SMLIB_C1_3WIRE          SMLIB_C1_269
#define SMLIB_C2_3WIRE          SMLIB_C2_269


// for forward reference
typedef struct  smartController;

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  This large structure holdes all the needed information for a single motor  */
/*                                                                             */
/*  The constants used in calculations are all set automatically by the init   */
/*  code.  Other variables are used by the different functions to calculate    */
/*  speed, current and temperature.  some of these are stored for debug        */
/*  purposes                                                                   */
/*                                                                             */
/*  V1.02 code uses 124 bytes (a couple are wasted due to word alignment)      */
/*  so 1240 bytes in all for the 10 motors.                                    */
/*-----------------------------------------------------------------------------*/

typedef struct {
    // the motor port
    tMotor      port;

    // the motor encoder port
    tMotor      eport;

    // copy of the system motor type
    TMotorTypes type;

    // due to alignment we get a free variable here
    // use for debugging loop delay
    short       delayTimeMs;

    // pointer to our control bank
    smartController *bank;

    // commanded speed comes from the user
    // requested speed is either the commanded speed or limited speed if the PTC
    // is about to trip
    short   motor_cmd;
    short   motor_req;
    short   motor_slew;

    // current limit and max cmd value
    short   limit_tripped;
    short   limit_cmd;
    float   limit_current;

    // the encoder associated with this motor
    short   encoder_id;
    // encoder ticks per rev
    float   ticks_per_rev;

    // variables used by rpm calculation
    long    enc;
    long    oldenc;
    float   delta;
    float   rpm;

    // variables used for current calculation
    float   i_free;
    float   i_stall;
    float   r_motor;
    float   l_motor;
    float   ke_motor;
    float   rpm_free;
    float   v_bemf_max;

    // instantaneous current
    float   current;
    // a filtered version of current to remove some transients
    float   filtered_current;
    // peak measured current
    float   peak_current;

    // holds safe current for this motor
    float   safe_current;
    // target current in limited mode
    float   target_current;

    // PTC monitor variables
    float   temperature;
    float   t_const_1;
    float   t_const_2;
    float   t_ambient;
    short   ptc_tripped;

    // Last program time we ran - may not keep this, bit overkill
    long    lastPgmTime;
    } smartMotor;

// storage for all motors
static smartMotor sMotors[ kNumbOfRealMotors ];
// We have no inline so use a macro as shortcut to get ptr
#define _SmartMotorGetPtr( index ) ((smartMotor *)&sMotors[ index ])

/*-----------------------------------------------------------------------------*/
/*  Motor control related definitions                                          */
/*-----------------------------------------------------------------------------*/

#define SMLIB_MOTOR_MAX_CMD             127     // maximum command value to motor
#define SMLIB_MOTOR_MIN_CMD             (-127)  // minimum command value to motor
#define SMLIB_MOTOR_DEFAULT_SLEW_RATE   10      // Default will cause 375mS from full fwd to rev
#define SMLIB_MOTOR_FAST_SLEW_RATE      256     // essentially off
#define SMLIB_MOTOR_DEADBAND            10      // values below this are set to 0

// When current limit is not needed set limit_cmd to this value
#define SMLIB_MOTOR_MAX_CMD_UNDEFINED   255     // special value for limit_motor

// backards compatibility with last years library
static TSemaphore  MotorSemaphore;

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  This structure holds all the information for a controller bank             */
/*  which is a PTC protected circuit in the cortex or power expander           */
/*                                                                             */
/*  We limit storage to three banks as that is the limit for a VEX competition */
/*  robot although you could have more power expanders in theory               */
/*                                                                             */
/*  V1.00 code uses 52 bytes per bank                                          */
/*-----------------------------------------------------------------------------*/

// 3 banks of maximum 5 motors (even though power expander is 4)
#define SMLIB_TOTAL_NUM_CONTROL_BANKS   3
#define SMLIB_TOTAL_NUM_BANK_MOTORS     5
// Index for banks
#define SMLIB_CORTEX_PORT_0             0
#define SMLIB_CORTEX_PORT_1             1
#define SMLIB_PWREXP_PORT_0             2

typedef struct {
    // array of pointers to the motors
    smartMotor *motors[SMLIB_TOTAL_NUM_BANK_MOTORS];

    // cumulative current
    float  current;
    // peak measured current
    float  peak_current;
    // holds safe current for this motor
    float  safe_current;

    // PTC monitor variables
    float  temperature;
    float  t_const_1;
    float  t_const_2;
    float  t_ambient;

    // flag for ptc status
    short  ptc_tripped;

    // Do we have an led to show tripped status
    tSensors statusLed;

    // Power expander may have a status port
    tSensors statusPort;
    } smartController;

static smartController sPorts[SMLIB_TOTAL_NUM_CONTROL_BANKS];
// We have no inline so use a macro as shortcut to get ptr
#define _SmartMotorControllerGetPtr( index ) ((smartController *)&sPorts[ index ])

/*-----------------------------------------------------------------------------*/
/*  Flags to determine behavior of the current limiting                        */
/*-----------------------------------------------------------------------------*/

// flag to hold global status to enable or disable the current limiter
// based on PTC temperature - defaults to off
static short    PtcLimitEnabled = false;

// flag to hold global status to enable or disable the current limiter
// based on preset threshold - defaults to on
static short    CurrentLimitEnabled = false;

/*-----------------------------------------------------------------------------*/
/*  External debug function called once per loop with smartMotor ptr           */
/*-----------------------------------------------------------------------------*/
#ifdef  __SMARTMOTORLIBDEBUG__
void    SmartMotorUserDebug( smartMotor *m );
#endif

/*******************************************************************************/
/*******************************************************************************/
/*  PUBLIC FUNCTIONS                                                           */
/*******************************************************************************/
/*******************************************************************************/

/*-----------------------------------------------------------------------------*/
/*  Get pointer to smartMotor structure - not used locally                     */
/*  use the _SmartMotorGetPtr instead for functions in this file               */
/*-----------------------------------------------------------------------------*/

smartMotor *
SmartMotorGetPtr( tMotor index )
{
    // bounds check index
    if((index < 0) || (index >= kNumbOfRealMotors))
        return( NULL );

    return( &sMotors[ index ] );
}

/*-----------------------------------------------------------------------------*/
/*  Get pointer to smartController structure - not used locally                */
/*  use the _SmartMotorControllerGetPtr instead for functions in this file     */
/*-----------------------------------------------------------------------------*/

smartController *
SmartMotorControllerGetPtr( short index )
{
    // bounds check index
    if((index < 0) || (index >= kNumbOfRealMotors))
        return( NULL );

    return( &sPorts[ index ] );
}

/*-----------------------------------------------------------------------------*/
/*  Get Motor speed                                                            */
/*-----------------------------------------------------------------------------*/

float
SmartMotorGetSpeed( tMotor index )
{
    // bounds check index
    if((index < 0) || (index >= kNumbOfRealMotors))
        return( 0 );

    return( sMotors[ index ].rpm );
}

/*-----------------------------------------------------------------------------*/
/*  Get Motor current                                                          */
/*-----------------------------------------------------------------------------*/

float
SmartMotorGetCurrent( tMotor index, int s = 0 )
{
    // bounds check index
    if((index < 0) || (index >= kNumbOfRealMotors))
        return( 0 );

    // normally return absolute current, s != 0 for signed
    if(s)
        return( sMotors[ index ].current );
    else
        return( abs( sMotors[ index ].current) );
}

/*-----------------------------------------------------------------------------*/
/*  Get Motor temperature                                                      */
/*-----------------------------------------------------------------------------*/

float
SmartMotorGetTemperature( tMotor index )
{
    // bounds check index
    if((index < 0) || (index >= kNumbOfRealMotors))
        return( 0 );

    return( sMotors[ index ].temperature );
}

/*-----------------------------------------------------------------------------*/
/*  Get Motor command limit                                                    */
/*-----------------------------------------------------------------------------*/

int
SmartMotorGetLimitCmd( tMotor index )
{
    // bounds check index
    if((index < 0) || (index >= kNumbOfRealMotors))
        return( 0 );

    return( sMotors[ index ].limit_cmd );
}

/*-----------------------------------------------------------------------------*/
/*  Set Motor current limit                                                    */
/*-----------------------------------------------------------------------------*/

void
SmartMotorSetLimitCurent( tMotor index, float current = 1.0 )
{
    // bounds check index
    if((index < 0) || (index >= kNumbOfRealMotors))
        return;

    sMotors[ index ].limit_current = current;
}

/*-----------------------------------------------------------------------------*/
/*  Set Motor maximum rpm                                                      */
/*-----------------------------------------------------------------------------*/

void
SmartMotorSetFreeRpm( tMotor index, short max_rpm )
{
    // bounds check index
    if((index < 0) || (index >= kNumbOfRealMotors))
        return;

    smartMotor *m = _SmartMotorGetPtr( index );

    // set the max rpm for this motor
    m->rpm_free = max_rpm;

    // recalculate back emf constant
    m->ke_motor = (7.2*(1-m->i_free/m->i_stall)/ m->rpm_free );

    // recalculate maximum theoretical v_bemf
    m->v_bemf_max = m->ke_motor * m->rpm_free;
}

/*-----------------------------------------------------------------------------*/
/*  Set Motor slew rate                                                        */
/*-----------------------------------------------------------------------------*/

void
SmartMotorSetSlewRate( tMotor index, int slew_rate = 10 )
{
    // bounds check index
    if((index < 0) || (index >= kNumbOfRealMotors))
        return;

    // negative or 0 is invalid
    if( slew_rate <= 0 )
        return;

    sMotors[ index ].motor_slew = slew_rate;
}

/*-----------------------------------------------------------------------------*/
/*  Get Controller current                                                     */
/*-----------------------------------------------------------------------------*/

float
SmartMotorGetControllerCurrent( short index )
{
    // bounds check index
    if((index < 0) || (index >= SMLIB_TOTAL_NUM_CONTROL_BANKS))
        return( 0 );

    return( sPorts[ index ].current );
}
/*-----------------------------------------------------------------------------*/
/*  Get Controller current                                                     */
/*-----------------------------------------------------------------------------*/

float
SmartMotorGetControllerTemperature( short index )
{
    // bounds check index
    if((index < 0) || (index >= SMLIB_TOTAL_NUM_CONTROL_BANKS))
        return( 0 );

    return( sPorts[ index ].temperature );
}

/*-----------------------------------------------------------------------------*/
/*  Set Controller status LED                                                  */
/*-----------------------------------------------------------------------------*/

void
SmartMotorSetControllerStatusLed( int index, tSensors port )
{
    // bounds check index
    if((index < 0) || (index >= SMLIB_TOTAL_NUM_CONTROL_BANKS))
        return;

    // digital outputs have reversed logic so automatically change
    if( SensorType[ port ] == sensorDigitalOut )
        SensorType[ port ] = sensorLEDtoVCC;

    // if an LED then use
    if( SensorType[ port ] == sensorLEDtoVCC )
        sPorts[ index ].statusLed = port;
}

/*-----------------------------------------------------------------------------*/
/*  Set power expander status port                                             */
/*-----------------------------------------------------------------------------*/

void
SmartMotorSetPowerExpanderStatusPort( tSensors port )
{
    // if an Analog input then use
    if( SensorType[ port ] == sensorAnalog )
        sPorts[ SMLIB_PWREXP_PORT_0 ].statusPort = port;
}

/*-----------------------------------------------------------------------------*/
/*  Enable current limit by monitoring the PTC temperatures                    */
/*-----------------------------------------------------------------------------*/

void
SmartMotorPtcMonitorEnable()
{
    CurrentLimitEnabled = false;
    PtcLimitEnabled     = true;
}

/*-----------------------------------------------------------------------------*/
/*  Disable current limit by monitoring the PTC temperatures                   */
/*-----------------------------------------------------------------------------*/

void
SmartMotorPtcMonitorDisable()
{
    PtcLimitEnabled     = false;
}

/*-----------------------------------------------------------------------------*/
/*  Enable current limit by using a preset threshold                           */
/*-----------------------------------------------------------------------------*/

void
SmartMotorCurrentMonitorEnable()
{
    PtcLimitEnabled     = false;
    CurrentLimitEnabled = true;
}

/*-----------------------------------------------------------------------------*/
/*  Disable current limit by using a preset threshold                          */
/*-----------------------------------------------------------------------------*/

void
SmartMotorCurrentMonitorDisable()
{
    CurrentLimitEnabled = false;
}

/*-----------------------------------------------------------------------------*/
/*  After initialization the smart motor tasks need to be started              */
/*-----------------------------------------------------------------------------*/

task SmartMotorTask();
task SmartMotorSlewRateTask();

void
SmartMotorRun()
{
    // Higher priority than slew rate task
    startTask( SmartMotorTask , 100 );
    // Higher priority than most user tasks
    startTask( SmartMotorSlewRateTask, 99 );

    // Initialize resource semaphore
    semaphoreInitialize(MotorSemaphore);
}

/*-----------------------------------------------------------------------------*/
/*  Stop smart motor tasks                                                     */
/*-----------------------------------------------------------------------------*/

void
SmartMotorStop()
{
    SmartMotorPtcMonitorDisable();
    SmartMotorCurrentMonitorDisable();

    stopTask( SmartMotorTask );
    stopTask( SmartMotorSlewRateTask );
}

/*-----------------------------------------------------------------------------*/
/*  Dump controller and motor status to the debug stream                       */
/*-----------------------------------------------------------------------------*/

void
SmartMotorDebugStatus()
{
    short   i, j;
    smartMotor      *m;
    smartController *s;

    // Cortext ports 1 - 5

    for(j=0;j<SMLIB_TOTAL_NUM_CONTROL_BANKS;j++)
        {
        s = _SmartMotorControllerGetPtr( j );
        if( j == SMLIB_CORTEX_PORT_0 )
            writeDebugStream("Cortex ports 1 - 5  - ");
        if( j == SMLIB_CORTEX_PORT_1 )
            writeDebugStream("Cortex ports 6 - 10 - ");
        if( j == SMLIB_PWREXP_PORT_0 )
            writeDebugStream("Power Expander      - ");

        writeDebugStream("Current:%5.2f ", s->current);
        writeDebugStream("Temp:%6.2f ", s->temperature);
        writeDebugStream("Status:%2d ", s->ptc_tripped);
        writeDebugStreamLine("");

        for(i=0;i<SMLIB_TOTAL_NUM_BANK_MOTORS;i++)
            {
            if( s->motors[i] != NULL )
                {
                m = s->motors[i];
                writeDebugStream("      Motor Port: %d - ", m->port );
                writeDebugStream("Current:%5.2f ", m->current);
                writeDebugStream("Temp:%6.2f ", m->temperature);
                writeDebugStream("Status:%2d ", m->ptc_tripped + (m->limit_tripped<<1) );
                writeDebugStreamLine("");
                }
            }
         writeDebugStreamLine("");
        }
}


/*-----------------------------------------------------------------------------*/
/*  New control value for the motor                                            */
/*  This is the only call that should be used to change motor speed            */
/*  using motor[] will completely bugger up the code - do not use it           */
/*                                                                             */
/*  The naming of this function is inconsistent with the rest of the library   */
/*  to allow backwards compatibility with team 8888's existing library         */
/*-----------------------------------------------------------------------------*/

void
SetMotor( int index, int value = 0, bool immeadiate = false )
{
    smartMotor  *m;

    // bounds check index
    if((index < 0) || (index >= kNumbOfRealMotors))
        return;

    // get motor
    m = _SmartMotorGetPtr( index );

    // limit value and set into motorReq
    if( value > SMLIB_MOTOR_MAX_CMD )
        m->motor_cmd = SMLIB_MOTOR_MAX_CMD;
    else
    if( value < SMLIB_MOTOR_MIN_CMD )
        m->motor_cmd = SMLIB_MOTOR_MIN_CMD;
    else
    if( abs(value) >= SMLIB_MOTOR_DEADBAND )
        m->motor_cmd = value;
    else
        m->motor_cmd = 0;

    // new - for hard stop
    if(immeadiate)
        motor[ index ] = value;
}

/*-----------------------------------------------------------------------------*/
/*  New users - do not use these calls, they are for backwards compatibity     */
/*  with the original team 8888 motorLib code                                  */
/*-----------------------------------------------------------------------------*/

void
MotorLibInit()
{
    SmartMotorStop();
    startTask( SmartMotorSlewRateTask );

    // Initialize resource semaphore
    semaphoreInitialize(MotorSemaphore);
}

int
MotorGetSemaphore()
{
    semaphoreLock( MotorSemaphore, 2);

    short s = getSemaphoreTaskOwner(MotorSemaphore);

    if ( s == nCurrentTask )
        return(1);
    else
        return(0);
}

void
MotorReleaseSemaphore()
{
    short s = getSemaphoreTaskOwner(MotorSemaphore);
    if ( s == nCurrentTask )
        semaphoreUnlock(MotorSemaphore);
}

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  Initialize the smart motor library - This function must be called once     */
/*  when the program starts.  Motors are automatically detected and assigned   */
/*  to the cortex banls, if a power expander is used then it should be added   */
/*  by using the SmartMotorsAddPowerExtender function after calling            */
/*  SmartMotorsInit                                                            */
/*-----------------------------------------------------------------------------*/

void
SmartMotorsInit()
{
    int         i, j;
    smartMotor  *m;

    // clear controllers
    for(j=0;j<SMLIB_TOTAL_NUM_CONTROL_BANKS;j++)
        {
        for(i=0;i<SMLIB_TOTAL_NUM_BANK_MOTORS;i++)
            sPorts[j].motors[i] = NULL;

        sPorts[j].t_const_1    = SMLIB_C1_CORTEX;
        sPorts[j].t_const_2    = SMLIB_C2_CORTEX;
        sPorts[j].t_ambient    = SMLIB_TEMP_AMBIENT;
        sPorts[j].temperature  = sPorts[j].t_ambient;
        sPorts[j].current      = 0;
        sPorts[j].peak_current = 0;
        sPorts[j].safe_current = SMLIB_I_SAFECORTEX; // cortex and PE the same
        sPorts[j].statusLed    = (tSensors)(-1);
        sPorts[j].statusPort   = (tSensors)(-1);
        }


    // unfortunate kludge as getEncoderForMotor will not accept a variable yet
    sMotors[0].encoder_id = getEncoderForMotor( port1  );
    sMotors[1].encoder_id = getEncoderForMotor( port2  );
    sMotors[2].encoder_id = getEncoderForMotor( port3  );
    sMotors[3].encoder_id = getEncoderForMotor( port4  );
    sMotors[4].encoder_id = getEncoderForMotor( port5  );
    sMotors[5].encoder_id = getEncoderForMotor( port6  );
    sMotors[6].encoder_id = getEncoderForMotor( port7  );
    sMotors[7].encoder_id = getEncoderForMotor( port8  );
    sMotors[8].encoder_id = getEncoderForMotor( port9  );
    sMotors[9].encoder_id = getEncoderForMotor( port10 );

    for(i=0;i<kNumbOfRealMotors;i++)
        {
        m = _SmartMotorGetPtr( i );
        m->port  = (tMotor)i;
        m->eport = (tMotor)i;
        m->type  = (TMotorTypes)motorType[ m->port ];

        switch( m->type )
            {
            // 393 set for high torque
#ifndef kRobotCHasPid
            case    tmotorVex393:
#else
            case    tmotorVex393_HBridge:
            case    tmotorVex393_MC29:
#endif
                m->i_free   = SMLIB_I_FREE_393;
                m->i_stall  = SMLIB_I_STALL_393;
                m->r_motor  = SMLIB_R_393;
                m->l_motor  = SMLIB_L_393;
                m->ke_motor = SMLIB_Ke_393;
                m->rpm_free = SMLIB_RPM_FREE_393;

                m->ticks_per_rev = SMLIB_TPR_393T;

                m->safe_current = SMLIB_I_SAFE393;

                m->t_const_1 = SMLIB_C1_393;
                m->t_const_2 = SMLIB_C2_393;
                break;

            // 393 set for high speed
#ifndef kRobotCHasPid
            case    tmotorVex393HighSpeed:
#else
            case    tmotorVex393HighSpeed_HBridge:
            case    tmotorVex393HighSpeed_MC29:
#endif
                m->i_free   = SMLIB_I_FREE_393;
                m->i_stall  = SMLIB_I_STALL_393;
                m->r_motor  = SMLIB_R_393;
                m->l_motor  = SMLIB_L_393;
                m->ke_motor = SMLIB_Ke_393/1.6;
                m->rpm_free = SMLIB_RPM_FREE_393 * 1.6;

                m->ticks_per_rev = SMLIB_TPR_393S;

                m->safe_current = SMLIB_I_SAFE393;

                m->t_const_1 = SMLIB_C1_393;
                m->t_const_2 = SMLIB_C2_393;
                break;

            // 393 set for Turbo
#ifndef kRobotCHasPid
            // ROBOTC V3.XX has not been updated yet
            // case    tmotorVex393TurboSpeed:
#else
            case    tmotorVex393TurboSpeed_HBridge:
            case    tmotorVex393TurboSpeed_MC29:
#endif
                m->i_free   = SMLIB_I_FREE_393;
                m->i_stall  = SMLIB_I_STALL_393;
                m->r_motor  = SMLIB_R_393;
                m->l_motor  = SMLIB_L_393;
                m->ke_motor = SMLIB_Ke_393/2.4;
                m->rpm_free = SMLIB_RPM_FREE_393 * 2.4;

                m->ticks_per_rev = SMLIB_TPR_393R;

                m->safe_current = SMLIB_I_SAFE393;

                m->t_const_1 = SMLIB_C1_393;
                m->t_const_2 = SMLIB_C2_393;
                break;

            // 269 and 3wire set the same
#ifndef kRobotCHasPid
            case    tmotorVex269:
#else
            case    tmotorVex269_HBridge:
            case    tmotorVex269_MC29:
#endif
            case    tmotorServoContinuousRotation:
                m->i_free   = SMLIB_I_FREE_269;
                m->i_stall  = SMLIB_I_STALL_269;
                m->r_motor  = SMLIB_R_269;
                m->l_motor  = SMLIB_L_269;
                m->ke_motor = SMLIB_Ke_269;
                m->rpm_free = SMLIB_RPM_FREE_269;

                m->ticks_per_rev = SMLIB_TPR_269;

                m->safe_current = SMLIB_I_SAFE269;

                m->t_const_1 = SMLIB_C1_269;
                m->t_const_2 = SMLIB_C2_269;
                break;

            default:
                // force OFF
                // Servos, flashlights etc. not considered.
                m->type = tmotorNone;
                break;
            }

        // Override encoder ticks if not an IME
        if( m->encoder_id < 0 )
            {
            // No encoder
            m->ticks_per_rev = -1;
            m->enc    = 0;
            m->oldenc = 0;
            }
        else
        if( m->encoder_id < 20 ) {
            // quad encoder
            m->ticks_per_rev = SMLIB_TPR_QUAD;
            m->enc    = nMotorEncoder[ m->eport ];
            m->oldenc = nMotorEncoder[ m->eport ];
            }
        else
            {
            m->enc    = nMotorEncoder[ m->eport ];
            m->oldenc = nMotorEncoder[ m->eport ];
            }

        // use until overidden by user
        m->t_ambient = SMLIB_TEMP_AMBIENT;
        m->temperature = m->t_ambient;

        // default for target is safe
        m->target_current = m->safe_current;
        // default for limit is safe
        m->limit_current  = m->safe_current;

        // reset stats
        m->peak_current = 0;

        // we are good to go
        m->limit_tripped = false;
        m->ptc_tripped   = false;
        m->limit_cmd     = SMLIB_MOTOR_MAX_CMD_UNDEFINED;

        // maximum theoretical v_bemf
        m->v_bemf_max = m->ke_motor * m->rpm_free;

        // add to controller
        if( m->type != tmotorNone )
            {
            if( i < SMLIB_TOTAL_NUM_BANK_MOTORS )
                sPorts[SMLIB_CORTEX_PORT_0].motors[i] = m;
            else
                sPorts[SMLIB_CORTEX_PORT_1].motors[i-SMLIB_TOTAL_NUM_BANK_MOTORS] = m;
            }

        // which bank are we on
        if( i < SMLIB_TOTAL_NUM_BANK_MOTORS )
            m->bank = &sPorts[SMLIB_CORTEX_PORT_0];
        else
            m->bank = &sPorts[SMLIB_CORTEX_PORT_1];

        // we have never run
        m->lastPgmTime = -1;
        }
}

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  This lets us use encoder counts from one motor for another                 */
/*  this can also be achieved in the motors & sensors setup but only           */
/*  if the motors are of the same type.                                        */
/*  Use this function if the motors differ, eg. slave a 269 to a 393           */
/*                                                                             */
/*-----------------------------------------------------------------------------*/

void
SmartMotorLinkMotors( tMotor master, tMotor slave )
{
    // bounds check master
    if((master < 0) || (master >= kNumbOfRealMotors))
        return;

    // bounds check master
    if((slave  < 0) || (slave  >= kNumbOfRealMotors))
        return;

    // get motor pointers for master ans slave
    smartMotor  *m = _SmartMotorGetPtr( master );
    smartMotor  *s = _SmartMotorGetPtr( slave  );

    // master need to have an encoder
    if( m->encoder_id == (-1) )
        return;

    // encoder port is the master port
    s->eport         = m->port;

    // link, assume 1:1 gearing
    s->ticks_per_rev = m->ticks_per_rev;
    s->encoder_id    = m->encoder_id;
    s->enc           = m->enc;
    s->oldenc        = m->oldenc;
}

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  If using a quad encoder then it may not be directly connected to the motor */
/*  This function allows a change of gearing by changing the ticks per rpm.    */
/*  Only call this function once per motor after calling init                  */
/*                                                                             */
/*  ratio is encoder revs/motor revs                                           */
/*-----------------------------------------------------------------------------*/

void
SmartMotorsSetEncoderGearing( tMotor index, float ratio )
{
    // bounds check master
    if((index < 0) || (index >= kNumbOfRealMotors))
        return;

    smartMotor  *m = _SmartMotorGetPtr( index );

    // Not for IMEs or non encoded motors
    if( (m->encoder_id >= 0) && (m->encoder_id < 20))
        m->ticks_per_rev = m->ticks_per_rev * ratio;
}

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  Add a power extender to the system                                         */
/*  Move the motors currently assigned to the cortex to the power extender     */
/*  Only call this function once after SmartMotorInit                          */
/*-----------------------------------------------------------------------------*/

void
SmartMotorsAddPowerExtender( int p0, int p1 = (-1), int p2 = (-1), int p3 = (-1) )
{
    int     i;
    int     p = 0;
    smartMotor  *m;

    for(i=0;i<kNumbOfRealMotors;i++)
        {
        m = _SmartMotorGetPtr( i );

        // has to be a real motor
        // ignore ports 1 & 10 which are 2 wire ports
        if( (m->type != tmotorNone) && (m->port > port1) && (m->port < port10) )
            {
            // assigning this port to the power expander ?
            if( (i==p0) || (i==p1) || (i==p2) || (i==p3))
                {
                // link motor to power expander bank
                sPorts[SMLIB_PWREXP_PORT_0].motors[p] = m;
                m->bank = &sPorts[SMLIB_PWREXP_PORT_0];
                p++;

                // remove from cortex
                if( i < SMLIB_TOTAL_NUM_BANK_MOTORS )
                    sPorts[SMLIB_CORTEX_PORT_0].motors[i] = NULL;
                else
                    sPorts[SMLIB_CORTEX_PORT_1].motors[i-SMLIB_TOTAL_NUM_BANK_MOTORS] = NULL;
                }
            }
        }
}

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  Assign a sensor to be used to calculate rpm for a motor                    */
/*  This is a special case and is used for, example, when a pot is geared      */
/*  to a lift motor                                                            */
/*                                                                             */
/*  ticks_per_rev for a pot geared 1:1 would be about 6000                     */
/*-----------------------------------------------------------------------------*/

#define ENCODER_ID_SENSOR   0x800

void
SmartMotorSetRpmSensor( tMotor index, tSensors port, float ticks_per_rev, bool reversed = false )
{
    // bounds check master
    if((index < 0) || (index >= kNumbOfRealMotors))
        return;

    smartMotor  *m = _SmartMotorGetPtr( index );

    // This could be an analog port or (unlikely) an encoder
    // that was not defined in the motors&sensors setup

    if( (port >= in1) && (port <= dgtl12 ) )
        {
        m->encoder_id    = ENCODER_ID_SENSOR + (short)port;
        m->ticks_per_rev = ticks_per_rev;
        // use negative ticks per rev if reversed sensor
        if( reversed )
            m->ticks_per_rev = -m->ticks_per_rev;
        }
}

/*******************************************************************************/
/*******************************************************************************/
/*  PRIVATE FUNCTIONS                                                          */
/*******************************************************************************/
/*******************************************************************************/
//
// No bounds checking in the private functions
//

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*    calculate speed in rpm for the motor                                     */
/*                                                                             */
/*-----------------------------------------------------------------------------*/

#ifdef _Target_Emulator_
    float   motordrag = 1.0;
#endif

static void
SmartMotorSpeed( smartMotor *m, int deltaTime )
{
#ifdef _Target_Emulator_
    // dummy increment based on speed
    int increment = m->ticks_per_rev * 0.2 * motordrag;
    // cap at 100 rpm
    if( abs(increment) > 100)
        increment = sgn(increment) * 100;
    // increase(or decrease) for testing in emulator
    m->enc = m->enc += increment; // debug
#else
    // Get encoder value
    m->enc = nMotorEncoder[m->eport];
#endif

    // calculate encoder delta
    m->delta  = m->enc - m->oldenc;
    m->oldenc = m->enc;

    // calculate the rpm for the motor
    m->rpm = (1000.0/deltaTime) * m->delta * 60.0 / m->ticks_per_rev;
}

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  We need to do something for motors which do not have encoders and are      */
/*  not coupled to another motor and able to share an encoder                  */
/*                                                                             */
/*-----------------------------------------------------------------------------*/

void
SmartMotorSimulateSpeed( smartMotor *m )
{
    static  short   spd_table[10] = {0, 26, 58, 77, 85, 92, 95, 98, 100, 100};
            short   cmd;
            short   index;
            float   f, scale, speed;

    // get estimated speed from table using bilinear interpolation
    // and simple speed LUT
    cmd   =  motor[ m->port ];
    // index to tabke
    index =  abs( cmd ) >> 4; // div by 16
    // fractional part indicates where we are between two table values
    f     = (abs( cmd ) - (index << 4)) / 16.0;

    // assume motor is running at 90% of commanded speed
    // we really have no idea and if we set this too slow it will
    // just trip the current limiters.
    scale = m->rpm_free / 100.0 * 0.90;
    speed = spd_table[index] + (spd_table[index+1] - spd_table[index]) * f;

    m->rpm = sgn(cmd) * (speed * scale);
}

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  Calculate speed using sensor rather than encoder for motor                 */
/*                                                                             */
/*-----------------------------------------------------------------------------*/

static void
SmartMotorSensorSpeed( smartMotor *m, int deltaTime )
{
    tSensors    port;

    // get port from special encoder_id
    port = (tSensors)(m->encoder_id - ENCODER_ID_SENSOR);
    if( (port < in1) || (port > dgtl12 ) )
        return;

    // Get sensor value
    m->enc = SensorValue[port];

    // calculate encoder delta
    m->delta  = m->enc - m->oldenc;
    m->oldenc = m->enc;

    // calculate the rpm for the motor
    m->rpm = (1000.0/deltaTime) * m->delta * 60.0 / m->ticks_per_rev;
}

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  Estimate current in Vex motor using vamfun's algorithm                     */
/*  subroutine written by Vamfun...Mentor Vex 1508, 599.                       */
/*  7.13.2012  vamfun@yahoo.com... blog info  http://vamfun.wordpress.com      */
/*                                                                             */
/*  Modified by James Pearman 7.28.2012                                        */
/*  Modified by James Pearman 10.1.2012 - more generalized code                */
/*                                                                             */
/*  If cmd is positive then rpm must also be positive for this to work         */
/*                                                                             */
/*-----------------------------------------------------------------------------*/

float
SmartMotorCurrent( smartMotor *m, float v_battery  )
{
    float   v_bemf;
    float   c1, c2;
    float   lamda;

    float   duty_on, duty_off;

    float   i_max, i_bar, i_0;
    float   i_ss_on, i_ss_off;

    int     dir;

    // get current cmd
    int     cmd = motor[ m->port ];

    // rescale control value
    // ports 2 through 9 behave a little differently
    if( m->port > port1 && m->port < port10 )
        cmd = (cmd * 128) / 90;

    // clip control value to +/- 127
    if( abs(cmd) > 127 )
        cmd = sgn(cmd) * 127;

    // which way are we turning ?
    // modified to use rpm near command value of 0 to reduce transients
    if( abs(cmd) > 10 )
        dir = sgn(cmd);
    else
        dir = sgn(m->rpm);


    duty_on = abs(cmd)/127.0;

    // constants for this pwm cycle
    lamda = m->r_motor/((float)SMLIB_PWM_FREQ * m->l_motor);
    c1    = exp( -lamda *    duty_on  );
    c2    = exp( -lamda * (1-duty_on) );

    // Calculate back emf voltage
    v_bemf  = m->ke_motor * m->rpm;

    // new - clip v_bemf, stops issues if motor runs faster than rpm_free
    if( abs(v_bemf) > m->v_bemf_max )
        v_bemf = sgn(v_bemf) * m->v_bemf_max;

    // Calculate staady state current for on and off pwm phases
    i_ss_on  =  ( v_battery * dir - v_bemf ) / (m->r_motor + SMLIB_R_SYS);
    i_ss_off = -( SMLIB_V_DIODE   * dir + v_bemf ) /  m->r_motor;

    // compute trial i_0
    i_0 = (i_ss_on*(1-c1)*c2 + i_ss_off*(1-c2))/(1-c1*c2);

    //check to see if i_0 crosses 0 during off phase if diode were not in circuit
    if(i_0*dir < 0)
        {
        // waveform reaches zero during off phase of pwm cycle hence
        // ON phase will start at 0
        // once waveform reaches 0, diode clamps the current to zero.
        i_0 = 0;

        // peak current
        i_max = i_ss_on*(1-c1);

        //where does the zero crossing occur
        duty_off = -log(-i_ss_off/(i_max-i_ss_off))/lamda ;
        }
    else
        {
        // peak current
        i_max = i_0*c1 + i_ss_on*(1-c1);

        // i_0 is non zero so final value of waveform must occur at end of cycle
        duty_off = 1 - duty_on;
        }

    // Average current for cycle
    i_bar = i_ss_on*duty_on + i_ss_off*duty_off;

    // Save current
    m->current = i_bar;

    // simple iir filter to remove transients
    m->filtered_current = (m->filtered_current * 0.8) + (i_bar * 0.2);

    // peak current - probably not useful
    if( abs(m->current) > m->peak_current )
        m->peak_current = abs(m->current);

    return i_bar;
}

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  calculate the current for a controller bank                                */
/*  A bank is ports 1-5, ports 6-10 on the cortex or a power expander          */
/*                                                                             */
/*-----------------------------------------------------------------------------*/

float
SmartMotorControllerCurrent( smartController *s )
{
    int     i;

    s->current = 0;

    // Controller current wil always be positive
    // always flows from battery :)
    for(i=0;i<SMLIB_TOTAL_NUM_BANK_MOTORS;i++)
        {
        if( s->motors[i] != NULL )
            {
            if( s->motors[i]->type != tmotorNone )
                s->current += abs( s->motors[i]->current );
            }
        }

    // peak current - probably no use
    if( s->current > s->peak_current )
        s->peak_current = s->current;

    return( s->current );
}

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*      Calculate a command for the motor which will result in a target        */
/*      current based on motor speed                                           */
/*                                                                             */
/*      target_current should be set prior to calling the function and must    */
/*      be POSITIVE.                                                           */
/*                                                                             */
/*      If command direction and rpm are not of the same polarity then this    */
/*      function tends to fallover due to back emf being of opposite direction */
/*      to the drive direction.  In this situation we have no choice but to    */
/*      let current go higher as the motor is changing directions and will in  */
/*      effect stall as it does through zero.                                  */
/*                                                                             */
/*-----------------------------------------------------------------------------*/

int
SmartMotorSafeCommand( smartMotor *m, float v_battery  )
{
    // get current cmd
    int     cmd = motor[ m->port ];

    // cmd polarity must match rpm polarity
    // broken up a bit to reduce multiplies
    if (cmd >= 0)
        {
        if (m->rpm >= 0)
            cmd = SMLIB_MOTOR_MAX_CMD * ((m->rpm * m->ke_motor) + (m->target_current * (m->r_motor + SMLIB_R_SYS)) + SMLIB_V_DIODE) / ( v_battery + SMLIB_V_DIODE );
        else
            cmd = SMLIB_MOTOR_MAX_CMD;

        // clip
        if( cmd > SMLIB_MOTOR_MAX_CMD )
            cmd = SMLIB_MOTOR_MAX_CMD;
        }
    else
        {
        if(m->rpm <= 0)
            cmd = SMLIB_MOTOR_MAX_CMD * ((m->rpm * m->ke_motor) - (m->target_current * (m->r_motor + SMLIB_R_SYS)) - SMLIB_V_DIODE) / ( v_battery + SMLIB_V_DIODE );
        else
            cmd = SMLIB_MOTOR_MIN_CMD;

        // clip
        if( cmd < SMLIB_MOTOR_MIN_CMD )
            cmd = SMLIB_MOTOR_MIN_CMD;
        }

    // override if current is 0
    if( m->target_current == 0 )
        cmd = 0;

    // ports 2 through 9 behave a little differently
    if( m->port > port1 && m->port < port10 )
        cmd = (cmd * 90) / 128;

    return( cmd );
}

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  Calculate the PTC temperature for a motor                                  */
/*  math by vamfun                                                             */
/*                                                                             */
/*-----------------------------------------------------------------------------*/

float
SmartMotorTemperature( smartMotor *m, int deltaTime )
{
    float   rate;

    rate = m->t_const_2 * (m->current * m->current * m->t_const_1 - (m->temperature - m->t_ambient));

    m->temperature = m->temperature + (rate * deltaTime);

    return( m->temperature );
}

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  Calculate the PTC temperature for a controller bank                        */
/*  math by vamfun                                                             */
/*                                                                             */
/*-----------------------------------------------------------------------------*/

float
SmartMotorControllerTemperature( smartController *s, int deltaTime  )
{
    float   rate;

    rate = s->t_const_2 * (s->current * s->current * s->t_const_1 - (s->temperature - s->t_ambient));

    s->temperature = s->temperature + (rate * deltaTime);

    return( s->temperature );
}

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  Monitor Motor PTC temperature                                              */
/*                                                                             */
/*  If the temperature is above the set point then calculate a command that    */
/*  will result in a safe current                                              */
/*                                                                             */
/*-----------------------------------------------------------------------------*/

void
SmartMotorMonitorPtc( smartMotor *m, float v_battery )
{
    if( !m->ptc_tripped ) {
        if( m->temperature > SMLIB_TEMP_TRIP )
            m->ptc_tripped = true;
    }
    else {
        // 10 deg hysterisis
        if( m->temperature < (SMLIB_TEMP_TRIP - SMLIB_TEMP_HYST) )
            m->ptc_tripped = false;
    }

    // Is the bank ptc tripped ?
    // If so then leave limit_cmd alone
    if( m->bank != NULL )
        {
        if( m->bank->ptc_tripped )
            return;
        }

    // Is (or was) the ptc tripped
    if( m->ptc_tripped )
        {
        // we are using target_current as a debugging means
        // it must be positive
        m->target_current = m->safe_current;
        // maximum cmd value
        m->limit_cmd = SmartMotorSafeCommand( m, v_battery);
        }
    else
        {
        // allow max speed
        m->limit_cmd = SMLIB_MOTOR_MAX_CMD_UNDEFINED;
        }
}

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  Monitor controller bank PTC temperature                                    */
/*                                                                             */
/*  If the temperature is above the set point then calculate a commands for    */
/*  each motor that will result in a safe current                              */
/*                                                                             */
/*-----------------------------------------------------------------------------*/

void
SmartMotorControllerMonitorPtc( smartController *s, float v_battery )
{
    smartMotor    *m;
    int            i;

    if( !s->ptc_tripped ) {
        if( s->temperature > SMLIB_TEMP_TRIP )
            s->ptc_tripped = true;
    }
    else {
        // 10 deg hysterisis
        if( s->temperature < (SMLIB_TEMP_TRIP - SMLIB_TEMP_HYST) )
            s->ptc_tripped = false;
    }

    // Is (or was) the PTC tripped
    // this will constantly be recalculated
    if( s->ptc_tripped )
        {
        // now decide how to fix it.
        // divide amongst active motors, same current for each one we are using
        // type does not matter
        int active_motors = 0;
        for(i=0;i<SMLIB_TOTAL_NUM_BANK_MOTORS;i++)
            {
            m = s->motors[i];
            if( m != NULL )
                {
                if( abs(m->current) > 0.1 )
                    active_motors++;
                }
            }

        // avoid divide by 0
        if( active_motors == 0)
            active_motors = 1;

        // calculate safe current based on number of active motors
        float m_safe_current = s->safe_current / active_motors;

        for(i=0;i<SMLIB_TOTAL_NUM_BANK_MOTORS;i++)
            {
            m = s->motors[i];
            if( m != NULL )
                {
                // using target_current as a debugging means
                // it must be positive
                // see if the motor is tripped as well and use lowest current
                if( m->ptc_tripped && (m->safe_current < m_safe_current) )
                    m->target_current = m->safe_current;
                else
                    m->target_current = m_safe_current;

                m->limit_cmd = SmartMotorSafeCommand( m, v_battery );
                }
            }
        }
}

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  Monitor Motor Current                                                      */
/*                                                                             */
/*-----------------------------------------------------------------------------*/

void
SmartMotorMonitorCurrent( smartMotor *m, float v_battery )
{
    m->target_current = m->limit_current;

    // The way this is setup is that if the limit is tripped you
    // will probably need to back off the controls or allow the motor to speed up
    // before the limit is cancelled.  This is to stop oscilation.
    // The 0.9 below controls this behavior.
    if( !m->limit_tripped ) {
        if( abs(m->filtered_current) > m->target_current )
            m->limit_tripped = true;
    }
    else {
        if( abs(m->filtered_current) < (m->target_current * 0.9) )
            m->limit_tripped = false;
    }

    if( m->limit_tripped )
        // maximum cmd value
        m->limit_cmd = SmartMotorSafeCommand( m, v_battery);
    else
        m->limit_cmd = SMLIB_MOTOR_MAX_CMD_UNDEFINED;
}

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  Turn on status LED if either controller ptc is tripped or any connected    */
/*  motors are tripped.                                                        */
/*  LED is only turned on in this function as it may be shared between more    */
/*  then one controller.  The LED is briefly turned off each loop around the   */
/*  calling task but only remains off for a very short period which is         */
/*  inperceptable.                                                             */
/*                                                                             */
/*-----------------------------------------------------------------------------*/

void
SmartMotorControllerSetLed( smartController *s )
{
    int         i;
    smartMotor *m;
    int         status;

    if(s->statusLed < 0)
        return;

    status = s->ptc_tripped;

    for(i=0;i<SMLIB_TOTAL_NUM_BANK_MOTORS;i++)
        {
        m = s->motors[i];

        if( m != NULL )
            status += (m->ptc_tripped + m->limit_tripped);
        }

    if(status)
        SensorValue[ s->statusLed ] = 1;
}

/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  Running a little different in this version, instead of a 100mS delay       */
/*  and then calculations on each motor we do one motor each itteration        */
/*                                                                             */
/*-----------------------------------------------------------------------------*/

task
SmartMotorTask()
{
    static  int loopDelay = 10;
    static  int delayTimeMs = kNumbOfRealMotors * loopDelay;
            int i;
            int nextMotor = 0;
            float   v_battery;

    while(1)
        {
#ifdef  _smTestPoint_1
        // debug time spent in this task
        SensorValue[ _smTestPoint_1 ] = 1;
#endif

        v_battery = nAvgBatteryLevel/1000.0;

        smartMotor *m = _SmartMotorGetPtr( nextMotor );

        // may be overkill, could just use default from above
        delayTimeMs = nPgmTime - m->lastPgmTime;
        m->lastPgmTime = nPgmTime;
        m->delayTimeMs = delayTimeMs; // debug

        // Set current etc. for one motor if it exists and has an encoder
        if( m->type != tmotorNone )
            {
            if( m->encoder_id >= 0)
                {
                if( m->encoder_id < ENCODER_ID_SENSOR )
                    SmartMotorSpeed( m, delayTimeMs );
                else
                    SmartMotorSensorSpeed( m, delayTimeMs );
                }
            else
                SmartMotorSimulateSpeed( m );

            SmartMotorCurrent( m, v_battery );
            SmartMotorTemperature( m, delayTimeMs );
            if( PtcLimitEnabled )
                SmartMotorMonitorPtc( m, v_battery );
            if( CurrentLimitEnabled )
                SmartMotorMonitorCurrent( m, v_battery );
            }

#ifdef  __SMARTMOTORLIBDEBUG__
        // Call user debug code
        SmartMotorUserDebug( m );
#endif
        // next motor
        if(++nextMotor == kNumbOfRealMotors)
            {
            nextMotor = 0;

            // now set cortext current
            // this is much quicker than setting the motor currents so do all
            // three ports, cortex and power expander.
            for( i=0;i<SMLIB_TOTAL_NUM_CONTROL_BANKS;i++ )
                {
                smartController *s = _SmartMotorControllerGetPtr( i );

                SmartMotorControllerCurrent( s );
                SmartMotorControllerTemperature( s, delayTimeMs );

                if( PtcLimitEnabled )
                    SmartMotorControllerMonitorPtc( s, v_battery );

                // turn off status leds here, more than one controller may
                // share an led so we turn them off each loop
                // and then any tripped controller may turn them on.
                if( s->statusLed >= dgtl1 )
                    SensorValue[ s->statusLed ] = 0;
                }

            // check status LED
            for( i=0;i<SMLIB_TOTAL_NUM_CONTROL_BANKS;i++ )
                {
                smartController *s = _SmartMotorControllerGetPtr( i );
                if( s->statusLed >= dgtl1 )
                    SmartMotorControllerSetLed(s);
                }

            // Monitor power expander status port
            smartController *s = _SmartMotorControllerGetPtr( SMLIB_PWREXP_PORT_0 );
            if( s->statusPort >= in1 )
                {
                // assume A2 power expander
                float pe_battery = SensorValue[ s->statusPort ] / 270.0;
                // Use 3 volts as threshold, should work for old and new power expanders
                if( pe_battery < 3.0 )
                    {
                    // tripped - bad !
                    s->temperature  = 110;
                    // drop safe current forever to 0
                    s->safe_current = 0;
                    }
                }
            }

#ifdef  _smTestPoint_1
        // debug time spent in this task
        SensorValue[ _smTestPoint_1 ] = 0;
#endif
        // wait
        wait1Msec(loopDelay);
        }
}


/*-----------------------------------------------------------------------------*/
/*                                                                             */
/*  Task  - compares the requested speed of each motor to the actual speed     */
/*          and increments or decrements to reduce the difference as necessary */
/*                                                                             */
/*-----------------------------------------------------------------------------*/

task SmartMotorSlewRateTask()
{
    static  int delayTimeMs = 15;
    int motorIndex;
    int motorTmp;
    smartMotor  *m;

    // Initialize stuff
    for(motorIndex=0;motorIndex<kNumbOfRealMotors;motorIndex++)
        {
        m = _SmartMotorGetPtr( motorIndex );
        m->motor_req  = 0;
        m->motor_cmd  = 0;
        m->motor_slew = SMLIB_MOTOR_DEFAULT_SLEW_RATE;
        }

    // run task until stopped
    while( true )
        {
#ifdef  _smTestPoint_2
        // debug time spent in this task
        SensorValue[ _smTestPoint_2 ] = 1;
#endif
        // run loop for every motor
        for( motorIndex=0; motorIndex<kNumbOfRealMotors; motorIndex++)
            {
            m = _SmartMotorGetPtr( motorIndex );

            // So we don't keep accessing the internal storage
            motorTmp = motor[ m->port ];

            // check for limiting
            if( (PtcLimitEnabled || CurrentLimitEnabled) && (m->limit_cmd != SMLIB_MOTOR_MAX_CMD_UNDEFINED) )
                {
                if( abs(m->motor_cmd) > abs(m->limit_cmd) ) {
                    // don't limit if we are reversing direction
                    if( sgn(m->motor_cmd) == sgn(m->limit_cmd) )
                        m->motor_req = m->limit_cmd;
                    else
                        m->motor_req = m->motor_cmd;
                    }
                else
                    m->motor_req = m->motor_cmd;
                }
            else
                m->motor_req = m->motor_cmd;

            // Do we need to change the motor value ?
            if( motorTmp != m->motor_req )
                {
                // increasing motor value
                if( m->motor_req > motorTmp )
                    {
                    motorTmp += m->motor_slew;
                    // limit
                    if( motorTmp > m->motor_req )
                        motorTmp = m->motor_req;
                    }

                // increasing motor value
                if( m->motor_req < motorTmp )
                    {
                    motorTmp -= m->motor_slew;
                    // limit
                    if( motorTmp < m->motor_req )
                        motorTmp = m->motor_req;
                    }

                // finally set motor
                motor[ m->port ] = motorTmp;
                }
            }

#ifdef  _smTestPoint_2
        // debug time spent in this task
        SensorValue[ _smTestPoint_2 ] = 0;
#endif
        // Wait approx the speed of motor update over the spi
        wait1Msec( delayTimeMs );
        }
}

#endif  // __SMARTMOTORLIB__
