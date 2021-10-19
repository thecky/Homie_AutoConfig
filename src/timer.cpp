/* forward-declaration */
void setTimerValue(hw_timer_t*, long);
void disableHoldTimer(void);
void disableScanTimer(void);
void resetTimer(hw_timer_t*);

/*
 * ESP32 initial Timersetup
 * Function used from the arduino-esp32 framework.
 * 
 * See also:
 *   https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/timer.html
 *   https://diyprojects.io/esp32-timers-alarms-interrupts-arduino-code/
 */
void setup_HoldTimer(long secvalue = 0)
{
    /* Use defined hardwaretimer, divider and set to count up direction */
    t_Hold_Timer = timerBegin(HOLD_TMR_NUMBER, TMR_DIVIDER, true);

    if (t_Hold_Timer != NULL)
    {
        /* callback function when timer reach compare-value (level-trigger)*/
        timerAttachInterrupt(t_Hold_Timer, &HoldTMRHandler, false);

        /* set comparevalue */
        setTimerValue(t_Hold_Timer, secvalue);

        /* stop timer */
        disableHoldTimer();
    }
}

void setup_ScanTimer(long secvalue = 0)
{
    /* Use defined hardwaretimer, divider and set to count up direction */
    t_ScanInt_Timer = timerBegin(SCAN_TMR_NUMBER, TMR_DIVIDER, true);

    if (t_ScanInt_Timer != NULL)
    {
        /* callback function when timer reach compare-value (level-trigger)*/
        timerAttachInterrupt(t_ScanInt_Timer, &ScanIntTMRHandler, false);

        /* set comparevalue */
        setTimerValue(t_ScanInt_Timer, secvalue);

        /* stop timer */
        disableScanTimer();
    }
}

/*
 * Calculate the comparevalue for the timer
 */
uint64_t tmr_calc_cmp(const long secvalue)
{
    return ((APB_FREQ * 1000 * 1000) / TMR_DIVIDER) * secvalue;
}

/*
 * Set a new comparevalue
 * Argument msecvalue has 0 as default (see declaration)
 */
void setTimerValue(hw_timer_t *tmrObj, long secvalue)
{
    if (tmrObj != NULL)
    {
        if (secvalue == 0) secvalue = 1;
        uint64_t cmpvalue = tmr_calc_cmp(secvalue);

        /* set the desired intervall, with autoreload */
        timerAlarmWrite(tmrObj, cmpvalue, true);

        /* If timer runs and alarm is enabled, restart */
        if (timerAlarmEnabled(tmrObj)) resetTimer(tmrObj);
    }
}

/*
 * Enable the timer (Holdtimer)
 */
void enableHoldTimer(void)
{
    if (t_Hold_Timer != NULL)
    {
        HoldTimerEnd = false;
        timerRestart(t_Hold_Timer);
        timerAlarmEnable(t_Hold_Timer);
    }
}

/*
 * Enable the timer (WiFi-Scan-Intervall)
 */
void enableScanTimer(void)
{
    if (t_ScanInt_Timer != NULL)
    {
        ScanIntervallTimer = false;
        timerRestart(t_ScanInt_Timer);
        timerAlarmEnable(t_ScanInt_Timer);
    }
}

/*
 * Disable the timer (Holdtimer)
 */
void disableHoldTimer(void)
{
    if (t_Hold_Timer != NULL)
    {
        timerAlarmDisable(t_Hold_Timer);
        timerStop(t_Hold_Timer);
        HoldTimerEnd = false;
    }
}

/*
 * Disable the timer (WiFi-Scan-Intervall)
 */
void disableScanTimer(void)
{
    if (t_ScanInt_Timer != NULL)
    {
        timerAlarmDisable(t_ScanInt_Timer);
        timerStop(t_ScanInt_Timer);
        ScanIntervallTimer = false;
    }
}

/*
 * Reset the timer
 */
void resetTimer(hw_timer_t *tmrObj)
{
    if (tmrObj != NULL)
    {
        timerRestart(tmrObj);
    }
}

/*
 * Callback for Offtimer (Hold-Timer)
 * Function is placed in RAM
 */
void IRAM_ATTR HoldTMRHandler(void)
{
    HoldTimerEnd = true;
}

/*
 * Callback for WiFi-Scan-Intervall timer
 * Function is placed in RAM
 */
void IRAM_ATTR ScanIntTMRHandler(void)
{
    ScanIntervallTimer = true;
}