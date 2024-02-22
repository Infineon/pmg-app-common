# PmgAppCommon Middleware Library 1.0

## Overview

The PmgAppCommon asset provides a set of files that are essential for a USB-C and Power Delivery application.

## Features

The following functionality is provided by the asset:

- Application level handler for PD stack events
- Provide the application status information
- Prepare application layer for device low-power mode
- Evaluate the source capabilities advertised by port partner and identify the optimal power contract entered
- Evaluate a PD request data object and determine whether to accept or reject the request
- Evaluate a power role (PR) swap, data role (DR) swap or a VConn role swap request from a port partner
- Enable/disable power source output
- Set the desired voltage and current for the power source output
- Enable/disable the power sink path
- Handler for voltage/current change
- Initialize vendor defined messages (VDM) handler
- Evaluate the received VDM messages and respond to them
- Enable/disable fault protections such as VBus OVP, VBus OCP, VBus SCP and VConn OCP
- Default handlers for fault protections
- Provides smart power management
- LED control (for example, ON, OFF, Blink, Breath) driver
- Provides UART and flash based data logging mechanism

## Quick Start

See the [API Reference Guide Quick Start Guide](https://infineon.github.io/pmg-app-common/html/index.html) section for step-by-step instruction how to enable the PmgAppCommon middleware library.

## Related resources

Resources    | Links
-------------|------------------------------------------------------------------
Libraries on GitHub | [mtb-pdl-cat2](https://github.com/Infineon/mtb-pdl-cat2) – Peripheral driver library (PDL) and docs
Middleware on GitHub | [pdstack](https://github.com/Infineon/pdstack) – PDStack middleware library and docs <br>[pdutils](https://github.com/Infineon/pdutils) – PDUtils middleware library and docs <br>[pdaltmode](https://github.com/Infineon/pdaltmode) – PDAltMode middleware library and docs <br>[hpi](https://github.com/Infineon/hpi) – HPI middleware library and docs <br>[usbdev](https://github.com/Infineon/usbdev) – USB Device middleware library and docs
Tools | [Eclipse IDE for ModusToolbox&trade; software](https://www.infineon.com/modustoolbox) <br> ModusToolbox&trade; software is a collection of easy-to-use software and tools enabling rapid development with Infineon MCUs, covering applications from embedded sense and control to wireless and cloud-connected systems using AIROC(TM) Wi-Fi & Bluetooth(R) combo devices.

## More information

For more information, see the following documents:

* [PmgAppCommon API Reference Guide](https://infineon.github.io/pmg-app-common/html/index.html)
* [ModusToolbox Software Environment, Quick Start Guide, Documentation, and Videos](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software)
* [Infineon Technologies AG](https://www.infineon.com)

---
© 2024, Cypress Semiconductor Corporation (an Infineon company) or an affiliate of Cypress Semiconductor Corporation.
