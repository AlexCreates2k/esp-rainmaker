# Matter + Rainmaker LED Light Example

## What to expect in this example?

- This demonstrates a Matter + RainMaker Light. Matter is used for commissioning (also known as Wi-Fi provisioning) and local control, whereas RainMaker is used for remote control and OTA upgrades.
- This example uses the BOOT button and RGB LED on the ESP32-C3-DevKitC board to demonstrate a lightbulb.
- The LED acts as a lightbulb with hue, saturation and brightness.
- To commission the device, scan the QR Code generated by the esp-matter's mfg_tool script using ESP RainMaker app.
- Pressing the BOOT button will toggle the power state of the lightbulb. This will also reflect on the phone app.
- Toggling the button on the phone app should toggle the LED on your board.
- You may also try changing the hue, saturation and brightness from the phone app. 
- To test remote control, change the network connection of the mobile.

> Please refer to the [README in the parent folder](../README.md) for instructions.