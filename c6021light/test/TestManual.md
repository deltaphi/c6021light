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

12. Switch at least one Turnout to Red. The indicator light on the Keyboard must light up.
13. Put the MS2 in STOP mode.
14. Test 7: Attempt to switch a RED turnout to GREEN and a GREEN turnout to RED from the Keyboard. Test is passed if the indicator on the Keyboard and in the MS2 do not change.
15. Test 8: Attempt to switch a RED turnout to GREEN and a GREEN turnout to RED from the MS2. Test is passed if the indicator on the Keyboard and in the MS2 do not change.