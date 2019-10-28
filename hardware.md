# Hardware

## Schematic
![schematic](/images/MHI-SPI_Schematic.png)

## Connector
The AC provides the signals via the CNS connector. It has 5 pins with a pitch of 2 mm (0.079''). It is out of the [PH series from JST](http://www.jst-mfg.com/product/detail_e.php?series=199). The position of the connector is visible on the following photo of the indoor unit PCB.
![Indoor PCB](/images/SRK-PCB.jpg)
**Opening of the indoor unit should be done by a qualified professional because faulty handling may cause leakage of water, electric shock or fire!**

## Power Supply
The connector provides +12V. The DC-DC converter [TSR 1-2450](https://www.tracopower.com/products/browse-by-category/find/tsr-1/3/) is used to convert it to +5V.

## MHI-SPI Testboard
The [Eagle](https://www.autodesk.com/products/eagle/overview) files (schematic & PCB) for the testboard are located in the sub-folder [eagle](/eagle). On the PCB are located many jumper and test pins to enable simple connections to measuring devices.

The following photo shows the assembled testboard.
![Testboard](/images/testboard.jpg)
The upper colored cables are connected to an AC remote control. The JST connector is connected to the MHI AC.

