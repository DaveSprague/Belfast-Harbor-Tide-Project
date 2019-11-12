<h1> Belfast Harbor Tide Project

This project uses two Adafruit Feather M0 Lora boards, one acting as a LoRa Transmitter and the other as a LoRa Receiver.  The Transmitter reads sensor from an arduino based underwater pressure/temperature sensor over it's serial channel in the form of two float numbers separated by a comma.  The underwater is supplied power and ground and sends back serial date of the measurements once per second that area captured on the LoRa Tx Feather board on Serial1.

The LoRa Transmitter board also has it's own BME 280 Temperature/Humidity/Pressure sensor that it uses to capture air pressure so that we can use the difference between the air pressure and depth pressure to calculate the height of the water column.

The LoRa Receiver receives all of these readings in a single packet from the LoRa Tx once per second and sends them along over it's USB connection to a Raspberry Pi which averages each measurement over sixty samples and uploads those average values to  Google Spreadsheet.

Requires these libraries in order to upload to google spreadsheet.
pip install --upgrade google-api-python-client google-auth-httplib2 google-auth-oauthlib

Refer to this tutorial fromg Google:
https://developers.google.com/sheets/api/quickstart/apps-script

And this blog post:
http://www.whatimade.today/log-sensor-data-straight-to-google-sheets-from-a-raspberry-pi-zero-all-the-python-code/
