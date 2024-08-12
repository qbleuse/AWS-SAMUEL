# Installing Gamelift SDK Unreal Engine Plugin

Once again, the installation process is fairly well documented by Amazon itself in multiple languages, via [this link](https://docs.aws.amazon.com/gamelift/latest/developerguide/integration-engines-setup-unreal.html#integration-engines-setup-unreal-setup).

We'll still do a step by step because for some reason, I had a hard time doing this.

This steps will be mostly for Windows Users.

## Download Gamelift Unreal Engine Plugin

This is simple and straightforward : download the Release from the [github page](https://github.com/aws/amazon-gamelift-plugin-unreal/releases/).

## Compiling Gamelift Server SDK

This is where it is difficult, you need to compile the cpp library of Gamelift Server SDK to make the Unreal Engine Plugin work.

The steps are fairly well explained [here](https://github.com/aws/amazon-gamelift-plugin-unreal?tab=readme-ov-file#build-the-amazon-gamelift-c-server-sdk), but there are some pitfalls.

Firstly, the lack of explnation about the need for OpenSSL, we already did it so we are okay with this, but this is clearly lacking here.

Secondly, the compilation is done with CMake in Command Line, so you may have two problems:

- you do not have CMake (in that case download it [here](https://cmake.org/download/), not a major problem)
- you have CMake but does not work in Command Line : you may need to add it to system variables or in your system %PATH%.

Thirdly, the generator used is Visual Studio, and you build in command line, meanging you need to ad your tools (VS[140/150/160]CMNTOOLS) as system variable, and add msvc or similar to your %PATH%.
Also, there may have compilation errors from different version of VS compared to the source.

Finally, CMake will need to download repository such as boost, so you need to have git, and an internet connection for it to work.

There is numerous other ways it failed but I cannot remember all of them.
It will mature with time and become more robust, but you may have a difficult time getting those two libraries that are the only thing that interests you.

## Installing Plugin in your project

After getting those two files and putting them in the plugin, you can install the plugin as every other Unreal Plugin in the Plugins folder, and add it to your .uproject files and .Build.cs files to use it.

You may also add the AWSOSS plugin as this is the only thing it really requires to work, outside of the configuration after intalling it.

For this, I invite you to look at [how to use the plugin](../Usage/Configuration.md).

This is the end for the installation part, good work !
Take a little break !
