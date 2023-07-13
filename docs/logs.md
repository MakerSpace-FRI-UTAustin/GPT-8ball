## **Logs**

### Thursday - 06/08/23
- Officially created a github repo for this project.
- Finalized recording audio clips to internal SPI RAM FS.
- Connect the ESP-32 to a personal hotspot and use it to test HTTP Post requests
- Audio are too low quality. May need to get a new mic.
- Should work on making a structured code base.

![video](https://github.com/MakerSpace-FRI-UTAustin/GPT-8ball/assets/55544605/a26528c3-20f4-483e-9b17-cfa84072ff38)


### Wednesday - 06/14/23
- Audio is really noisy.
- Using a publically hosted audio file, I tried "recording" the audio to memory.
- Turns out the transmission is still garbled. 
- I'm pretty sure, it's because i'm writing it to flash memory simultaneously.
- Will try other methods and may have to resort to a SD card. 

### Thursday - 06/22/23
- The problem of low quality audio was due to the site I was using to test the POST request. The site allowed me to download the raw data but the data was encoded for some reason? So, I switched to a python server and sent the request on my local network. Works now.
- I tried using an SD card and it works well. But I want to try to make it work on the flash memory. I think if I can figure it out, the solution can be applicable to future projects.
- The flash write speed is slower than a SD SPI write speed. So sampling from the ADC takes longer (~18s for 10s) and introduces gaps in the audio.
- Using the flash with a slower clock also means the sample rate gets messed up andIhave to sample at lower rates.
- Testing out with ChatGPT, it seems like the ai is more than powerful enough to interpolate the missing gaps in the audio and it is working well.
- After some more testing with small audio clips, if the speaking is more or less clear and well spoken, the AI can adequately transcribe it. For the purposes of this project it is fine, but I do want to note the poor quality in the audio.

### Friday - 06/23/23
- Scrapped the HTTPClient library as there is no clear way to send the data in multi form manner.
- Oppurtunity to learn and understand how HTTP requests are formatted. The whole request is written line by line.
- Using the Root CA cert of the api,Ican establish a secure transmission
- Successful response is given.Ican parse out this text and use it for the final Chat request

### Wednesday - 06/28/23
- Expanding upon the audio transmission, the audio prompt in text is now sent to the completions API.
- ChatGPT is successfully answering the prompt.
- There were problems with memory because Arduino `String` were being used.
- After switching to C based strings,Iencountered Stack and buffer based overflows.
- Switching all C string functions such as `strcpy` to their counterpart `strncpy` fixed the errors.

### Thursday - 07/13/23
- Ordered a Shake Sensor.
- Using an interrupt handler the sensor is able to switch on the procedure.
- Moved the main code to loop() so it can be continously ran.
- The main functionality of the code is functioning properly and stress testing reveals no discernable major errors.
- I find that the mic is not as reliable as I like it to be. To be clear, it is recording poorer quality, sped up audio, but to overcome that you would need to speak pretty loud but not too close to the mic. The best fix (and frankly what should be done) is adding an SD card for the audio to be saved on but I don't want to add more chaos to the less number of headers the ESP-32 has.
- After many failed 3D prints, the 8ball enclosure is now complete. It is a modified version of open8ball.com's Arduino Uno version. I will attribute them later in the repo docs soon.
