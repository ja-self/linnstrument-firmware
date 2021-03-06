/************************** ls_displayModes: LinnStrument display modes drawing *******************
This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/
or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
***************************************************************************************************
There are 13 different display modes.

These are the possible values of the global variable displayMode:

displayNormal               : normal performance display
displayPerSplit             : per-split settings (left or right split)
displayPreset               : preset number
displayVolume               : volume
displayOctaveTranspose      : octave and transpose settings
displaySplitPoint           : split point
displayGlobal               : global settings
displayGlobalWithTempo      : global settings with tempo
displayOsVersion            : version number of the OS
displayCalibration          : calibration process
displayReset                : global reset confirmation and wait for touch release
displayBendRange            ; custom bend range selection for X expression
displayCCForY               : custom CC number selection for Y expression
displayCCForZ               : custom CC number selection for Z expression
displayCCForFader           : custom CC number selection for a CC fader
displaySensorLoZ            : sensor low Z sensitivity selection
displaySensorFeatherZ       : sensor feather Z sensitivity selection
displaySensorRangeZ         : max Z sensor range selection
displayPromo                : display promotion animation
displayEditAudienceMessage  : edit an audience message

displayCustom               : display custom animations - jas 2015/01/05

These routines handle the painting of these display modes on LinnStument's 208 LEDs.
**************************************************************************************************/


unsigned long tapTempoLedOn = 0;       // indicates when the tap tempo clock led was turned on
unsigned long displayModeStart = 0;    // indicates when the current display mode was activated
bool blinkMiddleRootNote = false;      // indicates whether the middle root note should be blinking

// changes the active display mode
void setDisplayMode(DisplayMode mode) {
  boolean refresh = (displayMode != mode);
  if (refresh || displayModeStart == 0) {
    displayModeStart = millis();
    exitDisplayMode(displayMode);
  }

  displayMode = mode;
  if (refresh) {
    completelyRefreshLeds();
  }
}

// updates columns 1=25 of the LED display based on the current displayMode setting:
// 0:normal, 1:perSplit, 2:preset, 3:volume, 4:transpose, 5:split, 6:global
void updateDisplay() {
  if (animationActive) {
    return;
  }

  switch (displayMode)
  {
  case displayNormal:
  case displaySplitPoint:
    paintNormalDisplay();
    break;
  case displayPerSplit:
    paintPerSplitDisplay(Global.currentPerSplit);
    break;
  case displayPreset:
    paintPresetDisplay(Global.currentPerSplit);
    break;
  case displayOsVersion:
    paintOSVersionDisplay();
    break;
  case displayVolume:
    paintVolumeDisplay(Global.currentPerSplit);
    break;
  case displayOctaveTranspose:
    paintOctaveTransposeDisplay(Global.currentPerSplit);
    break;
  case displayGlobal:
  case displayGlobalWithTempo:
    paintGlobalSettingsDisplay();
    break;
  case displayCalibration:
    paintCalibrationDisplay();
    break;
  case displayReset:
    paintResetDisplay();
    break;
  case displayBendRange:
    paintBendRangeDisplay(Global.currentPerSplit);
    break;
  case displayCCForY:
    paintCCForYDisplay(Global.currentPerSplit);
    break;
  case displayCCForZ:
    paintCCForZDisplay(Global.currentPerSplit);
    break;
  case displayCCForFader:
    paintCCForFaderDisplay(Global.currentPerSplit);
    break;
  case displaySensorLoZ:
    paintSensorLoZDisplay();
    break;
  case displaySensorFeatherZ:
    paintSensorFeatherZDisplay();
    break;
  case displaySensorRangeZ:
    paintSensorRangeZDisplay();
    break;
  case displayCustom:            // Display Custom controls - display may overlay normal w/o animationActive --jas
    paintCustom();
    break;
  case displayEditAudienceMessage:
    paintEditAudienceMessage();
    break;
  }

  updateSwitchLeds();
}

// handle logic tied to exiting specific display mode, like post-processing or saving
void exitDisplayMode(DisplayMode mode) {
  switch (mode)
  {
  case displayEditAudienceMessage:
    trimEditedAudienceMessage();
    storeSettings();
    break;
  }
}

void updateSwitchLeds() {
  setLed(0, SWITCH_1_ROW, globalColor, switchState[SWITCH_SWITCH_1][focusedSplit] ? cellOn : cellOff);
  setLed(0, SWITCH_2_ROW, globalColor, switchState[SWITCH_SWITCH_2][focusedSplit] ? cellOn : cellOff);
  if (splitActive) {
    setLed(0, SPLIT_ROW, Split[focusedSplit].colorMain, cellOn);
  }
  else {
    clearLed(0, SPLIT_ROW);
  }
}

// paintNormalDisplay:
// Paints columns 1-26 of the display with the normal performance colors
void paintNormalDisplay() {
  // highlight global settings red when user firmware mode is active
  if (userFirmwareActive) {
    setLed(0, GLOBAL_SETTINGS_ROW, COLOR_YELLOW, cellOn);
  }
  else {
    clearLed(0, GLOBAL_SETTINGS_ROW);
  }

  // determine the splits and divider
  byte split = Global.currentPerSplit;
  byte divider = NUMCOLS;
  if (splitActive || displayMode == displaySplitPoint) {
    split = LEFT;
    divider = Global.splitPoint;
  }

  paintNormalDisplaySplit(split, 1, divider);
  if (divider != NUMCOLS) {
    paintNormalDisplaySplit(RIGHT, divider, NUMCOLS);
  }

  // light the octave/transpose switch if the pitch is transposed
  if ((Split[LEFT].transposePitch < 0 && Split[RIGHT].transposePitch < 0) ||
      (Split[LEFT].transposePitch < 0 && Split[RIGHT].transposePitch == 0) ||
      (Split[LEFT].transposePitch == 0 && Split[RIGHT].transposePitch < 0)) {
    setLed(0, OCTAVE_ROW, COLOR_RED, cellOn);
  }
  else if ((Split[LEFT].transposePitch > 0 && Split[RIGHT].transposePitch > 0) ||
           (Split[LEFT].transposePitch > 0 && Split[RIGHT].transposePitch == 0) ||
           (Split[LEFT].transposePitch == 0 && Split[RIGHT].transposePitch > 0)) {
    setLed(0, OCTAVE_ROW, COLOR_GREEN, cellOn);
  }
  else if (Split[LEFT].transposePitch != 0 && Split[RIGHT].transposePitch != 0) {
    setLed(0, OCTAVE_ROW, COLOR_YELLOW, cellOn);
  }
  else {
    clearLed(0, OCTAVE_ROW);
  }
}

void paintNormalDisplaySplit(byte split, byte leftEdge, byte rightEdge) {
  for (byte row = 0; row < NUMROWS; ++row) {
    if (Split[split].ccFaders) {
      paintCCFaderDisplayRow(split, row);
    }
    else if (isStrummingSplit(split)) {
      for (byte col = leftEdge; col < rightEdge; ++col) {
        paintStrumDisplayCell(split, col, row);
      }
    }
    else {
      for (byte col = leftEdge; col < rightEdge; ++col) {
        paintNormalDisplayCell(split, col, row);
      }
    }
    performContinuousTasks(micros());
  }
}

void paintCCFaderDisplayRow(byte split, byte row) {
  byte faderLeft, faderLength;
  determineFaderBoundaries(split, faderLeft, faderLength);

  unsigned short ccForFader = Split[split].ccForFader[row];

  // when the fader only spans one cell, it acts as a toggle
  if (faderLength == 0) {
      if (ccFaderValues[split][ccForFader] > 0) {
        setLed(faderLeft, row, Split[split].colorMain, cellOn);
      }
      else {
        clearLed(faderLeft, row);
      }
  }
  // otherwise calculate the fader position based on its value and light the appropriate leds
  else {
    int32_t fxdFaderPosition = fxdCalculateFaderPosition(ccFaderValues[split][ccForFader], faderLeft, faderLength);

    for (byte col = faderLength + faderLeft; col >= faderLeft; --col ) {
      if (Device.calRows[col][0].fxdReferenceX - CALX_HALF_UNIT > fxdFaderPosition) {
        clearLed(col, row);
      }
      else {

        setLed(col, row, Split[split].colorMain, cellOn);

      }
    }
  }
}

void paintStrumDisplayCell(byte split, byte col, byte row) {
  // by default clear the cell color
  byte colour = COLOR_OFF;
  CellDisplay cellDisplay = cellOff;

  if (row % 2 == 0) {
    colour = Split[split].colorAccent;
    cellDisplay = cellOn;
  }
  else {
    colour = Split[split].colorMain;
    cellDisplay = cellOn;
  }

  // actually set the cell's color
  setLed(col, row, colour, cellDisplay);
}

void paintNormalDisplayCell(byte split, byte col, byte row) {
  if (userFirmwareActive) {
    clearLed(col, row);
    return;
  }

  // by default clear the cell color
  byte colour = COLOR_OFF;
  CellDisplay cellDisplay = cellOff;

  short displayedNote = getNoteNumber(col,row) - Split[split].transposeLights;
  short actualnote = transposedNote(split, col, row);

  // the note is out of MIDI note range, disable it
  if (actualnote < 0 || actualnote > 127) {
    colour = COLOR_OFF;
    cellDisplay = cellOff;
  }
  else {
    byte octaveNote = abs(displayedNote % 12);

    // first paint all cells in split to its background color
    if (Global.mainNotes[octaveNote]) {
      colour = Split[split].colorMain;
      cellDisplay = cellOn;
    }

    // then paint notes marked as Alt notes with Alt color
    if (Global.altNotes[octaveNote]) {
      colour = Split[split].colorAlt;
      cellDisplay = cellOn;
    }

    // then paint only notes marked as Accent notes with Accent color
    if (Global.accentNotes[octaveNote]) {
      colour = Split[split].colorAccent;
      cellDisplay = cellOn;
    }
    
    // distinguish middle C (MIDI note number 60) - jas 2014/11/14
    // generalized to middle octave (60-71) & allow to turn it off -- jas 2015/03/23
    if (actualnote > 59 && actualnote < 72 && Global.midOctNotes[actualnote % 12]) {
      colour = Split[split].colorMidOct;
      cellDisplay = cellOn;
    }

    // if the low row is anything but normal, set it to the appropriate color
    if (row == 0 && Split[split].lowRowMode != lowRowNormal) {
      colour = Split[split].colorLowRow;
      cellDisplay = cellOn;
    }
  }

  // show pulsating middle root note
  if (blinkMiddleRootNote && displayedNote == 60) {
    colour = Split[split].colorAccent;
    cellDisplay = cellPulse;
  }

  // actually set the cell's color
  setLed(col, row, colour, cellDisplay);
}

// paintPerSplitDisplay:
// paints all cells with per-split settings for a given split
void paintPerSplitDisplay(byte side) {
  clearDisplay();

  doublePerSplit = false;  

  // set Midi Mode and channel lights
  switch (Split[side].midiMode)
  {
    case oneChannel:
      setLed(1, 7, Split[side].colorMain, cellOn);
      break;
    case channelPerNote:
      setLed(1, 6, Split[side].colorMain, cellOn);
      break;
    case channelPerRow:
      setLed(1, 5, Split[side].colorMain, cellOn);
      break;
  }

  switch (midiChannelSelect)
  {
    case MIDICHANNEL_MAIN:
      setLed(2, 7, Split[side].colorMain, cellOn);
      showMainMidiChannel(side);
      break;
    case MIDICHANNEL_PERNOTE:
      setLed(2, 6, Split[side].colorMain, cellOn);
      showPerNoteMidiChannels(side);
      break;
    case MIDICHANNEL_PERROW:
      setLed(2, 5, Split[side].colorMain, cellOn);
      showPerRowMidiChannel(side);
      break;
  }

  switch (Split[side].bendRange)
  {
    case 2:
      setLed(7, 7, Split[side].colorMain, cellOn);
      break;
    case 3:
      setLed(7, 6, Split[side].colorMain, cellOn);
      break;
    case 12:
      setLed(7, 5, Split[side].colorMain, cellOn);
      break;
    case 24:
      setLed(7, 4, Split[side].colorMain, cellOn);
      break;
    default:
      setLed(7, 3, Split[side].colorMain, cellOn);
      break;
  }

  // set Pitch/X settings
  if (Split[side].sendX == true)  {
    setLed(8, 7, Split[side].colorMain, cellOn);
  }

  if (Split[side].pitchCorrectQuantize == true) {
    setLed(8, 6, Split[side].colorMain, cellOn);
  }

  if (Split[side].pitchCorrectHold == pitchCorrectHoldMedium ||
      Split[side].pitchCorrectHold == pitchCorrectHoldSlow) {
    setLed(8, 5, Split[side].colorMain, cellOn);
  }

  if (Split[side].pitchCorrectHold == pitchCorrectHoldFast ||
      Split[side].pitchCorrectHold == pitchCorrectHoldSlow) {
    setLed(8, 4, Split[side].colorMain, cellOn);
  }

  if (Split[side].pitchResetOnRelease == true) {
    setLed(8, 3, Split[side].colorMain, cellOn);
  }

  // set Timbre/Y settings
  if (Split[side].sendY == true)  {
    setLed(9, 7, Split[side].colorMain, cellOn);
  }

  switch (Split[side].expressionForY)
  {
    case timbrePolyPressure:
    case timbreChannelPressure:
      setLed(9, 3, Split[side].colorMain, cellOn);
      break;
    case loudnessCC:
      switch (Split[side].ccForY)
      {
        case 1:
          setLed(9, 6, Split[side].colorMain, cellOn);
          break;
        case 74:
          setLed(9, 5, Split[side].colorMain, cellOn);
          break;
        default:
          setLed(9, 3, Split[side].colorMain, cellOn);
          break;
      }
      break;
  }

  if (Split[side].relativeY == true)
  {
      setLed(9, 4, Split[side].colorMain, cellOn);
  }

  // set Loudness/Z settings
  if (Split[side].sendZ == true)  {
    setLed(10, 7, Split[side].colorMain, cellOn);
  }

  switch (Split[side].expressionForZ)
  {
    case loudnessPolyPressure:
      setLed(10, 6, Split[side].colorMain, cellOn);
      break;
    case loudnessChannelPressure:
      setLed(10, 5, Split[side].colorMain, cellOn);
      break;
    case loudnessCC:
      if (Split[side].ccForZ == 11) {
        setLed(10, 4, Split[side].colorMain, cellOn);
      }
      else {
        setLed(10, 3, Split[side].colorMain, cellOn);
      }
      break;
  }

  // Set "Color" lights
  setLed(11, 7, Split[side].colorMain, cellOn);
  setLed(11, 6, Split[side].colorAccent, cellOn);
  setLed(11, 5, Split[side].colorNoteon, cellOn);
  setLed(11, 4, Split[side].colorLowRow, cellOn);
  setLed(11, 3, Split[side].colorMidOct, cellOn); //-- jas 2014/12/29
  setLed(11, 2, Split[side].colorAlt, cellOn);    //-- jas 2015/03/23

  // Set "Low row" lights
  switch (Split[side].lowRowMode)
  {
    case lowRowNormal:
      setLed(12, 7, Split[side].colorMain, cellOn);
      break;
    case lowRowRestrike:
      setLed(12, 6, Split[side].colorMain, cellOn);
      break;
    case lowRowStrum:
      setLed(12, 5, Split[side].colorMain, cellOn);
      break;
    case lowRowArpeggiator:
      setLed(12, 4, Split[side].colorMain, cellOn);
      break;
    case lowRowSustain:
      setLed(13, 7, Split[side].colorMain, cellOn);
      break;
    case lowRowBend:
      setLed(13, 6, Split[side].colorMain, cellOn);
      break;
    case lowRowCC1:
      setLed(13, 5, Split[side].colorMain, cellOn);
      break;
    case lowRowCCXYZ:
      setLed(13, 4, Split[side].colorMain, cellOn);
      break;
  }

  // set Arpeggiator
  if (Split[side].arpeggiator == true)  {
    setLed(14, 7, Split[side].colorMain, cellOn);
  }

  // set CC faders
  if (Split[side].ccFaders == true)  {
    setLed(14, 6, Split[side].colorMain, cellOn);
  }

  // set strum
  if (Split[side].strum == true)  {
    setLed(14, 5, Split[side].colorMain, cellOn);
  }

  // set "show split" led
  paintShowSplitSelection(side);
}

// paint one of the two leds that indicate which split is being controlled
// (e.g. when you're changing per-split settings, or changing the preset or volume)
void paintShowSplitSelection(byte side) {
  if (side == LEFT || doublePerSplit) {
    setLed(15, 7, Split[LEFT].colorMain, cellOn);
  }
  if (side == RIGHT || doublePerSplit) {
    setLed(16, 7, Split[RIGHT].colorMain, cellOn);
  }
}

void paintOSVersionDisplay() {
  clearDisplay();

  byte color = Split[LEFT].colorMain;
  smallfont_draw_string(0, 0, OSVersion, color);
}

// paint the current preset number for a particular side, in large block characters
void paintPresetDisplay(byte side) {
  clearDisplay();
  for (byte p = 0; p < NUMPRESETS; ++p) {
    setLed(NUMCOLS-2, p+2, globalColor, cellOn);
  }
  paintSplitNumericDataDisplay(side, midiPreset[side]+1);
}

void paintBendRangeDisplay(byte side) {
  clearDisplay();
  paintSplitNumericDataDisplay(side, Split[side].bendRange);
}

void paintCCForYDisplay(byte side) {
  clearDisplay();
  if (Split[side].ccForY == 128) {
    smallfont_draw_string(0, 0, "POPRS", Split[side].colorMain, false);
    paintShowSplitSelection(side);
  }
  else if (Split[side].ccForY == 129) {
    smallfont_draw_string(0, 0, "CHPRS", Split[side].colorMain, false);
    paintShowSplitSelection(side);
  }
  else {
    paintSplitNumericDataDisplay(side, Split[side].ccForY);
  }
}

void paintCCForZDisplay(byte side) {
  clearDisplay();
  if (Split[side].expressionForZ != loudnessCC) {
    setDisplayMode(displayPerSplit);
    updateDisplay();
  }
  else {
    paintSplitNumericDataDisplay(side, Split[side].ccForZ);
  }
}

void paintCCForFaderDisplay(byte side) {
  clearDisplay();
  for (byte r = 0; r < NUMROWS; ++r) {
    setLed(NUMCOLS-1, r, globalColor, cellOn);
  }
  setLed(NUMCOLS-1, currentEditedCCFader[side], COLOR_GREEN, cellOn);
  paintSplitNumericDataDisplay(side, Split[side].ccForFader[currentEditedCCFader[side]]);
}

void paintSensorLoZDisplay() {
  clearDisplay();
  paintNumericDataDisplay(globalColor, Device.sensorLoZ);
}

void paintSensorFeatherZDisplay() {
  clearDisplay();
  paintNumericDataDisplay(globalColor, Device.sensorFeatherZ);
}

void paintSensorRangeZDisplay() {
  clearDisplay();
  paintNumericDataDisplay(globalColor, Device.sensorRangeZ);
}

void paintSplitNumericDataDisplay(byte side, byte value) {
  paintShowSplitSelection(side);
  paintNumericDataDisplay(Split[side].colorMain, value);
}

void paintNumericDataDisplay(byte color, unsigned short value) {
  char str[10];
  char* format;
  byte offset;

  if (value < 100) {
    format = "%2d";
    offset = 5;
  }
  else if (value >= 100 && value < 200) {
    // Handle the "1" character specially, to get the spacing right
    smallfont_draw_string(0, 0, "1", color, false);
    value -= 100;
    format = "%02d";     // to make sure a leading zero is included
    offset = 5;
  }
  else {
    format = "%-d";
    offset = 0;
  }

  snprintf(str, sizeof(str), format, value);
  smallfont_draw_string(offset, 0, str, color, false);
}

// draw a horizontal line to indicate volume for a particular side
void paintVolumeDisplay(byte side) {
  clearDisplay();
  
  doublePerSplit = false;  

  int32_t fxdFaderPosition = fxdCalculateFaderPosition(ccFaderValues[side][6], 1, 24);

  for (byte col = 25; col >= 1; --col) {
    if (Device.calRows[col][0].fxdReferenceX - CALX_HALF_UNIT <= fxdFaderPosition) {
      setLed(col, 5, Split[side].colorMain, cellOn);
    }
  }

  paintShowSplitSelection(side);
}

void paintOctaveTransposeDisplay(byte side) {
  clearDisplay();
  blinkMiddleRootNote = true;

  // Paint the octave shift value
  if (!doublePerSplit || Split[LEFT].transposeOctave == Split[RIGHT].transposeOctave) {
    paintOctave(Split[Global.currentPerSplit].colorMain, 8, OCTAVE_ROW, Split[side].transposeOctave);
  }
  else if (doublePerSplit) {
    if (abs(Split[LEFT].transposeOctave) > abs(Split[RIGHT].transposeOctave)) {
      paintOctave(Split[LEFT].colorMain, 8, OCTAVE_ROW, Split[LEFT].transposeOctave);
      paintOctave(Split[RIGHT].colorMain, 8, OCTAVE_ROW, Split[RIGHT].transposeOctave);
    }
    else {
      paintOctave(Split[RIGHT].colorMain, 8, OCTAVE_ROW, Split[RIGHT].transposeOctave);
      paintOctave(Split[LEFT].colorMain, 8, OCTAVE_ROW, Split[LEFT].transposeOctave);
    }
  }

  // Paint the pitch transpose values
  if (!doublePerSplit || Split[LEFT].transposePitch == Split[RIGHT].transposePitch) {
    paintTranspose(Split[Global.currentPerSplit].colorMain, SWITCH_1_ROW, Split[side].transposePitch);
  }
  else if (doublePerSplit) {
    if (abs(Split[LEFT].transposePitch) > abs(Split[RIGHT].transposePitch)) {
      paintTranspose(Split[LEFT].colorMain, SWITCH_1_ROW, Split[LEFT].transposePitch);
      paintTranspose(Split[RIGHT].colorMain, SWITCH_1_ROW, Split[RIGHT].transposePitch);
    }
    else {
      paintTranspose(Split[RIGHT].colorMain, SWITCH_1_ROW, Split[RIGHT].transposePitch);
      paintTranspose(Split[LEFT].colorMain, SWITCH_1_ROW, Split[LEFT].transposePitch);
    }
  }

  // Paint the light transpose values
  if (!doublePerSplit || Split[LEFT].transposeLights == Split[RIGHT].transposeLights) {
    paintTranspose(Split[Global.currentPerSplit].colorMain, SWITCH_2_ROW, Split[side].transposeLights);
  }
  else if (doublePerSplit) {
    if (abs(Split[LEFT].transposeLights) > abs(Split[RIGHT].transposeLights)) {
      paintTranspose(Split[LEFT].colorMain, SWITCH_2_ROW, Split[LEFT].transposeLights);
      paintTranspose(Split[RIGHT].colorMain, SWITCH_2_ROW, Split[RIGHT].transposeLights);
    }
    else {
      paintTranspose(Split[RIGHT].colorMain, SWITCH_2_ROW, Split[RIGHT].transposeLights);
      paintTranspose(Split[LEFT].colorMain, SWITCH_2_ROW, Split[LEFT].transposeLights);
    }
  }

  paintShowSplitSelection(side);
}

void paintOctave(byte color, byte midcol, byte row, short octave) {
  setLed(midcol, row, Split[Global.currentPerSplit].colorAccent, cellOn);
  if (0 == color) color = octave > 0 ? COLOR_GREEN : COLOR_RED ;

  switch (octave) {
  case -60:
    setLed(midcol-5, row, color, cellOn);
    // lack of break here is purposeful, we want to fall through...
  case -48:
    setLed(midcol-4, row, color, cellOn);
    // lack of break here is purposeful, we want to fall through...
  case -36:
    setLed(midcol-3, row, color, cellOn);
    // lack of break here is purposeful, we want to fall through...
  case -24:
    setLed(midcol-2, row, color, cellOn);
    // lack of break here is purposeful, we want to fall through...
  case -12:
    setLed(midcol-1, row, color, cellOn);
    break;

  case 60:
    setLed(midcol+5, row, color, cellOn);
    // lack of break here is purposeful, we want to fall through...
  case 48:
    setLed(midcol+4, row, color, cellOn);
    // lack of break here is purposeful, we want to fall through...
  case 36:
    setLed(midcol+3, row, color, cellOn);
    // lack of break here is purposeful, we want to fall through...
  case 24:
    setLed(midcol+2, row, color, cellOn);
    // lack of break here is purposeful, we want to fall through...
  case 12:
    setLed(midcol+1, row, color, cellOn);
    break;
  }
}

void paintTranspose(byte color, byte row, short transpose) {
  byte midcol = 8;
  setLed(midcol, row, Split[Global.currentPerSplit].colorAccent, cellOn);    // paint the center cell of the transpose range

  if (transpose != 0) {
    if (0 == color) color = transpose < 0 ? COLOR_RED : COLOR_GREEN;
    byte col_from = (transpose < 0) ? (midcol + transpose) : (midcol + 1);
    byte col_to = (transpose > 0) ? (midcol + transpose) : (midcol - 1);
    for (byte c = col_from; c <= col_to; ++c) {
      setLed(c, row, color, cellOn);
    }
  }
}

void setNoteLights(boolean* notelights) {
  for (byte row = 0; row < 4; ++row) {
    for (byte col = 0; col < 3; ++col) {
      byte light = col + (row * 3);
      if (notelights[light]) {
        lightLed(2+col, row);
      }
    }
  }
}

void paintSwitchAssignment(byte mode) {
  switch (mode) {
    case ASSIGNED_OCTAVE_DOWN:
      lightLed(8, 2);
      break;
    case ASSIGNED_OCTAVE_UP:
      lightLed(9, 2);
      break;
    case ASSIGNED_SUSTAIN:
      lightLed(8, 1);
      break;
    case ASSIGNED_CC_65:
      lightLed(9, 1);
      break;
    case ASSIGNED_ARPEGGIATOR:
      lightLed(8, 0);
      break;
    case ASSIGNED_ALTSPLIT:
      lightLed(9, 0);
      break;
  }
}

void updateGlobalSettingsFlashTempo(unsigned long now) {
  if (displayMode == displayGlobal || displayMode == displayGlobalWithTempo) {
    paintGlobalSettingsFlashTempo(now);
  }
}

inline void paintGlobalSettingsFlashTempo(unsigned long now) {
  if (!animationActive && !userFirmwareActive) {
    // flash the tap tempo cell at the beginning of the beat
    if ((isMidiClockRunning() && getMidiClockCount() == 0) ||
        (!isMidiClockRunning() && getInternalClockCount() == 0)) {
      lightLed(14, 3);
      tapTempoLedOn = now;
    }

    // handle turning off the tap tempo led after minimum 30ms
    if (tapTempoLedOn != 0 && calcTimeDelta(now, tapTempoLedOn) > LED_FLASH_DELAY) {
      tapTempoLedOn = 0;
      clearLed(14, 3);
    }
  }
}

// paintGlobalSettingsDisplay:
// Paints LEDs with state of all global settings
void paintGlobalSettingsDisplay() {
  clearDisplay();

  // This code assumes the velocitySensitivity and pressureSensitivity
  // values are equal to the LED rows.
  lightLed(10, Global.velocitySensitivity);
  lightLed(11, Global.pressureSensitivity);

  // Show the MIDI input/output configuration
  if (Global.midiIO == 1) {
    lightLed(15, 0);       // for MIDI over USB
  } else {
    lightLed(15, 1);       // for MIDI jacks
  }

  // Show the low power mode
  if (Device.operatingLowPower) {
    lightLed(15, 3);
  }

  // set light for serial mode
  if (Device.serialMode) {
    lightLed(16, 2);
  }

  // clearly indicate the calibration status
  if (Device.calibrated) {
    setLed(16, 3, COLOR_GREEN, cellOn);
  }
  else {
    setLed(16, 3, COLOR_RED, cellOn);
  }

  if (!userFirmwareActive) {

    switch (lightSettings) {
      case LIGHTS_MAIN:
        lightLed(1, 0);
        setNoteLights(Global.mainNotes);
        break;
      case LIGHTS_ACCENT:
        lightLed(1, 1);
        setNoteLights(Global.accentNotes);
        break;
      case LIGHTS_ALT:
        lightLed(1, 2);
        setNoteLights(Global.altNotes);
        break;
      case LIGHTS_MIDOCT:
        lightLed(1, 3);
        setNoteLights(Global.midOctNotes);
        break;
    }

    switch (Global.rowOffset)
    {
      case 0:        // no overlap
        lightLed(5, 3);
        break;
      case 3:        // +3
        lightLed(5, 0);
        break;
      case 4:        // +4
        lightLed(6, 0);
        break;
      case 5:        // +5
        lightLed(5, 1);
        break;
      case 6:        // +6
        lightLed(6, 1);
        break;
      case 7:        // +7
        lightLed(5, 2);
        break;
      case 12:      // +octave
        lightLed(6, 2);
        break;
      case 13:      // guitar tuning
        lightLed(6, 3);
        break;
    }

  switch (Global.colOffset) //-- add this new setting to unused column 19 - jas 2014/12/11 --
  {
    case 1:        // 1 semitone (default)
      lightLed(19, 0);
      break;
    case 2:        // 2 semitone
      lightLed(19, 1);
      break;
    case 3:        // 3 semitone
      lightLed(19, 2);
      break;
    case 4:        // 4 semitone
      lightLed(19, 3);
      break;
  }


    // This code assumes that switchSelect values are the same as the row numbers
    lightLed(7, switchSelect);
    paintSwitchAssignment(Global.switchAssignment[switchSelect]);

    // Indicate whether switches operate on both splits or not
    if (Global.switchBothSplits[switchSelect]) {
      lightLed(8, 3);
    }

    // Indicate whether pressure is behaving like traditional aftertouch or not
    if (Global.pressureAftertouch) {
      lightLed(11, 3);
    }

    // Set the lights for the Arpeggiator settings
    switch (Global.arpDirection) {
      case ArpDown:
        lightLed(12, 0);
        break;
      case ArpUp:
        lightLed(12, 1);
        break;
      case ArpUpDown:
        lightLed(12, 0);
        lightLed(12, 1);
        break;
      case ArpRandom:
        lightLed(12, 2);
        break;
      case ArpReplayAll:
        lightLed(12, 3);
        break;
    }

    switch (Global.arpTempo) {
      case ArpSixteenthSwing:
        lightLed(13, 0);
        lightLed(13, 1);
        break;
      case ArpEighth:
        lightLed(13, 0);
        break;
      case ArpEighthTriplet:
        lightLed(13, 0);
        lightLed(13, 3);
        break;
      case ArpSixteenth:
        lightLed(13, 1);
        break;
      case ArpSixteenthTriplet:
        lightLed(13, 1);
        lightLed(13, 3);
        break;
      case ArpThirtysecond:
        lightLed(13, 2);
        break;
      case ArpThirtysecondTriplet:
        lightLed(13, 2);
        lightLed(13, 3);
        break;
    }

    // show the arpeggiator octave
    if (Global.arpOctave == 1) {
      lightLed(14, 0);
    }
    else if (Global.arpOctave == 2) {
      lightLed(14, 1);
    }

    paintGlobalSettingsFlashTempo(micros());
}

if (displayMode == displayGlobalWithTempo) {
  byte color = Split[LEFT].colorMain;
  char str[4];
  char* format = "%3d";
  snprintf(str, sizeof(str), format, FXD4_TO_INT(fxd4CurrentTempo));
  tinyfont_draw_string(0, 4, str, color);
}

#ifdef DEBUG_ENABLED
  // Column 17 is for setting/showing the debug level
  // The value of debugLevel is from -1 up.
  lightLed(17, debugLevel + 1);

  // The columns in column 18 are secret switches.
  for (byte ss = 0; ss < SECRET_SWITCHES; ++ss) {
    if (secretSwitch[ss]) {
      lightLed(18, ss);
    }
  }
#endif


  // Column 19 is set above, for Global.colOffset, following Global.rowOffset - jas 2014/12/11


  // custom Animation properties - jas 2015/01/07 --
  for (byte i=0; i<8; i++){
      if (Global.customAnimations[i]) {
          lightLed(17+i,7);
      }
  }

  // blink active animation triggers - jas 2015/01/07 --
    setLed(24, 7, COLOR_RED, cellOn);
    setLed(25, 7, COLOR_RED, cellOn);

}

void paintCalibrationDisplay() {
  clearDisplay();

  switch (calibrationPhase) {
    case calibrationRows:
      for (byte c = 1; c < NUMCOLS; ++c) {
        setLed(c, 0, COLOR_BLUE, cellOn);
        setLed(c, 2, COLOR_BLUE, cellOn);
        setLed(c, 5, COLOR_BLUE, cellOn);
        setLed(c, 7, COLOR_BLUE, cellOn);
      }
      break;
    case calibrationCols:
      for (byte r = 0; r < NUMROWS; ++r) {
        setLed(1, r, COLOR_BLUE, cellOn);
        setLed(4, r, COLOR_BLUE, cellOn);
        setLed(7, r, COLOR_BLUE, cellOn);
        setLed(10, r, COLOR_BLUE, cellOn);
        setLed(13, r, COLOR_BLUE, cellOn);
        setLed(16, r, COLOR_BLUE, cellOn);
        setLed(19, r, COLOR_BLUE, cellOn);
        setLed(22, r, COLOR_BLUE, cellOn);
        setLed(25, r, COLOR_BLUE, cellOn);
      }
      break;
  }
}

void paintResetDisplay() {
  clearDisplay();

  smallfont_draw_string(0, 0, "RESET", globalColor, true);
  for (byte row = 0; row < NUMROWS; ++row) {
    clearLed(0, row);
  }
}

void paintEditAudienceMessage() {
  bigfont_draw_string(audienceMessageOffset, 0, Device.audienceMessages[audienceMessageToEdit], Split[LEFT].colorMain, true, false, Split[LEFT].colorAccent);
}

void setMidiChannelLed(byte chan, byte color) {                       // chan value is 1-16
    if (chan > 16) {
      chan -= 16;
    }
    byte row = 7 - (chan - 1) / 4;
    byte col = 3 + (chan - 1) % 4;
    setLed(col, row, color, cellOn);
}

// light per-split midi mode and single midi channel lights
void showMainMidiChannel(byte side) {
  setMidiChannelLed(Split[side].midiChanMain, Split[side].colorMain);
}

void showPerRowMidiChannel(byte side) {
  setMidiChannelLed(Split[side].midiChanPerRow, Split[side].colorMain);
  for (byte i = 1; i < 8; ++i) {
    setMidiChannelLed(Split[side].midiChanPerRow + i, Split[side].colorMain);
  }
}

// light per-split midi mode and multi midi channel lights
void showPerNoteMidiChannels(byte side) {
  for (byte chan = 1; chan <= 16; ++chan) {
    if (Split[side].midiChanSet[chan-1]) {
      setMidiChannelLed(chan, Split[side].colorMain);
    }
  }
}
