# SimuLess

> **Simulation less, more reality**
>
> A real-time dataflow platform for designing, tuning, and deploying **discrete-time controllers and filters directly on embedded hardware and real systems**.

- ## Index

  - [Why I want to build this](#why-i-want-to-build-this)
  - [SimuLess vs Simulink](#simuless-vs-simulink)

SimuLess is designed for **real-time control engineering**, focusing on:

* **discrete-time control systems**
* **digital filters**
* **state-space systems**
* **online parameter tuning**
* **real hardware deployment**
* **live telemetry and visualization**

Unlike traditional simulation-first tools, SimuLess is built with a **runtime-first philosophy**:

> **design on real systems, validate with real data, deploy in real time**

This makes it ideal for:

* robotics
* drones
* motor control
* embedded control systems
* signal processing
* online system identification

## Roadmap

This project is currently under active development.

- [ ] IO abstraction
- [ ] telemetry core
- [ ] math and solver library
- [ ] real-time scheduler
- [ ] live scope / plotting
- [ ] online PID tuning
- [ ] Kalman filter blocks
- [ ] visual dataflow graph
- [ ] hardware deployment plugins

## Why I want to build this

This project comes directly from repeated pain points in my embedded control and signal-processing work.

In many real-world projects, I often need to move data from embedded hardware into Python scripts using different ad-hoc methods just to perform:

- data analysis
- signal processing
- controller design
- parameter tuning

This leads to a large amount of repeated engineering work around data transport, logging, parsing, and offline processing.

Another major issue is controller tuning.

In practice, adjusting controller parameters often requires repeated firmware flashing, or carefully designed custom parameter-update interfaces.

This process is time-consuming and slows down iteration significantly.

I want SimuLess to reduce this friction.

The goal is to make data flow, analysis, controller design, and parameter tuning part of a unified real-time workflow, directly connected to the real system.

At the same time, I want this workflow to be **open, lightweight, and engineering-friendly**, especially for embedded systems and robotics.

## SimuLess vs Simulink

Simulink is a powerful **simulation-first commercial platform** focused on model-based design and code generation.

SimuLess follows a different philosophy:

> **simulation less, more reality**

It is designed as an **open-source, runtime-first platform** for real-time discrete control and filtering systems.

Instead of starting from complex simulation models, SimuLess focuses on:

- real hardware
- live telemetry
- online tuning
- real-time execution

We aim to make controller and filter design more **lightweight, transparent, and open**, especially for embedded systems and robotics.

As an open-source project, SimuLess emphasizes:

- modular architecture
- extensibility
- developer-friendly APIs
- community-driven evolution
