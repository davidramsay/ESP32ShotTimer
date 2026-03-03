# ESP32ShotTimer
Shot timers are $120.. that is way too much for what is obviously a small PCB with an LCD and a few buttons, a piezo buzzer, and a sound sensor. So I made my own in a few hours using chatgpt and a XIAO SEEED ESP32S3. 

=================
bill of materials
=================
1. XIAO seeed esp32s3.
2. DIYables sound sensor or similar.
3. 3x buttons (i am using 6mm).
4. LCD screen that uses liquid crystal library and 4-pin setup.
5. a 4.7 volt battery, NiMH is available for about $5 a pop, less if you are willing to wait for temu or alibaba. These can solder straight to the xiao seeed.
6. slide switch for shutting off power.
7. active piezo buzzer.

no resistors required for buttons due to internal pull down resistors on xiao seeed espo32s3.
============
Sound sensor
============
I had to tune the sound sensor to get the correct configuration. I viewed the output on a serial monitor and tuned so the noise floor was around 1500/max when clapping hit the 4095 max.

in the case of the diyables sensor, I cranked the screw all the way counter-clockwise and rotated it back about 110 degrees. The fully counter-clockwise position  looked roughly like an x, I rotated it about 110 degrees clockwise so it looked like a +.) In otherwords, the "lower left" point of the x went from pointing at "8 o'clock" to pointing at "12 0'clock".
