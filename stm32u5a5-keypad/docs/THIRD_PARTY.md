# Sources and licenses

- HAL/CMSIS/startup/linker: existing RevK_Bringup project, STM32CubeU5 V1.8.0, with PCD/USB HAL modules added from the same package. Preserve ST/Arm source notices and licenses in firmware/Drivers.
- USB device core: https://github.com/STMicroelectronics/stm32-mw-usb-device commit `2df324bd60d4b0bb27404fd70b1c089b467f0e09`. Portable core copied without functional changes; license retained in firmware/USB/LICENSE.md.
- Embedded USB HS PHY and 160 MHz clock setup follow the local CubeU5 V1.8.0 NUCLEO-U5A5ZJ-Q USBX CDC example. USBX/ThreadX is not used by this application.
- Custom keyboard class/glue and input logic are project code. ST demonstration VID 0483/PID 5710 is for bench prototyping; obtain an assigned pair before distributing a product.
