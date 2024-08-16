# AWS-SAMUEL

A repository for quick integration and test of AWS Gamelift connection features locally, using SAM in Unreal Engine, namely :

[A]mazon [W]eb [S]ervices - [S]erverless [A]ccess [M]odel [U]nreal [E]ngine [L]ibrary.

It is also a play of word to include the name of a friend as the name of a repository.

## Table of Contents

- [AWS-SAMUEL](#aws-samuel)
  - [Table of Contents](#table-of-contents)
  - [What is it ?](#what-is-it-)
  - [What it is not](#what-it-is-not)
  - [Why ?](#why-)
  - [What is included](#what-is-included)
  - [How to install](#how-to-install)
  - [How to use / Get Started](#how-to-use--get-started)
  - [Walkthrough](#walkthrough)
  - [References and Links](#references-and-links)
  - [License](#license)


## What is it ?

This repository aims an easy and quick integration of basic connection feature with AWS Gamelift in Unreal Engine.

It uses and bends the definition of Unreal's Online SubSystem (OSS), so that existing projects already using OSS's would be able to just change the back-end to see their game work.

The expected usage of this library is of an Unreal Engine game already working with OSS wanting to test using AWS as their back-end, and this library allowing them to test quickly with the prerequisites and the plugin available.

As this is to be used locally, AWS API Gateway is simulated locally by using [AWS-SAM CLI](https://github.com/aws/aws-sam-cli).

So what this repository is :

- a simple way to integrate AWS in an existing implementation of UE's OSS, and to test it locally.
- a pseudo-tutorial, with the associated walkthrough of the implementation.
- an inspiration for your own implementation.

## What it is not

- A complete solution implementing all of the features of AWS Gamelift
- A robust and secure integration

While the code here may be able to be used outside of the scope of the project as SAM-CLI includes a deploy command to make the corresponding local implementation of the chosen server, and that local integration of Gamelift is supported both for legacy and new SDK, meaning it uses Gamelift Anywhere and that installing Gamelift Anywhere by using this codebase should be trivial ; both are not tested and outside the scope of the intended usage.

Also integration was done with "as long as it works" mindset, so some parts of the implementation (such as SAM lambdas) are very much not optimised.

You may extend the scope by doing a fork or simply taking and modifying the code as you will (the chosen License will allow you), but do understand that it necessitates additionnal work.

## Why ?

Originally, the need for an integration of AWS Gamelift in an OSS interface came from my graduation project, that needed AWS integration and LAN integration.

To have simple LAN integration in UE, you would just use the OSS Null interface, and I was too lazy to do something completely different for the distant connection with AWS.

As for why this focus on local testing, it is because while we were given the explicit obligation to use AWS as our distant server, AWS did not give credits to allow us to test our implementation.

We were then forced to come up with a solution to test our integration, completely free.

While the integration was not on-time for this project, I worked on it afterwards in hopes to help people in the same situation.

## What is included

- [An Unreal Engine custom Online SubSystem Plugin, integrating creating, finding, and joining AWS Gamelift game sessions](Plugins/AWSOSS/)
- [A SAM local API gateway for local test](Plugins/AWSOSS/SAM/)
- [Some source files as example to use said Plugin](Source/)
- [An .ini file with lines to copy to make the Plugin work](Config/DefaultEngine.ini)
- [A walkthrough explaining how the code works for those who want to go further](#walkthrough)

## How to install

Intalling is quite a pain, as it requires you to compile Gamelift Plugin and install other stuff, and actually half the work, here is the [guide to get it installed](Documentation/Install/Prerequisites.md).

Also one thing : this plugin and the test to verify it worked were done exclusively on Windows 11.

While, by seing the tool used, I very much think that it should not be that much work to make it work on other OS's, know that it was not tested and that they may be compatibility issues.

## How to use / Get Started

For how to use and get started on using the plugin, see [here](Documentation/Usage/Configuration.md)

## Walkthrough

If you want to know how the integration was done, maybe to develop your own version or out of curiosity, it is [this way](Documentation/Walkthrough/Design.md).

Once again, this walkthrough will be told from the point of view of a Windows User.
Also this is aimed towards beginner that never worked on how to connect to computers before (as this was my case when I did this), but I expect Unreal Engine and coding experiences, and will not explain basic programming principles.
Also, as I am also learning, there may be thing taht are false, or incomplete.

## References and Links

Here is a [list of all the links I used](Documentation/References.md) to do this project, view this as a researcher giving his bibliography.

## License

AWS-SAMUEL is licensed under the MIT License, see [LICENSE](LICENSE) for more information.

Long story short, do as you want.
