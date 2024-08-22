# Configure AWS OSS Plugin

To use the AWS OSS Plugin, you must configure some stuff to make it compatible with your own project.

Here you are expected to understand the concepts and the inner workings of how AWS connection works.

If you are unfamiliar with it, I invite you to read [this chapter](../Walkthrough/Design.md#amazon-web-services) of the walkthrough.

If you already configured your project to work with the plugin, look here to see [some example usage](Usage.md)

## Integrate the Plugin in your Project

The plugin integrates itself like any other Unreal Engine plugins.

- Put the AWSOSS folder in your Plugins folder
- Add this line to your .uproject under the "Plugins" array
  
```json
    {
      "Name": "AWSOSS",
      "Enabled": true
    },
```

- Add this line to your project's .Build.cs to be able to call the plugin's own method (optional)

```cs
  PublicDependencyModuleNames.Add("AWSOSS");
```

For server targets, I'll advise you to add this to your project's .Build.cs:

```cs
  PublicDependencyModuleNames.Add("GameLiftServerSDK");
```

The plugin will be enabled, now let us get it working.

## Configure AWS OSS for latest SDK

The integration supports both legacy gamelift SDK and latest, configuration changes slightly between the two.

In both, you need to configure the API Gateway URI and the name of the lambda that the plugin will use.

> [!NOTE]
> You may use your own distant URI and lambdas, if you have some, as long as it uses API gateway and that the responses are the same.

An example of lines of configuration of the AWS OSS can be seen in the [DefaultEngine.ini](Config/DefaultEngine.ini) file in the repository.

```ini
[OnlineSubsystem]
DefaultPlatformService=AWS

[OnlineSubsystemAWS]
;APIGatewayURL needs to be between quotation marks if it has numbers in it (as it is a URL)
APIGatewayURL="http://127.0.0.1:3000"
StartSessionURI=/create_session
FindSessionURI=/find_session
JoinSessionURI=/join_session
```

Here the APIGateway URI redirects to localhost, so the local machine at port 3000, wich is the port that uses AWS SAM when it activates its own API Gateway.

> [!NOTE]
> you may change the port in the config file, and in AWS SAM if you need to.

In latest SDK, local testing relies on Gamelift Anywhere, you then need to be able to access internet and communicate with gamelift in the region you configured your local test.

Do not forget to configure your region in the AWS CLI otherwisem, it will not work.
You may do this by using the [configure](https://docs.aws.amazon.com/cli/latest/reference/configure/) command or changing your [~/.aws/config](~/.aws/config) file (this link may not work) directly.

Also to access Gamelift  you will need to have valid credentials in the AWS CLI
that can be configured wit the [same command]((https://docs.aws.amazon.com/cli/latest/reference/configure/)) or by changing the [~/.aws/credentials](~/.aws/credentials) file (this link may not work either).

For more information on local testing with the new SDK, refer to [AWS's own documentation](https://docs.aws.amazon.com/gamelift/latest/developerguide/integration-testing.html)

## Configure AWS OSS for legacy

Configuring for legacy Gamelift would look like this:

```ini
[OnlineSubsystem]
DefaultPlatformService=AWS

[OnlineSubsystemAWS]
;APIGatewayURL needs to be between quotation marks if it has numbers in it (as it is a URL)
APIGatewayURL="http://127.0.0.1:3000"
StartSessionURI=/create_session_local
FindSessionURI=/find_session_local
JoinSessionURI=/join_session_local
```

Lambdas for legacy are different as not all features of Gamelift in legacy SDK.

Also you will need to configure in each lambda the gamelift local endpoint (it may be done as a AWS SAM system wide variable, but I prefered putting it in lambdas):

```py
#the gamelift interface (you can define your own endpoint here, a normal endpoint would be: https://gamelift.ap-northeast-1.amazonaws.com, but we're using a local address for our use case to use Gamelift Local)
game_lift = boto3.client("gamelift", endpoint_url='http://host.docker.internal:9870')
```

Be sure to match the port in the lambdas and in your command when launching your legacy local gamelift.

"http://host.docker.internal" redirects to the machine localhost from inside docker on Windows, it allows us to connect to the gamelift legacy running on our machine. it may be different on other OS's.

For how to launch and test with a legacy Gamelift, refer yourself to [AWS's own documentation on the subject](https://docs.aws.amazon.com/gamelift/latest/developerguide/integration-testing-local.html)


## Configure SAM to your environment

Also, I use python 3.9 to run the lambdas but you can change it to your own version of python, or to another programming language, by changing the [template.yaml](../../Plugins/AWSOSS/SAM/template.yaml) file.

```yaml
# More info about Globals: https://github.com/awslabs/serverless-application-model/blob/master/docs/globals.rst
Globals:
  Function:
    Runtime: python3.9 # change this to your prefered version of python
    Handler: app.lambda_handler
    Architectures:
      - x86_64 # equally you may need to change this if you are on a Apple Silicon Mac.
    # not sure about the Globals commented down here 
    Timeout: 15 # may seem long but the lambdas sometime take more than 3 seconds, it is to be sure
```

You can also tweak the requirements.txt files of each lambda to your environment.
