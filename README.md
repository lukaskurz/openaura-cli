# OpenAura CLi

At the moment "CLI" is a bit of an overstatement. 
This repo is basically a copy-paste of the [OpenAuraSDK](https://gitlab.com/CalcProgrammer1/OpenAuraSDK), stripped of its UI and windows related code.
My C++ skills are barely above Hello World level, so all I did, was change the code inside the `main` function to set some hardcoded values. If you want to change the colors (you probably will) just go to t he `OpenAuraSDK.cpp` file, scroll down to the `main` function and change the RGB values and the behaviour there.

## Build it

I changed the includes so it imports the code and not the header files, so i can just do this to build the thing.

```bash
$ g++ OpenAuraSDK.cpp
```

I understand that this may be a big nono in the c++ community, but didn't want to write a makefile or anything like that. Maybe I will change this later.

You can just do this and it should build.

```bash
chmod +x ./build.sh
./build.sh
```

## Run it

You have to do some `modprobe` stuff, otherwise the sdk can't find your stuff.
All the relevant stuff is inside the `run.sh`.

```
chmod +x ./run.sh
./run.sh
```

If you want your rgb colors to be set on startup, you can copy the code inside `run.sh` or just call `run.sh`.

## Contribute

Feel free to contribute:

* Makefile or something to build the thing
* Add CLI functionality like params or a config
* Make this windows compatible
* Go to the [OpenAuraSDK REPO](https://gitlab.com/CalcProgrammer1/OpenAuraSDK) and leave those guys a star for their awesome work.
