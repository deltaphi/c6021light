# c6021light Test Manual

## Test CAN/I2C Interaction

Setup: Connect MS2 via CAN and Keyboard via I2C.

Test Steps:

1. Power up the System.
2. Test 1: Do the last Turnout settings on the Keyboard come up? Test is passed if the lights come up.
3. Bring up the Turnout screen on the MS2.
4. Put the MS2 in GO mode.
5. Switch Turnout 1 on the Keyboard Red-Green-Red-Green-Red.
6. Test 2: Does the light change on the Keyboard? Test is passed if the light changes according to each button press.
7. Test 3: Does the indicator on the MS2 change? Test is passed if the indicator changes according to each button press.
8. Repeat Steps 5.-7. for Turnouts 2, 4, 5, 15, 16 on the Keyboard.
9. Test 4 & 5: Repeat Steps 5.-7. but control the turnout from the MS2. Test for Turnouts 1, 2, 4, 5.

10. Set Turnouts 1-4 to Red on the Keyboard. The indicator lights must light up.
11. Test 6: Turn Turnout 1 to Green on the Keyboard. The test is passed if exactly the indicator light for Turnout 1 changes.

