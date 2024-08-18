# Installing OpenSSL

You may not know what OpenSSL is, if that is the case, I'll redirect you to the [wikipedia article](https://fr.wikipedia.org/wiki/OpenSSL) that talks about it. In short, it is a popular cryptographic library for net security. 

Otherwise, OpenSSL is trickier to install and choose, as the major problem here is compatibility.

Both Gamelift and Unreal uses OpenSSL, but depending on what version of the Engine you use, the version of OpenSSL used may defer.
It should always be better to use the latest version of OpenSSL for security purposes, but for compatibility, we may not.
I actually think using another version should work, but the problem is that while compiling may work, packaging of the Engine may not because of this.
Or at least you may need to add the library by hand.

To verify what version of OpenSSL your version of UE uses, you may look in [Engine/Source/ThirdParty/OpenSSL](Engine/Source/ThirdParty/OpenSSL).

As for installing, there is actually quite a lot of choice:

- [precompiled version to download (only 3.0+)](https://openssl-library.org/source/index.html)
- [compile from source](https://github.com/openssl/openssl/tree/master?tab=readme-ov-file#build-and-install)
- [precompiled version to download (legacy + windows only)](https://wiki.openssl.org/index.php/Binaries)
- [download version and compile for you with vcpkg (windows only)](https://vcpkgx.com/details.html?package=openssl)

After having downloaded and compiled working binaries, you need to add it to your environment variable.

On Windows you would need to access the system properties > environment variable > and add and OPENSSL_ROOT_DIR to your system vriables.

I do not know how this would work on other OS's, but I figure you might need to do something similar.

Next, [download and compile the Gamelift Plugin](Install_GameliftSDK_UE_Plugin.md).
