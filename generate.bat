@echo off
stareo %1.gif %2.gif -m 0.2 -x
echo.
echo Now printing stereo image...
copy output.ps lpt1:
echo.
echo Done!
