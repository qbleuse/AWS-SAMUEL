# AWS-SAMUEL

A repository for quick integration and test of AWS Gamelift connection features locally, using SAM in Unreal Engine, namely :

[A]mazon [W]eb [S]ervices - [S]erverless [A]ccess [M]odel [U]nreal [E]ngine [L]ibrary.

It is also a play of word to include the name of a friend as the name of a repository.

## Table of Contents

- [AWS-SAMUEL](#AWS-SAMUEL)
 - [Table of Contents](#table-of-contents)


## What is it ?

This repository aims an easy and quick integration of basic connection feature with AWS Gamelift in Unreal Engine, but locally to not lose money.

It uses and bends the definition of Unreal's Online SubSystem (OSS), so that existing projects already using OSS's would be able to just change the back-end to see their game work.

The expected usage of this repository is of a working game wanting to change their back-end, would be able to integrate this plugin with the prerequisites in a few days and test Gamelift features.

As this is to be used locally, AWS API Gateway is simulated locally by using [AWS-SAM CLI](https://github.com/aws/aws-sam-cli), the implemtation being straightforward : UE Client sends to SAM -> SAM sends to Gamelift -> gamelift redirects to UE Server.

So what this repository is :
- a simple way to integrate AWS in an existing implementation of UE's OSS, and to test it locally.
- a pseudo-tutorial, with the associated walkthrough of the implementation.
- an inspiration for your own implementation.

## What it is not

- A complete solution implementing all of the features of AWS Gamelift
- A robust and secure integration

While the code here may be able to be used outside of the scope of the project as SAM-CLI includes a deploy command to make the corresponding local implementation of the chosen server, and that local integration of Gamelift is supported both for legacy and new SDK, meaning it uses Gamelift Anywhere and that installing Gamelift Anywhere by using this codebase should be trivial ; both are not tested and outside the scope of the intended usage.

Also integration was done with "as long as it works" mindset, so some parts of the implementation (such as SAM lambdas) are very much not optimised for vertical scaling, as they were designed.

You may extend the scope by doing a fork or simply taking and modifying the code as you will (the chosen License will allow you), but do understand that it necessitates additionnal work.

## Why ?

Originally, the need for an integration of AWS Gamelift in an OSS interface came from my graduation project, that needed AWS integration and LAN integration.

To have simple LAN integration in UE, you would just use the OSS Null interface, and I was too lazy to do something completely different for the distant connection with AWS.

As for why this focus on local testing, it is because while we were given the explicit obligation to use AWS as our distant server, AWS did not give credits to allow us to test our implementation.

We were then forced to come up with a solution to test our integration, completely free.

While the integration was not on-time for this project, I worked on it afterwards in hope to help people in the same situation.

