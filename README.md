
[![CrowdSupply Image](http://planet.timvideos.us/numato-opsis-crowdfunding-campaign-master/badge.png)](https://www.crowdsupply.com/numato-lab/opsis)

The <a href="http://hdmi2usb.tv/numato-opsis">Numato Opsis</a>, the first open hardware for the
<a href="http://hdmi2usb.tv">HDMI2USB firmware</a>,
<a href="https://www.crowdsupply.com/numato-lab/opsis">can now be ordered on 
 ![CrowdSupply](https://www.crowdsupply.com/_teal/images/crowd-supply-logo-dark.png)
</a>

## Low Speed I/O - TOFE Expansion board

While the [Numato Opsis board](https://www.crowdsupply.com/numato-lab/opsis)
has a massive amount of high speed I/O interfaces, it doesn't include many
features for interfacing with more pedestrian speed devices such as buttons or
LEDs. To help solve this problem “Low speed I/O” TOFE expansion board was
created.

![Board Image](img/above.jpg)

The "Low Speed I/O" board has the following features;

 * 1 x Arduino compatible pin header. (With on board ADC enabling analog pins
   support.)

 * 4 x [Digilent Pmod™ Compatible](http://www.digilentinc.com/Pmods/licensing.cfm)
   headers[1] to allow usage of existing expansion boards,
   including boards from 
   [Numato Lab](http://numato.com/fpga-boards/filter/cat/expansion-modules.html?limit=30),
   [Digilent](http://digilentinc.com/pmods),
   [Maxim](https://www.maximintegrated.com/en/design/design-technology/fpga-design-resources/pmod-compatible-plug-in-peripheral-modules.html) and 
   [Analog](https://wiki.analog.com/resources/alliances/xilinx#pmods).

 * 1 x USB-UART, allowing low speed serial operation.

 * 6 x Indicator LEDs

 * 4 x Push Buttons

[1]: Two of the Pmod headers share pins with the Arduino header. This means
that when the Arduino compatible pin header is in use only 2 Pmod headers are
available.

A PDF version of schematic can be found in the [docs](docs) directory.

## Editing

The design in this repository was made using [KiCad](http://www.kicad-pcb.org/)
version **2013-07-07 BZR 4022**
([the version is Ubuntu Trusty](http://packages.ubuntu.com/trusty/kicad)).

## License

TOFE Low Speed I/O Expansion board by Numato Systems Pvt. Ltd

The TOFE Low Speed I/O Expansion board is licensed under a
Creative Commons Attribution-ShareAlike 4.0 International License.

You should have received a copy of the license along with this
work. If not, see <http://creativecommons.org/licenses/by-sa/4.0/>.

![Creative Commons License Logo](https://i.creativecommons.org/l/by-sa/4.0/88x31.png)
