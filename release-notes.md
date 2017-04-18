### Release Notes 2.2.1

HiDPI issues windows
===============================================
Windows Resize/DPI awareness problems are now solved.

History loading bug
===============================================
Solved a problem where the history loading got stuck

Reset U2F feature
===============================================
Added a reset-u2f function (experts settings)

### Release Notes 2.2.0

Compatibility with the new 2.1.0 firmware (U2F)
===============================================
The new firmware (>=2.1.0) uses a different communication protocol.
The dbb-app can now distinct between the new and old firmware and use the corresponding communication protocol.

Better shutdown handling (Windows shutdown crashes)
===============================================
The shutdown process does now respect platforms like Windows.
There should be no more crashes on Windows during shutdown of the app.

Better loading handling
===============================================
Loading the balance and history the first time will now be visible with a simple text.


### Release Notes 2.1.1

Multiple signing steps
======================
There is no longer an max-inputs limit. If a transaction has more then 12 inputs, it will be split into multiple signing steps (requiring multiple times to confirm over the touchbutton)

SmartVerification simplification
================================
The app does no longer warn if the SmartVerification device (Smartphone App) is not connected.

Integer overflow bugfix
=======================
The dbb-app does no longer reject signing of transactions with a large anount that overflows int32.

