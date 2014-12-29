/********************************** ls_switches: LinnStrument Switches ****************************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
These routines handle LinnStrument's 2 button switches and 2 foot switch inputs.

The 2 foot switch input lines are pulled up so a read of HIGH means foot switch is open or not
connected:
In order to accomodate both normally-open and normally-closed foot switches, unpressed switch state
is read on startup and saved in leftFootSwitchOffState and rightFootSwitchOffState. When switches
are read subsequently, state is compared to these OFF states to insure valid presses for both
normally-open and normally-closed switches.
**************************************************************************************************/

boolean switchArpeggiatorPressed[2];

void initializeSwitches() {
  // read initial state of each in order to determine if nornally-open or
  // normally-closed (like VFP2) switches are connected, or if nothing's connected.
  footSwitchOffState[SWITCH_FOOT_L] = digitalRead(FOOT_SW_LEFT);              // check left input
  footSwitchOffState[SWITCH_FOOT_R] = digitalRead(FOOT_SW_RIGHT);             // check right input
  footSwitchState[SWITCH_FOOT_L] = footSwitchOffState[SWITCH_FOOT_L];
  footSwitchState[SWITCH_FOOT_R] = footSwitchOffState[SWITCH_FOOT_R];

  lastSwitchPress[SWITCH_FOOT_L] = 0;
  lastSwitchPress[SWITCH_FOOT_R] = 0;
  lastSwitchPress[SWITCH_SWITCH_2] = 0;
  lastSwitchPress[SWITCH_SWITCH_1] = 0;

  switchState[SWITCH_FOOT_L][LEFT] = (footSwitchState[SWITCH_FOOT_L] != footSwitchOffState[SWITCH_FOOT_L]);
  switchState[SWITCH_FOOT_R][LEFT] = (footSwitchState[SWITCH_FOOT_R] != footSwitchOffState[SWITCH_FOOT_R]);
  switchState[SWITCH_SWITCH_2][LEFT] = false;
  switchState[SWITCH_SWITCH_1][LEFT] = false;
  switchState[SWITCH_FOOT_L][RIGHT] = (footSwitchState[SWITCH_FOOT_L] != footSwitchOffState[SWITCH_FOOT_L]);
  switchState[SWITCH_FOOT_R][RIGHT] = (footSwitchState[SWITCH_FOOT_R] != footSwitchOffState[SWITCH_FOOT_R]);
  switchState[SWITCH_SWITCH_2][RIGHT] = false;
  switchState[SWITCH_SWITCH_1][RIGHT] = false;

  for (byte i = 0; i < 6; ++i) {
    switchTargetEnabled[i][LEFT] = false;
    switchTargetEnabled[i][RIGHT] = false;
  }

  switchArpeggiatorPressed[LEFT] = false;
  switchArpeggiatorPressed[RIGHT] = false;
}

void doSwitchPressed(byte whichSwitch) {
  byte assignment = Global.switchAssignment[whichSwitch];
  if (!splitActive || assignment == ASSIGNED_ALTSPLIT || !Global.switchBothSplits[whichSwitch]) {
    doSwitchPressedForSplit(whichSwitch, assignment, focusedSplit);
  }
  else {
    doSwitchPressedForSplit(whichSwitch, assignment, LEFT);
    doSwitchPressedForSplit(whichSwitch, assignment, RIGHT);
  }
}

void doSwitchPressedForSplit(byte whichSwitch, byte assignment, byte split) {
  byte enabledCount = switchTargetEnabled[assignment][split];

  // keep track of when the last press happened to be able to differentiate
  // between toggle and hold
  lastSwitchPress[whichSwitch] = millis();

  // by default, the state of this switch will by on when pressed,
  // unless it's determined later to be a toggle action
  boolean resultingState = true;

  // perform the switch assignment on action if this hasn't been enabled
  // by another switch yet
  if (enabledCount == 0) {
    performSwitchAssignmentOn(assignment, split);
  }
  // if the switch is enabled for the split, it's toggled on
  // go through the assignment off logic
  else if (switchState[whichSwitch][split]) {
    // turn the switch state off
    resultingState = false;

    // if this is the last switch that has this assignment enabled
    // perform the assignment off logic
    if (enabledCount == 1) {
      performSwitchAssignmentOff(assignment, split);
    }
  }

  // change the state of the switch and the assignment based on the previous logic
  changeSwitchState(whichSwitch, assignment, split, resultingState);
}

void doSwitchReleased(byte whichSwitch) {
  byte assignment = Global.switchAssignment[whichSwitch];
  if (!splitActive || assignment == ASSIGNED_ALTSPLIT || !Global.switchBothSplits[whichSwitch]) {
    doSwitchReleasedForSplit(whichSwitch, assignment, focusedSplit);
  }
  else {
    doSwitchReleasedForSplit(whichSwitch, assignment, LEFT);
    doSwitchReleasedForSplit(whichSwitch, assignment, RIGHT);
  }
}

void doSwitchReleasedForSplit(byte whichSwitch, byte assignment, byte split) {
  // check whether this is a hold operation by comparing the release time with
  // the last time the switch was pressed
  if (millis() - lastSwitchPress[whichSwitch] > SWITCH_HOLD_DELAY) {      
    // perform the assignment off logic, but when a split is active, it's possible that the
    // switch started being held on the other split, so we need to check which split is actually
    // active before changing the state
    if (splitActive) {
      if (switchTargetEnabled[assignment][LEFT] == 1) {
        performSwitchAssignmentHoldOff(assignment, LEFT);
        changeSwitchState(whichSwitch, assignment, LEFT, false);
      }
      if (switchTargetEnabled[assignment][RIGHT] == 1) {
        performSwitchAssignmentHoldOff(assignment, RIGHT);
        changeSwitchState(whichSwitch, assignment, RIGHT, false);
      }
    }
    else {
      if (switchTargetEnabled[assignment][split] == 1) {
        performSwitchAssignmentHoldOff(assignment, split);
        changeSwitchState(whichSwitch, assignment, split, false);
      }
    }
  }
  // this is a toggle action, however only some assignment have toggle behavior
  else {
    // the switch is already off, don't proceed
    if (assignment != ASSIGNED_ALTSPLIT && !switchState[whichSwitch][split]) {
      return;
    }

    boolean turnSwitchOff = false;

    switch (assignment)
    {
      // these assignments don't have visible toggle behaviours, they're more
      // like one shot triggers
      case ASSIGNED_OCTAVE_DOWN:
      case ASSIGNED_OCTAVE_UP:
      case ASSIGNED_ALTSPLIT:
        // never keep the switch on after release for these assignments
        turnSwitchOff = true;
        break;

      // these assignments can be toggle on or off visually
      case ASSIGNED_SUSTAIN:
      case ASSIGNED_CC_65:
      case ASSIGNED_ARPEGGIATOR:
        // preserve the current state if the switch is not held
        break;
    }

    if (turnSwitchOff) {
      changeSwitchState(whichSwitch, assignment, split, false);
    }
  }
}

void changeSwitchState(byte whichSwitch, byte assignment, byte split, boolean enabled) {
  if (enabled) {
    // keep track of the assignment enabled count for the split
    switchTargetEnabled[assignment][split]++;
    // use both splits to keep track of the alt split enabled count for the alt split assignment
    if (assignment == ASSIGNED_ALTSPLIT) {
      switchTargetEnabled[assignment][!split]++;
    }

    switchState[whichSwitch][split] = true;
  }
  else {
    // keep track of the assignment enabled count for the split
    switchTargetEnabled[assignment][split]--;
    // use both splits to keep track of the alt split enabled count for the alt split assignment
    if (assignment == ASSIGNED_ALTSPLIT) {
      switchTargetEnabled[assignment][!split]--;
    }

    switchState[whichSwitch][split] = false;
  }
}

void performSwitchAssignmentOn(byte assignment, byte split) {
  switch (assignment)
  {
    case ASSIGNED_OCTAVE_DOWN:
      Split[split].transposeOctave = constrain(Split[split].transposeOctave - 12, -60, 60);
      break;

    case ASSIGNED_OCTAVE_UP:
      Split[split].transposeOctave = constrain(Split[split].transposeOctave + 12, -60, 60);
      break;

    case ASSIGNED_SUSTAIN:
      preSendControlChange(split, 64, 127);
      break;

    case ASSIGNED_CC_65:
      preSendControlChange(split, 65, 127);
      break;

    case ASSIGNED_ALTSPLIT:
      performAltSplitAssignment();
      break;

    case ASSIGNED_ARPEGGIATOR:
      temporarilyEnableArpeggiator();
      switchArpeggiatorPressed[split] = true;
      break;
  }
}

void performAltSplitAssignment() {
  resetAllTouches();
  Global.currentPerSplit = !Global.currentPerSplit;
  focusedSplit = Global.currentPerSplit;
  updateDisplay();
}

void performSwitchAssignmentHoldOff(byte assignment, byte split) {
  switch (assignment)
  {
    case ASSIGNED_OCTAVE_DOWN:
      Split[split].transposeOctave = constrain(Split[split].transposeOctave + 12, -60, 60);
      break;

    case ASSIGNED_OCTAVE_UP:
      Split[split].transposeOctave = constrain(Split[split].transposeOctave - 12, -60, 60);
      break;

    case ASSIGNED_SUSTAIN:
    case ASSIGNED_CC_65:
    case ASSIGNED_ARPEGGIATOR:
      performSwitchAssignmentOff(assignment, split);
      break;

    case ASSIGNED_ALTSPLIT:
      performAltSplitAssignment();
      break;
  }
}

void performSwitchAssignmentOff(byte assignment, byte split) {
  switch (assignment)
  {
    case ASSIGNED_SUSTAIN:
      preSendControlChange(split, 64, 0);
      break;
    case ASSIGNED_CC_65:
      preSendControlChange(split, 65, 0);
      break;
    case ASSIGNED_ARPEGGIATOR:
      disableTemporaryArpeggiator();
      switchArpeggiatorPressed[split] = false;
      break;
  }
}

void handleFootSwitchState(byte whichSwitch, boolean state) {
  if (footSwitchOffState[whichSwitch] == HIGH) state = !state;       // If a normally-open switch (or no switch) is connected, invert state of switch read

  if (footSwitchState[whichSwitch] != state) {                       // if state if changed since last read...
    DEBUGPRINT((2,"handleFootSwitchState"));
    DEBUGPRINT((2," pedal="));DEBUGPRINT((2,(int)whichSwitch));
    DEBUGPRINT((2," state="));DEBUGPRINT((2,(int)state));
    DEBUGPRINT((2,"\n"));

    footSwitchState[whichSwitch] = state;

    // merely light leds in test mode
    if (operatingMode == modeManufacturingTest) {
      switchState[whichSwitch][focusedSplit] = state;
      if (state) {
        setLed(24 + whichSwitch, 6, COLOR_GREEN, cellOn);
      }
      else {
        clearLed(24 + whichSwitch, 6);
      }
    }
    // handle the foot switch assignments in regular mode
    else {
      if (state) {                                                   // if the switch is pressed...
        doSwitchPressed(whichSwitch);
      }
      else {                                                         // if the switch is released...
        doSwitchReleased(whichSwitch);
      }
    }
  }
}

void resetSwitchStates(byte whichSwitch) {
  for (byte sp = 0; sp < 2; ++ sp) {
    if (switchState[whichSwitch][sp]) {
      byte assignment = Global.switchAssignment[whichSwitch];
      changeSwitchState(whichSwitch, assignment, sp, false);
      performSwitchAssignmentOff(assignment, sp);
    }
  }
}

// checkFootSwitches:
// Once every 20 ms, this is called to read foot switch inputs and take action if state changed
void checkFootSwitches() {
  handleFootSwitchState(SWITCH_FOOT_L, digitalRead(FOOT_SW_LEFT));   // check left input raw state
  handleFootSwitchState(SWITCH_FOOT_R, digitalRead(FOOT_SW_RIGHT));  // check raw right input state
}

inline boolean isSwitchArpeggiatorPressed(byte split) {
  return switchArpeggiatorPressed[split];
}