# Real-Time Audio Loopback System using TMS320C6748 DSP

## Overview

This project implements a **Real-Time Audio Loopback System** on the **Texas Instruments TMS320C6748 DSP Starter Kit** using the **TLV320AIC3106 Audio Codec**.

The system captures audio from the microphone through the codec, transfers the digital samples to the DSP using the **McASP (Multichannel Audio Serial Port)** interface, performs real-time processing, and immediately sends the processed samples back to the codec for playback through speakers or headphones.

The project demonstrates the implementation of embedded digital audio processing using the **TI C6748 DSP**, **McASP**, and **I2C** interfaces.

---

# Features

- Real-time audio acquisition
- Audio loopback with very low latency
- Audio amplification
- McASP-based audio streaming
- I2C communication with TLV320AIC3106 codec
- Embedded C implementation
- Code Composer Studio (CCS) project

---

# Hardware Used

- TI TMS320C6748 DSP Development Board
- TLV320AIC3106 Audio Codec
- Microphone
- Speaker / Headphones
- USB Cable

---

# Software Used

- Code Composer Studio (CCS)
- Embedded C
- TI Processor SDK
- VSK-6748 Common Board Support Package (BSP)

---

# Project Workflow

```
Microphone
      │
      ▼
TLV320AIC3106 Audio Codec
      │
      ▼
McASP Interface
      │
      ▼
TMS320C6748 DSP
      │
      ▼
Audio Processing
(Audio Amplification)
      │
      ▼
McASP Interface
      │
      ▼
TLV320AIC3106 Codec
      │
      ▼
Speaker / Headphones
```

---

# Repository Structure

```
Real-Time-Audio-Loopback-System/
│
├── source/
│   ├── main.c
│   ├── VSK_6748.c
│   └── VSK_6748.h
│
├── ccs_project/
│   ├── .project
│   ├── .cproject
│   ├── .ccsproject
│   ├── C6748.cmd
│   ├── .settings/
│   ├── .launches/
│   └── targetConfigs/
│
├── docs/
│   └── Project_Report.pdf
│
├── README.md
├── LICENSE
└── .gitignore
```

---

# Working Principle

1. Initialize the DSP peripherals.
2. Configure the McASP interface.
3. Configure the TLV320AIC3106 audio codec using I2C.
4. Capture audio samples from the codec.
5. Process the received samples on the DSP.
6. Amplify the audio signal.
7. Send the processed audio back to the codec.
8. Play the amplified signal through the speaker.

---

# Audio Processing

The received audio samples are amplified before transmission.

```c
dat = MCASP->XBUF4;
MCASP->XBUF3 = dat * 3;
```

The amplification factor can be modified according to application requirements.

---

# Prerequisites

Before building the project, install:

- Code Composer Studio (CCS)
- TI Processor SDK
- VSK-6748 Common Board Support Package (BSP)

The project requires the **VSK-6748-Common** library for board initialization, McASP configuration, I2C communication, and codec support.

Example installation path:

```
D:\ti_kit\mic\PROGRAMS\VSK-6748-Common
```

---

# Build Instructions

1. Install Code Composer Studio.
2. Install the TI Processor SDK and VSK-6748 Common BSP.
3. Import the CCS project.
4. Configure the include paths to the BSP.
5. Build the project.
6. Load the executable onto the TMS320C6748 DSP board.
7. Connect a microphone and speaker.
8. Run the application.

---

# Applications

- Digital Audio Processing
- Voice Communication Systems
- Public Address Systems
- Embedded DSP Learning
- Audio Signal Processing
- Real-Time Embedded Systems

---

# Future Improvements

- Digital Audio Filters
- Noise Reduction
- Echo Cancellation
- Graphic Equalizer
- Audio Recording
- Audio Effects (Echo, Reverb)
- FFT Spectrum Analyzer

---

# Author

**Sathish M**

B.E. Electronics and Communication Engineering

PSG College of Technology

GitHub: https://github.com/rio-sathish

LinkedIn: https://www.linkedin.com/in/sathish-m-b19844289/

---

# License

This project is licensed under the MIT License.
