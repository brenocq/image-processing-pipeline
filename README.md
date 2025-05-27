# Image Processing Pipeline
<div align="center">
  <img src="https://github.com/user-attachments/assets/4f8a7c8a-3452-4ca1-8092-c39c30f222cf"/>
  <img src="https://github.com/user-attachments/assets/103acf04-e7bb-4433-9dcf-8fba2d9a2db8"/>
  <img src="https://github.com/user-attachments/assets/64fd339c-5d63-4276-a2e0-fae45d4fb25b"/>
</div>

## Overview

This project provides a practical implementation of an image processing pipeline, designed to simulate and correct common image imperfections. The core idea is to take a "perfect" input image, degrade it with various sensor and lens-related artifacts, and then process this "corrupted" image through a series of correction stages to restore it as close as possible to the original quality. This simulation environment is highly valuable for understanding, developing, and testing algorithms for Image Signal Processors (ISPs), particularly relevant for embedded camera systems.

## Features

The project's pipeline is structured into two main sequential stages:

### 1. Image Degradation Pipeline

This pipeline takes a clean, high-quality image and introduces various common imperfections that would typically arise from an imperfect camera sensor and lens:

* **Input Reference Image:**

  <img src="https://github.com/user-attachments/assets/53a9b86f-1ab3-44c3-b9fe-8ce8636a87eb" width="150"/>
* **White Balance Error:** Simulates color casts caused by non-neutral environmental illumination (e.g., incandescent, shade).
  <img src="https://github.com/user-attachments/assets/cf5b420b-4c8d-4a13-b7f8-bc66186cd1a6" width="150"/>
* **Barrel Lens Distortion:** Introduces geometric warping where straight lines in the scene appear to bend outwards from the center.
  <img src="https://github.com/user-attachments/assets/623d842e-d36b-43d9-93fa-38b8cba74b6b" width="150"/>
* **Color Shading Error:** Applies color tints that vary across the image frame, often seen as a color shift towards the edges.
  <img src="https://github.com/user-attachments/assets/370dba88-67e9-45e1-b7a1-4838b48de56c" width="150"/>
* **Chromatic Aberration:** Simulates color fringing (usually red/cyan or blue/yellow) around high-contrast edges due to the lens's inability to focus all colors to the same point.
  <div align="left">
    <img src="https://github.com/user-attachments/assets/0d9ca23c-5963-4817-8846-33bd6906e81e" width="150"/>
    <img src="https://github.com/user-attachments/assets/ea73a613-641c-4361-bb49-f881d4b87a51" width="150"/>
  </div>
* **Vignetting Error:** Introduces a falloff in brightness towards the edges and corners of the image.
  
  <img src="https://github.com/user-attachments/assets/f417b895-4013-4f6f-a2f3-b0b9882dc70f" width="150"/>
* **Black Level Offset:** Adds a constant baseline signal to all pixels, lifting the true black point above zero.
  
  <img src="https://github.com/user-attachments/assets/55b372d8-82fb-4532-b37c-5fdd38851f2a" width="150"/>
* **Dead Pixel Injection:** Randomly inserts non-functional pixels (e.g., pixels stuck at a zero value) into the image data.
  <div align="left">
    <img src="https://github.com/user-attachments/assets/f69cd73b-59bb-4ee1-89e9-ec1a661e0b06" width="150"/>
    <img src="https://github.com/user-attachments/assets/fe6bfa2b-764b-418f-a5dc-474e0cce8aa7" width="150"/>
  </div>

### 2. Image Processing Pipeline

This pipeline receives the degraded image (the output of the degradation pipeline) and applies a series of correction algorithms to attempt to restore it closer to the original, ideal image:

* **Dead Pixel Correction:** Identifies and interpolates values for defective pixels based on their good neighbors.
* **Black Level Correction:** Subtracts the overall baseline offset to correctly set the image's black point.
* **Vignetting Correction:** Compensates for brightness falloff towards image edges, making illumination uniform.
* **Chromatic Aberration Correction:** Spatially shifts the affected color channels to realign them at edges, removing color fringes.
* **Color Shading Correction:** Corrects for spatially varying color tints to ensure uniform color balance across the frame.
* **Lens Correction:** Corrects for geometric lens distortion (e.g., barrel distortion), straightening lines.
* **White Balance Correction:** Adjusts the image's color balance to neutralize color casts, with options for both manual (based on Kelvin temperature) and automatic correction (using the White Patch method).

## How to Build and Run

This project was developed using [Atta](https://github.com/brenocq/atta) v0.3.11, which is not yet released. Atta provides the necessary infrastructure for:
  - Loading and managing image resources.
  - Rendering image outputs and pipeline stages to a graphical user interface (using ImGui/ImPlot for interactive visualization).
  - Structuring the project's execution logic.
  - **Note:** Atta 0.3.11 is not yet officially released.

1.  **Clone the repository:**
    ```bash
    git clone git@github.com:brenocq/image-processing-pipeline.git
    cd image-processing-pipeline
    ```
2.  **Ensure Atta is available:**
    You must have `Atta` installed locally on your system. Note that a Linux/MacOS system is recommended to use Atta.
    ```
    git clone git@github.com:brenocq/atta.git
    cd atta
    git checkout feat/save-window-layout
    ./build.sh --help
    ./build.sh --install
    atta --version # Check if atta was properly installed
    ```
4.  **Run the application:**
    ```
    cd image-processing-pipeline
    atta image-processing-pipeline.atta
    ```

## Future Work / Potential Improvements
- Implement more advanced algorithms for noise reduction (e.g., non-local means, wavelet-based), tone mapping, and sharpening.
- Explore highly optimized fixed-point arithmetic implementations for all stages to enhance performance on resource-constrained embedded MCUs.
- Add support for processing higher bit-depth images (e.g., 10-bit, 12-bit) throughout the pipeline.
- Improve the UI for real-time visual parameter tuning and direct comparison of original, degraded, and corrected images.
- Expand the range and complexity of simulated degradation effects.
