## Documentation

### Basics

- Load a file containing comma seperated values, by dropping it on the window.
- Run a script file containing commands, by dropping it on the window (there is also a `run script` command).

#### Movement

- Pan the view, by holding the left mouse button and draging.
- Move the coordinate system, by holding the right mouse button and dragging.
- Zoom using the scroll wheel. The zoom is centered around the coordinat system.
- Zoom the X-axis by scrolling while pressing left-shift.
- Zoom the Y-axis by scrolling while pressing left-ctrl.
- Fit the window to the content by pressing space.

#### Commands

The commands are explained by examples. The pattern is not that complicated.\

##### iterators
Iterators can be used everywhere in place of integer values. They can be understood as loops over the commands.\
Multiple iterators can be used in the same command, like nested loops.\
- `1,2,5,3` (iterates over the comma seperated indices).
- `23..199` (iterates from 23 to 199 (inclusive)).
- `4,1,2,23..199,500` (they can be combined)
- `400..2` This iterator iterates backwards, although this usually doesn't have any effect.

##### `fit`
- `function new = fit sinusoid data 3`
- `function new "my fit of data 0" = fit sinusoid data 0`
##### `hide`
- `hide function 10`
##### `show`
- `show data 5` (if data 5 )

## Building on Windows

Run the `build.bat` script from the Microsoft Visual Studio _"x64 Native Tools Command Prompt"_ or from any command prompt with the `vcvars64.bat` environment.\
Alternatively, prebuilt binaries can be found in the relases.
 
