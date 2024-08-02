
## Building on Windows

Run the `build.bat` script from the Microsoft Visual Studio _"x64 Native Tools Command Prompt"_ or from any command prompt with the `vcvars64.bat` environment.\
Alternatively, prebuilt binaries can be found in the [releases](https://github.com/yuzeni/FasterPlot/releases).
 
## Documentation

#### Basics
- Load a file containing comma seperated values, by dropping it on the window.
- Run a script file containing commands, by dropping it on the window (there is also a `run script` command).

#### Reverting commands
- Use *ctrl + left arrow key* to revert any command.
- Use *ctrl + right arrow key* to revert, reverting a command.

When loading a new file, you can only revert back up until this event.

#### Movement
- Pan the view, by holding the *left mouse button* and draging.
- Move the coordinate system, by holding the *right mouse button* and dragging. Or by clicking somewhere with the *right mouse button*.
- Zoom using the *scroll wheel*. The zoom is centered around the coordinat system.
- Zoom the X-axis by scrolling while pressing *left-shift*.
- Zoom the Y-axis by scrolling while pressing *left-ctrl*.
- Fit the window to the content by pressing *space*.

#### Commands
The commands are explained mostly by examples.\
Just start typing in the window and the command input will appear.
- Press *backspace* to delete the last character, holding *left-ctrl* deletes the whole word (token).
- Press *enter* to execute the command.
- Press *ESC* to exit the command input.

The commands as well as infos and errors are written to the shell.

##### iterators
Iterators can be used everywhere in place of integer values. They can be understood as loops over the commands.\
Multiple iterators can be used in the same command, like nested loops.
- `1,2,5,3` (iterates over the comma seperated indices.)
- `23..199` (iterates from 23 to 199 (inclusive).)
- `4,1,2,23..199,500` (they can be combined)
- `400..2` (This iterator iterates backwards, although this usually doesn't have any effect.)

##### `zero`
Resets to origin to (0,0), while fitting the window to the content.
- `zero`

##### `fit`
Fits a **function** to **data**. Currently there is only the `sinusoid` option.
- `function 0 = fit sinusoid data 3 0` (with 0 refine iterations)
- `function new = fit sinusoid data 3 10` (with 10 refine iterations)
- `function new "my fit of data 0" = fit sinusoid data 0 0,100,1000` (with 0,10,1000 refine iterations)

##### `help`
Prints this documentation to the shell.
- `help`

##### `hide`
Hides objects..
- `hide function 5..10`

..and specific visualizations of objects.
- `hide points data 1` (disables point visualization)
- `hide index data 1` (disables index visualization)
- `hide lines data 1` (disables line visualization)

##### `show`
If the object is hidden, `show` will make it visible.\
If the object is already visible, `show` will fit the window to the object.
- `show data 5`

Also used to show specific visualizations of object.
- `show points data 1` (enables point visualization)
- `show index data 1` (enables index visualization)
- `show lines data 1` (enables line visualization)

##### `smooth`
Averages **data** over a specified window size.
- `smooth data 1,2,3 5` (window size 5)
- `smooth data 8 20` (window size 20)

##### `interp`
Interpolates the **data** linearly. An integer argument specifies how many times the data should be interpolated (doubled in size).
- `interp data 0 1` (doubles the points of data 0)
- `interp data 0 2` (quadruples the points of data 0)

##### setting the X axis
Any **data** is interpreted as a set of Y values which are by default plotted by their index.
However **data** can reference other **data** as its X axis.
- `data 1..4 x = data 0`

##### `extrema`
Get the local extrema of **data**. They must be saved in another data object. Usually applied after smoothing the **data**.
- `data 10 = extrema data 3`
- `data new = extrema data 1,3,5`

##### deleting things
- `delete data 0..2`
- `delete funciton 4`

Delete specific **data** points.
- `delete points 100..1000 data 9`

##### exporting data and functions
Exports **data** as comma seperated .txt files or **functions** as their variable and value form.\
They will be located in the *exports* folder.
- `export data 1,2,3` (default file name *export*)
- `export data 1,2,3 "my_data"` (specified file name)
- `export function 1,2,3 "my_functions"`

##### saving all executed commands to a script
Saves .script files to the scripts folder.
- `save script` (default file name *save*)
- `save script "my script"` (specified file name)

Keep in mind that all commands are cleared from the command list, omitting them from being saved, if a new file is loaded, 
because a script should not depend on any external files.\
Similarly if a command file is loaded, all its commands will be copied to the command list, not the command of loading the script.

##### running command scripts
- `run script "my script"`

Dropping the script onto the window is also supported.\
The script must have the file extension *.script*.

##### printing data
The commands must be prefixed with `=` to print the result.
- `= x points 20 data 7` (prints the x value of the 20th points of **data 7**)
- `= y points 0..4 data 7` (prints the y value of points 0..4 of **data 7**)
- `= function 3 4525.67` (prints the result of **function 3** at X `4525.67`)
- `= function 5 b` (prints the parameter **b** of **function 5**)

These expressions can also be used with the basic mathematical operators.

##### basic mathematical operations
- Assigning things.\
  Currently it is not possible to assign to single values, like `y points 9 data 2`.
  - `data 0 = data 1`
  - `data new = data 1..3`
  - `function new = functin 8`
  - `data new = data 3 + data 6` (requires **data 3 x** = **data 6 x**)
  - `data 4 = data 3 - data 6` (requires **data 3 x** = **data 6 x**)
  - `data new = data 3 * data 1..8` (requires **data 3 x** = **data 1..8 x**)
  - `data new = data 3 / data 6` (requires **data 3 x** = **data 6 x**)
  - `data new = data 0..10 * data 0..10` (double iteration)

- Operation assinging things. Supports the same operations as above.
  - `data 0 += data 1,2`

- Cacluating with single values. Also supports the same operations.\
  - `= function 3 d - function 4 d`
