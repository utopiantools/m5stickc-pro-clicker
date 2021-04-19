# m5stickc-pro-clicker

This Arduino Sketch will turn your M5StickC into a miniscule remote control for ProPresenter that will work at any distance since it uses WiFi rather than bluetooth.

The only limitation is that the M5StickC device has about 60 minutes of battery life, so it will need to be charged before any presentation, or a battery upgrade needs to be made. With minor modifications to the sketch, this code can easily be adapted to work with an M5Atom device and the battery tailpiece which will give much longer battery life.

## installation

1. Set up your Arduino IDE according to the M5Stack documentation.
2. Add the "Arduino Websockets" library in the Arduino IDE library manager.
3. Edit the variables at the top of the sketch to include your network and ProPresenter settings.
4. Compile and upload the sketch.

## usage

1. When the stick boots, it will connect to WiFi first, and then attempt to connect to ProPresenter. The screen should show the current status.
2. Clicking the M5 button will advance to the next slide.
3. Clicking the Secondary button will go to the previous slide.
4. A Long press on the M5 button will also go to the previous slide.
5. After every user interaction, the screen brightness will increase for a moment to show the results of the command and to show the battery life.
