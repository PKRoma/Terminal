/*++
Copyright (c) Microsoft Corporation
Licensed under the MIT license.

Module Name:
- AppearanceConfig

Abstract:
- The implementation of the AppearanceConfig winrt class. Provides settings related
  to the appearance of the terminal, in both terminal control and terminal core.

Author(s):
- Pankaj Bhojwani - Nov 2020

--*/

#pragma once

#include "AppearanceConfig.g.h"
#include "JsonUtils.h"
#include "TerminalSettingsSerializationHelpers.h"
#include "IInheritable.h"
#include "MTSMSettings.h"
#include "MediaResourceSupport.h"
#include <DefaultSettings.h>

// ColorSchemeReference: a pair of color scheme names for dark and light themes.
// Represents the polymorphic "colorScheme" JSON key, which can be either:
//   "colorScheme": "Campbell"                     → { dark: "Campbell", light: "Campbell" }
//   "colorScheme": { "dark": "X", "light": "Y" } → { dark: "X", light: "Y" }
struct ColorSchemeReference
{
    winrt::hstring dark{ L"Campbell" };
    winrt::hstring light{ L"Campbell" };

    static ColorSchemeReference Default() { return {}; }
    bool operator==(const ColorSchemeReference&) const = default;
};

namespace Microsoft::Terminal::Settings::Model::JsonUtils
{
    template<>
    struct ConversionTrait<ColorSchemeReference>
    {
        ColorSchemeReference FromJson(const Json::Value& json)
        {
            ColorSchemeReference result;
            if (json.isString())
            {
                // Simple form: both dark and light use the same scheme
                const auto name = winrt::hstring{ til::u8u16(json.asString()) };
                result.dark = name;
                result.light = name;
            }
            else if (json.isObject())
            {
                // Structured form: { "dark": "...", "light": "..." }
                if (json.isMember("dark"))
                {
                    result.dark = winrt::hstring{ til::u8u16(json["dark"].asString()) };
                }
                if (json.isMember("light"))
                {
                    result.light = winrt::hstring{ til::u8u16(json["light"].asString()) };
                }
            }
            return result;
        }

        bool CanConvert(const Json::Value& json) const
        {
            return json.isString() || json.isObject();
        }

        Json::Value ToJson(const ColorSchemeReference& val)
        {
            // Collapse to string when dark == light
            if (val.dark == val.light)
            {
                return til::u16u8(val.dark);
            }
            Json::Value obj{ Json::ValueType::objectValue };
            obj["dark"] = til::u16u8(val.dark);
            obj["light"] = til::u16u8(val.light);
            return obj;
        }

        std::string TypeDescription() const
        {
            return "color scheme name (string or {dark, light})";
        }
    };
}

namespace winrt::Microsoft::Terminal::Settings::Model::implementation
{
    struct AppearanceConfig : AppearanceConfigT<AppearanceConfig, IMediaResourceContainer>, IInheritable<AppearanceConfig>
    {
    public:
        AppearanceConfig(winrt::weak_ref<Profile> sourceProfile);
        static winrt::com_ptr<AppearanceConfig> CopyAppearance(const AppearanceConfig* source, winrt::weak_ref<Profile> sourceProfile);
        Json::Value ToJson() const;
        void LayerJson(const Json::Value& json);
        void LogSettingChanges(std::set<std::string>& changes, const std::string_view& context) const;

        Model::Profile SourceProfile();

        void ResolveMediaResources(const Model::MediaResourceResolver& resolver);

        // Generic setting access via SettingKey
        bool HasSetting(AppearanceSettingKey key) const;
        void ClearSetting(AppearanceSettingKey key);
        std::vector<AppearanceSettingKey> CurrentSettings() const;

        // Nullable color settings (JSON-backed)
        INHERITABLE_NULLABLE_SETTING(Model::IAppearanceConfig, Microsoft::Terminal::Core::Color, Foreground, "foreground", nullptr)
        INHERITABLE_NULLABLE_SETTING(Model::IAppearanceConfig, Microsoft::Terminal::Core::Color, Background, "background", nullptr)
        INHERITABLE_NULLABLE_SETTING(Model::IAppearanceConfig, Microsoft::Terminal::Core::Color, SelectionBackground, "selectionBackground", nullptr)
        INHERITABLE_NULLABLE_SETTING(Model::IAppearanceConfig, Microsoft::Terminal::Core::Color, CursorColor, "cursorColor", nullptr)

        // Opacity: JSON-backed with normalization (int percent → float in LayerJson)
        INHERITABLE_SETTING(Model::IAppearanceConfig, float, Opacity, "opacity", 1.0f)

        // ColorScheme: JSON-backed via INHERITABLE_SETTING with ColorSchemeReference struct.
        // The polymorphic JSON key ("colorScheme" as string or object) is handled by
        // ConversionTrait<ColorSchemeReference>. The macro provides Has/Clear/OverrideSource.
        // Thin wrappers expose the IDL-required DarkColorSchemeName / LightColorSchemeName.
        INHERITABLE_SETTING(Model::IAppearanceConfig, ColorSchemeReference, ColorSchemeRef, "colorScheme", ColorSchemeReference::Default())
    public:
        hstring DarkColorSchemeName() const { return ColorSchemeRef().dark; }
        void DarkColorSchemeName(const hstring& value)
        {
            auto ref = ColorSchemeRef();
            ref.dark = value;
            ColorSchemeRef(ref);
        }
        bool HasDarkColorSchemeName() const { return HasColorSchemeRef(); }
        void ClearDarkColorSchemeName() { ClearColorSchemeRef(); }
        Model::IAppearanceConfig DarkColorSchemeNameOverrideSource() { return ColorSchemeRefOverrideSource(); }

        hstring LightColorSchemeName() const { return ColorSchemeRef().light; }
        void LightColorSchemeName(const hstring& value)
        {
            auto ref = ColorSchemeRef();
            ref.light = value;
            ColorSchemeRef(ref);
        }
        bool HasLightColorSchemeName() const { return HasColorSchemeRef(); }
        void ClearLightColorSchemeName() { ClearColorSchemeRef(); }
        Model::IAppearanceConfig LightColorSchemeNameOverrideSource() { return ColorSchemeRefOverrideSource(); }

#define APPEARANCE_SETTINGS_INITIALIZE(type, name, jsonKey, ...) \
    INHERITABLE_SETTING(Model::IAppearanceConfig, type, name, jsonKey, ##__VA_ARGS__)
        MTSM_APPEARANCE_SETTINGS(APPEARANCE_SETTINGS_INITIALIZE)
#undef APPEARANCE_SETTINGS_INITIALIZE

        // IMediaResource settings with backing fields for resolution lifecycle.
        // See IInheritable.h for the INHERITABLE_MEDIA_RESOURCE_SETTING macro definition.
        INHERITABLE_MEDIA_RESOURCE_SETTING(Model::IAppearanceConfig, PixelShaderPath, "experimental.pixelShaderPath", implementation::MediaResource::Empty())
        INHERITABLE_MEDIA_RESOURCE_SETTING(Model::IAppearanceConfig, PixelShaderImagePath, "experimental.pixelShaderImagePath", implementation::MediaResource::Empty())
        INHERITABLE_MEDIA_RESOURCE_SETTING(Model::IAppearanceConfig, BackgroundImagePath, "backgroundImage", implementation::MediaResource::Empty())

    private:
        winrt::weak_ref<Profile> _sourceProfile;

        // Raw JSON for this layer (appearance-relevant keys only).
        Json::Value _json{ Json::ValueType::objectValue };

        std::set<std::string> _changeLog;

        void _logSettingSet(const std::string_view& setting);
        void _logSettingIfSet(const std::string_view& setting, const bool isSet);

        std::tuple<winrt::hstring, Model::OriginTag> _getSourceProfileBasePathAndOrigin() const;
    };
}
