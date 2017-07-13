# OpenGoldSource

>Gold Source is a version of Quake and Quake is open source so maybe that is an option for you.
>
>Kind regards,
>
>Mike

![OGS Logo](docs/OGSLogo1280x512.png?raw=true "OGS Logo")

Open Source implementation (a recreation) of the GoldSrc engine   
Based on Reverse-engineered (and bugfixed) HLDS aka ReHLDS and Id Tech sources (+ Xash and Metahook)

For more information about the project you can visit it's official [Wiki](https://github.com/Sh1ft0x0EF/OpenGoldSrc/wiki)

Since no one from Valve Corp doesn't want to even touch their original GoldSrc engine sources the decision to write a custom implementation of it was made  
**OpenGoldSrc** (or simply **OGS**) is a custom implementation of the original GoldSrc engine based on the source code that was
used to create the GS - licensed by Valve version of QuakeWorld engine developed by [Id Software](https://github.com/id-Software)  
**OGS** isn't oriented on fully cloning the GoldSrc - mostly cloning the same module structure and provide some new features  
**OGS RW17** is a spiritual successor of [Magenta Engine](https://github.com/projectmagenta) which is still in development and written using the same Quake engine sources using the modern C++ features    
**OGS** is an different signt on how the original GoldSrc should look like in present time if Valve still had work on it

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

What things you need to install the software and how to install them

```
Give examples
```

### Building (REMOVE?)

TODO

### Installing

A step by step series of examples that tell you have to get a development env running

Say what the step will be

```
Give the example
```

And repeat

```
until finished
```

End with an example of getting some data out of the system or using it for a little demo

## Contributing

Feel free to create issues or pull-requests if you have any problems or you want to support the project  
Any help is appreciated (mostly coders and documentation/wiki writers are required)
Please read [CONTRIBUTING](CONTRIBUTING.md) before pushing any changes and for additional information

## Credits

* Thanks to ReHLDS team for their [ReHLDS](https://github.com/dreamstalker/rehlds) - reverse-engineered implementation of dedicated server of GoldSrc engine (aka HLDS) with lot of bugfixes and improvements of original code;
* Lot of Id Tech engines sources by [Id Software](https://github.com/id-Software) were used; thanks them for that;
* Also a lot of [Valve Software](https://github.com/ValveSoftware) sources from their [HLSDK](https://github.com/ValveSoftware/halflife) / [SourceSDK](https://github.com/ValveSoftware/source-sdk-2013) were used;

You can see [CREDITS](CREDITS.md) file for more details

## License

Licensed under GPLv3, see the [LICENSE](LICENSE) file for details  
Some of the code is originally written by Valve LLC and licensed under terms of Valve license which not fully compatible with GPL because it doesn't allow to sell the product  
This product is also not fully legal because it uses the sources that aren't officialy open-sourced (sorry V) by Valve LLC (and won't be because they doesn't want even to touch the engine now)  
The sources probably would be rewritten to new impl (that will use the new C++ standards features) to be more legal  
**This project isn't aimed to earn any commercial profit from it in any way**