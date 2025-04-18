// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"

#include "PowershellCoreProfileGenerator.h"
#include "LegacyProfileGeneratorNamespaces.h"
#include "../../types/inc/utils.hpp"
#include "../../inc/DefaultSettings.h"
#include "Utils.h"
#include "DynamicProfileUtils.h"

// These four are headers we do not want proliferating, so they're not in the PCH.
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Management.Deployment.h>
#include <appmodel.h>
#include <shlobj.h>
#include <LibraryResources.h>

static constexpr std::wstring_view POWERSHELL_PFN{ L"Microsoft.PowerShell_8wekyb3d8bbwe" };
static constexpr std::wstring_view POWERSHELL_PREVIEW_PFN{ L"Microsoft.PowerShellPreview_8wekyb3d8bbwe" };
static constexpr std::wstring_view PWSH_EXE{ L"pwsh.exe" };
static constexpr std::wstring_view POWERSHELL_ICON{ L"ms-appx:///ProfileIcons/pwsh.png" };
static constexpr std::wstring_view POWERSHELL_PREVIEW_ICON{ L"ms-appx:///ProfileIcons/pwsh-preview.png" };
static constexpr std::wstring_view GENERATOR_POWERSHELL_ICON{ L"ms-appx:///ProfileGeneratorIcons/PowerShell.png" };
static constexpr std::wstring_view POWERSHELL_PREFERRED_PROFILE_NAME{ L"PowerShell" };

using namespace ::Microsoft::Terminal::Settings::Model;

namespace winrt::Microsoft::Terminal::Settings::Model
{
    DEFINE_ENUM_FLAG_OPERATORS(PowershellCoreProfileGenerator::PowerShellFlags);

    constexpr bool PowershellCoreProfileGenerator::PowerShellInstance::operator<(const PowerShellInstance& second) const
    {
        if (majorVersion != second.majorVersion)
        {
            return majorVersion < second.majorVersion;
        }
        if (flags != second.flags)
        {
            return flags > second.flags; // flags are inverted because "0" is ideal; see above
        }
        return executablePath < second.executablePath; // fall back to path sorting
    }

    // Method Description:
    // - Generates a name, based on flags, for a powershell instance.
    // Return value:
    // - the name
    std::wstring PowershellCoreProfileGenerator::PowerShellInstance::Name() const
    {
        std::wstringstream namestream;
        namestream << L"PowerShell";

        if (WI_IsFlagSet(flags, PowerShellFlags::Store))
        {
            if (WI_IsFlagSet(flags, PowerShellFlags::Preview))
            {
                namestream << L" Preview";
            }
            namestream << L" (msix)";
        }
        else if (WI_IsFlagSet(flags, PowerShellFlags::Dotnet))
        {
            namestream << L" (dotnet global)";
        }
        else if (WI_IsFlagSet(flags, PowerShellFlags::Scoop))
        {
            namestream << L" (scoop)";
        }
        else
        {
            if (majorVersion < 7)
            {
                namestream << L" Core";
            }
            if (majorVersion != 0)
            {
                namestream << L" " << majorVersion;
            }
            if (WI_IsFlagSet(flags, PowerShellFlags::Preview))
            {
                namestream << L" Preview";
            }
            if (WI_IsFlagSet(flags, PowerShellFlags::WOWx86))
            {
                namestream << L" (x86)";
            }
            if (WI_IsFlagSet(flags, PowerShellFlags::WOWARM))
            {
                namestream << L" (ARM)";
            }
        }
        return namestream.str();
    }

    // Function Description:
    // - Finds all powershell instances with the traditional layout under a directory.
    // - The "traditional" directory layout requires that pwsh.exe exist in a versioned directory, as in
    //   ROOT\6\pwsh.exe
    // Arguments:
    // - directory: the directory under which to search
    // - flags: flags to apply to all found instances
    // - out: the list into which to accumulate these instances.
    static void _accumulateTraditionalLayoutPowerShellInstancesInDirectory(std::wstring_view directory, PowershellCoreProfileGenerator::PowerShellFlags flags, std::vector<PowershellCoreProfileGenerator::PowerShellInstance>& out)
    {
        const std::filesystem::path root{ wil::ExpandEnvironmentStringsW<std::wstring>(directory.data()) };
        if (std::filesystem::exists(root))
        {
            for (const auto& versionedDir : std::filesystem::directory_iterator(root))
            {
                const auto versionedPath = versionedDir.path();
                const auto executable = versionedPath / PWSH_EXE;
                if (std::filesystem::exists(executable))
                {
                    const auto preview = versionedPath.filename().native().find(L"-preview") != std::wstring::npos;
                    const auto previewFlag = preview ? PowershellCoreProfileGenerator::PowerShellFlags::Preview : PowershellCoreProfileGenerator::PowerShellFlags::None;
                    out.emplace_back(PowershellCoreProfileGenerator::PowerShellInstance{ std::stoi(versionedPath.filename()),
                                                                                         PowershellCoreProfileGenerator::PowerShellFlags::Traditional | flags | previewFlag,
                                                                                         executable });
                }
            }
        }
    }

    // Function Description:
    // - Finds the store package, if one exists, for a given package family name
    // Arguments:
    // - packageFamilyName: the package family name
    // Return Value:
    // - a package, or nullptr.
    static winrt::Windows::ApplicationModel::Package _getStorePackage(const std::wstring_view packageFamilyName) noexcept
    try
    {
        winrt::Windows::Management::Deployment::PackageManager packageManager;
        auto foundPackages = packageManager.FindPackagesForUser(L"", packageFamilyName);
        auto iterator = foundPackages.First();
        if (!iterator.HasCurrent())
        {
            return nullptr;
        }
        return iterator.Current();
    }
    catch (...)
    {
        LOG_CAUGHT_EXCEPTION();
        return nullptr;
    }

    // Function Description:
    // - Finds all powershell instances that have App Execution Aliases in the standard location
    // Arguments:
    // - out: the list into which to accumulate these instances.
    static void _accumulateStorePowerShellInstances(std::vector<PowershellCoreProfileGenerator::PowerShellInstance>& out)
    {
        wil::unique_cotaskmem_string localAppDataFolder;
        if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppDataFolder)))
        {
            return;
        }

        std::filesystem::path appExecAliasPath{ localAppDataFolder.get() };
        appExecAliasPath /= L"Microsoft";
        appExecAliasPath /= L"WindowsApps";

        if (std::filesystem::exists(appExecAliasPath))
        {
            // App execution aliases for preview powershell
            const auto previewPath = appExecAliasPath / POWERSHELL_PREVIEW_PFN;
            if (std::filesystem::exists(previewPath))
            {
                const auto previewPackage = _getStorePackage(POWERSHELL_PREVIEW_PFN);
                if (previewPackage)
                {
                    out.emplace_back(PowershellCoreProfileGenerator::PowerShellInstance{
                        gsl::narrow_cast<int>(previewPackage.Id().Version().Major),
                        PowershellCoreProfileGenerator::PowerShellFlags::Store | PowershellCoreProfileGenerator::PowerShellFlags::Preview,
                        previewPath / PWSH_EXE });
                }
            }

            // App execution aliases for stable powershell
            const auto gaPath = appExecAliasPath / POWERSHELL_PFN;
            if (std::filesystem::exists(gaPath))
            {
                const auto gaPackage = _getStorePackage(POWERSHELL_PFN);
                if (gaPackage)
                {
                    out.emplace_back(PowershellCoreProfileGenerator::PowerShellInstance{
                        gaPackage.Id().Version().Major,
                        PowershellCoreProfileGenerator::PowerShellFlags::Store,
                        gaPath / PWSH_EXE,
                    });
                }
            }
        }
    }

    // Function Description:
    // - Finds a powershell instance that's just a pwsh.exe in a folder.
    // - This function cannot determine the version number of such a powershell instance.
    // Arguments:
    // - directory: the directory under which to search
    // - flags: flags to apply to all found instances
    // - out: the list into which to accumulate these instances.
    static void _accumulatePwshExeInDirectory(const std::wstring_view directory, const PowershellCoreProfileGenerator::PowerShellFlags flags, std::vector<PowershellCoreProfileGenerator::PowerShellInstance>& out)
    {
        const std::filesystem::path root{ wil::ExpandEnvironmentStringsW<std::wstring>(directory.data()) };
        const auto pwshPath = root / PWSH_EXE;
        if (std::filesystem::exists(pwshPath))
        {
            out.emplace_back(PowershellCoreProfileGenerator::PowerShellInstance{ 0 /* we can't tell */, flags, pwshPath });
        }
    }

    // Function Description:
    // - Builds a comprehensive priority-ordered list of powershell instances.
    // Return value:
    // - a comprehensive priority-ordered list of powershell instances.
    static std::vector<PowershellCoreProfileGenerator::PowerShellInstance> _collectPowerShellInstances()
    {
        std::vector<PowershellCoreProfileGenerator::PowerShellInstance> versions;

        _accumulateTraditionalLayoutPowerShellInstancesInDirectory(L"%ProgramFiles%\\PowerShell", PowershellCoreProfileGenerator::PowerShellFlags::None, versions);

#if defined(_M_AMD64) || defined(_M_ARM64) // No point in looking for WOW if we're not somewhere it exists
        _accumulateTraditionalLayoutPowerShellInstancesInDirectory(L"%ProgramFiles(x86)%\\PowerShell", PowershellCoreProfileGenerator::PowerShellFlags::WOWx86, versions);
#endif

#if defined(_M_ARM64) // no point in looking for WOA if we're not on ARM64
        _accumulateTraditionalLayoutPowerShellInstancesInDirectory(L"%ProgramFiles(Arm)%\\PowerShell", PowershellCoreProfileGenerator::PowerShellFlags::WOWARM, versions);
#endif

        _accumulateStorePowerShellInstances(versions);

        _accumulatePwshExeInDirectory(L"%USERPROFILE%\\.dotnet\\tools", PowershellCoreProfileGenerator::PowerShellFlags::Dotnet, versions);
        _accumulatePwshExeInDirectory(L"%USERPROFILE%\\scoop\\shims", PowershellCoreProfileGenerator::PowerShellFlags::Scoop, versions);

        std::sort(versions.rbegin(), versions.rend()); // sort in reverse (best first)

        return versions;
    }

    // Legacy GUIDs:
    //   - PowerShell Core       574e775e-4f2a-5b96-ac1e-a2962a402336
    static constexpr winrt::guid PowershellCoreGuid{ 0x574e775e, 0x4f2a, 0x5b96, { 0xac, 0x1e, 0xa2, 0x96, 0x2a, 0x40, 0x23, 0x36 } };

    std::wstring_view PowershellCoreProfileGenerator::GetNamespace() const noexcept
    {
        return PowershellCoreGeneratorNamespace;
    }

    std::wstring_view PowershellCoreProfileGenerator::GetDisplayName() const noexcept
    {
        return RS_(L"PowershellCoreProfileGeneratorDisplayName");
    }

    std::wstring_view PowershellCoreProfileGenerator::GetIcon() const noexcept
    {
        return GENERATOR_POWERSHELL_ICON;
    }

    // Method Description:
    // - Checks if pwsh is installed, and if it is, creates a profile to launch it.
    // Arguments:
    // - <none>
    // Return Value:
    // - a vector with the PowerShell Core profile, if available.
    void PowershellCoreProfileGenerator::GenerateProfiles(std::vector<winrt::com_ptr<implementation::Profile>>& profiles)
    {
        GetPowerShellInstances();
        auto first = true;

        for (const auto& psI : _powerShellInstances)
        {
            const auto name = psI.Name();
            auto profile{ CreateDynamicProfile(name) };

            const auto& unquotedCommandline = psI.executablePath.native();
            std::wstring quotedCommandline;
            quotedCommandline.reserve(unquotedCommandline.size() + 2);
            quotedCommandline.push_back(L'"');
            quotedCommandline.append(unquotedCommandline);
            quotedCommandline.push_back(L'"');
            profile->Commandline(winrt::hstring{ quotedCommandline });

            profile->StartingDirectory(winrt::hstring{ DEFAULT_STARTING_DIRECTORY });
            profile->DefaultAppearance().DarkColorSchemeName(L"Campbell");
            profile->DefaultAppearance().LightColorSchemeName(L"Campbell");
            profile->Icon(winrt::hstring{ WI_IsFlagSet(psI.flags, PowerShellFlags::Preview) ? POWERSHELL_PREVIEW_ICON : POWERSHELL_ICON });

            if (first)
            {
                // Give the first ("algorithmically best") profile the official, and original, "PowerShell Core" GUID.
                // This will turn the anchored default profile into "PowerShell Core Latest for Native Architecture through Store"
                // (or the closest approximation thereof). It may choose a preview instance as the "best" if it is a higher version.
                profile->Guid(PowershellCoreGuid);
                profile->Name(winrt::hstring{ POWERSHELL_PREFERRED_PROFILE_NAME });

                first = false;
            }

            profiles.emplace_back(std::move(profile));
        }
    }

    std::vector<PowershellCoreProfileGenerator::PowerShellInstance> PowershellCoreProfileGenerator::GetPowerShellInstances() noexcept
    {
        if (_powerShellInstances.empty())
        {
            _powerShellInstances = _collectPowerShellInstances();
        }
        return _powerShellInstances;
    }

    // Function Description:
    // - Returns the thing it's named for.
    // Return value:
    // - the thing it says in the name
    const std::wstring_view PowershellCoreProfileGenerator::GetPreferredPowershellProfileName()
    {
        return POWERSHELL_PREFERRED_PROFILE_NAME;
    }
}
