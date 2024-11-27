# PmgAppCommon Middleware Library 2.0

## Overview

The PmgAppCommon asset provides a set of files that are essential for a USB-C and Power Delivery application.

## Features

The following functionality is provided by the asset:

- Application-level handler for PD stack events
- Provides the application status information
- Prepares application layer for device low-power mode
- Evaluates the source capabilities advertised by the port partner and identify the optimal power contract entered
- Evaluates a PD request data object and determine whether to accept or reject the request
- Evaluates a power role (PR) swap, data role (DR) swap, or a VCONN role swap request from a port partner
- Enables/disables power source output
- Sets the desired voltage and current for the power source output
- Enables/disables the power sink path
- Handler for voltage/current change
- Initializes vendor defined messages (VDM) handler
- Evaluates the received VDM messages and respond to them
- Enables/disables fault protections such as VBUS OVP, VBUS OCP, VBUS SCP, and VConn OCP
- Default handlers for fault protections
- Provides smart power management
- LED control (for example, ON, OFF, blink, breath) driver
- Provides UART and flash based data logging mechanism

## Quick start

See the [API reference guide quick start guide](https://infineon.github.io/pmg-app-common/html/index.html) section for step-by-step instruction how to enable the PmgAppCommon middleware library.

## Related resources

Resources    | Links
-------------|------------------------------------------------------------------
Libraries on GitHub | [mtb-pdl-cat2](https://github.com/Infineon/mtb-pdl-cat2) – Peripheral Driver Library (PDL) and docs
Middleware on GitHub | [pdstack](https://github.com/Infineon/pdstack) – PDStack middleware library and docs <br>[pdutils](https://github.com/Infineon/pdutils) – PDUtils middleware library and docs <br>[pdaltmode](https://github.com/Infineon/pdaltmode) – PDAltMode middleware library and docs <br>[hpi](https://github.com/Infineon/hpi) – HPI middleware library and docs <br>[usbdev](https://github.com/Infineon/usbdev) – USB Device middleware library and docs
Tools | [ModusToolbox&trade;](https://www.infineon.com/modustoolbox) – ModusToolbox&trade; software is a collection of easy-to-use libraries and tools enabling rapid development with Infineon MCUs for applications ranging from wireless and cloud-connected systems, edge AI/ML, embedded sense and control, to wired USB connectivity using PSOC&trade; Industrial/IoT MCUs, AIROC&trade; Wi-Fi and Bluetooth&reg; connectivity devices, XMC&trade; Industrial MCUs, and EZ-USB&trade;/EZ-PD&trade; wired connectivity controllers. ModusToolbox&trade; incorporates a comprehensive set of BSPs, HAL, libraries, configuration tools, and provides support for industry-standard IDEs to fast-track your embedded application development.

## More information

For more information, see the following documents:

* [PmgAppCommon API reference guide](https://infineon.github.io/pmg-app-common/html/index.html)
* [ModusToolbox&trade;, quick start guide, documentation, and videos](https://www.infineon.com/modustoolbox)
* [Infineon Technologies AG](https://www.infineon.com)

---
© 2024, Cypress Semiconductor Corporation (an Infineon company) or an affiliate of Cypress Semiconductor Corporation.
