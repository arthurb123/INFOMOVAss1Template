# INFOMOV 2023/2024 Assignment 1 Template

### Setup
Clone the github repository using `git clone --recursive`, to make sure you get all the submodules.
<br>
If you are on windows;
* Run the `bootstrap_windows.bat` file (this requires Visual Studio 17 2022 by default).
* Open the generated Visual Studio project located in the build folder.
* Run the project in Release mode, making sure you select GAME as the build target.
<br>
If you are on Linux/Mac;
* Depending on your architecture, run either `bootstrap_linux_mac_x86.sh` or `bootstrap_linux_mac_ARM.sh` (making sure you can run it using `chmod +x bootstrap_linux_mac_*.sh`)
* In your build folder you can now run `make` in the future if you want to recompile after making changes to the code.
* Execute `./GAME` to run the project (making sure you can run it using `chmod +x GAME`)

### Notes
Has been tested on;
* Windows
* (Arch) Linux x86 
* MacOS ARM