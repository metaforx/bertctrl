# BertCTRL

```bash
              ____...---...___
___.....---"""        .       ""--..____
     .                  .            .
 .             _.--._       /|
        .    .'()..()`.    / /
            (`-.__.-' )  ( (    .
   .         \        /    \ \
       .      \      /      ) )        .
            .' -.__.- `.-.-'_.'
 .        .'  /-____-\`.-'       .
          \  /-.____.-\  /-.
           \ \`-.__.-'/ /\|\|           .
          .'  `.    .'`.
          |/\/\|    |/\/\|

```

Control dwarf bearded dragon Bertis world with a Particle Photon (Arduino compatible IoT dev board), temperature sensors and a 4x relay board.

- Turn on/off heat lamps, UVB light and control temperature.
- Track data using self-hosted Thingsboard IoT platform.

## Hardware
- Particle Photon
- 3x Dallas DS18B20 temperature sensors
- 1x Seedstudio Grove - 4xRelay Board SPDT

## Sensor Mapping
- Dallas DS18B20
  - 1: Yellow wire
  - 2: Red wire
  - 3: Purple wire
