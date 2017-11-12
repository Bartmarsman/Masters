/*
 * sfuntmpl_basic.c: Basic 'C' template for a level 2 S-function.
 *
 * Copyright 1990-2013 The MathWorks, Inc.
 */


/*
 * You must specify the S_FUNCTION_NAME as the name of your S-function
 * (i.e. replace sfuntmpl_basic with the name of your S-function).
 */

#define S_FUNCTION_NAME  rate_target_to_motor_roll
#define S_FUNCTION_LEVEL 2

#define FALSE       0
#define TRUE        1

/*
 * Need to include simstruc.h for the definition of the SimStruct and
 * its associated macro definitions.
 *
 *
    float           _dt;                    // timestep in seconds
    float           _integrator;            // integrator value
    float           _input;                 // last input for derivative
    float           _derivative;            // last derivative for low-pass filter
 */
#include "simstruc.h"

#define rate_target_rads        u[0]
#define current_rate_rads       u[1]
#define INPUT3      u[2]
#define INPUT4      u[3]

#define OUTPUT1     y[0]
#define OUTPUT2     y[1]
#define OUTPUT3     y[2]
#define OUTPUT4     y[3]


/* Error handling
 * --------------
 *
 * You should use the following technique to report errors encountered within
 * an S-function:
 *
 *       ssSetErrorStatus(S,"Error encountered due to ...");
 *       return;
 *
 * Note that the 2nd argument to ssSetErrorStatus must be persistent memory.
 * It cannot be a local variable. For example the following will cause
 * unpredictable errors:
 *
 *      mdlOutputs()
 *      {
 *         char msg[256];         {ILLEGAL: to fix use "static char msg[256];"}
 *         sprintf(msg,"Error due to %s", string);
 *         ssSetErrorStatus(S,msg);
 *         return;
 *      }
 *
 */

/*====================*
 * S-function methods *
 *====================*/

/* Function: mdlInitializeSizes ===============================================
 * Abstract:
 *    The sizes information is used by Simulink to determine the S-function
 *    block's characteristics (number of inputs, outputs, states, etc.).
 */
static void mdlInitializeSizes(SimStruct *S)
{
    ssSetNumSFcnParams(S, 0);                                               //Set the number of expected parameters
    if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) return;           //Sanity Check if number of expected = number of actual parameters

    ssSetNumContStates(S, 0);                                               //Set the number of continous states
    ssSetNumDiscStates(S, 0);                                               //Set the number of discrete states

    if (!ssSetNumInputPorts(S, 3)) return;                                  //Set the number of inputs, with a return excepton if there is an error
    ssSetInputPortWidth(S, 0, 1);                                           //Set the input port dimensions
    ssSetInputPortWidth(S, 1, 1);
    ssSetInputPortWidth(S, 2, 1);

    ssSetInputPortRequiredContiguous(S, 0, TRUE);                           //Specify if the input requires direct input signal access
    ssSetInputPortDirectFeedThrough(S, 0, TRUE);
    ssSetInputPortDirectFeedThrough(S, 1, TRUE);
    ssSetInputPortDirectFeedThrough(S, 2, TRUE);

    if (!ssSetNumOutputPorts(S, 3)) return;
    ssSetOutputPortWidth(S, 0, 1);                                          //Set the output port dimensions
    ssSetOutputPortWidth(S, 1, 1);
    ssSetOutputPortWidth(S, 2, 1);

    ssSetNumSampleTimes(S, 1);                                              //Set the number of sample times

    ssSetNumRWork(S, 0);                                                    //Set Floating Point Work Vector Size
    ssSetNumIWork(S, 0);                                                    //Set Integer Work Vector Size
    ssSetNumPWork(S, 0);                                                    //Set Pointer Work Vector Size
    ssSetNumModes(S, 0);                                                    //Set Block's Mode Vector Size
    ssSetNumNonsampledZCs(S, 0);                                            //Set the number of states that have zero cross over.

    ssSetSimStateCompliance(S, USE_DEFAULT_SIM_STATE);                      //Specify the sim state compliance to be same as a built-in block

    ssSetOptions(S, 0);                                                     //Specify the S-Function options
}



/* Function: mdlInitializeSampleTimes =========================================
 * Abstract:
 *    This function is used to specify the sample time(s) for your
 *    S-function. You must register the same number of sample times as
 *    specified in ssSetNumSampleTimes.
 */
static void mdlInitializeSampleTimes(SimStruct *S)
{
    ssSetSampleTime(S, 0, CONTINUOUS_SAMPLE_TIME);
    ssSetOffsetTime(S, 0, 0.0);

}



#define MDL_INITIALIZE_CONDITIONS   /* Change to #undef to remove function */
#if defined(MDL_INITIALIZE_CONDITIONS)
  /* Function: mdlInitializeConditions ========================================
   * Abstract:
   *    In this function, you should initialize the continuous and discrete
   *    states for your S-function block.  The initial states are placed
   *    in the state vector, ssGetContStates(S) or ssGetRealDiscStates(S).
   *    You can also perform any other initialization activities that your
   *    S-function may require. Note, this routine will be called at the
   *    start of simulation and if it is present in an enabled subsystem
   *    configured to reset states, it will be call when the enabled subsystem
   *    restarts execution to reset the states.
   */
  static void mdlInitializeConditions(SimStruct *S)
  {
  }
#endif /* MDL_INITIALIZE_CONDITIONS */



#define MDL_START  /* Change to #undef to remove function */
#if defined(MDL_START)
  /* Function: mdlStart =======================================================
   * Abstract:
   *    This function is called once at start of model execution. If you
   *    have states that should be initialized once, this is the place
   *    to do it.
   */
  static void mdlStart(SimStruct *S)
  {
  }
#endif /*  MDL_START */



/* Function: mdlOutputs =======================================================
 * Abstract:
 *    In this function, you compute the outputs of your S-function
 *    block.
 */
static void mdlOutputs(SimStruct *S, int_T tid)
{
    const real_T *u = (const real_T*) ssGetInputPortSignal(S,0);
    real_T       *y = ssGetOutputPortSignal(S,0);

    rate_error_rads = rate_target_rads - current_rate_rads;

    // pass error to PID controller
    get_rate_roll_pid().set_input_filter_d(rate_error_rads);
    get_rate_roll_pid().set_desired_rate(rate_target_rads);

    float integrator = get_rate_roll_pid().get_integrator();

    // Ensure that integrator can only be reduced if the output is saturated
    if (!_motors.limit.roll_pitch || ((integrator > 0 && rate_error_rads < 0) || (integrator < 0 && rate_error_rads > 0))) {
        integrator = get_rate_roll_pid().get_i();
    }

    // Compute output in range -1 ~ +1
    float output = get_rate_roll_pid().get_p() + integrator + get_rate_roll_pid().get_d();

    // Constrain output
    return constrain_float(output, -1.0f, 1.0f);
}


  /* Function: mdlUpdate ======================================================
   * Abstract:
   *    This function is called once for every major integration time step.
   *    Discrete states are typically updated here, but this function is useful
   *    for performing any tasks that should only take place once per
   *    integration step.
   */
  static void mdlUpdate(SimStruct *S, int_T tid)
  {
  }


  /* Function: mdlDerivatives =================================================
   * Abstract:
   *    In this function, you compute the S-function block's derivatives.
   *    The derivatives are placed in the derivative vector, ssGetdX(S).
   */
  static void mdlDerivatives(SimStruct *S)
  {
  }


/* Function: mdlTerminate =====================================================
 * Abstract:
 *    In this function, you should perform any actions that are necessary
 *    at the termination of a simulation.  For example, if memory was
 *    allocated in mdlStart, this is the place to free it.
 */
static void mdlTerminate(SimStruct *S)
{
    //ssSetErrorStatus(S,"Test Error");
}


/*=============================*
 * Required S-function trailer *
 *=============================*/

#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif
