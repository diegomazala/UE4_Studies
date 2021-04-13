// Some copyright should be here...

using UnrealBuildTool;

public class LicenseSystemPlugin : ModuleRules
{
	public LicenseSystemPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
        // Add any include paths for the plugin
        //PublicIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "opensslcpp/include"));

        // Add any import libraries or static libraries
        //PublicAdditionalLibraries.Add(System.IO.Path.Combine(ModuleDirectory, "lib", "libcrypto.lib", "libssl.lib", "openssl_cpp_crypto.lib"));


		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"PlatformCryptoOpenSSL"
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
